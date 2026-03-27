// SPDX-License-Identifier: GPL-2.0
// Copyright 2016 Netflix, Inc.
// 27-Mar-2025   ksight project   Ported to libbpf/CO-RE

#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_endian.h>

#include "tcpretrans.h"

#ifndef AF_INET
#define AF_INET     2
#endif

#ifndef AF_INET6
#define AF_INET6    10
#endif

// Filter arguments
const volatile bool targ_lossprobe = false;
const volatile bool targ_count = false;
const volatile int  targ_family = 0; // AF_INET, AF_INET6

struct {
    __uint(type, BPF_MAP_TYPE_HASH);
    __uint(max_entries, 10240);
    __type(key, struct flow_key);
    __type(value, u64);
} counts SEC(".maps");

struct {
    __uint(type, BPF_MAP_TYPE_PERF_EVENT_ARRAY);
    __uint(key_size, sizeof(u32));
    __uint(value_size, sizeof(u32));
} events SEC(".maps");

static __always_inline int trace_event(struct sock *sk, struct sk_buff *skb, int type, void *ctx)
{
    if (sk == NULL)
        return 0;

    u16 family = BPF_CORE_READ(sk, __sk_common.skc_family);
    if (targ_family && family != targ_family)
        return 0;

    struct flow_key key = {};
    if (family == AF_INET) {
        key.family = AF_INET;
        BPF_CORE_READ_INTO(&key.saddr, sk, __sk_common.skc_rcv_saddr);
        BPF_CORE_READ_INTO(&key.daddr, sk, __sk_common.skc_daddr);
    } else if (family == AF_INET6) {
        key.family = AF_INET6;
        BPF_CORE_READ_INTO(&key.saddr, sk, __sk_common.skc_v6_rcv_saddr);
        BPF_CORE_READ_INTO(&key.daddr, sk, __sk_common.skc_v6_daddr);
    } else {
        return 0;
    }
    key.lport = BPF_CORE_READ(sk, __sk_common.skc_num);
    key.dport = bpf_ntohs(BPF_CORE_READ(sk, __sk_common.skc_dport));

    if (targ_count) {
        u64 *val = bpf_map_lookup_elem(&counts, &key);
        if (val) {
            __sync_fetch_and_add(val, 1);
        } else {
            u64 initial_val = 1;
            bpf_map_update_elem(&counts, &key, &initial_val, BPF_ANY);
        }
    } else {
        struct event evt = {};
        evt.pid = bpf_get_current_pid_tgid() >> 32;
        evt.family = family;
        evt.ip = (family == AF_INET) ? 4 : 6;
        evt.type = type;
        evt.lport = key.lport;
        evt.dport = key.dport;
        __builtin_memcpy(evt.saddr, key.saddr, 16);
        __builtin_memcpy(evt.daddr, key.daddr, 16);
        evt.state = BPF_CORE_READ(sk, __sk_common.skc_state);
        bpf_get_current_comm(&evt.comm, sizeof(evt.comm));

        if (skb) {
            struct tcp_skb_cb *tcb = ((struct tcp_skb_cb *)&((skb)->cb[0]));
            evt.seq = BPF_CORE_READ(tcb, seq);
        }

        bpf_perf_event_output(ctx, &events, BPF_F_CURRENT_CPU, &evt, sizeof(evt));
    }

    return 0;
}

SEC("tracepoint/tcp/tcp_retransmit_skb")
int tcp_retransmit_skb_tp(struct trace_event_raw_tcp_event_sk_skb *args)
{
    return trace_event((struct sock *)args->skaddr, (struct sk_buff *)args->skbaddr, RETRANSMIT, args);
}

// Fallback kprobe for kernels without tracepoint or for lossprobe
SEC("kprobe/tcp_retransmit_skb")
int BPF_KPROBE(tcp_retransmit_skb_kp, struct sock *sk, struct sk_buff *skb)
{
    return trace_event(sk, skb, RETRANSMIT, ctx);
}

SEC("kprobe/tcp_send_loss_probe")
int BPF_KPROBE(tcp_send_loss_probe_kp, struct sock *sk)
{
    if (!targ_lossprobe)
        return 0;
    return trace_event(sk, NULL, TLP, ctx);
}

char LICENSE[] SEC("license") = "GPL";
