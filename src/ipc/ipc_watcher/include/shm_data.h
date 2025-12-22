/*!
 * \brief 共享内存eBPF内核态程序使用的结构体等定义
 * */

#ifndef IPC_IPC_WATCHER_SHM_DATA_H
#define IPC_IPC_WATCHER_SHM_DATA_H

#include "vmlinux.h"
#include <asm-generic/errno.h>
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_endian.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

#define MAP_SHARED	0x01		/* Share changes.  */
#define MAP_ANONYMOUS	0x20		/* don't use a file */

#define MAX_KERNEL_FUNCS 64  /** 最多64个函数记录 */

/**
 * 如何获得key：
 *      1. sys_enter_mmap，获得 pid + tid（或者仅tid），然后获得所有的输入参数，存入结构体
 *      2. sys_exit_mmap，根据pid + tid（或者仅tid），获得所有的输入参数，然后获得返回的虚拟地址，送入用户空间
 * 上述方案如何唯一标记一个共享内存区域？会不会有并发问题：
 *       不会，因为我们采用的tid线程ID，线程内部不会有并发问题，先进入mmap，必然先出mmap
 * */

/*!
 * \brief pid + mmap返回的vm起始地址，唯一标记一段内存区域
 * */
struct mmap_key {
    pid_t pid;
    unsigned long mmap_addr;   // mmap_addr + size == 共享内存区域VM
};

struct mmap_temp_key {
    pid_t pid;
    tid_t tid;
};

struct shm_mmap_enter_data {
    unsigned int pid;
    unsigned int tid;
    unsigned long len;
    unsigned long prot;
    unsigned long flags;
    unsigned long fd;
    unsigned long off;
};
/*!
 * \brief 内核态程序记录的共享内存运行脉络
 * */
struct shm_trace_data {
    unsigned int pid;
    unsigned int tid;
    int kernel_funcs_recode[MAX_KERNEL_FUNCS];
};


struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 1 << 24);  // 16MB ring buffer
} shm_events SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, struct mmap_temp_key*);
    __type(value, struct shm_mmap_enter_data);
} shm_temp_map SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 1024);
    __type(key, struct mmap_key*);
    __type(value, struct shm_mmap_enter_data);
} shm_map SEC(".maps");

#endif //IPC_IPC_WATCHER_SHM_DATA_H
