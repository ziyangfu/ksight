1. ksight解决什么问题？
观测、资源管理及隔离、数据面加速
对于加速，有TCP在本地通信的加速，有面向低延迟用户态调度器Sched_ext的探索。
2.整体的通信拓扑是什么样的？
对于人作为调用方来说，直接使用ksightCli统一入口调用工具，即可。
对于AI agent作为调用方来说，采用的是多对一（N:1）的通信拓扑设计。
AI Agent 作为服务端（Server），它可以同时处理多个诊断工具的连接。
服务端 (Python AI Agent)： 绑定一个固定的 UDS 路径（例如 /tmp/ksight_agent.sock），使用 asyncio 或多线程监听连接。
客户端 (C++ Tools)： 启动后立即连接该路径，将结构化数据（JSON/Protobuf）推送到 Socket，任务结束或被手动停止后关闭连接。这是针对对于需要“持续监控、实时上报”的工具。
【考虑要不要】对于那些“运行一次、输出一次结果”的诊断工具，如tcpnagle，tcpnagle支持json格式输出，然后AI agent直接通过subprocess.run() 或 Popen 捕获 C++ 程序的 stdout

3. 智能诊断是怎么做的？

4. 垂向全链路跟踪是什么？

5. 在中间件中使用USDT打点，怎么打的点，在哪里打的点？

6. 在中间件中提供了打点的API，这样用户App也可以接入整个诊断体系


