// Copyright 2025 The LMP Authors.
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
/*!
\brief
    1. 简介
        本文件主要跟踪 Linux IPC 进程间通信的相关信息
        主要包括： unix domain socket（UDS）、mmap共享内存、信号
    uds部分：
        0. 功能：
            1. 跟踪并输出uds的基本信息，包括可选的发送与接收信息
            2. 跟踪uds从发送到接收的路径跟踪
                unix_xx_sendmsg --> 拷贝skb到接收队列 --> unix_xx_recvmsg 的路径跟踪
        1. 挂载点
            unix_dgram
                kprobe:unix_dgram_sendmsg
                kprobe:unix_dgram_recvmsg
                kprobe:unix_dgram_poll      [opt]
                kprobe:unix_dgram_connect   [opt]
            unix_stream
                kprobe:unix_stream_recvmsg
                kprobe:unix_stream_sendmsg
                kprobe: skb_copy_datagram_from_iter
                kprobe:unix_stream_connect  [opt]
                
        2. 内核文件： net/unix/af_unix.c
相关的工具，BCC undump： 转储 payload
              sofdsnoop 跟踪 uds 传递文件描述符fd
*/

/**
 * 在 unix_stream_sendmsg中获取发送数据端的pid
 * 获取自己的sock与对方的sock
 * 通过自己的sock，强制类型转换为unix_sock，获取unix_address，并获取到path
 * 获取发送数据端的skb，并从中获取payload 【可选输出】
 *
 * 在unix_stream_recvmsg中获取接收数据端的pid
 * 获取自己的sock，判断跟数据发送端获取的other sock一致
 *      如果一致
 *            清理临时map结构体
 *            poll到用户空间
 *      如果不一致
 *            暂时不处理，因为可能数据还没来，或者不是同一个客户端调用。并不一定出错
 *
 * */



/** FIXME: 可以考虑通过 uprobe 截取 uds 的数据 */

#include "common.bpf.h"

/** 通过发送端的sock与接收端的sock来唯一标识一条连接 */
struct uds_indicate {
    struct sock* send_sk;
    struct sock* recv_sk;
};

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} uds_events SEC(".maps");


struct {
    __uint(type, BPF_MAP_TYPE_LRU_HASH);
    __uint(max_entries, 256 * 1024);
    __type(key, struct uds_indicate*);
    __type(value, struct uds_event);
} uds_data_map SEC(".maps");

/*!
 * uds 强制类型转换函数，拷贝自：include/net/af_unix.h
 * */
static inline struct unix_sock *unix_sk(const struct sock *sk)
{
    return (struct unix_sock *)sk;
}
// 获取UDS路径的辅助函数
static void get_uds_path(struct unix_sock *u, char *path) {
    struct unix_address *addr;
    struct sockaddr_un *sun;
    addr = BPF_CORE_READ(u, addr);
    if (!addr) {
        bpf_probe_read_kernel_str(path, 6, "<none>");
        return;
    }
    sun = BPF_CORE_READ(addr, name);
    if (!sun) {
        bpf_probe_read_kernel_str(path, 6, "<none>");
        return;
    }
    bpf_probe_read_kernel_str(path, sizeof(sun->sun_path), sun->sun_path);
}


/*!
 * \brief 获取 uds payload
 * */
 SEC("kprobe/maybe_add_creds")
 void BPF_KPROBE(maybe_add_creds, struct sk_buff *skb, const struct socket *sock,
                 const struct sock *other) {
    struct uds_indicate uds_id = {
            .send_sk = NULL,
            .recv_sk = NULL
    };
    struct sock* sk = BPF_CORE_READ(sock, sk);
    uds_id.send_sk = sk;
    uds_id.recv_sk = (struct sock*)other;

    struct uds_event* event = bpf_map_lookup_elem(&uds_data_map, &uds_id);
    if (event) {
        /** 拷贝skb中的payload到 event->payload*/
        u32 payload_size = BPF_CORE_READ(skb, len);
        // 限制最大读取长度为 MAX_PAYLOAD_SIZE（例如 64 字节）
        payload_size = payload_size > sizeof(event->payload) ? sizeof(event->payload) : payload_size;
        bpf_probe_read_kernel_str(event->payload, sizeof(event->payload), BPF_CORE_READ(skb, data));
    }
 }



