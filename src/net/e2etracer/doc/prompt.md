# Role
你是一位精通 Linux 内核网络协议栈、eBPF 技术以及车载 SOA (SOME/IP & DDS) 架构的资深系统架构师。你擅长在嵌入式高性能计算平台（如 NVIDIA Orin, DRIVE Thor）上进行非侵入式性能诊断工具的开发。

# Context
我正在开发一个名为 "ksight" 的车载场景全链路诊断工具。
1. **现状**：车内是静态以太网，拥有完整的 JSON 格式 SOA 通信汇总表（包含 IP, ServiceID, MethodID, VLAN, 实例 ID 等映射）。
2. **目标**：实现“垂向全链路跟踪”。即：App (User Space) -> Middleware (SOA Stack) -> Kernel (Transport/Network/Link Layer) -> NIC (Physical)。
3. **痛点**：需要解决报文周期性抖动、端到端延迟高的问题。
4. **定位**：研发阶段的问题定位工具，要求低侵入、高精度，需兼容 PREEMPT_RT 实时内核环境。

# Task
请针对“垂向全链路跟踪”的实现方案，提供详细的技术设计路线，重点包含以下三个方面：

## 1. 跨层关联策略
- 如何在不修改业务报文（不增加 TraceID）的前提下，利用 eBPF (fentry/fexit) 捕获 `sk_buff`，并结合用户态读取的 JSON 配置表，实现从内核 `skb` 到业务层 `ServiceID` 的精准语义映射？
- 给出 eBPF Map 的设计建议，用于存储这种映射关系。

## 2. 内核协议栈精细化埋点
- 请列出在 Linux 内核网络栈中监控“分层耗时”的关键 hook 点（例如 `netif_receive_skb`, `ip_rcv`, `tcp_v4_rcv` 等）。
- 如何利用 `bpf_ktime_get_ns()` 计算每层之间的 Delta 时间，并处理 PREEMPT_RT 环境下的潜在干扰？

## 3. 中间件与用户态插桩 (USDT/Uprobe)
- 针对中间件（如 SOME/IP 栈）这个薄弱点，请说明如何使用 USDT (User-level Statically Defined Tracing) 实现低损耗的时间戳捕获。
- 如何在 eBPF 中将用户态线程的 PID/TID 与内核态的 `skb` 轨迹进行逻辑绑定？

# Constraints & Philosophy
- **不迎合**：如果某个想法（如共享内存跟踪）在 eBPF 中实现代价过高或不可行，请直接指出并提供替代方案。
- **专业性**：使用标准的 Linux 内核术语。
- **可落地**：考虑到车载内核版本（如 5.10+ 或 5.15+），优先使用 BTF 和 Libbpf 现代开发范式。

# Output Format
请以架构设计文档的形式输出，包含：逻辑拓扑、核心 eBPF Hook 点清单、关键数据结构定义、以及潜在的性能风险预警。