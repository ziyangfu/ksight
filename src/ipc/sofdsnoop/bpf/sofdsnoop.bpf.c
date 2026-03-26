// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2018 Jiri Olsa
// Ported to libbpf/CO-RE by ksight project
// vmlinux.h 必须第一个 include，它定义了 __u64/__u32 等内核类型
#include <vmlinux.h>
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

#include "sofdsnoop.h"

// SCM_RIGHTS 在 vmlinux.h 中无法获得，手动定义
#define SCM_RIGHTS 0x01

// --- 过滤参数 (由用户态通过 rodata 配置) ---
const volatile pid_t targ_pid = 0;
const volatile pid_t targ_tid = 0;

// --- Maps ---
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, 4096);
  __type(key, u64);
  __type(value, int);
} sock_fd_map SEC(".maps");

struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, 4096);
  __type(key, u64);
  __type(value, struct cmsghdr *);
} detach_ptr SEC(".maps");

struct {
  __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
  __uint(key_size, sizeof(u32));
  __uint(value_size, sizeof(u32));
} events SEC(".maps");

// --- 辅助函数 ---
static __always_inline bool allow_pid(u64 id) {
  u32 pid = id >> 32;
  u32 tid = (u32)id;
  if (targ_tid && targ_tid != (pid_t)tid)
    return false;
  if (targ_pid && targ_pid != (pid_t)pid)
    return false;
  return true;
}

static __always_inline void set_fd(int fd) {
  u64 id = bpf_get_current_pid_tgid();
  bpf_map_update_elem(&sock_fd_map, &id, &fd, BPF_ANY);
}

static __always_inline int get_fd(void) {
  u64 id = bpf_get_current_pid_tgid();
  int *fd = bpf_map_lookup_elem(&sock_fd_map, &id);
  return fd ? *fd : -1;
}

static __always_inline void put_fd(void) {
  u64 id = bpf_get_current_pid_tgid();
  bpf_map_delete_elem(&sock_fd_map, &id);
}

static __always_inline int emit_event(void *ctx, struct event *val, int num,
                                      void *data) {
  u32 cnt = num < MAX_FD ? (u32)num : (u32)MAX_FD;
  val->fd_cnt = (int)cnt;
  if (bpf_probe_read_kernel(&val->fd[0], MAX_FD * sizeof(int), data))
    return -1;
  bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, val, sizeof(*val));
  return 0;
}

static __always_inline int do_send(void *ctx, struct cmsghdr *cmsg,
                                   int action) {
  struct event val = {};
  u64 cmsg_len = 0;

  bpf_probe_read_kernel(&cmsg_len, sizeof(cmsg_len), &cmsg->cmsg_len);
  if (cmsg_len < sizeof(struct cmsghdr))
    return 0;

  int *data = (int *)((char *)cmsg + sizeof(struct cmsghdr));
  int num = (int)((cmsg_len - sizeof(struct cmsghdr)) / sizeof(int));

  val.id = bpf_get_current_pid_tgid();
  val.action = action;
  val.sock_fd = get_fd();
  val.ts = bpf_ktime_get_ns() / 1000;
  if (bpf_get_current_comm(&val.comm, sizeof(val.comm)) != 0)
    return 0;

  emit_event(ctx, &val, num, data);
  return 0;
}

// msg_control 是 void * 联合体，不能用 BPF_CORE_READ 进行 CO-RE 访问，
// 直接用 bpf_probe_read_kernel 以字节方式读取指针
static __always_inline struct cmsghdr *read_msg_control(struct msghdr *hdr) {
  void *ctrl = NULL;
  bpf_probe_read_kernel(&ctrl, sizeof(ctrl), (void *)&hdr->msg_control);
  return (struct cmsghdr *)ctrl;
}

static __always_inline u64 read_msg_controllen(struct msghdr *hdr) {
  u64 len = 0;
  bpf_probe_read_kernel(&len, sizeof(len), &hdr->msg_controllen);
  return len;
}

// --- kprobe 探针 ---

SEC("kprobe/__scm_send")
int BPF_KPROBE(trace_scm_send_entry, struct socket *sock, struct msghdr *hdr) {
  if (!allow_pid(bpf_get_current_pid_tgid()))
    return 0;
  if (read_msg_controllen(hdr) < sizeof(struct cmsghdr))
    return 0;

  struct cmsghdr *cmsg = read_msg_control(hdr);
  if (!cmsg)
    return 0;

  int cmsg_type = 0;
  bpf_probe_read_kernel(&cmsg_type, sizeof(cmsg_type), &cmsg->cmsg_type);
  if (cmsg_type != SCM_RIGHTS)
    return 0;

  return do_send(ctx, cmsg, ACTION_SEND);
}

SEC("kprobe/scm_detach_fds")
int BPF_KPROBE(trace_scm_detach_fds_entry, struct msghdr *hdr) {
  u64 id = bpf_get_current_pid_tgid();
  if (!allow_pid(id))
    return 0;
  if (read_msg_controllen(hdr) < sizeof(struct cmsghdr))
    return 0;

  struct cmsghdr *cmsg = read_msg_control(hdr);
  if (!cmsg)
    return 0;

  bpf_map_update_elem(&detach_ptr, &id, &cmsg, BPF_ANY);
  return 0;
}

SEC("kretprobe/scm_detach_fds")
int BPF_KRETPROBE(trace_scm_detach_fds_return) {
  u64 id = bpf_get_current_pid_tgid();
  if (!allow_pid(id))
    return 0;

  struct cmsghdr **cmsgp = bpf_map_lookup_elem(&detach_ptr, &id);
  if (!cmsgp)
    return 0;

  return do_send(ctx, *cmsgp, ACTION_RECV);
}

// 使用 ksyscall/kretsyscall section，libbpf 会自动根据架构添加正确前缀
// AMD64: __x64_sys_sendmsg, ARM64: __arm64_sys_sendmsg
SEC("ksyscall/sendmsg")
int BPF_KSYSCALL(trace_sendmsg_entry, int fd) {
  if (!allow_pid(bpf_get_current_pid_tgid()))
    return 0;
  set_fd(fd);
  return 0;
}

SEC("kretsyscall/sendmsg")
int BPF_KRETPROBE(trace_sendmsg_return) {
  if (!allow_pid(bpf_get_current_pid_tgid()))
    return 0;
  put_fd();
  return 0;
}

SEC("ksyscall/recvmsg")
int BPF_KSYSCALL(trace_recvmsg_entry, int fd) {
  if (!allow_pid(bpf_get_current_pid_tgid()))
    return 0;
  set_fd(fd);
  return 0;
}

SEC("kretsyscall/recvmsg")
int BPF_KRETPROBE(trace_recvmsg_return) {
  if (!allow_pid(bpf_get_current_pid_tgid()))
    return 0;
  put_fd();
  return 0;
}

char LICENSE[] SEC("license") = "GPL";
