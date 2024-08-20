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

#include "common.bpf.h"
/*
in_ipv4:
    kprobe/eth_type_trans
    kprobe/ip_rcv_core.isra.0
    kprobe/tcp_v4_rcv
    kprobe/tcp_v4_do_rcv
    kprobe/skb_copy_datagram_iter

in_ipv6:
    kprobe/eth_type_trans
    kprobe/ip6_rcv_core.isra.0
    kprobe/tcp_v6_rcv
    kprobe/tcp_v6_do_rcv
    kprobe/skb_copy_datagram_iter
*/
static __always_inline
int __eth_type_trans(struct sk_buff *skb)
{
    const struct ethhdr *eth =
        (struct ethhdr *)BPF_CORE_READ(skb, data); // 读取里面的报文数据
    u16 protocol = BPF_CORE_READ(eth, h_proto);    // 读取包ID
    // bpf_printk("protocol: %d\n", __bpf_ntohs(protocol));
    if (protocol == __bpf_htons(ETH_P_IP)) { // Protocol is IP  0x0800
        // 14 --> sizeof(struct ethhdr)   / define
        struct iphdr *ip =
            (struct iphdr *)(BPF_CORE_READ(skb, data) +
                             14); // 链路层头部长度为14 源端口6字节
                                  // 目的端口6字节 类型2字节
        struct tcphdr *tcp = (struct tcphdr *)(BPF_CORE_READ(skb, data) +
                                               sizeof(struct iphdr) + 14);
        struct packet_tuple pkt_tuple = {0}; // 声明packet_tuple结构pkt_tuple
        get_pkt_tuple(&pkt_tuple, ip, tcp);  // 初始化pkt_tuple

        struct ktime_info *tinfo, zero = {0}; // 定义ktime_info结构zero以及tinfo

        tinfo = (struct ktime_info *)bpf_map_lookup_or_try_init(
            &timestamps, &pkt_tuple, &zero);
        if (tinfo == NULL) { // 初始化失败
            // bpf_printk("v4 rx tinfo init fail.\n");
            return 0;
        }
        // 成功则获取当前内核时间并转换成毫秒
        tinfo->mac_time = bpf_ktime_get_ns() / 1000;
        // bpf_printk("v4 rx init.\n");
    } else if (protocol == __bpf_htons(ETH_P_IPV6)) { // Protocol is IPV6
        struct ipv6hdr *ip6h =
            (struct ipv6hdr *)(BPF_CORE_READ(skb, data) + 14);
        struct tcphdr *tcp = (struct tcphdr *)(BPF_CORE_READ(skb, data) +
                                               sizeof(struct ipv6hdr) + 14);
        struct packet_tuple pkt_tuple = {0};
        get_pkt_tuple_v6(&pkt_tuple, ip6h, tcp);

        struct ktime_info *tinfo, zero = {0};

        tinfo = (struct ktime_info *)bpf_map_lookup_or_try_init(
            &timestamps, &pkt_tuple, &zero);
        if (tinfo == NULL) {
            // bpf_printk("v6 rx tinfo init fail.\n");
            return 0;
        }
        tinfo->mac_time = bpf_ktime_get_ns() / 1000;
        // bpf_printk("v6 rx init.\n");
    }
    return 0;
}

static __always_inline
int __ip_rcv_core(struct sk_buff *skb)
{
    if (!layer_time) {
        return 0;
    }
    if (skb == NULL)
        return 0;
    struct iphdr *ip = skb_to_iphdr(skb);    // 通过skb获取ipv4包头信息
    struct tcphdr *tcp = skb_to_tcphdr(skb); // 获取tcp包头信息
    struct packet_tuple pkt_tuple = {
        0}; // 定义一个packet_tuple结构体变量pkt_tuple并初始化
    get_pkt_tuple(&pkt_tuple, ip, tcp);
    struct ktime_info *tinfo;
    tinfo = bpf_map_lookup_elem(
        &timestamps, &pkt_tuple); // 在timestamps映射中查找元素pkt_tuple
    if (tinfo == NULL) {
        return 0;
    }
    tinfo->ip_time = bpf_ktime_get_ns() / 1000;
    // bpf_printk("rx enter ipv4 layer.\n");
    return 0;
}

