# netstack_probe (nsp) 开发计划

`netstack_probe` (简称 `nsp`) 被定位为 ksight 项目中的 1 级网络诊断入口。它是一个纯 Python 实现的非 BPF 工具，旨在不依赖 eBPF 的环境下，为 Agent 提供快速、全面的网络健康状态评估。

## 用户审核记录

> [!IMPORTANT]
> 1. 本工具由于涉及网络探测（Ping）和压测（iperf3），在高负载生产环境下调用主动探测功能时需谨慎。
> 2. 部分硬件级审计需要 `ethtool` 支持，若系统中未安装，该模块将自动跳过。

## 拟议方案

### 核心架构

工具采用模块化设计，分为以下三个维度：

#### 1. 维度 A：声明式配置审计 (Sysctl Auditor)
- **输入**：`desired_state.json`。
- **逻辑**：递归读取 `/proc/sys/net/` 下的参数，与 JSON 中定义的期望值进行逐一比对。
- **输出**：列出所有不符合预期的参数项及其当前值。

#### 2. 维度 B：端到端质量探测 (Quality Prober)
- **连通性**：封装 `ping` (L3) 和 `arping` (L2)。
- **延迟与抖动**：通过解析 `ping` 的 RTT 统计信息（mdev 字段）计算抖动。
- **吞吐量**：
    - **被动统计**：计算 `/proc/net/dev` 在 1s 内的差值。
    - **主动压测**：集成 `iperf3 --json` 模式，解析出吞吐量、丢包和重传。
- **丢包率**：提取 `ping` 的 `packet loss` 比例。

#### 3. 维度 C：实时性与中断平衡审计 (RT Auditor)
- **中断分布**：解析 `/proc/interrupts`，确认核心网卡中断是否分散或集中。
- **软中断饱和度**：解析 `/proc/softirqs`，识别 `NET_RX` / `NET_TX` 的处理瓶颈。
- **硬件协商**：通过 `ethtool` 检查 Speed/Duplex/Pause frames 特性。

### 目录结构

```text
src_gtools/net/netstack_checker/
├── nsp.py                # 主程序入口
├── nsp_modules/          # 模块化实现
│   ├── auditor.py        # 维度 A
│   ├── prober.py         # 维度 B
│   └── helper.py         # 系统文件解析工具
└── desired_state.json    # 默认期望状态配置
```

## 待办事项 (Tasks)

- [ ] 设计通用的 `desired_state.json` 模板。
- [ ] 实现 `/proc/sys/net` 递归遍历与比对逻辑。
- [ ] 实现 `ping` 输出解析器（需兼容不同版本的 ping 输出）。
- [ ] 实现基于 `/proc/net/dev` 的一秒级瞬时流量计算。
- [ ] 集成 `iperf3` JSON 解析功能。
- [ ] 开发命令行的汇总输出（人类友好）与 JSON 输出（Agent 友好）。

## 验证计划

### 自动化测试
- 编写脚本模拟 `/proc` 文件系统内容，验证解析器逻辑。
- 在本地使用 `veth` 对测试 `iperf3` 和 `ping` 模块。

### 手动验证
- 创建不符合规范的 `desired_state.json`，验证 `nsp --check-config` 是否能准确报错。
- 在有负载的情况下运行 `nsp --stat`，观察吞吐量显示是否准确。
- 断开网络连接，验证 `nsp --probe` 的 `[FAIL]` 反馈。
