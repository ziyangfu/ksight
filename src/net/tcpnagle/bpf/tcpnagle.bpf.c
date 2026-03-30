#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "tcpnagle_common.h"

char LICENSE[] SEC("license") = "GPL";

/**
 * Nagle 算法状态位定义:
 * TCP_NAGLE_OFF = 1
 * TCP_NAGLE_CORK = 2
 * 其余为 0 (Nagle Enabled)
 */

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} events SEC(".maps");

SEC("iter/tcp")
int tcpnagle_iter(struct bpf_iter__tcp *ctx) {
    struct sock_common *skc = ctx->sk_common;
    if (!skc) return 0;
    
    // 只处理 IPv4
    if (skc->skc_family != 2)
        return 0;

    struct tcp_sock *tp = bpf_skc_to_tcp_sock(skc);
    if (!tp)
        return 0;

    struct tcpnagle_event *event;
    event = bpf_ringbuf_reserve(&events, sizeof(*event), 0);
    if (!event)
        return 0;

    // 填充数据模型
    event->saddr = skc->skc_rcv_saddr;
    event->daddr = skc->skc_daddr;
    event->sport = skc->skc_num;
    event->dport = __builtin_bswap16(skc->skc_dport);
    event->nonagle = BPF_CORE_READ(tp, nonagle);
    event->inode = 0;
    
    // 获取 Inode 用于在用户态匹配 PID
    // 路径: sock -> sk_socket -> file -> f_inode -> i_ino
    struct sock *sk = (struct sock *)skc;
    struct socket *sock = BPF_CORE_READ(sk, sk_socket);
    if (sock) {
        struct file *file = BPF_CORE_READ(sock, file);
        if (file) {
            struct inode *inode = BPF_CORE_READ(file, f_inode);
            if (inode) {
                event->inode = BPF_CORE_READ(inode, i_ino);
            }
        }
    }

    bpf_ringbuf_submit(event, 0);
    return 0;
}
