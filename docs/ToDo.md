## ToDo Lists

- [x] 聚合所有工具至一个命令行前端，统一入口，支持命令行补全
- [ ] 提供man信息，写法可模仿nettrace，在cmake中写好，随着make install进行安装。 man信息安装在 `/usr/share/man/zh_CN/man8`下面
- [ ] 提供工具内部complete自动补全功能，可模仿nettrace，complete在 `/usr/share/bash-completion/completions/`下，工具写一个bash-complete.sh脚本，在scripts下
- [x] 如何交叉编译ARM64架构可执行文件（是否有必要？优先级：后）
- [ ] bridge开发，采用python开发
- [ ] 人性化的可视化前端
- [ ] 可视化前端假如采用prometheus+grafana， 能否直接在前端设置参数，进而修改后端的参数与数据输出。MQTT交互（与车云通信协议一致）？
  - [ ] 不能，第一步仅做显示
- [ ] 如何启动所有工具？并且能够按照需要启动



mem_watcher:

> 内存管理专属部分，伙伴系统与slab分配器，物理内存管理
>
> 内存与进程
>
> 内存与文件
>
> DMA与RDMA跟踪

1. 内存与文件缓存部分，添加缓存计算功能，参照bcc cachestat工具



net_watcher:



net_trace:

1. 集成腾讯nettrace工具

