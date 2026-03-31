# tcpretrans 诊断指南

## 概述

`tcpretrans` 实时追踪内核 TCP 协议栈中的重传事件（Retransmissions）及尾部丢失探测（TLP）。它能够精确揭示网络路径上的丢包现象及连接稳定性问题。

**核心场景**：

1. **网络丢包分析**：识别特定目标、特定连接的丢包频率。
2. **连接稳定性评估**：在建连阶段（SYN 重传）或数据传输阶段（ESTABLISHED 重传）识别瓶颈。
3. **尾部丢失诊断**：通过 TLP 探测识别流结束时的潜在丢包。

## 命令行参数

| 参数 | 缩写 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- | --- |
| `--sequence` | `-s` | bool | false | 在输出中显示 TCP 序列号 (SEQ) |
| `--lossprobe`| `-l` | bool | false | 包含尾部丢失探测 (TLP) 的尝试记录 |
| `--count`    | `-c` | bool | false | 统计模式：按流计算重传发生的次数，而非实时打印 |
| `--ipv4`     | `-4` | bool | false | 仅追踪 IPv4 流量 |
| `--ipv6`     | `-6` | bool | false | 仅追踪 IPv6 流量 |
| `--verbose`  | -    | bool | false | 打印调试详细信息 |

## AI 逻辑与调用指引

### 何时调用

AI Agent 在观测到以下指标异常或收到相关用户投诉时，应考虑调用此工具：

- **重传计数升高**: `netstat -s` 中的 `segments retransmitted` 指标异常增长。
- **建连超时**: 业务侧反馈 `Connection timeout`，AI 需通过 `-s` 观察是否有 SYN 包的重复发送。
- **响应延迟长尾**: 怀疑是因为丢包重传导致的应用层感知延迟（TLP 对识别此类问题至关重要）。

### 输出解读 (AI 视角)

- **T (Type)**: `R` 代表真实重传，`L` 代表 TLP 探测。连续的 `R` 通常意味着路径深度丢包或对端接收窗口长期阻塞。
- **STATE**: 关注重传发生时的 TCP 状态。若在 `SYN_SENT` 发生重传，指示握手失败；若在 `ESTABLISHED` 发生，指示传输路径故障。
- **SEQ**: AI 应分析 SEQ 是否重复。相同的 SEQ 多次重传说明该段数据始终未到达或未被确认。

## 与相关工具的关系

- **与 tcpconnlat 的联动**: 如果 `tcpconnlat` 指示建连延迟极高，AI 应启动 `tcpretrans -4` 确认是否是因为 SYN 包重传导致的伪延迟。
- **与 tcptop 的联动**: 发现某个高带宽连接吞吐下降时，使用 `tcpretrans` 确认是否受丢包限速影响。
