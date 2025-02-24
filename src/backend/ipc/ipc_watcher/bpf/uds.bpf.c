// Copyright 2025 The LMP Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://github.com/linuxkerneltravel/lmp/blob/develop/LICENSE
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
/*!
\brief
    1. 简介
        本文件主要跟踪 Linux IPC 进程间通信的相关信息
        主要包括： unix domain socket（UDS）、mmap共享内存、信号
    uds部分：
        0. 功能：
            1. 跟踪并输出uds的基本信息，包括可选的发送与接收信息
            2. 跟踪uds从发送到接收的路径跟踪
                unix_xx_sendmsg --> VFS --> unix_xx_recvmsg 的路径跟踪
        1. 挂载点
            unix_dgram
                kprobe:unix_dgram_sendmsg
                kprobe:unix_dgram_recvmsg
                kprobe:unix_dgram_poll      [opt]
                kprobe:unix_dgram_connect   [opt]
            unix_stream
                kprobe:unix_stream_recvmsg
                kprobe:unix_stream_sendmsg
                kprobe: skb_copy_datagram_from_iter
                kprobe:unix_stream_connect  [opt]
                
        2. 内核文件： net/unix/af_unix.c
    mmap shm：
        TODO
    signal：
        TODO
*/

/*!
 * uds部分：
 *      1. 在如/tmp下的显性文件的跟踪
 * */

#include "common.bpf.h"

// 获取UDS路径的辅助函数
static void get_uds_path(struct unix_sock *u, char *path) {
    struct unix_address *addr;
    struct sockaddr_un *sun;
    addr = BPF_CORE_READ(u, addr);
    if (!addr) {
        bpf_probe_read_kernel_str(path, 6, "<none>");
        return;
    }
    sun = BPF_CORE_READ(addr, name);
    if (!sun) {
        bpf_probe_read_kernel_str(path, 6, "<none>");
        return;
    }
    bpf_probe_read_kernel_str(path, sizeof(sun->sun_path), sun->sun_path);
}

/*!
\brief
    挂载点 unix_dgram_sendmsg, 负责采集uds dgram的基本信息与发送的数据
*/
SEC("kprobe/unix_dgram_sendmsg")
int BPF_KPROBE(unix_dgram_sendmsg, const struct socket *sock, const struct msghdr *msg,
			      size_t len) {
    u64 current_pid = bpf_get_current_pid_tgid() >> 32;
    struct sock *sk = BPF_CORE_READ(sock, sk);
    struct uds_event zero = {0};
    struct uds_event* event;
    event = (struct uds_event*)bpf_map_lookup_or_try_init(&uds_data_map, &sk, &zero);
    if (event == NULL) {
        return 0;
    }
    struct unix_sock *unix_sk = (struct unix_sock*)sk;
    const struct unix_address *addr = BPF_CORE_READ(unix_sk, addr);
    /** 存在显性路径 */
    if (addr) {
        const char *path = BPF_CORE_READ(addr, name->sun_path);
        bpf_probe_read_kernel_str(event->path, sizeof(event->path), path);
    }
    else {
        bpf_probe_read_kernel_str(event->path, 7, "<none>");
    }
    event->send_pid = current_pid;
    event->size = (u32)len;
    event->type = BPF_CORE_READ(sk, sk_type);
    event->timestamp = bpf_ktime_get_ns() / 1000;
    return 0;
}