static __always_inline
int __ip6_rcv_core( struct sk_buff *skb)
{
    if (!layer_time) {
        return 0;
    }
    if (skb == NULL)
        return 0;
    struct ipv6hdr *ip6h = skb_to_ipv6hdr(skb);
    struct tcphdr *tcp = skb_to_tcphdr(skb);
    struct packet_tuple pkt_tuple = {0};
    get_pkt_tuple_v6(&pkt_tuple, ip6h, tcp);

    struct ktime_info *tinfo;
    tinfo = bpf_map_lookup_elem(&timestamps, &pkt_tuple);
    if (tinfo == NULL) {
        return 0;
    }

    tinfo->ip_time = bpf_ktime_get_ns() / 1000;
    // bpf_printk("rx enter ipv6 layer.\n");
    return 0;
}
static __always_inline
int __tcp_v4_rcv(struct sk_buff *skb)
{
    if (!layer_time) {
        return 0;
    }
    if (skb == NULL)
        return 0;
    struct iphdr *ip = skb_to_iphdr(skb);
    struct tcphdr *tcp = skb_to_tcphdr(skb);
    struct packet_tuple pkt_tuple = {0};
    get_pkt_tuple(&pkt_tuple, ip, tcp);
    struct ktime_info *tinfo;
    tinfo = bpf_map_lookup_elem(&timestamps, &pkt_tuple);
    if (tinfo == NULL) {
        return 0;
    }
    tinfo->tran_time = bpf_ktime_get_ns() / 1000;
    // bpf_printk("rx enter tcp4 layer.\n");
    return 0;
}
static __always_inline
int __tcp_v6_rcv(struct sk_buff *skb)
{
    if (!layer_time) {
        return 0;
    }
    if (skb == NULL)
        return 0;
    struct ipv6hdr *ip6h = skb_to_ipv6hdr(skb);
    struct tcphdr *tcp = skb_to_tcphdr(skb);
    struct packet_tuple pkt_tuple = {0};
    get_pkt_tuple_v6(&pkt_tuple, ip6h, tcp);

    struct ktime_info *tinfo;
    tinfo = bpf_map_lookup_elem(&timestamps, &pkt_tuple);
    if (tinfo == NULL) {
        return 0;
    }
    tinfo->tran_time = bpf_ktime_get_ns() / 1000;
    // bpf_printk("rx enter tcp6 layer.\n");
    return 0;
}
static __always_inline
int __tcp_v4_do_rcv(struct sock *sk,struct sk_buff *skb)
{
    if (sk == NULL || skb == NULL)
        return 0;
    struct conn_t *conn = bpf_map_lookup_elem(&conns_info, &sk);
    if (conn == NULL) {
        // bpf_printk("get a v4 rx pack but conn not record, its sock is:
        // %p",sk);
        return 0;
    }
    struct iphdr *ip = skb_to_iphdr(skb);
    struct tcphdr *tcp = skb_to_tcphdr(skb);
    struct packet_tuple pkt_tuple = {0};
    get_pkt_tuple(&pkt_tuple, ip, tcp);

    struct ktime_info *tinfo;
    tinfo = bpf_map_lookup_elem(&timestamps, &pkt_tuple);
    if (tinfo == NULL) {
        return 0;
    }

    CONN_INFO_TRANSFER

    // bpf_printk("rx enter tcp4_do_rcv, sk: %p \n", sk);

    CONN_ADD_EXTRA_INFO

    return 0;
}
static __always_inline
int __tcp_v6_do_rcv(struct sock *sk, struct sk_buff *skb)
{
    if (sk == NULL || skb == NULL)
        return 0;
    // bpf_printk("rx enter tcp6_do_rcv. \n");
    struct conn_t *conn = bpf_map_lookup_elem(&conns_info, &sk);
    if (conn == NULL) {
        // bpf_printk("get a v6 rx pack but conn not record, its sock is: %p",
        // sk);
        return 0;
    }

    struct ipv6hdr *ip6h = skb_to_ipv6hdr(skb);
    struct tcphdr *tcp = skb_to_tcphdr(skb);
    struct packet_tuple pkt_tuple = {0};
    get_pkt_tuple_v6(&pkt_tuple, ip6h, tcp); // 使用ip和tcp信息填充pkt_tuple

    struct ktime_info *tinfo;
    tinfo = bpf_map_lookup_elem(&timestamps, &pkt_tuple);
    if (tinfo == NULL) {
        return 0;
    }

    CONN_INFO_TRANSFER

    // bpf_printk("rx enter tcp6_do_rcv, sk: %p \n", sk);

    CONN_ADD_EXTRA_INFO

    return 0;
}
static __always_inline
int __skb_copy_datagram_iter(struct sk_buff *skb)
{
    if (skb == NULL)
        return 0;
    __be16 protocol = BPF_CORE_READ(skb, protocol); // 读取skb协议字段
    struct tcphdr *tcp = skb_to_tcphdr(skb);
    struct packet_tuple pkt_tuple = {0};
    struct ktime_info *tinfo;
    if (protocol == __bpf_htons(ETH_P_IP)) { /** ipv4 */

        struct iphdr *ip = skb_to_iphdr(skb);
        get_pkt_tuple(&pkt_tuple, ip, tcp);
        tinfo = bpf_map_lookup_elem(&timestamps, &pkt_tuple);
        if (tinfo == NULL) {
            return 0;
        }

        tinfo->app_time = bpf_ktime_get_ns() / 1000;
    } else if (protocol == __bpf_ntohs(ETH_P_IPV6)) {
        /** ipv6 */
        struct ipv6hdr *ip6h = skb_to_ipv6hdr(skb);
        get_pkt_tuple_v6(&pkt_tuple, ip6h, tcp);

        if ((tinfo = bpf_map_lookup_elem(&timestamps, &pkt_tuple)) == NULL) {
            return 0;
        }
        tinfo->app_time = bpf_ktime_get_ns() / 1000;
    } else {
        return 0;
    }
    /*----- record packet time info ------*/

    if (tinfo == NULL) {
        return 0;
    }
    struct sock *sk = tinfo->sk;
    if (sk == NULL) {
        return 0;
    }
    // bpf_printk("rx enter app layer.\n");

    PACKET_INIT_WITH_COMMON_INFO
    packet->saddr = pkt_tuple.saddr;
    packet->daddr = pkt_tuple.daddr;
    packet->sport = pkt_tuple.sport;
    packet->dport = pkt_tuple.dport;

    if (layer_time) {
        packet->mac_time = tinfo->ip_time - tinfo->mac_time;
        // 计算MAC层和ip层之间的时间差
        packet->ip_time = tinfo->tran_time - tinfo->ip_time;
        // 计算ip层和tcp层之间的时间差
        packet->tran_time = tinfo->app_time - tinfo->tran_time;
        // 计算tcp层和应用层之间的时间差
    }
    packet->rx = 1; // 数据包已经被接收

    // RX HTTP INFO
    if (http_info) {
        int doff =
            BPF_CORE_READ_BITFIELD_PROBED(tcp, doff); // 得用bitfield_probed
        // 读取tcp头部中的数据偏移字段
        u8 *user_data =
            (u8 *)((u8 *)tcp + (doff * 4));
        // 计算tcp的负载开始位置就是tcp头部之后的数据，将tcp指针指向tcp头部位置将其转换成unsigned
        // char类型
        // doff *
        // 4数据偏移值(tcp的头部长度20个字节)乘以4计算tcp头部实际字节长度，32位为单位就是4字节
        bpf_probe_read_str(packet->data, sizeof(packet->data),
                           user_data); // 将tcp负载数据读取到packet->data
    }
    bpf_ringbuf_submit(packet, 0); // 将packet提交到缓冲区
    return 0;
}
/*
out_ipv4:
    kprobe/tcp_sendmsg
    kprobe/ip_queue_xmit
    kprobe/dev_queue_xmit
    kprobe/dev_hard_start_xmit

out_ipv6:
    kprobe/tcp_sendmsg
    kprobe/inet6_csk_xmit
    kprobe/dev_queue_xmit
    kprobe/dev_hard_start_xmit
*/
static __always_inline
int __tcp_sendmsg(struct sock *sk, struct msghdr *msg, size_t size)
{
    struct conn_t *conn = bpf_map_lookup_elem(&conns_info, &sk);
    if (conn == NULL) {
        return 0;
    }

    u16 family = BPF_CORE_READ(sk, __sk_common.skc_family);
    struct ktime_info *tinfo, zero = {0}; // 存储时间
    struct packet_tuple pkt_tuple = {0};  // 存储数据包信息
    /** ipv4 */
    if (family == AF_INET) {
        u16 dport = BPF_CORE_READ(sk, __sk_common.skc_dport);
        pkt_tuple.saddr = BPF_CORE_READ(sk, __sk_common.skc_rcv_saddr); // 源ip
        pkt_tuple.daddr = BPF_CORE_READ(sk, __sk_common.skc_daddr); // 目的ip
        pkt_tuple.sport = BPF_CORE_READ(sk, __sk_common.skc_num);   // 源端口
        pkt_tuple.dport = __bpf_ntohs(dport); // 目的端口并进行字节序转换

        u32 snd_nxt =
            BPF_CORE_READ(tcp_sk(sk), snd_nxt); // tcp要发送的下一个字节序列号
        u32 rcv_nxt = BPF_CORE_READ(tcp_sk(sk),
                                    rcv_nxt); // tcp接收的下一个字节的期望序列号
        pkt_tuple.seq = snd_nxt;
        pkt_tuple.ack = rcv_nxt;
        pkt_tuple.tran_flag = TCP;
        tinfo = (struct ktime_info *)bpf_map_lookup_or_try_init(
            &timestamps, &pkt_tuple,
            &zero); // timestamps的BPF map保存数据包与时间戳的映射
        if (tinfo == NULL) {
            return 0;
        }
        tinfo->tran_time = bpf_ktime_get_ns() / 1000;
    } else if (family == AF_INET6) {
        // 读取ipv6源地址
        bpf_probe_read_kernel(
            &pkt_tuple.saddr_v6,
            sizeof(sk->__sk_common.skc_v6_rcv_saddr.in6_u.u6_addr32),
            &sk->__sk_common.skc_v6_rcv_saddr.in6_u.u6_addr32);
        // 获取ipv6目的地址
        bpf_probe_read_kernel(
            &pkt_tuple.daddr_v6,
            sizeof(sk->__sk_common.skc_v6_daddr.in6_u.u6_addr32),
            &sk->__sk_common.skc_v6_daddr.in6_u.u6_addr32);
        // sk套接字中获取源端口号
        pkt_tuple.sport = BPF_CORE_READ(sk, __sk_common.skc_num);
        // 获取目的端口
        u16 dport = BPF_CORE_READ(sk, __sk_common.skc_dport);
        pkt_tuple.dport = __bpf_ntohs(dport);

        u32 snd_nxt =
            BPF_CORE_READ(tcp_sk(sk), snd_nxt); // 发送的下一个字节序列号
        u32 rcv_nxt =
            BPF_CORE_READ(tcp_sk(sk), rcv_nxt); // 期望接收的下一个字节序列号
        pkt_tuple.seq = snd_nxt;
        pkt_tuple.ack = rcv_nxt;
        pkt_tuple.tran_flag = TCP;

        tinfo = (struct ktime_info *)bpf_map_lookup_or_try_init(
            &timestamps, &pkt_tuple, &zero);
        if (tinfo == NULL) {
            return 0;
        }
        tinfo->tran_time = bpf_ktime_get_ns() / 1000;
    }

    CONN_INFO_TRANSFER

    CONN_ADD_EXTRA_INFO

    // TX HTTP info
    if (http_info) {
        u8 *user_data = GET_USER_DATA(msg);
        tinfo = (struct ktime_info *)bpf_map_lookup_or_try_init(
            &timestamps, &pkt_tuple, &zero);
        if (tinfo == NULL) {
            return 0;
        }
        bpf_probe_read_str(tinfo->data, sizeof(tinfo->data), user_data);
    }
    return 0;
}
static __always_inline
int __ip_queue_xmit(struct sock *sk, struct sk_buff *skb)
{
    if (!layer_time) {
        return 0;
    }
    u16 family = BPF_CORE_READ(sk, __sk_common.skc_family);
    struct packet_tuple pkt_tuple = {0};
    struct ktime_info *tinfo;
    struct tcphdr *tcp = skb_to_tcphdr(skb);
    if (family == AF_INET) {
        u16 dport;
        u32 seq, ack;
        pkt_tuple.saddr = BPF_CORE_READ(sk, __sk_common.skc_rcv_saddr);
        pkt_tuple.daddr = BPF_CORE_READ(sk, __sk_common.skc_daddr);
        pkt_tuple.sport = BPF_CORE_READ(sk, __sk_common.skc_num);
        dport = BPF_CORE_READ(sk, __sk_common.skc_dport);
        pkt_tuple.dport = __bpf_ntohs(dport);
        seq = BPF_CORE_READ(tcp, seq);
        ack = BPF_CORE_READ(tcp, ack_seq);
        pkt_tuple.seq = __bpf_ntohl(seq);
        pkt_tuple.ack = __bpf_ntohl(ack);
        pkt_tuple.tran_flag = TCP;
        if ((tinfo = bpf_map_lookup_elem(&timestamps, &pkt_tuple)) == NULL) {
            return 0;
        }
        tinfo->ip_time = bpf_ktime_get_ns() / 1000;
    }
    return 0;
}
static __always_inline
int __inet6_csk_xmit(struct sock *sk, struct sk_buff *skb)
{
    if (!layer_time) {
        return 0;
    }
    u16 family = BPF_CORE_READ(sk, __sk_common.skc_family);
    struct tcphdr *tcp = skb_to_tcphdr(skb);
    struct packet_tuple pkt_tuple = {0};
    struct ktime_info *tinfo;
    if (family == AF_INET6) {
        u16 dport;
        u32 seq, ack;

        bpf_probe_read_kernel(
            &pkt_tuple.saddr_v6,
            sizeof(sk->__sk_common.skc_v6_rcv_saddr.in6_u.u6_addr32),
            &sk->__sk_common.skc_v6_rcv_saddr.in6_u.u6_addr32);

        bpf_probe_read_kernel(
            &pkt_tuple.daddr_v6,
            sizeof(sk->__sk_common.skc_v6_daddr.in6_u.u6_addr32),
            &sk->__sk_common.skc_v6_daddr.in6_u.u6_addr32);

        pkt_tuple.sport = BPF_CORE_READ(sk, __sk_common.skc_num);
        dport = BPF_CORE_READ(sk, __sk_common.skc_dport);
        pkt_tuple.dport = __bpf_ntohs(dport);
        seq = BPF_CORE_READ(tcp, seq);
        ack = BPF_CORE_READ(tcp, ack_seq);
        pkt_tuple.seq = __bpf_ntohl(seq);
        pkt_tuple.ack = __bpf_ntohl(ack);
        pkt_tuple.tran_flag = TCP;
        if ((tinfo = bpf_map_lookup_elem(&timestamps, &pkt_tuple)) == NULL) {
            return 0;
        }
        tinfo->ip_time = bpf_ktime_get_ns() / 1000;
    }
    return 0;
}
static __always_inline
int dev_queue_xmit(struct sk_buff *skb)
{
    if (!layer_time) {
        return 0;
    }
    // 从skb中读取以太网头部
    const struct ethhdr *eth = (struct ethhdr *)BPF_CORE_READ(skb, data);
    u16 protocol = BPF_CORE_READ(
        eth,
        h_proto); // 以太网头部协议字段该字段存储的是以太网帧所封装的上层协议类型
    struct tcphdr *tcp = skb_to_tcphdr(skb);
struct packet_tuple pkt_tuple = {0};
    struct ktime_info *tinfo;
    if (protocol == __bpf_ntohs(ETH_P_IP)) {
        /** ipv4 */
        struct iphdr *ip = skb_to_iphdr(skb);
        get_pkt_tuple(&pkt_tuple, ip, tcp);

        if ((tinfo = bpf_map_lookup_elem(&timestamps, &pkt_tuple)) == NULL) {
            return 0;
        }
        tinfo->mac_time = bpf_ktime_get_ns() / 1000;
    } else if (protocol == __bpf_ntohs(ETH_P_IPV6)) {
        /** ipv6 */
        struct ipv6hdr *ip6h = skb_to_ipv6hdr(skb);
        get_pkt_tuple_v6(&pkt_tuple, ip6h, tcp);

        if ((tinfo = bpf_map_lookup_elem(&timestamps, &pkt_tuple)) == NULL) {
            return 0;
        }
        tinfo->mac_time = bpf_ktime_get_ns() / 1000;
    }
    return 0;
}
static __always_inline
int __dev_hard_start_xmit(struct sk_buff *skb)
{
    const struct ethhdr *eth = (struct ethhdr *)BPF_CORE_READ(skb, data);
    u16 protocol = BPF_CORE_READ(eth, h_proto);
    struct tcphdr *tcp = skb_to_tcphdr(skb);
    struct packet_tuple pkt_tuple = {0};
    struct ktime_info *tinfo;
    if (protocol == __bpf_ntohs(ETH_P_IP)) {
        /** ipv4 */
        struct iphdr *ip = skb_to_iphdr(skb);
        get_pkt_tuple(&pkt_tuple, ip, tcp);

        if ((tinfo = bpf_map_lookup_elem(&timestamps, &pkt_tuple)) == NULL) {
            return 0;
        }
        // 数据包在队列中等待的时间
        tinfo->qdisc_time = bpf_ktime_get_ns() / 1000;
    } else if (protocol == __bpf_ntohs(ETH_P_IPV6)) {
        /** ipv6 */
        struct ipv6hdr *ip6h = skb_to_ipv6hdr(skb);
        get_pkt_tuple_v6(&pkt_tuple, ip6h, tcp);

        if ((tinfo = bpf_map_lookup_elem(&timestamps, &pkt_tuple)) == NULL) {
            return 0;
        }
        tinfo->qdisc_time = bpf_ktime_get_ns() / 1000;
    } else {
        return 0;
    }

    /*----- record packet time info ------*/
    if (tinfo == NULL) {
        return 0;
    }
    struct sock *sk = tinfo->sk;
    if (!sk) {
        return 0;
    }
    PACKET_INIT_WITH_COMMON_INFO
    packet->saddr = pkt_tuple.saddr;
    packet->daddr = pkt_tuple.daddr;
    packet->sport = pkt_tuple.sport;
    packet->dport = pkt_tuple.dport;
    // 记录各层的时间差值
    if (layer_time) {
        packet->tran_time = tinfo->ip_time - tinfo->tran_time;
        packet->ip_time = tinfo->mac_time - tinfo->ip_time;
        packet->mac_time =tinfo->qdisc_time -tinfo->mac_time; // 队列纪律层，处于网络协议栈最底层，负责实际数据传输与接收
    }
    packet->rx = 0; // 发送一个数据包

    // TX HTTP Info
    if (http_info) {
        bpf_probe_read_str(packet->data, sizeof(packet->data), tinfo->data);
       // bpf_printk("%s", packet->data);
    }
    bpf_ringbuf_submit(packet, 0);

    return 0;
}
