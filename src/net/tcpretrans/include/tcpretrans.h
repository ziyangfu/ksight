// SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */
#ifndef __TCPRETRANS_H
#define __TCPRETRANS_H

#ifdef __bpf__
// 不在这里包含 vmlinux.h，由 .bpf.c 包含
#else
#include <linux/types.h>
#endif

#define MAX_COMM_LEN 16

#ifndef RETRANSMIT
#define RETRANSMIT  1
#endif

#ifndef TLP
#define TLP         2
#endif

struct event {
    __u32 pid;
    __u32 seq;
    __u32 ip;
    __u8  saddr[16];
    __u8  daddr[16];
    __u16 lport;
    __u16 dport;
    __u16 family;
    __u32 state;
    __u32 type;
    char  comm[MAX_COMM_LEN];
};

struct flow_key {
    __u8  saddr[16];
    __u8  daddr[16];
    __u16 lport;
    __u16 dport;
    __u16 family;
};

#endif /* __TCPRETRANS_H */
