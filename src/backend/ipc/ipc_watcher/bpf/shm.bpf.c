/*!
 * \brief 共享内存的追踪与观测
 * \file  shm.bpf.c
 * \details
 *      ARCH Support: X86-64, ARM64
 * */
#include "common.bpf.h"
#include "shm_trace.bpf.h"


/** FIXME： 匿名 文件共享映射， 如何获取它的addr，无path
 *
 * TODO，feature，物理地址/address_space地址，同一块共享内存，有多少个进程在映射
 * 有没有进程 mmap了，但没有munmap就退出了（只有所有映射结束，内存才能回收）
 * physical_addr    num         pids                    comms[opt]
 *  XXX - XXX        3       128922,212121,128923    /XXX, /xxx, /xxx
 *
 *  监控共享内存区域的变化，迭代的显示（迭代显示模式）
 *  当引用计数为0，但这块共享内存还在，那么这个共享内存就泄露了（悬空了）
 * */

/*!
 * 对于mmap映射来说，有如下4种映射方式
 *              共享              私有
 * 匿名页     父子进程通信        申请虚拟内存
 * 文件页        IPC            页高速缓存 page_cache
 * 因此，对于共享内存，只需要关注文件共享映射，不过，文件共享映射
 * 中的文件，有2种，一种是显性的文件，通过open，shm_open等系统
 * 调用创建，在文件系统中有关联的inode。另一种的匿名文件，一般用过
 * memfd_create创建，也可以通过open，加上flag O_TMPFILE创建，
 *
 * int shm_fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
 *
 * int fd = open("/path/to/dir", O_TMPFILE | O_RDWR,
                                          S_IRUSR | S_IWUSR）
 *
 *  1. 匿名共享内存
    2. 文件共享内存
    3. 支持共享内存头的自定义，以一种格式，例如yaml或json
    4. 零拷贝调试与追踪

1. 帮助理解Linux内核共享内存机制
2. 观测共享内存的数据流，无侵入、以一种无需修改代码与编译、无需源码的方式，只要用共享内存，就能捕捉到数据流（可能是加密数据，提供yaml配置选项）
3. 共享内存的基本数据观测，通用工具也能做到，不局限于使用eBPF在内核中采集数据，可能在用户态，或者直接使用通用工具，获得数据
4. 开发共享内存IPC的应用或中间件时的调试辅助，例如：共享内存的无侵入式内存泄露检测



异构共享，异构多核（A-M核），CPU-GPU共享观测
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

 如果追踪共享内存的改变困难，可以考虑追踪同步机制

 提供2种方式
 1. 内核追踪
 2. uprobe机制，方便那些知道访问了共享内存的函数的程序


ipc_watcher 需要先启动，不然追踪不到mmap，也就拿不到虚拟地址
在第一次写入时，服务端与客户端都会触发缺页异常
 后续就直接读取映射区


如果是共享文件映射的话
    共享一块页高速缓存 page cache


 1. 捕获共享内存的基本数据
    pid， comm， flag（MAP_SHARED）,Addr, len, prot(读写权限), path(通过fd->file->path)
 2. 过滤PID，用户空间创建一个共享内存监视器， mmap映射同一个块映射区，设置权限为只读。
 3. 追踪共享内存缺页异常，pte填充物理内存，再到映射同一块address_space(page_cache)，是否可以打印出物理内存的地址？
 4. 如何捕获共享内存中的数据？ hook几种常用的同步机制？ uds或者二元信号量？
 5. uprobe捕获基本数据与共享内存区数据包

 *
 * */

/**
 * 共享内存计数器，目标为检测共享内存的内存泄露问题
 *
 * 实现共享内存的快照，获取共享内存内存，用于调试数据变化、同步异常等问题
 *
 * 自动清理机制（opt），对于内存泄露的端，调用 shm_unlink进行清理。
 *
 *
 * 获取page cache地址，打印内存连接图
类似于下面这种：
 pid， fd， vm_addr...                              pid， fd， vm_addr
                   \                               /
                    \_______ page cache addr  ____/
                    /                             \
 pid， fd， vm_addr /                               \   pid， fd， vm_addr

 *
 * */
#define MAP_SHARED	0x01		/* Share changes.  */
#define MAP_ANONYMOUS	0x20		/* don't use a file */

/*!
 * \brief 使用pid+fd作为key，不能够唯一标记一个共享内存，当使用多次映射时
 * \details  pid + mmap返回的vm起始地址，也可以标记一段内存区域
 * */
struct mmap_key {
    pid_t pid;
    unsigned long fd;
};

struct mmap_key_new {
    pid_t pid;
    unsigned long mmap_addr;   // mmap_addr + size == 共享内存区域VM
};

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24);  // 16MB ring buffer
} shm_events SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, struct mmap_key*);
    __type(value, struct shm_basic_info_event);
} shm_basic_info_map SEC(".maps");


/*!
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
     // bpf_printk("for trace mmap");
     return 0;
}
/*!
 * \brief
 * \details 所有的mmap都要经过ksys_mmap_pgoff，流量很大，要注意性能影响
 *          立即排除 私有匿名映射、私有文件映射与匿名文件映射
 * */
