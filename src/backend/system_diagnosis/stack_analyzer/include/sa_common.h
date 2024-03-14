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
// author: luiyanbing@foxmail.com
//
// 通用数据结构

#ifndef STACK_ANALYZER_COMMON
#define STACK_ANALYZER_COMMON

#include <asm/types.h>

#define COMM_LEN 16        // 进程名最大长度
#define MAX_STACKS 32      // 栈最大深度
#define MAX_ENTRIES 102400 // map容量

/// @brief 栈计数的键，可以唯一标识一个用户内核栈
typedef struct {
    __u32 pid;
    __s32 ksid, usid;
} psid;

/// @brief 进程名
typedef struct {
    char str[COMM_LEN];
} comm;

typedef struct {
    __u64 truth;
    __u64 expect;
} ra_tuple;

typedef struct {
    __u64 count;
    __u64 size;
} io_tuple;

#endif