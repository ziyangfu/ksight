1. 核心原理分析
Nagle 算法在 Linux 内核中使用 struct tcp_sock 中的 nonagle 字段表示。 该字段的取值如下（参考内核 include/linux/tcp.h）：

TCP_NAGLE_OFF (1): Nagle 算法被禁用（对应 TCP_NODELAY 被设置）。
TCP_NAGLE_CORK (2): 强制合并小包（对应 TCP_CORK）。
0: 默认开启 Nagle 算法。
我们将使用 BPF Iterator (iter/tcp) 来遍历系统中所有的 TCP Socket，并读取该字段的状态。这种方式相比传统的 
/proc/net/tcp
 更加高效且能够通过 BPF 的类型检查在内核层安全地读取私有结构。

2. 功能设计
实时快照：运行后遍历全系统 Socket，汇报状态。
程序识别：通过 Socket 关联的 sk 关联到 task。在内核中，获取 sk 的所有者（PID）可以通过遍历当前进程列表的 
fd
 匹配来实现，或者对于已建立的连接，如果是当前进程创建的，可以用 bpf_get_current_pid_tgid（在交互式设置时）。
改进建议：全量快照中，我们可以结合 libbpf 的 netlink 接口或搜索 /proc 同步得到 PID。BPF 侧负责核心数据（nonagle）。
过滤功能：
-d, --disabled-only: 只显示已经禁用了 Nagle 的连接。
-p, --pid <PID>: 只检查指定进程 ID 的连接。
输出格式：美化表格输出，包含本地/远端地址、进程名(PID)、Nagle 状态。
3. 实现计划
3.1 BPF 部分 (tcpnagle.bpf.c)
使用 SEC("iter/tcp") 钩子。
从 ctx->sk_common 转换到 struct tcp_sock (使用 bpf_skc_to_tcp_sock)。
获取 tp->nonagle 数据。
通过数据结构回传给用户态。
3.2 用户态部分 (tcpnagle.cpp / TcpNagleBpf.cpp)
使用 libbpf 加载并运行 BPF 程序。
使用 argparse 处理命令行参数。
完成对 PID 的过滤。
4. 目录结构
text
src/net/tcpnagle/
├── CMakeLists.txt
├── bpf/
│   └── tcpnagle.bpf.c
├── include/
│   ├── TcpNagleBpf.h
│   └── tcpnagle_common.h (共享结构定义)
└── src/
    ├── tcpnagle.cpp      (main 入口)
    ├── TcpNagleBpf.cpp   (bpf 加载器封装)
    └── ArgParser.cpp     (参数解析)




参考ksight的标准开发框架，bpf程序使用C语言编写，用户态使用C++17开发，实现以下的功能。
1. 核心功能，观测系统中，哪些在运行的用户态程序，禁用了nagle算法，哪些没有禁用，并输出结果。
2. 支持过滤，可以只显示禁用了nagle算法的程序。
3. 可以只检查给定的用户态程序的nagle算法状态。以pid的形式给定。
4. cgroups + eBPF SOC_OPS钩子： 禁用cgroups中的所有程序的nagle算法。




