# sofdsnoop 诊断指南

## 概述

`sofdsnoop` 专门用于追踪通过 Unix Domain Socket (UDS) 传递的文件描述符 (FD)。它能详细记录哪个进程发送了哪个 FD，以及哪个进程成功接收了它，解决复杂多进程架构中的资源流转不透明问题。

**核心场景**：

1. **复杂架构 FD 传递审计**：例如在 Master-Worker 模型中，Master 接收连接后通过 UDS 将 Socket FD 传递给 Worker，AI 需要验证此传递链路是否准确。
2. **FD 丢失分析**：排除 FD 在发送或接收过程中由于协议解析错误或业务逻辑导致的丢失（Leak）。
3. **跨进程交互可见性**：揭示隐藏在 UDS `SCM_RIGHTS` 控制消息背后的数据交互。

## 命令行参数

| 参数 | 缩写 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- | --- |
| `--timestamp` | `-T` | bool | false | 在输出中包含时间戳 |
| `--pid`       | `-p` | uint32 | 0 | 仅追踪指定 PID 发送或接收 FD 的事件 |
| `--tid`       | `-t` | uint32 | 0 | 仅追踪指定线程 TID |
| `--name`      | `-n` | string | "" | 仅输出进程名包含此字符串的记录 |
| `--duration`  | `-d` | uint32 | 0 | 追踪的总时长（秒），结束后自动退出 |
| `--verbose`   | -    | bool | false | 打印调试详细信息 |

## AI 逻辑与调用指引

### 何时调用

AI Agent 在观测到以下异常时，应考虑调用此工具：

- **业务逻辑异常断裂**: 发现 Master 接收请求后，Worker 并未处理，AI 需通过 `sofdsnoop` 确认 FD 是否已成功送达 Worker。
- **文件描述符数值不匹配**: 当日志记录指示操作了非预期的 FD，AI 需追溯该 FD 的分发源头。
- **多进程负载均衡失效**: 怀疑某些 Worker 接收到了过多的 FD 而其他 Worker 处于空闲。

### 输出解读 (AI 视角)

- **ACTION (SEND/RECV)**: **SEND** 表示进程正通过 `sendmsg` 发送 FD；**RECV** 表示进程通过 `recvmsg` 获得 FD。
- **FDs**: 输出包含被传递的 FD 列表。AI 应核对发送端和接收端的 FD 列表是否对应。
- **SOCK_FD**: 用于传递控制消息的 UDS 套接字 FD，用于将不同的 SEND/RECV 操作关联起来。

## 与相关工具的关系

- **与 tcptracer 的联动**: `tcptracer` 捕捉到 `Accept` 后，AI 通常紧接着使用 `sofdsnoop` 查看该接受到的 Socket 是否被分发给了其他的进程。
- **全链路视野**: 对于基于 UDS 传递的网络服务，`sofdsnoop` 是连接在不同进程间流转的关键观测桥梁。
