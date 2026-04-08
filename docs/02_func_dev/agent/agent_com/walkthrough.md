# Agent 与诊断工具通信功能实现总结

本功能实现了 Agent 与诊断工具之间的高效、结构化通信。工具现在可以通过 Unix Domain Socket (UDS) 将结果以 JSON 格式直接发送给 Agent，避免了传统的 stdout 解析带来的语义丢失和格式不稳定性。

## 主要变更项

---

### [Component] C++ 工具端 (tcpnagle)

#### 1. 参数与配置支持
- 修改了 [ConfigArgs.h](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/src/net/tcpnagle/include/ConfigArgs.h) 和 [ArgParser.cpp](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/src/net/tcpnagle/src/ArgParser.cpp)，增加了 `--agent` 和 `--ojson` 启动参数。

#### 2. UDS 通信实现
- 新增了 [AgentCom.hpp](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/src/net/tcpnagle/include/AgentCom.hpp)，实现了 `AgentCom` 类，用于连接环境变量 `KSIGHT_AGENT_SOCK` 指定的 UDS 并发送 JSON 数据。

#### 3. 结构化输出逻辑
- 修改了 [TcpNagleBpf.cpp](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/src/net/tcpnagle/src/TcpNagleBpf.cpp)，在 `agentMode` 下实时收集快照信息到 `nlohmann::json` 数组，并在运行结束时统一种子发送。

---

### [Component] Python Agent 端

#### 1. IPC 通信层
- 新增了 [agent/ipc.py](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/agent/ipc.py)，实现了基于 `AF_UNIX` 的 `UdsServer`，支持同步非阻塞地接收 JSON 响应。

#### 2. 工具执行器适配
- 修改了 [agent/tools.py](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/agent/tools.py)：
    - 增加了 `_run_with_uds` 私有方法，封装了 UDS Server 启动、环境变量注入及结果接收的完整流程。
    - 在 `KSIGHT_TOOLS` 中添加了 `ksight_tcpnagle` 的定义。
    - 更新了所有 `ksight_` 系列工具的调用逻辑，优先使用 UDS 模式接收结构化数据。

---

## 验证结果

### 1. 编译验证
通过 `build_tmp` 目录完成了 `tcpnagle` 的编译，验证了 C++ 代码的正确性和依赖项的链接。

### 2. 通信逻辑验证
编写了 [test_uds_mock.py](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/test_uds_mock.py)，模拟工具端向 Agent 发送 JSON 数据。
- **测试状态**：✅ **PASSED**
- **接收到的数据示例**：
  ```json
  [
    {
      "type": "connection",
      "local": "127.0.0.1:1234",
      "remote": "127.0.0.1:80",
      "status": "DISABLED"
    },
    ...
  ]
  ```

---

## 后续建议

> [!TIP]
> 1. **权限说明**：由于 BPF 工具运行需要 root 权限，在生产或测试环境中调用时，请确保运行 Agent 的用户具有 passwordless sudo 权限，或者通过 root 身份启动。
> 2. **持续监控模式**：当前 `tcpnagle` 采用单次扫描模式。若未来需要支持 `sockops` 持续监控，Agent 可通过 UDS 向工具发送退出码来终止其运行。
