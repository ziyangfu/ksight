#ifndef __SOLISTEN_H
#define __SOLISTEN_H

#ifdef __bpf__
#include <vmlinux.h>
#else
#include <linux/types.h>
#endif

#define TASK_COMM_LEN 16

struct event {
  __u32 addr[4];
  __u32 pid;
  __u32 proto;
  int backlog;
  int ret;
  __u16 port;
  char task[TASK_COMM_LEN];
};

#endif /* __SOLISTEN_H */
