## ToDo Lists

- [ ] 如何启动所有工具？并且能够按照需要启动
- [ ] 如何聚合所有工具到一个命令行前端，形成一个好用的命令行工具

> 这个可以参照buildHelper，以及 ip 指令
>
> 采用python开发，shell辅助
>
> 支持命令自动补全（GNU readline库？）
>
> ```bash
> # 检查所有工具的运行环境是否满足
> # 如内核相关配置选项是否开启： BTF,BPF等等
> # 每一个工具写一个脚本，写上它需要的条件是什么？
> # 然后是否可以一个个的检查条件是否满足
> ./sbin/MagicEyes --check   
> ./sbin/MagicEyes -h   # MagicEyes的帮助文件
> ./sbin/MagicEyes cpu cpuwatcher -h # 单个工具的帮助文件
> ./sbin/MagicEyes --list   # 列出所有可用的工具
> ```

- [ ] 如何交叉编译ARM64架构可执行文件（是否有必要？优先级：后）
- [ ] bridge开发，采用python开发
- [ ] 人性化的可视化前端











```bash
install
	|--- bin_backend
	|--- etc
			|--- fs
			|--- net
			|--- process
			|--- memory
	|--- brige
	|		|--- prometheus_client
	|---visualization
			|--- run docker(grafana + prometheus)
	|--- run.sh/py
```

run.sh: 启动run docker, prometheus_client。prometheus_client启动backend后端的工具 