/*!
\brief
    挂载点 unix_dgram_sendmsg, 负责采集uds dgram的基本信息与发送的数据
*/
SEC("kprobe/unix_dgram_sendmsg")
int BPF_KPROBE(unix_dgram_sendmsg, const struct socket *sock, const struct msghdr *msg,
			      size_t len) {
    u64 current_pid = bpf_get_current_pid_tgid() >> 32;
    struct uds_indicate uds_id = {
            .send_sk = NULL,
            .recv_sk = NULL
    };
    struct sock *sk = BPF_CORE_READ(sock, sk);
    struct unix_sock *uds_sk = unix_sk(sk);
    uds_id.send_sk = sk;
    uds_id.recv_sk = BPF_CORE_READ(uds_sk, peer);

    struct uds_event zero = {0};
    struct uds_event* event;
    event = (struct uds_event*)bpf_map_lookup_or_try_init(&uds_data_map, &uds_id, &zero);
    if (event == NULL) {
        return 0;
    }

    const struct unix_address *addr = BPF_CORE_READ(uds_sk, addr);
    /** 存在显性路径 */
    if (addr) {
        const char *path = BPF_CORE_READ(addr, name->sun_path);
        bpf_probe_read_kernel_str(event->path, sizeof(event->path), path);
    }
    else {
        bpf_probe_read_kernel_str(event->path, 7, "<none>");
    }
    event->send_pid = current_pid;
    event->size = (u32)len;
    event->type = BPF_CORE_READ(sk, sk_type);
    event->timestamp = bpf_ktime_get_ns() / 1000;
    return 0;
}

/*!
\brief
    挂载点 unix_dgram_recvmsg, 负责采集uds dgram的基本信息与接收的数据
*/
SEC("kprobe/unix_dgram_recvmsg")
int BPF_KPROBE(unix_dgram_recvmsg, const struct socket *sock, const struct msghdr *msg,
			      size_t size, int flags) {
    struct uds_indicate uds_id = {
            .send_sk = NULL,
            .recv_sk = NULL
    };

    struct sock* sk = BPF_CORE_READ(sock, sk);
    struct unix_sock *uds_sk = unix_sk(sk);
    uds_id.recv_sk = sk;
    uds_id.send_sk = BPF_CORE_READ(uds_sk, peer);

    struct uds_event* event = bpf_map_lookup_elem(&uds_data_map, &uds_id);
    if (!event)
        return 0;
    event->recv_pid = bpf_get_current_pid_tgid() >> 32;

    struct uds_event* trans_rb_event =
            bpf_ringbuf_reserve(&uds_events, sizeof(struct uds_event), 0);
    if (trans_rb_event == NULL) {
        bpf_map_delete_elem(&uds_data_map, &uds_id);
        return 0;
    }
    trans_rb_event->send_pid = event->send_pid;
    trans_rb_event->recv_pid = event->recv_pid;
    bpf_probe_read_kernel_str(trans_rb_event->path, sizeof(event->path), event->path);
    trans_rb_event->size = event->size;
    trans_rb_event->type = event->type;
    trans_rb_event->timestamp = event->timestamp;
    bpf_probe_read_kernel(trans_rb_event->payload, sizeof(trans_rb_event->payload),
                          event->payload);

    bpf_map_delete_elem(&uds_data_map, &uds_id);

    bpf_ringbuf_submit(trans_rb_event, 0);
    return 0;
}

