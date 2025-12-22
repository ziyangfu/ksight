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
