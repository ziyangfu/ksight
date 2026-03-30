# tcpnagle 诊断与治理指南

## 概述

`tcpnagle` 是一个用于实时观测和强制治理 TCP Nagle 算法（TCP_NODELAY）状态的工具。在车载通信、工业控制等对实时性要求极高的场景中，Nagle 算法虽然能提高带宽利用率，但会导致 40ms~200ms 的延迟，必须通过此工具进行审计和干预。

**核心功能**：

1. **状态扫描**：快照式遍历全系统所有 TCP 连接，识别哪些连接启用了 Nagle，哪些已设置 `TCP_NODELAY`。
2. **进程识别**：基于 Inode 匹配机制，精准锁定开启/关闭 Nagle 算法的进程 PID 与进程名。
3. **强制治理**：利用 eBPF `sockops` 钩子，针对特定容器或 cgroup 强制禁用 Nagle 算法，无需修改应用程序源码。

## 命令行参数

| 参数 | 缩写 | 类型 | 默认值 | 说明 |
| --- | --- | --- | --- | --- |
| `--pid` | `-p` | uint32 | 0 | 仅观测/检查指定 PID 的连接状态 |
| `--disabled-only` | `-d` | bool | false | 仅显示已禁用 Nagle 算法（已设置 TCP_NODELAY）的连接 |
| `--cgroup` | `-c` | string | "" | 挂载到指定 cgroup v2 路径，强制禁用该线下所有新连接的 Nagle 算法 |
| `--verbose` | `-vvv` | bool | false | 打印 eBPF 内部调试日志 |

## AI 逻辑与调用指引

### 何时调用

AI Agent 在遇到以下异常信号时，应优先调用此工具：

- **交互延迟 (High Jitter)**：用户反馈系统操作“跟手感”差，或者 RPC 调用存在偶发性 40ms+ 的延迟，但网络带宽负载并不高。
- **协议审计**：车载中间件要求全系统禁用 Nagle，AI 需要核实是否有第三方组件“漏掉”了 `setsockopt` 设置。
- **治理决策**：当 AI 确认延迟由 Nagle 引起，且无法立即修改业务代码时，可调用本工具的 `-c` 参数进行内核层动态修复。

### 输出解读 (AI 视角)

- **连接状态 = ❌ ENABLED**: 该连接正在使用 Nagle 算法。如果该连接承载的是小包交互（如控制指令），AI 应识别此为延迟隐患。
- **连接状态 = ✅ DISABLED**: 该连接已正确设置 `TCP_NODELAY`。
- **连接状态 = ⚠ CORKED**: 该连接设置了 `TCP_CORK`，会将数据完全聚合成满包发送，延迟会比 Nagle 更大。

## 与相关工具的关系

- **与 tcpconnlat/tcppktlat 的联动**：如果 AI 通过 `tcppktlat` 观测到数据包发送延迟极高，但 `tcpretrans` 显示没有丢包，则应调用 `tcpnagle` 检查是否由 Nagle 算法导致的数据积压。
- **作为修复手段**：`tcpnagle -c <group_path>` 是针对 Nagle 导致实时性问题的“外科手术级”修复手段，AI 在确认故障路径后可作为 Action 执行。