/*!
\brief
    挂载点 unix_stream_sendmsg, 负责采集流式uds的基本信息与发送的数据
    获取发送侧 PID， uds path， 发送的size大小， 发送时间点
*/
SEC("kprobe/unix_stream_sendmsg")
int BPF_KPROBE(unix_stream_sendmsg, struct socket *sock, struct msghdr *msg,
               size_t len) {
    u64 current_pid = bpf_get_current_pid_tgid() >> 32;
    struct sock *send_sk = BPF_CORE_READ(sock, sk);
    struct uds_event zero = {0};
    struct uds_event* event;
    struct uds_indicate uds_id = {
        .send_sk = NULL,
        .recv_sk = NULL
    };
    struct unix_sock *unix_sk = (struct unix_sock*)send_sk;
    uds_id.recv_sk = BPF_CORE_READ(unix_sk, peer);
    uds_id.send_sk = send_sk;

    event = (struct uds_event*)bpf_map_lookup_or_try_init(&uds_data_map, &uds_id, &zero);
    if (event == NULL) {
        return 0;
    }

    const struct unix_address *addr = BPF_CORE_READ(unix_sk, addr);
    /** 存在显性路径 */
    if (addr) {
        const char *path = BPF_CORE_READ(addr, name->sun_path);
        bpf_probe_read_kernel_str(event->path, sizeof(event->path), path);
    }
    else {
        bpf_probe_read_kernel_str(event->path, 7, "<none>");
    }
    /** 指定PID 过滤 */
//    if (send_pid != 0) {
//        if (current_pid != send_pid) {
//            return 0;
//        }
//    }
    event->send_pid = current_pid;
    event->size = (u32)len;
    event->type = BPF_CORE_READ(send_sk, sk_type);
    event->timestamp = bpf_ktime_get_ns() / 1000;
    return 0;
}

/*!
\brief
    挂载点 unix_stream_recvmsg, 负责采集流式uds的基本信息与接收的数据
    获取接收侧 PID
*/
SEC("kprobe/unix_stream_recvmsg")
int BPF_KPROBE(unix_stream_recvmsg, const struct socket *sock, const struct msghdr *msg,
			       size_t size, int flags) {
    struct uds_indicate uds_id = {
        .send_sk = NULL,
        .recv_sk = NULL
    };
    struct sock* sk = BPF_CORE_READ(sock, sk);

    uds_id.recv_sk = sk;
    struct unix_sock *recv_uk = unix_sk(sk);
    uds_id.send_sk = BPF_CORE_READ(recv_uk, peer);

    struct uds_event* event = bpf_map_lookup_elem(&uds_data_map, &uds_id);
    if (!event) {
        return 0;
    }
    event->recv_pid = bpf_get_current_pid_tgid() >> 32;

    struct uds_event* trans_rb_event =
            bpf_ringbuf_reserve(&uds_events, sizeof(struct uds_event), 0);
    if (trans_rb_event == NULL) {
        bpf_map_delete_elem(&uds_data_map, &uds_id);
        return 0;
    }

//    /** 指定PID 过滤 */
//    if (send_pid != 0) {
//        // 不拷贝数据到用户空间
//        bpf_map_delete_elem(&uds_data_map, &uds_id);
//        return 0;
//    }
//    /** 按照recv_pid过滤 */
//    if (recv_pid != 0) {
//        if (event->recv_pid != recv_pid) {
//            // 不拷贝数据到用户空间
//            bpf_map_delete_elem(&uds_data_map, &uds_id);
//            return 0;
//        }
//    }
    /** FIXME：低功耗调试模式，直接打印到pipe上，不拷贝输出 */
    // pr_debug("uds_id: %d, send_pid: %d, recv_pid: %d, size: %d, timestamp: %d, type: %d",
    //         uds_id, event->send_pid, event->recv_pid, event->size, event->timestamp, event->type);



    trans_rb_event->send_pid = event->send_pid;
    trans_rb_event->recv_pid = event->recv_pid;
    bpf_probe_read_kernel_str(trans_rb_event->path, sizeof(event->path), event->path);
    trans_rb_event->size = event->size;
    trans_rb_event->type = event->type;
    trans_rb_event->timestamp = event->timestamp;
    bpf_probe_read_kernel(trans_rb_event->payload, sizeof(trans_rb_event->payload),
                          event->payload);

    bpf_map_delete_elem(&uds_data_map, &uds_id);

    bpf_ringbuf_submit(trans_rb_event, 0);

    return 0;
}

char LICENSE[] SEC("license") = "GPL";