/*!
\brief
    挂载点 unix_dgram_recvmsg, 负责采集uds dgram的基本信息与接收的数据
*/
SEC("kprobe/unix_dgram_recvmsg")
int BPF_KPROBE(unix_dgram_recvmsg, const struct socket *sock, const struct msghdr *msg,
			      size_t size, int flags) {
    struct sock* sk = BPF_CORE_READ(sock, sk);
    struct uds_event* event = bpf_map_lookup_elem(&uds_data_map, &sk);
    if (!event)
        return 0;
    event->recv_pid = bpf_get_current_pid_tgid() >> 32;
    //event->payload[0] = '\0';

    struct uds_event* trans_rb_event =
            bpf_ringbuf_reserve(&uds_events, sizeof(struct uds_event), 0);
    if (trans_rb_event == NULL) {
        bpf_map_delete_elem(&uds_data_map, &sk);
        return 0;
    }
    trans_rb_event->send_pid = event->send_pid;
    trans_rb_event->recv_pid = event->recv_pid;
    bpf_probe_read_kernel_str(trans_rb_event->path, sizeof(event->path), event->path);
    trans_rb_event->size = event->size;
    trans_rb_event->type = event->type;
    trans_rb_event->timestamp = event->timestamp;
    //trans_rb_event->payload[0] = '\0';

    bpf_map_delete_elem(&uds_data_map, &sk);

    bpf_ringbuf_submit(trans_rb_event, 0);
    return 0;
}

/*!
\brief
    挂载点 unix_stream_sendmsg, 负责采集流式uds的基本信息与发送的数据
    获取发送侧 PID， uds path， 发送的size大小， 发送时间点
*/
SEC("kprobe/unix_stream_sendmsg")
int BPF_KPROBE(unix_stream_sendmsg, struct socket *sock, struct msghdr *msg,
               size_t len) {
    u64 current_pid = bpf_get_current_pid_tgid() >> 32;
    struct sock *sk = BPF_CORE_READ(sock, sk);
    struct uds_event zero = {0};
    struct uds_event* event;
    event = (struct uds_event*)bpf_map_lookup_or_try_init(&uds_data_map, &sk, &zero);
    if (event == NULL) {
        return 0;
    }
    struct unix_sock *unix_sk = (struct unix_sock*)sk;
    const struct unix_address *addr = BPF_CORE_READ(unix_sk, addr);
    /** 存在显性路径 */
    if (addr) {
        const char *path = BPF_CORE_READ(addr, name->sun_path);
        bpf_probe_read_kernel_str(event->path, sizeof(event->path), path);
    }
    else {
        bpf_probe_read_kernel_str(event->path, 7, "<none>");
    }
    event->send_pid = current_pid;
    event->size = (u32)len;
    event->type = BPF_CORE_READ(sk, sk_type);
    event->timestamp = bpf_ktime_get_ns() / 1000;
    return 0;
}

/*!
\brief
    挂载点 unix_stream_recvmsg, 负责采集流式uds的基本信息与接收的数据
    获取接收侧 PID， uds path， 接收的size大小， payload，接收时间点
*/
SEC("kprobe/unix_stream_recvmsg")
int BPF_KPROBE(unix_stream_recvmsg, const struct socket *sock, const struct msghdr *msg,
			       size_t size, int flags) {
    struct sock* sk = BPF_CORE_READ(sock, sk);
    struct uds_event* event = bpf_map_lookup_elem(&uds_data_map, &sk);
    if (!event)
        return 0;
    event->recv_pid = bpf_get_current_pid_tgid() >> 32;
    //event->payload[0] = '\0';

    struct uds_event* trans_rb_event =
            bpf_ringbuf_reserve(&uds_events, sizeof(struct uds_event), 0);
    if (trans_rb_event == NULL) {
        bpf_map_delete_elem(&uds_data_map, &sk);
        return 0;
    }
    trans_rb_event->send_pid = event->send_pid;
    trans_rb_event->recv_pid = event->recv_pid;
    bpf_probe_read_kernel_str(trans_rb_event->path, sizeof(event->path), event->path);
    trans_rb_event->size = event->size;
    trans_rb_event->type = event->type;
    trans_rb_event->timestamp = event->timestamp;
    //trans_rb_event->payload[0] = '\0';

    bpf_map_delete_elem(&uds_data_map, &sk);

    bpf_ringbuf_submit(trans_rb_event, 0);

    return 0;
}

