/*!
 * \brief 共享内存的追踪与观测
 * \file  shm.bpf.c
 * \details
 *      ARCH Support: X86-64, ARM64
 * */
#include "common.bpf.h"


/*!
 *              共享              独享
 * 匿名页        IPC             动态链接库映射
 * 文件页        IPC
 *
 *
 * mmap机制
 * shm_create，创建匿名页
 *
 *
 *  1. 匿名共享内存
    2. 文件共享内存
    3. 支持共享内存头的自定义，以一种格式，例如yaml或json
    4. 零拷贝调试与追踪

1. 帮助理解Linux内核共享内存机制
2. 观测共享内存的数据流，无侵入、以一种无需修改代码与编译、无需源码的方式，只要用共享内存，就能捕捉到数据流（可能是加密数据，提供yaml配置选项）
3. 共享内存的基本数据观测，通用工具也能做到，不局限于使用eBPF在内核中采集数据，可能在用户态，或者直接使用通用工具，获得数据
4. 开发共享内存IPC的应用或中间件时的调试辅助，例如：共享内存的无侵入式内存泄露检测


  监控共享内存区域的变化，迭代的显示（迭代显示模式）

5. Linux与RTOS通过共享内存通信时的数据包观测。 https://github.com/nxp-auto-linux/ipc-shm

 * */

/**
POSIX 共享内存，支持
    shm_open
    mmap
    munmap
    shm_unlink
    unlink

    tracepoint:mmap:vm_unmapped_area
    tracepoint:mmap_lock:mmap_lock_start_locking
    tracepoint:mmap_lock:mmap_lock_acquire_returned
    tracepoint:mmap_lock:mmap_lock_released
    tracepoint:syscalls:sys_enter_mmap
    tracepoint:syscalls:sys_exit_mmap
System V 共享内存，暂无支持计划
    tracepoint:syscalls:sys_enter_shmget
    tracepoint:syscalls:sys_exit_shmget
    tracepoint:syscalls:sys_enter_shmctl
    tracepoint:syscalls:sys_exit_shmctl
    tracepoint:syscalls:sys_enter_shmat
    tracepoint:syscalls:sys_exit_shmat
    tracepoint:syscalls:sys_enter_shmdt
    tracepoint:syscalls:sys_exit_shmdt


 映射了一块内存，大小已知，地址已知
 同步机制： 不管怎么同步，发送端与接收端都要去读取映射区，并把数据写入映射区。
 因此我只需要hook发送端与接收端去映射区读取的函数，就可以实现

 在使用POSIX共享内存IPC通信时，不会经过内核态与用户态的转换，也就是说在数据的发送与接收过程中，数据是直接在用户态进行传递的。
 不需要内核的参与，因此在内核使用eBPF是无法捕捉到数据的。
 可以考虑使用 uprobe 在用户态获取数据。

 *
 * */


struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24);  // 16MB ring buffer
} events SEC(".maps");


/*!
 * \brief Step 1：监控 mmap 调用，记录共享内存地址范围
 * \details 记录都谁调用了mmap，记录下pid，每个进程的虚拟内存地址是不同的，
 *          所以修改同步记录pid与对应的虚拟内存地址，使用hash表
 * */
SEC("tracepoint/syscalls/sys_exit_mmap")
int handle_syscall_mmap(struct trace_event_raw_sys_exit *ctx)
{
    void *addr = (void *)bpf_get_retval(ctx);
    if (IS_ERR(addr))
        return 0;

    // 记录 mmap 地址到 map 中供后续分析
    pid_t pid = bpf_get_current_pid_tgid() >> 32;
    struct mmap_info info = {
            .start = addr,
            .end = addr + ctx->args[1],  // len 参数
    };
    bpf_map_update_elem(&mmap_map, &pid, &info, BPF_ANY);
    return 0;
}

SEC("kprobe/sys_mmap_pgoff")
int handle_sys_mmap(struct pt_regs *ctx)
{
    struct task_struct *task = (struct task_struct *)bpf_get_current_task();
    pid_t pid = bpf_get_current_pid_tgid() >> 32;

    // 获取 mmap 参数
    unsigned long addr;
    unsigned long len;
    unsigned long prot;
    unsigned long flags;
    unsigned long fd;
    unsigned long offset;

    bpf_probe_read_user(&addr, sizeof(addr), (void *)PT_REGS_PARM1(ctx));
    bpf_probe_read_user(&len, sizeof(len), (void *)PT_REGS_PARM2(ctx));
    bpf_probe_read_user(&prot, sizeof(prot), (void *)PT_REGS_PARM3(ctx));
    bpf_probe_read_user(&flags, sizeof(flags), (void *)PT_REGS_PARM4(ctx));
    bpf_probe_read_user(&fd, sizeof(fd), (void *)PT_REGS_PARM5(ctx));
    bpf_probe_read_user(&offset, sizeof(offset), (void *)PT_REGS_PARM6(ctx));

    // 输出信息到用户空间
    bpf_printk("PID %d: mmap called with addr=%lx, len=%lx, prot=%lx, flags=%lx, fd=%ld, offset=%lx",
               pid, addr, len, prot, flags, fd, offset);

    return 0;
}


/*!
 * \brief step2: 跟踪写入共享内存的操作
 * \details
 *      有2种方式
 *          方案1：通过 uprobe 监控用户空间写入行为
 *                  这种方式要知道用户空间写入的函数，然后用 uprobe钩子，在用户态dump数据
 *          方案2：通过 kprobe 监控 page fault 或写保护中断，这种方式更底层
 * */

// sudo bpftool uprobe add /path/to/app write_shmem_data ./my_bpf_prog.o:handle_uprobe
//SEC("uprobe//path/to/app:write_shmem_data")
//int handle_uprobe(struct pt_regs *ctx)
//{
//    void *shmem_addr = (void *)PT_REGS_PARM1(ctx);
//    char data[128];
//    bpf_probe_read_user(data, sizeof(data), shmem_addr);
//    bpf_printk("Shared memory written: %s", data);
//    return 0;
//}
/** 方案2 通过 kprobe 监控 page fault 或写保护中断 */
SEC("kprobe/do_wp_page")
int handle_do_wp_page(struct pt_regs *ctx)
{
    struct vm_area_struct *vma = (struct vm_area_struct *)PT_REGS_PARM1(ctx);
    unsigned long address = PT_REGS_PARM2(ctx);

    // 判断是否是共享内存
    if (!(vma->vm_flags & VM_SHARED))
        return 0;

    pid_t pid = bpf_get_current_pid_tgid() >> 32;
    struct mmap_info *info = bpf_map_lookup_elem(&mmap_map, &pid);
    if (!info)
        return 0;

    // 如果地址落在共享内存范围内，则 dump 内容
    if (address >= info->start && address < info->end) {
        char buf[128];
        bpf_probe_read_user(buf, sizeof(buf), (void *)address);
        bpf_printk("Write to shared memory: %s", buf);
    }


    // -----------------------------------------------------------------
    // Step 3：将 payload 数据传递给用户空间
    // 通知用户空间去读取数据
    char *data = bpf_ringbuf_reserve(&events, sizeof(payload), 0);
    if (!data)
        return 0;
    memcpy(data, payload, sizeof(payload));
    bpf_ringbuf_submit(data, 0);

    return 0;
}


char LICENSE[] SEC("license") = "GPL";