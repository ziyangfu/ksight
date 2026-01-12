/*!
 * \brief 用于TCP在本地通信中加速的守护进程
 * \file： tcpaccd.bpf.c
 * \details
 *  1. 握手劫持 (sockops)： 利用 eBPF 的 sockops 程序在 TCP 三次握手期间拦截连接请求
 *  2. 数据流重定向 (sockmap)： 使用 eBPF 的 sockmap 功能，结合 BPF_SK_MSG_VERDICT 钩子，
 *      将数据包从发送方的 sendmsg() 直接重定向到接收方的 socket，从而实现通信通道的建立
 * 相关的文章：字节跳动：Transparent Shared Memory Communications with eBPF
 * 更多：
 * bpf arenas, Linux kernel 6.9 内核中引入的
 * BPF Arena 允许 eBPF 程序和用户态应用程序 共享一块公共的内存区域，并允许在这块内存中直接使用
 * 传统的 C 语言指针进行读写
 * 见：A look at what‘s possible with BPF arenas
*/


#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_endian.h>

#define MAX_ENTRIES 65535

struct {
    __uint(type, BPF_MAP_TYPE_SOCKHASH);
    __uint(key_size, sizeof(__u32) * 4 + sizeof(__u16) * 2);  // ipv4 + ports
    __uint(value_size, sizeof(int));
    __uint(max_entries, MAX_ENTRIES);
} sock_ops_map SEC(".maps");

// sock_key 结构定义
struct sock_key {
    __u32 sip4;
    __u32 dip4;
    __u8  family;
    __u8  pad1;
    __u16 pad2;
    __u32 pad3;
    __u32 sport;
    __u32 dport;
} __attribute__((packed));

/*
 * extract the key that identifies the destination socket in the sock_ops_map
 */
static inline
void extract_key4_from_msg(struct sk_msg_md *msg, struct sock_key *key)
{
    key->sip4 = msg->remote_ip4;
    key->dip4 = msg->local_ip4;
    key->family = 1;

    key->dport = (bpf_htonl(msg->local_port) >> 16);
    key->sport = msg->remote_port >> 16;
}

SEC("sk_msg")
int bpf_redir(struct sk_msg_md *msg)
{
    struct sock_key key = {};
    extract_key4_from_msg(msg, &key);
    long ret = bpf_msg_redirect_hash(msg, &sock_ops_map, &key, BPF_F_INGRESS);
    return SK_PASS;
}

/*
 * Socket operations program - handles connection establishment
 */
static inline
void extract_key4_from_ops(struct bpf_sock_ops *ops, struct sock_key *key)
{
    key->dip4 = ops->remote_ip4;
    key->sip4 = ops->local_ip4;
    key->family = 1;

    key->sport = (bpf_htonl(ops->local_port) >> 16);
    key->dport = ops->remote_port >> 16;
}

static inline
void bpf_sock_ops_ipv4(struct bpf_sock_ops *skops)
{
    struct sock_key key = {};
    int ret;

    extract_key4_from_ops(skops, &key);

    ret = bpf_sock_hash_update(skops, &sock_ops_map, &key, BPF_NOEXIST);
    if (ret) {
        bpf_printk("sock_hash_update() failed, ret: %d\n", ret);
    }

    bpf_printk("sockmap: op %d, port %d --> %d\n",
               skops->op, skops->local_port, bpf_ntohl(skops->remote_port));
}

SEC("sockops")
int bpf_sockmap(struct bpf_sock_ops *skops)
{
    switch (skops->op) {
        case BPF_SOCK_OPS_PASSIVE_ESTABLISHED_CB:
        case BPF_SOCK_OPS_ACTIVE_ESTABLISHED_CB:
            if (skops->family == 2) { // AF_INET
                bpf_sock_ops_ipv4(skops);
            }
            break;
        default:
            break;
    }
    return 0;
}

char LICENSE[] SEC("license") = "GPL";





// func：强制使用mptcp？
// 参考：https://github.com/iovisor/bcc/blob/master/tools/mptcpify.py


































// #include "vmlinux.h"
// #include <bpf/bpf_helpers.h>
// #include <bpf/bpf_tracing.h>

// #define TASK_COMM_LEN 16
// #define AF_INET 2
// #define AF_INET6 10
// #define SOCK_STREAM 1
// #define IPPROTO_TCP 6
// #define IPPROTO_MPTCP 262

// // 定义应用名称结构体
// struct app_name {
//     char name[TASK_COMM_LEN];
// };

// // 定义 Map：工作模式
// struct {
//     __uint(type, BPF_MAP_TYPE_ARRAY);
//     __uint(max_entries, 1);
//     __type(key, int);
//     __type(value, int);
// } work_mode SEC(".maps");

// // 定义 Map：目标应用列表
// struct {
//     __uint(type, BPF_MAP_TYPE_HASH);
//     __uint(max_entries, 1024);
//     __type(key, struct app_name);
//     __type(value, int);
// } support_apps SEC(".maps");

// SEC("fmod_ret/update_socket_protocol")
// int BPF_PROG(update_socket_protocol, int family, int type, int protocol, int ret)
// {
//     struct app_name target = {};
//     int index = 0;
    
//     // 获取当前进程名
//     bpf_get_current_comm(&target.name, sizeof(target.name));

//     int *mode = bpf_map_lookup_elem(&work_mode, &index);
    
//     // 核心逻辑判断
//     if ((family == AF_INET || family == AF_INET6) &&
//         type == SOCK_STREAM &&
//         (protocol == 0 || protocol == IPPROTO_TCP)) {
        
//         // 如果是全系统模式 (mode == 0) 或者在名单中
//         if ((mode && *mode == 0) || bpf_map_lookup_elem(&support_apps, &target)) {
//             return IPPROTO_MPTCP;
//         }
//     }

//     return protocol;
// }

// char LICENSE[] SEC("license") = "GPL";