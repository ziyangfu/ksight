# 复盘记录：Uprobe 动态自动挂载的实现 (Auto-Discovery & Attach)

## 1. 做的事情 (What was done)

在 `e2etracer` 的 C++ 用户态程序中，增加了一套能够**动态探测中间件运行环境**并**自动完成底层 Uprobe 挂载**的工具机制。
修改了 `E2ETracer` 的 BPF 加载逻辑，禁用了骨架（Skeleton）默认的 `autoattach`，改为使用 libbpf 的 `bpf_program__attach_uprobe_opts` 功能在用户态进行精准的手动动态注入。

## 2. 为什么这么做 (Why it was done)

* **环境的不可预测性**：真实的自动驾驶计算域环境（如 Orin、8155）非常复杂。SOA 中间件（如 vsomeip 或 DDS）所在的动态库（`.so` 文件）绝对路径可能因车企、主板包（BSP）或容器化部署的不同而发生变化。
* **规避写死路径的崩溃**：如果我们遵循传统的 BPF 示例，将 Uprobe 的路径硬编码在 `SEC("uprobe//opt/libvsomeip.so")` 内，一旦库文件位置变了，BPF 程序在启动阶段就会整体挂载失败。这对于一个期望“即插即用”的通用型诊断工具来说是不可接受的。
* **精确追踪目标应用**：依靠内核去全局 Hook 系统的动态库往往会抓到很多毫不相干的其他驻留程序，手动控制 PID 挂载不仅安全还能减少性能损耗。

## 3. 怎么做的 (How it was done)

1. **分离 Open 与 Load 阶段**：
   在 `E2ETracer.cpp` 的 `init()` 中，不再直接调用 `open_and_load`。而是在 `open` 后，对 `uprobe_middleware_send` 这个具体的探针显式声明 `bpf_program__set_autoattach(..., false)`。这样一来，核心的网络通讯探测（如 IP 层、网络栈层）得以加载，而业务层的探针被挂起保留，防止了直接崩溃。
2. **实现 `/proc` 内存映射解析**：
   设计了 `find_library_path` 函数。它的核心逻辑是读取 Linux 下指定的 `/proc/<pid>/maps` 文件。通过检索记录了内存装载信息的配置项，找到包含我们定义的 `lib_name` 且具有执行权限（`r-xp`）的那一条映射记录，提取出此时由于运行时产生的**真实绝对路径**。
3. **调用高级 libbpf Option 进行动态绑定**：
   开发了 `attach_middleware_uprobe` 核心方法。不再苦恼于如何自己在 ELF 文件里反汇编算 Offset 函数偏移量，而是利用 `bpf_uprobe_opts` 结构体的 `.func_name` 字段，把经 C++ Name Mangling（名字粉碎）后的目标函数名（比如 `_ZN10vsomeip_v3...send...`）传进去，并指示偏移为 `0`。让底层的 libbpf 库智能地去目标 `.so` 中提取地址并完成探针 `Attach`。

## 4. 结果是什么 (What is the result)

实现该功能后，我们的 `e2etracer` 成为了一个具备“自适应寻路”能力的诊断利器。
用户只需传入想要诊断的进程 `PID` 和期待的通信库关键字（例如传递 `vsomeip`），工具就会自己：**找程序 -> 找地址 -> 找函数名 -> 静默挂载**。这一切使得我们的 SOA 垂直应用层染色机制（Coloring）能够稳定、高效地运行于任何汽车软件容器架构中。
