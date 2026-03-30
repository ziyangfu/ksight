#ifndef NET_TCP_NAGLE_COMMON_H
#define NET_TCP_NAGLE_COMMON_H

#ifndef TASK_COMM_LEN
#define TASK_COMM_LEN 16
#endif

// BPF 传向用户态的事件结构
struct tcpnagle_event {
    unsigned int saddr;
    unsigned int daddr;
    unsigned short sport;
    unsigned short dport;
    unsigned char nonagle;
    unsigned long long inode;
};

#endif // NET_TCP_NAGLE_COMMON_H