SEC("kprobe/ksys_mmap_pgoff")
int BPF_KPROBE(ksys_mmap_pgoff, unsigned long addr, unsigned long len,
               unsigned long prot, unsigned long flags,
               unsigned long fd) {
    bool is_shared = flags & MAP_SHARED;  /** 排除 私有匿名映射、私有文件映射 */
    if (!is_shared) {
        return 0;
     }
    bool is_anonymous = flags & MAP_ANONYMOUS; /** 排除匿名文件映射 */
    if (is_anonymous) {
        return 0;
    }
//    struct mmap_key key = {
//        .pid = bpf_get_current_pid_tgid() >> 32,
//        .fd = fd
//    };
//    struct shm_basic_info_event info = {0};
//    struct shm_basic_info_event* info_ptr;
//    info_ptr = (struct shm_basic_info_event*)bpf_map_lookup_or_try_init(&shm_basic_info_map, &key, &info);
//    if (info_ptr == NULL) {
//        return 0;
//    }
//    info_ptr->pid  = bpf_get_current_pid_tgid() >> 32;
//
//    if (bpf_get_current_comm(&info_ptr->comm, sizeof(info_ptr->comm)) != 0) {
//        bpf_map_delete_elem(&shm_basic_info_map, &key);
//    }
//    info_ptr->fd = fd;
//    info_ptr->flag = flags;
//    info_ptr->len = len;
//    info_ptr->prot = prot;
//
//    info_ptr->timestamp = bpf_ktime_get_ns() / 1000;


    // 向 ringbuf 提交数据
    struct shm_basic_info_event* trans_rb_event =
            bpf_ringbuf_reserve(&shm_events, sizeof(struct shm_basic_info_event), 0);
    if (!trans_rb_event) {
        return 0;
    }
    trans_rb_event->timestamp = bpf_ktime_get_ns() / 1000;
    trans_rb_event->pid = bpf_get_current_pid_tgid() >> 32;
    trans_rb_event->fd = fd;
    trans_rb_event->flag = flags;
    trans_rb_event->len = len;
    trans_rb_event->prot = prot;

    if (bpf_get_current_comm(&trans_rb_event->comm, sizeof(trans_rb_event->comm)) != 0) {
        //bpf_map_delete_elem(&shm_basic_info_map, &key);
    }
    bpf_ringbuf_submit(trans_rb_event, 0);

    // 如果是匿名文件共享映射，则没有文件路径path，显示 anoy
    return 0;
}

/*!
 * \brief Step 1：监控 mmap 调用，记录共享内存地址范围
 * \details 记录都谁调用了mmap，记录下pid，每个进程的虚拟内存地址是不同的，
 *          所以修改同步记录pid与对应的虚拟内存地址(起始地址与size)，使用hash表
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
    // bpf_printk("trace mmap exit");
//    struct mmap_key key = {
//        .pid = bpf_get_current_pid_tgid() >> 32,
//        .fd = 0
//    };
//    struct shm_basic_info_event* event = bpf_map_lookup_elem(&shm_basic_info_map, &key);
//
//
//    // 向 ringbuf 提交数据
//    struct shm_basic_info_event* trans_rb_event =
//            bpf_ringbuf_reserve(&shm_events, sizeof(struct shm_basic_info_event), 0);
//    if (!trans_rb_event) {
//        return 0;
//    }
//    trans_rb_event->pid = event->pid;
//    trans_rb_event->addr = BPF_CORE_READ(ctx, ret);
//
//    bpf_ringbuf_submit(trans_rb_event, 0);
    return 0;
}


/*!
 * \brief 针对匿名文件共享映射，memfd_create
 *
 * pid，+ path， fd， struct file*
 *
 *  memfd_create
 *  mmap
 *
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
//SEC("kprobe/do_wp_page")
//int handle_do_wp_page(struct pt_regs *ctx)
//{
//    struct vm_area_struct *vma = (struct vm_area_struct *)PT_REGS_PARM1(ctx);
//    unsigned long address = PT_REGS_PARM2(ctx);
//
//    // 判断是否是共享内存
//    if (!(vma->vm_flags & VM_SHARED))
//        return 0;
//
//    pid_t pid = bpf_get_current_pid_tgid() >> 32;
//    struct mmap_info *info = bpf_map_lookup_elem(&mmap_map, &pid);
//    if (!info)
//        return 0;
//
//    // 如果地址落在共享内存范围内，则 dump 内容
//    if (address >= info->start && address < info->end) {
//        char buf[128];
//        bpf_probe_read_user(buf, sizeof(buf), (void *)address);
//        bpf_printk("Write to shared memory: %s", buf);
//    }
//
//
//    // -----------------------------------------------------------------
//    // Step 3：将 payload 数据传递给用户空间
//    // 通知用户空间去读取数据
//    char *data = bpf_ringbuf_reserve(&events, sizeof(payload), 0);
//    if (!data)
//        return 0;
//    memcpy(data, payload, sizeof(payload));
//    bpf_ringbuf_submit(data, 0);
//
//    return 0;
//}


char LICENSE[] SEC("license") = "GPL";