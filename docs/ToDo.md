## ToDo Lists

### V0.0.2

- [X] 修改代码补全的实现逻辑。
  - 当前由工具生成json文件，然后在ksightCli中读取json文件，方案不合适。不合适的原因是，当前的cmdParser，不直观，并且如果要修改，依然需要修改2处
  - 更换为：
  - 每个工具自己写一个bash-complete脚本。然后有一个工具，读取bash-complete，生成ksightCli的commands_data.py。同时，如果工具想单独使用，也可以有自动补全功能。
- [X] BUG修复：netwatcher运行异常，显示killed， x86-64上测试无异常
- [ ] 新工具：wirefisher，用于流量控制，适配当前架构
- [ ] 新工具：cpuwatcher，使用新开发语言重构，并适配当前架构
- [ ] 新工具：tcpaccd，用于TCP在本地IPC通信中加速的守护进程
- [X] ARM64架构编译支持。当前run.sh在ARM64平台下无法编译
  - 当前暂不考虑交叉编译，交叉编译bootstrap源仓库已支持，可以后续移植。即x86-64及ARM64平台的原生编译
- [ ] libbpf及bpftool，不用每次都自己编译，可以用现成的
  - ubuntu20.04中的libbpf版本过低，否则可以直接apt 安装libbpf
- [ ] libbpf-bootstrap作为第三方库引入？迭代更新整个构建工程
- [ ] uprobe 存在性能问题，明确，且考虑bpftime
- [ ] 集成rms用户态调度器。因为主要为内核态程序，用户态程序当前沿用C即可。
- [ ] BUG修复：GCC编译器IPA编译优化时，带来的部分kprobe挂载点带有尾缀：.isra.0
  - 短期解决：执行编译器检查，并检查grep "ip_rcv_core" /proc/kallsyms，编译前决定。
  - 长期解决：BTF 使用fentry替代kprobe，BTF自己处理

### V0.0.3

- [ ] connector，使用Python开发，为HTTP server，功能为终端管理与消息转发路由。
  - 该程序默认随系统systemd启动，名称为ksight-connector，放在新仓库中。
  - 后端采集器与前端程序，均作为client接入。
  - 前端程序启动后，发送消息，接入connector。若需要启动后端采集器，发送通信协议中定义的消息给connector，connector再启动后端采集器。后端采集器获得的消息，发送给connector，connector再发送给前端程序。
  - 前后端的控制交互，均在通信协议中定义。
    至于connector是与前端放在一个设备上，还是与后端采集器放在一个设备上，待定，倾向于与后端采集器放在一个设备上。
  - 可能会有一个前端，对接多台设备的情况。可以选择设备ID，或者设备名称。
  - 更像一个管理系统？

### V0.0.4

- [ ] 集成更多的工具
- [ ] AI协同，前端开发

### V0.0.5

### V0.0.6
