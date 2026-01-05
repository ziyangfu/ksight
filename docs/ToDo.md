## ToDo Lists

### V0.0.2
- [ ] 修改代码补全的实现逻辑。
    - 当前由工具生成json文件，然后在ksightCli中读取json文件，方案不合适。不合适的原因是，当前的cmdParser，不直观，并且如果要修改，依然需要修改2处
    - 考虑更换为：
    - 每个工具自己写一个bash-complete脚本。然后有一个工具，读取bash-complete，生成ksightCli的command_data.py。同时，如果工具想单独使用，也可以有自动补全功能。
- [ ] BUG修复：netwatcher运行异常，显示killed
- [ ] 新工具：wirefisher，适配当前架构
- [ ] 新工具：cpuwatcher，使用新开发语言重构，并适配当前架构
- [x] ARM64架构编译支持。当前run.sh在ARM64平台下无法编译
    - 当前暂不考虑交叉编译，即x86-64及ARM64平台的原生编译
- [ ] uprobe 存在性能问题，明确，且考虑bpftime
- [ ] 集成rms用户态调度器。因为主要为内核态程序，用户态程序当前沿用C即可。
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

### V0.0.5

### V0.0.6
