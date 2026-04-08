agent的安装与调用
最终的使用方式是这样的：
用户在任意文件夹，都可以通过ksightCli agent chat调用agent智能体。
基于目前的现状，ksightCli agent -h可以正常调起，相关的信息你看可以查看下面的这些文本资料。

```bash
fzy@fzy-Lenovo:~$ ksightCli agent -h
Usage: ksightCli agent [OPTIONS] COMMAND [ARGS]...

  ksight AI 智能诊断：具备 AI 根因分析及自动工具调用能力的运维 Agent。

Options:
  -h, --help  Show this message and exit.

Commands:
  chat  交互式故障诊断：启动 AI 会话，AI 会根据对话内容自动调动工具排查问题。
  run   启动后台监控 Agent：自动监听系统运行状态。
fzy@fzy-Lenovo:~$ ksightCli agent chat
启动 Agent 失败: No module named 'agent'

```

usr/local/bin/
```bash
fzy@fzy-Lenovo:~$ tree /usr/local/bin/ksight/
/usr/local/bin/ksight/
├── agent
│   ├── config.py
│   ├── engine.py
│   ├── __init__.py
│   ├── llm_client.py
│   ├── prompts.py
│   └── tools.py
├── ipc
│   ├── ipcwatcher
│   │   ├── bin
│   │   │   └── ipcwatcher
│   │   ├── config
│   │   │   └── brief.json
│   │   └── scripts
│   │       └── bash-complete.sh
│   ├── shm_checker
│   │   ├── bin
│   │   ├── config
│   │   └── scripts
│   └── sofdsnoop
│       ├── bin
│       │   └── sofdsnoop
│       ├── config
│       │   └── brief.json
│       └── scripts
│           └── bash-complete.sh
├── ksightCli
│   ├── commands_data.py
│   ├── ksightCli
│   └── __pycache__
│       └── commands_data.cpython-38.pyc
├── memory
│   └── memory_leak_checker
│       ├── bin
│       ├── config
│       └── scripts
└── net
    ├── bindsnoop
    │   ├── bin
    │   │   └── bindsnoop
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── dds_tracer
    │   ├── bin
    │   ├── config
    │   └── scripts
    ├── e2e_pkg_tracer
    │   ├── bin
    │   │   ├── e2e_pkg_tracer
    │   │   └── e2e_pkg_tracer.py
    │   ├── config
    │   └── scripts
    ├── e2etracer
    │   ├── bin
    │   │   └── e2etracer
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── gethostlatency
    │   ├── bin
    │   │   └── gethostlatency
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── netstack_probe
    │   ├── bin
    │   │   ├── desired_state.json
    │   │   ├── netstack_probe
    │   │   └── nsp.py
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── nettrace
    │   ├── bin
    │   │   └── nettrace
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── netwatcher
    │   ├── bin
    │   │   └── netwatcher
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── network_probe
    │   ├── bin
    │   ├── config
    │   └── scripts
    ├── solisten
    │   ├── bin
    │   │   └── solisten
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── someip_tracer
    │   ├── bin
    │   ├── config
    │   └── scripts
    ├── tcpaccd
    │   ├── bin
    │   │   └── tcpaccd
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── tcpconnect
    │   ├── bin
    │   │   └── tcpconnect
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── tcpconnlat
    │   ├── bin
    │   │   └── tcpconnlat
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── tcplife
    │   ├── bin
    │   │   └── tcplife
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── tcpnagle
    │   ├── bin
    │   │   └── tcpnagle
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── tcppktlat
    │   ├── bin
    │   │   └── tcppktlat
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── tcpretrans
    │   ├── bin
    │   │   └── tcpretrans
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── tcprtt
    │   ├── bin
    │   │   └── tcprtt
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── tcpstates
    │   ├── bin
    │   │   └── tcpstates
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── tcpsynbl
    │   ├── bin
    │   │   └── tcpsynbl
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── tcptop
    │   ├── bin
    │   │   └── tcptop
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    ├── tcptracer
    │   ├── bin
    │   │   └── tcptracer
    │   ├── config
    │   │   └── brief.json
    │   └── scripts
    │       └── bash-complete.sh
    └── vlan_checker
        ├── bin
        │   ├── vlan_checker
        │   └── vlan_checker.py
        ├── config
        └── scripts

118 directories, 78 files

```
```bash
fzy@fzy-Lenovo:~$ ll /usr/local/bin/ksightCli 
lrwxrwxrwx 1 root root 41 4月   8 10:30 /usr/local/bin/ksightCli -> /usr/local/bin/ksight/ksightCli/ksightCli*

```

agent需要openai的库，我现在是装在venv虚拟环境中，正常使用需要激活它。
另外，agent连接的大模型，它的api key我写在了当前文件夹的.env中。

agent的安装，你需要写一个agent_install.sh的bash脚本，在agent的文件夹下，它负责agent的所有安装事宜。然后在run.sh中调用这个agent_install.sh来实现agent的整个安装。

请你理解后，给出技术方案，你作为专业的软件开发工程师与架构师，如果有更好的方案，请你提出，不要迎合，基于事实与最优方案。