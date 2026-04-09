# vsomeip USDT 插桩分析计划

本计划旨在深入分析 `vsomeip` 库并在关键路径上确定 USDT（User-level Statically Defined Tracing）插桩点，以支持全链路跟踪诊断。

## 用户审核事项

> [!IMPORTANT]
> USDT 插桩需要修改 `vsomeip` 源码并重新编译。
> 我们需要平衡“性能损耗”与“可观测性”。过多的插桩点会增加编译复杂度和运行时略微的开销（虽然 USDT 在关闭时损耗极低）。

## 任务分解

### 1. 技术调研与原理解析
- [ ] 阐述 USDT 的定义、工作原理以及在 eBPF 生态中的地位。
- [ ] 比较 USDT 与 Uprobe 的优劣。

### 2. vsomeip 源码深度分析
- [ ] **接口层 (Application)**: 分析 `send`, `on_message` 的入口。
- [ ] **协议层 (Message/Protocol)**: 寻找序列化与反序列化的核心函数。
- [ ] **传输层 (Endpoints)**: 定位 TCP/UDP/Local 实际写入 Socket 的位置。
- [ ] **路由层 (Routing)**: 分析消息分发逻辑。

### 3. 生成插桩建议报告
- [ ] 编写 `vsomeip_usdt_analysis.md` 详细记录建议的插桩点、参数定义及业务价值。

## 待确认问题
- 是否需要支持 SOME/IP-SD (Service Discovery) 的跟踪？
- 是否关注 E2E 保护 (E2E Protection) 逻辑的插桩？

## 验证计划
- 模拟 `vsomeip` 应用，通过 `bpftrace` 或 `ksight` 工具捕获插桩点数据。
