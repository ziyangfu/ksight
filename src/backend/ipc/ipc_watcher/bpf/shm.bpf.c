/*!
 * \brief 共享内存的追踪与观测
 * \file  shm.bpf.c
 * \details
 *      ARCH Support: X86-64, ARM64
 * */
#include "common.bpf.h"
#include "shm_trace.bpf.h"
#include "shm_data.h"

/*!
 * \brief 监控 mmap 调用
 * \details
 * 立即排除 私有匿名映射、私有文件映射与匿名文件映射
 * root@fzy-Lenovo:/home/fzy# bpftrace -l tracepoint:syscalls:sys_enter_mmap -v
        tracepoint:syscalls:sys_enter_mmap
            int __syscall_nr;
            unsigned long addr;
            unsigned long len;
            unsigned long prot;
            unsigned long flags;
            unsigned long fd;
            unsigned long off;
 * */
 SEC("tracepoint/syscalls/sys_enter_mmap")
 int handle_syscall_enter_mmap(struct trace_event_raw_sys_enter *ctx)
{
     /** 确定是共享映射，排除匿名映射，则只剩下了共享文件映射，即共享内存 */
    unsigned long flags = (unsigned long) ctx->args[3];  // 获取flags参数
    bool is_shared = flags & MAP_SHARED;  /** 检查是否为共享映射 */
    if (!is_shared) {
        return 0;
    }
    bool is_anonymous = flags & MAP_ANONYMOUS; /** 检查是否为匿名映射 */
    //if (is_anonymous && fd != -1UL) {
    if (is_anonymous) {   /** fd 返回的是无符号整数，如果有符号应该是-1，所以使用fd的超限判断 */
        return 0;
    }
    unsigned long fd = (unsigned long) ctx->args[4];
    // 获取pid和tid
    u64 id = bpf_get_current_pid_tgid();
    pid_t pid = id >> 32;
    tid_t tid = (u32) id;

    // 创建临时键
    struct mmap_temp_key temp_key = { .pid = pid, .tid = tid };

    // 创建数据结构
    struct shm_mmap_enter_data data = {0};
    data.pid = pid;
    data.tid = tid;
    data.len = (unsigned long) ctx->args[1];   // len
    data.prot = (unsigned long) ctx->args[2];  // prot
    data.flags = flags;                        // flags
    data.fd = fd;                              // fd
    data.off = (unsigned long) ctx->args[5];   // offset

    // 存储到临时映射中
    bpf_map_lookup_or_try_init(&shm_temp_map, &temp_key, &data);
    return 0;
}
/*!
 * \brief
 * \details 所有的mmap都要经过ksys_mmap_pgoff，流量很大，要注意性能影响
 *
 * */
SEC("kprobe/ksys_mmap_pgoff")
int BPF_KPROBE(ksys_mmap_pgoff, unsigned long addr, unsigned long len,
               unsigned long prot, unsigned long flags,
               unsigned long fd)
{
    return  0;
}

/*!
 * \brief Step 1：监控 mmap 调用，记录共享内存地址范围
 * \details
 *  root@fzy-Lenovo:/home/fzy# cat /sys/kernel/debug/tracing/events/syscalls/sys_exit_mmap/format
        name: sys_exit_mmap
        ID: 100
        format:
            field:unsigned short common_type;	offset:0;	size:2;	signed:0;
            field:unsigned char common_flags;	offset:2;	size:1;	signed:0;
            field:unsigned char common_preempt_count;	offset:3;	size:1;	signed:0;
            field:int common_pid;	offset:4;	size:4;	signed:1;

            field:int __syscall_nr;	offset:8;	size:4;	signed:1;
            field:long ret;	offset:16;	size:8;	signed:1;

        print fmt: "0x%lx", REC->ret

 * */
// 挂载点：sys_exit_mmap

SEC("tracepoint/syscalls/sys_exit_mmap")
int handle_syscall_exit_mmap(struct trace_event_raw_sys_exit *ctx)
{
    // 获取pid和tid
    u64 id = bpf_get_current_pid_tgid();
    pid_t pid = id >> 32;
    tid_t tid = (u32) id;

    // 创建临时键
    struct mmap_temp_key temp_key = { .pid = pid, .tid = tid };

    // 查找之前保存的数据
    struct shm_mmap_enter_data *data = bpf_map_lookup_elem(&shm_temp_map, &temp_key);
    if (!data) {
        return 0; // 没有找到对应的数据
    }

    // 删除临时数据
    bpf_map_delete_elem(&shm_temp_map, &temp_key);

    // 向 ringbuf 提交数据
    struct shm_transfer_basic_data* trans_rb_event =
            bpf_ringbuf_reserve(&shm_events, sizeof(struct shm_transfer_basic_data), 0);
    if (!trans_rb_event) {
        return 0;
    }

    trans_rb_event->timestamp = bpf_ktime_get_ns() / 1000;
    trans_rb_event->pid = data->pid;
    trans_rb_event->fd = data->fd;
    trans_rb_event->flags = data->flags;
    trans_rb_event->len = data->len;
    trans_rb_event->prot = data->prot;
    trans_rb_event->off = data->off;
    trans_rb_event->mmap_addr = ctx->ret;  // mmap的返回值（映射的虚拟地址）

    bpf_ringbuf_submit(trans_rb_event, 0);
    return 0;
}


char LICENSE[] SEC("license") = "GPL";