/* SPDX-License-Identifier: (LGPL-2.1 OR BSD-2-Clause) */
#ifndef __SOFDSNOOP_H
#define __SOFDSNOOP_H

#ifdef __bpf__
#include <vmlinux.h>
#else
#include <linux/types.h>
#endif

#define TASK_COMM_LEN 16
#define MAX_FD        10

#define ACTION_SEND   0
#define ACTION_RECV   1

struct event {
    __u64 id;        /* pid_tgid: high=pid, low=tid */
    __u64 ts;        /* timestamp in us */
    int   action;    /* ACTION_SEND or ACTION_RECV */
    int   sock_fd;   /* socket fd used for sendmsg/recvmsg */
    int   fd_cnt;    /* number of FDs in this batch */
    int   fd[MAX_FD];
    char  comm[TASK_COMM_LEN];
};

#endif /* __SOFDSNOOP_H */
