通信功能开发
现在我们需要实现一个功能，通信，agnet调用工具后，它需要能够读取到工具的输出。也就是说agent启动时，同步启动一个uds server，它在调用工具运行后，工具会创建一个uds client，与它通信，将输出的结果发给它。我们现在以tcpnagle这个工具为例。agent调用tcpnagle，tcpnagle发送结果给它。tcpnagle需要有一个 --ojson表示结果会以json的形式输出，--agent表示这是agent模式。因此它需要启动uds client并发送结果。你是专业的软件开发工程师，请你先理解后，给出技术方案。

**AI Agent 扮演的是“数据中心/控制塔”的角色**，而底层的 C++ 诊断工具则是“数据生产者”。

以下是基于架构师视角对该模式的深度细化建议：

### 1. 通信拓扑设计：多对一 (N:1)

由于 AI Agent 作为服务端（Server），它可以同时处理多个诊断工具的连接。

* **服务端 (Python AI Agent)：** 绑定一个固定的 UDS 路径（例如 `/tmp/ksight_agent.sock`），使用 `asyncio` 或多线程监听连接。
* **客户端 (C++ Tools)：** 启动后立即连接该路径，将结构化数据（JSON/Protobuf）推送到 Socket，任务结束或被手动停止后关闭连接。

### 2. 为什么这种“Agent 为 Server”的模式更好？

1.  **生命周期解耦：** AI Agent 通常是常驻进程，负责决策和 UI 交互；而诊断工具（如 `nettrace`）往往是按需启动、任务完结即退出的。让长寿命进程当 Server 更稳定。
2.  **自动发现：** 只要 C++ 工具知道固定的 Socket 路径，它不需要知道 Agent 的 PID，直接连上去就能发数据。
3.  **权限管理：** 在车载 Linux 中，你可以通过设置 UDS 文件的权限（`chmod 660`），确保只有特定组的用户（如 `ksight` 组）能与 Agent 通信。

---

### 3. 工程实现中的“避坑”指南

#### A. 粘包与拆包问题 (Framing)
UDS 是基于流的（Stream-oriented）。如果 C++ 发送数据太快，Python 可能会一次性读到两个 JSON 对象的拼接，导致解析失败。
* **建议：** 在每个数据包前加一个 **4 字节的 Header**，表示后续 Payload 的长度。
* **逻辑：** `[Length(4 bytes)] + [JSON Data]`。Python 先读 4 字节，再按长度读取完整内容。

#### B. 异常处理 (Graceful Shutdown)
* **Client 端：** 如果 AI Agent 崩溃了，C++ 工具应该有重连机制，或者在尝试写入失败后优雅退出，避免僵死。
* **Server 端：** 某个工具异常退出（没有关闭 Socket 直接杀进程），Agent 需要能捕捉到 `ConnectionResetError` 并清理对应的资源。

#### C. 数据格式的一致性
建议定义一套通用的 **Message Envelope**（消息信封）：
```json
{
    "tool_name": "tcpnagle",
    "command": "...... --ojson --agent",
    "timestamp": 1712486527,
    "msg_type": "DATA",  // 或者 "HEARTBEAT", "ERROR", "STATUS"
    "payload": { ... 每个工具特有的数据 ... }
}
```

---

### 4. 极简代码原型示例

#### **C++ 客户端 (简略版)**
```cpp
#include <sys/socket.h>
#include <sys/un.h>

void send_to_agent(const std::string& json_data) {
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, "/tmp/ksight_agent.sock", sizeof(addr.sun_path)-1);

    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == 0) {
        // 实际开发建议加上 4 字节长度头
        send(sock, json_data.c_str(), json_data.size(), 0);
    }
    close(sock);
}
```

#### **Python 服务端 (使用 asyncio)**
```python
import asyncio
import os

async def handle_tool_data(reader, writer):
    try:
        while True:
            data = await reader.read(4096) # 实际应按长度头读取
            if not data: break
            message = data.decode()
            print(f"收到诊断数据: {message}")
            # AI Agent 在此进行逻辑处理
    finally:
        writer.close()

async def main():
    server = await asyncio.start_unix_server(
        handle_tool_data, path='/tmp/ksight_agent.sock'
    )
    async with server:
        await server.serve_forever()

if __name__ == "__main__":
    if os.path.exists('/tmp/ksight_agent.sock'):
        os.remove('/tmp/ksight_agent.sock')
    asyncio.run(main())
```
