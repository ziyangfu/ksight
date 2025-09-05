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

/*!
 * \brief ipcwatcher工具的头文件，定义内核态与用户态数据传递的结构体
 * \file ipcwatcher.h
 * */

#ifndef IPC_IPC_WATCHER_IPC_WATCHER_H
#define IPC_IPC_WATCHER_IPC_WATCHER_H

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

#define MAX_PAYLOAD_LEN 64  // 512
#define MAX_COMM_LEN 64
#define MAX_SHM_PATH_LEN 128

struct uds_event {
    u32 send_pid;                       /** 发送进程 */
    u32 recv_pid;                       /** 接收进程 */
    char path[108];                     /** UNIX域socket路径最大长度（sun_path长度） */
    u32 size;                           /** 发送/接收的数据大小 */
    u16 type;                           /** SOCK_STREAM(1) / SOCK_DGRAM(2) / ... */
    u64 timestamp;                      /** 记录发送的时间戳 */
    char payload[MAX_PAYLOAD_LEN]; /** 记录发送/接收的实际数据 */
};
/** 定义通过 ringbuffer 传递到用户态的数据结构 */
struct uds_transfer_data {
    struct uds_event event;
    char payload[MAX_PAYLOAD_LEN]; /** 记录发送/接收的实际数据 */
};


/*!
 *     pid， comm， ,Addr, len, prot(读写权限),flag（MAP_SHARED, path(通过fd->file->path)
 * */
struct shm_basic_info_event {
    u64 timestamp;
    u32 pid;
    char comm[MAX_COMM_LEN];
    unsigned long fd;
    //unsigned long addr;
    unsigned long len;
    unsigned long prot;
    unsigned long flag;
    //char path[MAX_SHM_PATH_LEN];
};

/*!
 * \brief 用于呈现共享内存在内核中运行的轨迹
 *      pid
 * */
struct shm_path_trace_event {
    u64 timestamp;
    u32 pid;
    char comm[MAX_COMM_LEN];
    unsigned long fd;
    unsigned long addr;
    unsigned long len;
    unsigned long prot;
    unsigned long flag;
    char path[MAX_SHM_PATH_LEN];
};



#endif /* IPC_IPC_WATCHER_IPC_WATCHER_H */
