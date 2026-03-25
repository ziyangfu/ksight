#ifndef __TCPRTT_H
#define __TCPRTT_H

#ifdef __bpf__
#include <vmlinux.h>
#else
#include <linux/types.h>
#endif

#define MAX_SLOTS 27
#define IPV6_LEN 16

struct hist {
  __u64 latency;
  __u64 cnt;
  __u32 slots[MAX_SLOTS];
};

struct hist_key {
  __u16 family;
  __u8 addr[IPV6_LEN];
};

#endif /* __TCPRTT_H */
