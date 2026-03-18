#! /usr/bin/env python3

from bcc import BPF

bpf_code = """
#include <uapi/linux/ptrace.h>
#include <linux/sched.h>

struct event_t {
    u64 pid;
    char data[128];
};

BPF_RINGBUF_OUTPUT(events, 1 << 12);

// Hook mmap 返回值
int handle_mmap_return(struct pt_regs *ctx) {
    pid_t pid = bpf_get_current_pid_tgid() >> 32;
    void *addr = (void *)PT_REGS_RC(ctx);
    u64 len = PT_REGS_PARM2(ctx);
    // 存储 mmap 映射信息
    //bpf_printk("PID %d mapped %lx with length %lx", pid, addr, len);
    printk("PID %d mapped %lx with length %lx", pid, addr, len);
    return 0;
}

// Hook 写入 mmap 区域的指令
int handle_shared_memory_write(struct pt_regs *ctx) {
    pid_t pid = bpf_get_current_pid_tgid() >> 32;
    void *addr = (void *)PT_REGS_PARM1(ctx);
    struct event_t evt = {};
    evt.pid = pid;
    bpf_probe_read_user(evt.data, sizeof(evt.data), addr);
    events.ringbuf_output(&evt, sizeof(evt), 0);
    return 0;
}
"""

# 加载 eBPF 程序
b = BPF(text=bpf_code)

# Attach to mmap syscall exit
b.attach_kretprobe(event="do_mmap", fn_name="handle_mmap_return")

# 假设你知道写入函数名，比如 "write_to_shmem"
b.attach_uprobe(name="/path/to/myapp", sym="write_to_shmem", fn_name="handle_shared_memory_write")

# 读取 ringbuf
def print_event(cpu, data, size):
    event = b["events"].event(data)
    print(f"PID {event.pid}: {event.data}")

b["events"].open_ring_buffer(print_event)

while True:
    try:
        b.ring_buffer_poll()
    except KeyboardInterrupt:
        break