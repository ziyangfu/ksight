/*!
	\brief 用于跟踪POSIX SHM 的底层运行链路
    \details 仅限于验证学习，里面有很多热点函数，可能会拖慢系统运行性能
*/

/**
 *
 * 跟踪某个进程
 * */

/*!
 * \brief 针对匿名文件共享映射，memfd_create
 * */
SEC("tracepoint/syscalls/sys_enter_memfd_create")
int handle_memfd_create(struct trace_event_raw_sys_enter *ctx)
{
    /** 获取当前pid */
    /** 获取name与flag */
    return 0;
}

SEC("tracepoint/syscalls/sys_enter_ftruncate")
int handle_ftruncate_enter(struct trace_event_raw_sys_enter *ctx) {
    return 0;
}


SEC("kprobe/__do_fault")
int get_page_cache(struct vm_fault *vmf) {
    return 0;
}



//SEC("tracepoint/sys_enter_mmap")
//inline int tracepoint__sys_enter_mmap(struct trace_event_raw_sys_enter *ctx)
//{
//    return 0;
//}


SEC("tracepoint/syscalls/sys_enter_munmap")
int tp__sys_enter_munmap(struct trace_event_raw_sys_enter *ctx)
{
    return 0;
}

SEC("kprobe/__vm_munmap")
int kprobe__vm_munmap(struct pt_regs *ctx)
{
    return 0;
}

SEC("kprobe/__do_munmap")
int kprobe__do_munmap(struct pt_regs *ctx)
{
    return 0;
}

SEC("kprobe/unmap_region")
int kprobe__unmap_region(struct pt_regs *ctx)
{
    return 0;
}

/*!
 * \brief 解除 VMA 与物理页之间的映射
 * */
SEC("kprobe/unmap_vmas")
int kprobe__unmap_vmas(struct pt_regs *ctx)
{
    return 0;
}

SEC("kprobe/unmap_single_vma")
int kprobe__unmap_single_vma(struct pt_regs *ctx)
{
    return 0;
}

SEC("kprobe/unmap_page_range")
int kprobe__unmap_page_range(struct pt_regs *ctx)
{
    return 0;
}

//SEC("kprobe/zap_pte_range")
SEC("kprobe/zap_pte_range.isra.0")
int kprobe__zap_pte_range(struct pt_regs *ctx)
{
    return 0;
}


/*!
 * \brief 释放页表, 在 unmap_region中调用
 *  释放指定虚拟内存区域（vma）的页表，并优化相邻区域的页表释放操作
 * */
SEC("kprobe/free_pgtables")
int kprobe__free_pgtables(struct pt_regs *ctx)
{
    return 0;
}


SEC("tracepoint/syscalls/sys_exit_munmap")
int tp__sys_exit_munmap(struct trace_event_raw_sys_exit *ctx)
{
    return 0;
}


SEC("tracepoint/syscalls/sys_enter_open")
int tp__sys_enter_open(struct trace_event_raw_sys_enter *ctx)
{
    return 0;
}