int probe_unix_socket_sendmsg(struct pt_regs *ctx,
                              struct socket *sock,
                              struct msghdr *msg,
                              size_t len)
{
    struct packet *packet;
    struct unix_address *addr;
    char *buf, *sock_path;
    unsigned long path[__PATH_LEN_U64__] = {0};
    unsigned int n, match = 0, offset;
    struct iov_iter *iter;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
    const struct iovec *iov;
#else
    const struct kvec *iov;
#endif
    struct pid *peer_pid;

    addr = ((struct unix_sock *)sock->sk)->addr;
    if (addr->len > 0) {
        sock_path = (char *)addr + SOCK_PATH_OFFSET;
        if (*sock_path == 0) {
            // abstract sockets start with \\0 and the name comes after
            // (they actually have no @ prefix but some tools use that)
            bpf_probe_read(&path, __PATH_LEN__ - 1, sock_path + 1);
        } else {
            bpf_probe_read(&path, __PATH_LEN__, sock_path);
        }
        __PATH_FILTER__
    }

    addr = ((struct unix_sock *)((struct unix_sock *)sock->sk)->peer)->addr;
    if (match == 0 && addr->len > 0) {
        sock_path = (char *)addr + SOCK_PATH_OFFSET;
        if (*sock_path == 0) {
            // abstract sockets start with \\0 and the name comes after
            // (they actually have no @ prefix but some tools use that)
            bpf_probe_read(&path, __PATH_LEN__ - 1, sock_path + 1);
        } else {
            bpf_probe_read(&path, __PATH_LEN__, sock_path);
        }
        __PATH_FILTER__
    }

    if (match == 0)
        return 0;

    n = bpf_get_smp_processor_id();
    packet = packet_array.lookup(&n);
    if (packet == NULL)
        return 0;

    packet->pid = bpf_get_current_pid_tgid() >> 32;
    bpf_get_current_comm(&packet->comm, sizeof(packet->comm));
    bpf_probe_read(&packet->path, UNIX_PATH_MAX, sock_path);
    packet->peer_pid = sock->sk->sk_peer_pid->numbers->nr;

    __PID_FILTER__

            iter = &msg->msg_iter;
#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
    if (iter->iter_type == ITER_UBUF) {
        packet->len = len;
        packet->flags = 0;
        buf = iter->ubuf;
        n = len;

        bpf_probe_read(
            &packet->data,
            // check size in args to make compiler/validator happy
            n > sizeof(packet->data) ? sizeof(packet->data) : n,
            buf);

        n += offsetof(struct packet, data);
        events.perf_submit(
            ctx,
            packet,
            // check size in args to make compiler/validator happy
            n > sizeof(*packet) ? sizeof(*packet) : n);

        return 0;
    }

    if (iter->iter_type != ITER_IOVEC || iter->iov_offset != 0) {
#else
    if (iter->iov_offset != 0) {
#endif
        packet->len = len;
        packet->flags = SS_PACKET_F_ERR;
        events.perf_submit(ctx, packet, offsetof(struct packet, data));
        return 0;
    }

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6,3,0)
    iov = iter->__iov;
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(6,0,0)
    iov = iter->iov;
#else
    iov = iter->kvec;
#endif

#pragma unroll
    for (int i = 0; i < SS_MAX_SEGS_PER_MSG; i++) {
        if (i >= iter->nr_segs)
            break;

        packet->len = iov->iov_len;
        packet->flags = 0;

        buf = iov->iov_base;
        n = iov->iov_len;
        bpf_probe_read(
                &packet->data,
                // check size in args to make compiler/validator happy
                n > sizeof(packet->data) ? sizeof(packet->data) : n,
                buf);

        n += offsetof(struct packet, data);
        events.perf_submit(
                ctx,
                packet,
                // check size in args to make compiler/validator happy
                n > sizeof(*packet) ? sizeof(*packet) : n);

        iov++;
    }

    return 0;
}

char LICENSE[] SEC("license") = "GPL";
