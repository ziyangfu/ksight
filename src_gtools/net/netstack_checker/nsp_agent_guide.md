# netstack_probe (nsp) 诊断指南

## 概述

`netstack_probe` (简称 `nsp`) 是 `ksight` 诊断体系中的**第一级 (Level 1) 网络排障入口**。它是一个纯 Python 实现的轻量化工具，不依赖 eBPF，旨在 5 秒内为 Agent 提供全方位的网络状态“体检报告”。

**核心功能**：

1. **协议栈一致性审计 (Dimension A)**：基于 [desired_state.json](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/src_gtools/net/netstack_checker/desired_state.json) 核查内核 `sysctl` 参数，识别配置漂移。
2. **多层连通性探测 (Dimension B)**：集成 L3 (ping) 和 L2 (arping) 探测，量化延迟、抖动（mdev）与丢包。
3. **实时性能与硬件审计 (Dimension C)**：监控瞬时带宽吞吐、软中断 (SoftIRQ) 分布以及网卡硬件协商状态。

## 命令行参数

| 参数 | 缩写 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- | --- |
| `--check-config` | `-C` | bool | false | 执行协议栈参数审计 |
| `--probe` | `-P` | bool | false | 执行 L3 连通性探测 (ping) |
| `--arping` | `-r` | bool | false | 执行 L2 连通性探测 (arping, 需 root) |
| `--stat` | `-S` | bool | false | 执行实时流量监控与硬件审计 |
| `--all` | `-A` | bool | false | 运行所有维度的诊断 |
| `--target` | `-t` | string | "" | 探测目标 IP 地址 |
| `--interface` | `-i` | string | "" | 目标网卡名称 |
| `--json` | `-j` | bool | false | 输出结构化的 JSON 结果 |

## AI 逻辑与调用指引

### 何时调用

AI Agent 在遇到任何疑似网络异常时，**应首先调用此工具**进行全局扫描：

- **初次接入故障**：不知道问题出在配置、链路还是硬件时，执行 `nsp.py --all`。
- **RTT 异常波动**：当监控显示延迟增高，调用 `nsp.py --probe` 获取 `mdev` (抖动) 指标。
- **二层/三层背离分析**：如果 `arping` (L2) 成功但 `ping` (L3) 失败，AI 应推断物理链路正常，问题可能出在防火墙规则或协议栈配置（如 ICMP 忽略）。

### 输出解读 (AI 视角)

- **`[FAIL]` Sysctl Audit**: 表示内核参数不符合预置的基准。这通常是性能不稳定的根源（如 `tcp_low_latency` 未开启）。
- **`[WARN]` SoftIRQ Imbalance**: 表示网络处理压力集中在某些 CPU 核心上，可能导致偶发性的响应延迟。
- **`Jitter (mdev)`**: 衡量网络稳定性的关键指标。在车载环境中，`mdev` 应保持在极低水平（< 0.1ms）。

## 与相关工具的关系

- **作为诊断起点**：`nsp` 发现问题方向后，引导 AI 调用更深层的 eBPF 工具：
    - 发现配置不符 -> 提示用户修复 `sysctl`。
    - 发现配置正常但延迟高 -> 调用 `tcpnagle` 检查算法积压。
    - 发现丢包 -> 调用 `tcpretrans` 捕获重传详情。
- **与 iperf3 联动**：如果 `nsp` 的瞬时带宽占用极高，AI 可进一步调用 `nsp.py --iperf` 进行压力测试，确认带宽瓶颈。
