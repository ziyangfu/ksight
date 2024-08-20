// Copyright 2023 The LMP Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://github.com/linuxkerneltravel/lmp/blob/develop/LICENSE
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// author: blown.away@qq.com
//
// netwatcher libbpf 内核<->用户 传递信息相关结构体

#ifndef __NETWATCHER_H
#define __NETWATCHER_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#define ETH_P_IP 0x0800   /* Internet Protocol packet	*/
#define ETH_P_IPV6 0x86DD /* IPv6 over bluebook		*/
#define MAX_SLOTS 27

#ifndef AF_INET
#define AF_INET 2
#endif

#ifndef AF_INET6
#define AF_INET6 10 /* IP version 6	*/
#endif

#define TCP_SKB_CB(__skb) ((struct tcp_skb_cb *)&((__skb)->cb[0]))

#define MAX_COMM 16
#define TCP 1
#define UDP 2

struct conn_t {
    void *sock;          // 此tcp连接的 socket 地址
    int pid;             // pid
    u64 ptid;            // 此tcp连接的 ptid(ebpf def)
    char comm[MAX_COMM]; // 此tcp连接的 command
    u16 family;          // 10(AF_INET6):v6 or 2(AF_INET):v4
    unsigned __int128 saddr_v6;
    unsigned __int128 daddr_v6;
    u32 saddr;
    u32 daddr;
    u16 sport;
    u16 dport;
    int is_server; // 1: 被动连接 0: 主动连接

    u32 tcp_backlog;     // backlog
    u32 max_tcp_backlog; // max_backlog
    u64 bytes_acked;     // 已确认的字节数
    u64 bytes_received;  // 已接收的字节数

    u32 snd_cwnd;       // 拥塞窗口大小
    u32 rcv_wnd;        // 接收窗口大小
    u32 snd_ssthresh;   // 慢启动阈值
    u32 sndbuf;         // 发送缓冲区大小(byte)
    u32 sk_wmem_queued; // 已使用的发送缓冲区
    u32 total_retrans;  // 重传包数
    u32 fastRe;         // 快速重传次数
    u32 timeout;        // 超时重传次数

    u32 srtt;           // 平滑往返时间
    u64 init_timestamp; // 建立连接时间戳
    u64 duration;       // 连接已建立时长
};

#define MAX_PACKET 1000
#define MAX_HTTP_HEADER 256
#define NUM_LAYERS 5

struct pack_t {
    int err;      // no err(0) invalid seq(1) invalid checksum(2)
    u64 mac_time; // mac layer 处理时间(us)
    u64 ip_time;  // ip layer 处理时间(us)
    // u64 tcp_time; // tcp layer 处理时间(us)
    u64 tran_time;            // tcp layer 处理时间(us)
    u32 seq;                  // the seq num of packet
    u32 ack;                  // the ack num of packet
    u8 data[MAX_HTTP_HEADER]; // 用户层数据
    const void *sock;         // 此包tcp连接的 socket 指针
    int rx;                   // rx packet(1) or tx packet(0)
    u32 saddr;
    u32 daddr;
    unsigned __int128 saddr_v6;
    unsigned __int128 daddr_v6;
    u16 sport;
    u16 dport;
};

struct udp_message {
    u32 saddr;
    u32 daddr;
    u16 sport;
    u16 dport;
    u64 tran_time;
    int rx;
    int len;
};
struct netfilter {
    u32 saddr;
    u32 daddr;
    u16 sport;
    u16 dport;
    u64 local_input_time;
    u64 pre_routing_time;
    u64 forward_time;
    u64 local_out_time;
    u64 post_routing_time;
    u32 rx;
};
struct reasonissue {
    u32 saddr;
    u32 daddr;
    u16 sport;
    u16 dport;
    long location;
    u16 protocol;
    int drop_reason;
};
struct icmptime {
    unsigned int saddr;
    unsigned int daddr;
    unsigned long long icmp_tran_time;
    unsigned int flag; // 0 send 1 rcv
};

struct tcp_state {
    u32 saddr;
    u32 daddr;
    u16 sport;
    u16 dport;
    int oldstate;
    int newstate;
    u64 time;
};

struct dns_information {
    u32 saddr;
    u32 daddr;
    u16 id;
    u16 flags;
    u16 qdcount;
    u16 ancount;
    u16 nscount;
    u16 arcount;
    char data[64];
    int rx;
    int response_count;
    int request_count;
};
#define MAX_STACK_DEPTH 128
typedef u64 stack_trace_t[MAX_STACK_DEPTH];
struct stacktrace_event {
    u32 pid;
    u32 cpu_id;
    char comm[16];
    signed int kstack_sz;
    signed int ustack_sz;
    stack_trace_t kstack;
    stack_trace_t ustack;
};

typedef struct mysql_query {
    int pid;
    int tid;
    char comm[20];
    u32 size;
    char msql[256];
    u64 duratime;
    int count;
} mysql_query;

struct redis_query {
    int pid;
    int tid;
    char comm[20];
    u32 size;
    char redis[4][8];
    u64 duratime;
    int count;
    u64 begin_time;
    int argc;
};

struct RTT {
    u32 saddr;
    u32 daddr;
    u64 slots[64];
    u64 latency;
    u64 cnt;
};

struct reset_event_t {
    int pid;
    char comm[16];
    u16 family;
    unsigned __int128 saddr_v6;
    unsigned __int128 daddr_v6;
    u32 saddr;
    u32 daddr;
    u16 sport;
    u16 dport;
    u8 direction; // 0 for send, 1 for receive
    u64 count;
    u64 timestamp;
    u8 state;
};
#endif /* __NETWATCHER_H */