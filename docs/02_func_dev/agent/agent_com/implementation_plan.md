# Agent 与诊断工具通信功能实现计划

本计划旨在实现 Agent 调用诊断工具后，通过 Unix Domain Socket (UDS) 同步读取工具输出结果的功能。

## 用户审核事项

> [!IMPORTANT]
> 1. **UDS 路径约定**：建议工具端通过环境变量 `KSIGHT_AGENT_SOCK` 获取 UDS 路径，Agent 在启动工具前设置该变量。
> 2. **数据格式**：统一使用 JSON 格式。工具端需要根据输出逻辑组织结构化的 JSON 数据。
> 3. **输出逻辑**：开启 `--agent` 模式后，**不再保留** stdout 的实时输出，结果仅通过 UDS 发送。
> 4. **控制流**：对于持续监控类工具，Agent 应当能通过 UDS 向工具发送退出指令，工具接收后主动退出。当前 `tcpnagle` 优先实现单次运行模式。

## 提议的变更

---

### [Component] C++ 工具端 (tcpnagle)

#### [MODIFY] [ConfigArgs.h](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/src/net/tcpnagle/include/ConfigArgs.h)
- 增加 `bool agentMode` 和 `bool outputJson` 成员。

#### [MODIFY] [ArgParser.cpp](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/src/net/tcpnagle/src/ArgParser.cpp)
- 解析 `--agent` 和 `--ojson` 命令行参数。

#### [NEW] [AgentCom.hpp](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/src/net/tcpnagle/include/AgentCom.hpp)
- 实现 UDS 客户端类，支持将 `nlohmann::json` 对象发送到指定或环境变量读取的 Socket。

#### [MODIFY] [TcpNagleBpf.cpp](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/src/net/tcpnagle/src/TcpNagleBpf.cpp)
- 在 `processEvent` 中根据 `agentMode` 收集数据。
- 在 `run` 结束时，若为 `agentMode`，则构造最终 JSON 并通过 `AgentCom` 发送。

---

### [Component] Python Agent 端

#### [NEW] [ipc.py](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/agent/ipc.py)
- 实现 `UdsServer` 类，提供简单的上下文管理器或异步接口来接收工具端的 JSON 输出。

#### [MODIFY] [tools.py](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/agent/tools.py)
- 修改 `ToolExecutor.execute`：
    - 在运行 `ksight_` 系列工具前，若检测到是代理请求，则注入环境变量 `KSIGHT_AGENT_SOCK`。
    - 在后台或同步等待循环中启动 `UdsServer` 接收数据。
    - 将接收到的 JSON 内容作为工具执行结果返回。

---

## 验证计划

### 自动化测试
- 编写脚本模拟 Agent 启动 UDS Server，并调用带参数的 `tcpnagle`。
- 校验 UDS 接收到的数据是否为合法的 JSON 且内容正确。

### 自动化测试
- 编写脚本模拟 Agent 启动 UDS Server，并调用带参数的 `tcpnagle`。
- 校验 UDS 接收到的数据是否为合法的 JSON 且内容正确。

### 手动验证
- 使用 Agent 进入交互模式。
- 执行询问 nagle 状态的指令。
- 观察 Agent 是否能正确解析由 UDS 传回的 JSON 数据并给出 AI 回复。
