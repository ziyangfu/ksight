# netstack_probe (nsp) 开发总结

我已完成 `netstack_probe` (以下简称 `nsp`) 工具的初期开发。这是一个面向 AI Agent 的 Level 1 网络诊断工具，旨在秒级时间内对 Linux 网络状态进行全方位扫描。

## 核心实现说明

工具通过 Python 实现了三个维度的诊断逻辑：

### [维度 A] 协议栈配置审计 (Sysctl Auditor)
- **实现文件**：[nsp.py](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/src_gtools/net/netstack_checker/nsp.py)
- **逻辑**：读取 [desired_state.json](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/src_gtools/net/netstack_checker/desired_state.json) 中的配置，并与 `/proc/sys/net/` 路径下的实际值进行实时比对。
- **验证**：能够准确发现 `net.ipv4.tcp_low_latency` 等关键参数不匹配的情况。

### [维度 B] 质量探测 (Quality Prober)
- **逻辑**：封装系统 `ping` 命令，解析输出结果以获取 RTT 统计（min/avg/max/mdev）和丢包率。
- **关键指标**：使用 `mdev` 字段作为网络抖动 (Jitter) 的首选衡量指标。

### [维度 C] 系统实时监控 (System Monitor)
- **逻辑**：
    - 解析 `/proc/net/dev` 确认网卡是否有 RX/TX 错包或丢包。
    - 解析 `/proc/softirqs` 分析 `NET_RX/TX` 是否在 CPU 核心间均衡分布，识别中断瓶颈。

## 验证结果展示

```bash
python3 nsp.py -A -t 127.0.0.1 -i lo
```

**运行输出：**
````text
--- [Dimension A] Sysctl Configuration Audit ---
[OK] net.ipv4.icmp_echo_ignore_all is 0
[OK] net.ipv4.icmp_echo_ignore_broadcasts is 1
[FAIL] net.ipv4.tcp_low_latency: expected 1, got 0  <-- 成功发现配置偏差
[OK] net.ipv4.tcp_sack is 1
...
--- [Dimension B] Quality Probing (Target: 127.0.0.1) ---
[OK] Ping 127.0.0.1 successful.
[*] RTT Avg: 0.040ms, Jitter(mdev): 0.017ms
[*] Packet Loss: 0%
...
--- [Dimension C] Real-time & Hardware Audit ---
[OK] lo is clean (No RX errors/drops)
[WARN] NET_TX is imbalanced across CPUs.          <-- 成功捕获软中断分布不均
[OK] NET_RX distribution is acceptable.
````

## 后续建议
- **Agent 调用建议**：Agent 遇到任何网络问题（如延迟高、丢包），应首选调用 `nsp.py --all` 进行体检，并根据报告中的 `[FAIL]` 或 `[WARN]` 项决定后续的精细化排查动作。
- **扩展性**：如果需要更强的吞吐量压测，建议在 Dimension B 中集成 `iperf3` 服务器探测和客户端压测逻辑。
