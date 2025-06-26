# ipcwatcher - 进程间通信观测工具
## 1. 工具介绍
- **简介**

  unix domian socket（uds）作为一种IPC方式，与TCP/IP采用相同的socket接口，在本机通信中广泛应用。本机环境下由于不用经过网络协议栈，所以在性能上比TCP更有优势。比如Linux的X11，就是通过uds通信的。
  同时由于uds具有传递文件描述符的能力，且共享内存作为最快的IPC方式，没有自己的同步机制。因此，一种常规做法是采用uds作为共享内存的同步机制，并通过uds在进程间传递memfd。这种方式在通信中间件的设计实现上很常见，例如字节跳动的采用go语言实现的shmipc。
  **目前的观测手段**
  目前查看下来，观测手段较少。

  - 在系统调用跟踪方面，可以用strace跟踪到。
  - 在信息展示方面，可以用ss观测。
  - 而在数据包跟踪方面，没看到原生的跟踪工具，除了使用bpftrace可以跟踪一下之外，一种常规做法是，借助socat将uds的数据包转发到一个TCP连接上，然后使用wireshark监控TCP数据，间接观测uds的数据。

  不知道是否还有其他的观测工具，大家知道可以说下。
## 2. 依赖库
spdlog
fmt
argparse
libpacap


需求描述：
UDS

关键点：在不修改代码，重新编译程序，或者是一个第三方可执行文件时，可以
监控UDS IPC消息

1. 查看当前系统中所有的UDS连接信息，类似于ss命令 ss -x
2. 连接状态跟踪，显示UDS连接状态，构建类似于TCP的状态机跟踪机制
3. 统计信息展示，收集并展示每个UDS的流量统计，如发送或接收的数据量，错误数等
4. 进程相关，通过进程号，将UDS的通信关联到具体的用户空间进程


低功耗运行模式，即使用pr_debug打印到pipe上，而不是poll到用户空间并打印
2. 抓包与数据监控，追踪特定的UDS数据包信息，数据量统计、数据内容
3. 将追踪的信息保存为pcap文件，可以在wireshark中回放
4. uds消息头自定义，根据给定的消息头配置文件，解析UDS消息




shm








TODO:

共享内存的数据发送与接收观测

1. 匿名共享内存
2. 文件共享内存
3. 支持共享内存头的自定义，以一种格式，例如yaml或json
4. 零拷贝调试与追踪



要在 Linux 内核中像 Wireshark 追踪 TCP/IP 协议栈那样查看 Unix 域套接字（UDS）的方方面面，需要实现以下关键功能和需求：

---

### 1. **抓包与数据监控**
- **需求**：捕获 UDS 的通信数据流，包括发送和接收的数据内容。
- **实现方式**：
  - 利用 [skb](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/drivers/atm/he.h#L132-L132)（socket buffer）在 UDS 的收发路径上进行钩子插入（如通过 kprobe 或 tracepoint）。
  - 使用 eBPF 程序拦截 `unix_stream_sendmsg`、`unix_dgram_recvmsg` 等函数。

---

### 2. **连接状态追踪**
- **需求**：实时查看 UDS 的连接状态（如 [CONNECTED](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/drivers/scsi/53c700.h#L372-L373), [LISTEN](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/drivers/infiniband/hw/cxgb4/iw_cxgb4.h#L777-L777), [CLOSED](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/net/atm/mpoa_caches.h#L90-L91)）。
- **实现方式**：
  - 监控 `unix_accept`, `unix_connect`, `unix_release` 等函数。
  - 构建类似 TCP 的状态机跟踪机制。

---

### 3. **统计信息展示**
- **需求**：收集并展示每个 UDS 的流量统计（如发送/接收的数据量、错误数等）。
- **实现方式**：
  - 在 `struct unix_sock` 中维护计数器。
  - 提供 `/proc/<pid>/unix_stats` 接口或 sysfs 展示。

---

### 4. **进程关联**
- **需求**：将 UDS 的通信关联到具体的用户空间进程。
- **实现方式**：
  - 记录 socket 对应的 [task_struct](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/include/linux/sched.h#L662-L1225) 和 PID。
  - 结合 [proc](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/scripts/gdb/linux/proc.py#L0-L0) 文件系统显示 PID、进程名等信息。

---

### 5. **文件路径解析**
- **需求**：显示 UDS 的绑定路径（如 `/tmp/mysocket`）。
- **实现方式**：
  - 从 `struct unix_address` 获取路径信息。
  - 支持抽象命名空间（以 `\0` 开头的路径）的识别与展示。

---

### 6. **支持监听与事件通知**
- **需求**：提供类似于 `tcpdump` 的实时监听工具，支持过滤和事件触发。
- **实现方式**：
  - 使用 netlink 或字符设备接口向用户态推送事件。
  - 支持按路径、PID、协议类型（SOCK_STREAM/SOCK_DGRAM）过滤。

---

### 7. **安全与隔离**
- **需求**：确保 UDS 抓包行为不会影响系统稳定性或泄露敏感信息。
- **实现方式**：
  - 限制只有 root 权限才能启用监控。
  - 提供 SELinux/AppArmor 集成控制访问权限。

---

### 8. **调试与日志记录**
- **需求**：为内核模块添加调试日志输出，便于排查问题。
- **实现方式**：
  - 使用 [pr_debug()](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/tools/perf/util/debug.h#L25-L27) / [printk()](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/include/linux/printk.h#L174-L175) 输出详细信息。
  - 支持动态开关调试选项（如通过 module_param 控制）。

---

### 9. **eBPF 集成**
- **需求**：利用现代 eBPF 技术实现高效、低开销的 UDS 监控。
- **实现方式**：
  - 编写 BCC/BTF 工具挂钩到 `sys_enter_unix_*` 系列函数。
  - 收集上下文信息并导出到用户空间。

---

### 10. **用户空间工具开发**
- **需求**：构建一个类 `Wireshark` 的 GUI 或命令行工具来展示 UDS 数据。
- **实现方式**：
  - 使用 Python + PySide/PyQt 实现图形界面。
  - 命令行工具可基于 `libnl` 或自定义 socket 接口与内核交互。

---

### 总结
| 功能点 | 描述 |
|--------|------|
| 抓包与监控 | 捕获 UDS 发送/接收的数据 |
| 状态追踪 | 显示连接状态变化 |
| 统计信息 | 流量、错误计数等 |
| 进程关联 | 查看 PID、进程名 |
| 路径解析 | 显示绑定路径 |
| 实时监听 | 类似 tcpdump 的监听 |
| 安全机制 | 权限控制、防止滥用 |
| 日志与调试 | 方便内核模块调试 |
| eBPF 支持 | 高效低开销的监控方案 |
| 用户工具 | 提供可视化分析界面 |

这些功能的实现将帮助开发者深入理解 UDS 的运行机制，并用于调试复杂的应用场景。
