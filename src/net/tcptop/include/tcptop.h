#ifndef __TCPTOP_H
#define __TCPTOP_H

#ifdef __bpf__
#include <vmlinux.h>
#else
#include <linux/types.h>
#endif

#define TASK_COMM_LEN 16

struct ip_key_t {
  unsigned __int128 saddr;
  unsigned __int128 daddr;
  __u32 pid;
  char name[TASK_COMM_LEN];
  __u16 lport;
  __u16 dport;
  __u16 family;
};

struct traffic_t {
  __u64 sent;
  __u64 received;
};

#endif /* __TCPTOP_H */
