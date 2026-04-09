## ksight & ksightCli

## 1. 简述

ksight：采用eBPF技术的后端数据采集器

ksightCli：将所有的后端工具统一到一个命令行前端，并且具备Tab自动补全功能





tool_config_generator：

功能：为每个自己开发的工具，生成一个专属的config_<tool_name>.yaml 文件，这个文件，定义了当前工具的所有参数。这个文件，会在编译时，自动调用tool_config_generator生成对应的yaml文件。

这个tool_name从哪里来？

硬性规定，tool_name与文件夹的名字一致，那么tool_name直接读取src下的对应子系统的文件夹名即可。

对于每个第三方工具，目前是手动为其添加yaml文件。









```bash
./netwatcher -h

Usage: net_watcher [--help] [--version] [--all] [--err] [--extra] [--retrans] [--time] [--http] [--sport VAR] [--dport VAR] [--udp] [--net_filter] [--drop_reason] [--addr_to_func] [--icmptime] [--tcpstate] [--timeload] [--dns] [--stack] [--mysql] [--redis] [--count VAR] [--rtt] [--rst_counters]

Watch tcp/ip in network subsystem

Optional arguments:
  -h, --help          shows help message and exits 
  -v, --version       prints version information and exits 
  -a, --all           set to trace CLOSED connection 
  -e, --err           set to trace TCP error packets 
  -x, --extra         set to trace extra conn info 
  -r, --retrans       set to trace extra retrans info 
  -t, --time          set to trace layer time of each packet 
  -i, --http          set to trace http info 
  -s, --sport         trace this source port only [nargs=0..1] [default: 0]
  -d, --dport         trace this destination port only [nargs=0..1] [default: 0]
  -u, --udp           trace the udp message 
  -n, --net_filter    trace ipv4 packget filter 
  -k, --drop_reason   trace kfree 
  -F, --addr_to_func  translation addr to func and offset 
  -I, --icmptime      set to trace layer time of icmp 
  -S, --tcpstate      set to trace tcpstate 
  -L, --timeload      analysis time load 
  -D, --dns           set to trace dns information 
  -A, --stack         set to trace of stack 
  -M, --mysql         set to trace mysql information 
  -R, --redis         set to trace redis information 
  -C, --count         specify the time to count the number of requests [nargs=0..1] [default: 0]
  -T, --rtt           set to trace rtt 
  -U, --rst_counters  set to trace rst
```





```bash
./nettrace -h

nettrace: a tool to trace skb in kernel and diagnose network problem

Usage:
    -s, --saddr      filter source ip/ipv6 address
    -d, --daddr      filter dest ip/ipv6 address
    --addr           filter source or dest ip/ipv6 address
    -S, --sport      filter source TCP/UDP port
    -D, --dport      filter dest TCP/UDP port
    -P, --port       filter source or dest TCP/UDP port
    -p, --proto      filter L3/L4 protocol, such as 'tcp', 'arp'
    --netns          filter by net namespace inode
    --netns-current  filter by current net namespace
    --pid            filter by current process id(pid)
    --min-latency    filter by the minial time to live of the skb in ms

    -t, --trace      enable trace group or trace. Some traces are disabled by default, use "all" to enable all
    --force          skip some check and force load nettrace
    --ret            show function return value
    --detail         show extern packet info, such as pid, ifname, etc
    --date           print timestamp in date-time format
    --basic          use 'basic' trace mode, don't trace skb's life
    --diag           enable 'diagnose' mode
    --diag-quiet     only print abnormal packet
    --diag-keep      don't quit when abnormal packet found
    --hooks          print netfilter hooks if dropping by netfilter
    --drop           skb drop monitor mode, for replace of 'droptrace'
    --drop-stack     print the kernel function call stack of kfree_skb
    --sock           enable 'sock' mode
    --monitor        enable 'monitor' mode
    --pkt-fixed      set this option if you are sure the target packet is not NATed to get better performance
    --trace-stack    print call stack for traces or group

    -v               show log information
    --debug          show debug information
    -h, --help       show help information
    -V, --version    show nettrace version

```



```bash
./ipcwatcher -h

Usage: ipcwatcher [--help] [--version] [--uds] [--mmap] [--pid VAR] [--filterPath VAR] [--traceNoAnonUds] [--payload] [--force] [--pcapFile VAR] [--fromJson] [--verbose] [--version]

Linux IPC Watcher 工具 - 用于监控 Unix 域套接字通信及 mmap 活动。
                   "支持过滤、打印载荷、保存到 pcap 文件等功能

Optional arguments:
  -h, --help        shows help message and exits 
  -v, --version     prints version information and exits 
  -u, --uds         Trace unix domain socket 
  -m, --mmap        Trace mmap 
  -p, --pid         filter via send pid [nargs=0..1] [default: 0]
  --filterPath      Filter path [nargs=0..1] [default: ""]
  --traceNoAnonUds  only trace no anon uds like /tmp/sample.uds 
  --payload         Print payload 
  --force           Force enable payload printing 
  --pcapFile        Save output to pcap file [nargs=0..1] [default: ""]
  --fromJson        read config args from json file 
  --vvv, --verbose  Output more information 
  -v, --version     Output version information 

使用示例:
root, using sudo in ubuntu
  ipcwatcher --uds --pid=1234 --payload            # 跟踪指定 PID 的 UDS 通信并打印数据
  ipcwatcher --uds --filterPath=/tmp/test.sock     # 只跟踪路径为 /tmp/test.sock 的 UDS
  ipcwatcher --uds --pcapFile=output.pcap          # 将抓取的数据保存到 pcap 文件
  ipcwatcher --version                             # 显示版本信息 
```









随着工具的开发，这些工具的命令行参数，是可能变化的。如果使用click，如何同步这些变化。我初步的设计是使用一个tool_config_generator。来读取工具的命令，生成一个专属的config_<tool_name>.yaml 文件，这个文件，定义了当前工具的所有参数。这个文件，会在编译时，自动调用tool_config_generator生成对应的yaml文件。然后ksightCli会读取yaml文件。这并不需要每次运行时读取。整个工具集的编译安装，我是用一个run.sh脚本完成的。它会完成工具的编译，安装，yaml文件生成等等工作。请评估该方案，你有什么更好方案吗









我是一个C、C++及Python开发者，目前正在开发一个一款用于Linux内核可观测性的工具，它采用eBPF技术，将会涉及CPU、文件、内存以及网络子系统，目前你看到的ksight是采用eBPF技术的后端数据采集器。采用的设计架构是，针对不同的内核子系统，它会有不同的二进制工具，这些工具，内核代码采用C语言编写，用户态代码使用C++17编写。这些工具，一部分是自己开发的工具，相关代码在src文件夹下。一部分是集成第三方的优质工具，例如third_tools/binary下的二进制工具nettrace。这些工具会有很多的命令行参数，以实现不同的功能。

如果针对不同的需求，需要寻找并逐一使用不同的工具，用户体验不佳。因此提供了一个叫做ksightCli的Python3程序，它的功能是将所有的后端工具统一到一个命令行前端，并且具备Tab自动补全功能。

整个工作流是这样的。

首先，用户运行`sudo run.sh`。它会完成程序的前置条件检测、工具的编译、安装、以及ksightCli的配置。更具体的run.sh的功能定义，会在后续给出。

在运行结束run.sh后，默认情况下，程序会安装在：` /usr/local/bin/ksight`下。

`/usr/local/bin`下呈现的与ksight有关的文件夹目录类似于下面这种形式：

当前只有2个自己开发工具ipcwatcher与netwatcher，以及一个第三方工具nettrace。后续会集成更多。

```bash
├── ksight
│   ├── ipc
│   │   └── ipcwatcher
│   │       ├── bin
│   │       │   └── ipcwatcher
│   │       └── config
│   │           └── ipcwatcher_args.json
│   ├── ksightCli
│   │   └── ksightCli
│   └── net
│       ├── nettrace
│       │   ├── bin
│       │   │   └── nettrace
│       │   └── config
│       │       └── nettrace_args.json
│       └── netwatcher
│           ├── bin
│           │   └── netwatcher
│           └── config
│               └── netwatcher_args.json
└── ksightCli -> ./ksight/ksightCli/ksightCli
```

`/usr/local/bin/ksightCli`是一个软链接，链接到`./ksight/ksightCli/ksightCli`

因为Linux 的env的path是包括`/usr/local/bin`的，因此用户可以在任一目录下直接使用ksightCli。用户使用的方式是类似下面这样的：

```bash
ksightCli nettrace -h

nettrace: a tool to trace skb in kernel and diagnose network problem

Usage:
    -s, --saddr      filter source ip/ipv6 address
    -d, --daddr      filter dest ip/ipv6 address
    --addr           filter source or dest ip/ipv6 address
    -S, --sport      filter source TCP/UDP port
    -D, --dport      filter dest TCP/UDP port
    -P, --port       filter source or dest TCP/UDP port
    -p, --proto      filter L3/L4 protocol, such as 'tcp', 'arp'
    --netns          filter by net namespace inode
    --netns-current  filter by current net namespace
    --pid            filter by current process id(pid)
    --min-latency    filter by the minial time to live of the skb in ms

    -t, --trace      enable trace group or trace. Some traces are disabled by default, use "all" to enable all
    --force          skip some check and force load nettrace
    --ret            show function return value
    --detail         show extern packet info, such as pid, ifname, etc
    --date           print timestamp in date-time format
    --basic          use 'basic' trace mode, don't trace skb's life
    --diag           enable 'diagnose' mode
    --diag-quiet     only print abnormal packet
    --diag-keep      don't quit when abnormal packet found
    --hooks          print netfilter hooks if dropping by netfilter
    --drop           skb drop monitor mode, for replace of 'droptrace'
    --drop-stack     print the kernel function call stack of kfree_skb
    --sock           enable 'sock' mode
    --monitor        enable 'monitor' mode
    --pkt-fixed      set this option if you are sure the target packet is not NATed to get better performance
    --trace-stack    print call stack for traces or group

    -v               show log information
    --debug          show debug information
    -h, --help       show help information
    -V, --version    show nettrace version

```

用户可以使用Tab自动补全，类似如下：

```bash
ksightCli <Tab>
ipcwatcher nettrace nettrace

ksightCli nettrace <Tab>
--addr           --diag           --min-latency    -s
--basic          --diag-keep      --monitor        -S
--bpf-debug      --diag-quiet     --netns          --saddr
-d               --dport          --netns-current  --sock
-D               --drop           -p               --sport
--daddr          --drop-stack     -P               -t
--date           -h               --pid            --trace
--debug          --help           --port           --trace-stack
--detail         --hooks          --ret            -v
```

现在来说明ksightCli与run.sh的核心功能。先说明ksightCli。

ksightCli的核心功能有2个。

第一个是参数的自动补全。这部分的工作是分2步完成的。

- 第一步，在src下的自己开发的工具，在argParser中，均有一个generateConfigJson的命令行参数。它的核心功能，就是将parser中有的参数，例如 --vvv --verbose等参数，写入到一个叫做`<tool_name>_args.json`的json文件中，如果命令有多级子命令，在按照多级子命令的顺序布局，写入到json文件中。
- 第二步，有一个名为“gen_cmd_data.py”的Python程序。它会收集他们的JSON元数据，假如工具在`<tool_name>/bin/`下，则对应的json文件在`<tool_name>/config/`下。gen_cmd_data.py然后会读取`kight/<ipc、net...>/<tool_name>/config/<tool_name>_args.json`，基于一个给定的模板，生成一个特定的Python文件，命名为commands_data.py，放置在ksightCli文件夹下。
- 上述2步工作，都是在run.sh中自动完成的。
- kisghtCli是基于click实现自动补全的。注意，我当前的Python版本是3.8，注意click的版本，所需要的Python不要超过3.8。

第二个是工具的运行。

最后，再来说明run.sh的核心功能与原理。

options:

- --install-dir <dir>: 安装目录。默认为 /usr/local/bin/ksight下 

功能：用户通过运行这个脚本，实现以下功能：

- 前置条件检测，包括Python、click、cmake、make等等，包括但不限于此，若你觉得必要，请补充。
- 自有工具的编译安装
  - 在当前文件夹新建build文件夹，cd到build文件夹，并通过cmake与make编译各个工具。
  - 通过make install 安装工具到安装目录。
  - 遍历安装文件夹下的所有二进制工具，逐一执行该二进制文件，并带有命令行参数generateConfigJson。它的核心功能，就是将parser中有的参数，例如 --vvv --verbose等参数，写入到一个叫做`<tool_name>_args.json`的json文件中，如果命令有多级子命令，在按照多级子命令的顺序布局，写入到json文件中。这个文件放置在对应工具bin目录同级的config目录下。

上述步骤完成后，安装目录类似于下面：

```bash
├── ksight
│   ├── ipc
│   │   └── ipcwatcher
│   │       ├── bin
│   │       │   └── ipcwatcher
│   │       └── config
│   │           └── ipcwatcher_args.json
│   └── net
│       └── netwatcher
│           ├── bin
│           │   └── netwatcher
│           └── config
│               └── netwatcher_args.json
```

- 第三方工具的编译安装
  - 给定一个bool值，is_build_from_src 默认为false，false的意思是不编译，直接复制二进制文件。true的意思是直接从源码编译。这部分请暂时空缺，不实现。
  - is_build_from_src == false的情况下，通过cp将所有的第三方工具安装到安装目录对应的位置，这里需要查看当前平台的架构，并选择将架构特定的工具安装。当前仅支持x86-64与aarch64 2种架构。同样的，二进制文件，放置bin下，配置文件放在config下。第三方工具的<tool_name>_args.json文件，当前是手写的，格式会参照自有工具生成的json文件。

上述步骤完成后，安装目录类似于下面：

```bash
├── ksight
│   ├── ipc
│   │   └── ipcwatcher
│   │       ├── bin
│   │       │   └── ipcwatcher
│   │       └── config
│   │           └── ipcwatcher_args.json
│   └── net
│       ├── nettrace
│       │   ├── bin
│       │   │   └── nettrace
│       │   └── config
│       │       └── nettrace_args.json
│       └── netwatcher
│           ├── bin
│           │   └── netwatcher
│           └── config
│               └── netwatcher_args.json
```

- ksightCli的安装
  - 复制ksightCli的所有文件到安装目录/ksightCli下。
  - 运行`sudo gen_cmd_data.py`。它会扫描所有的JSON文件，基于模板参数，自动生成可被click识别的commands_data.py，并将文件放置在ksightCli下。
  - 创建`/usr/local/bin/ksightCli`软链接，链接到`<install_dir>/ksightCli/ksightCli`

上述步骤完成后，安装目录类似于下面：

```bash
├── ksight
│   ├── ipc
│   │   └── ipcwatcher
│   │       ├── bin
│   │       │   └── ipcwatcher
│   │       └── config
│   │           └── ipcwatcher_args.json
│   ├── ksightCli
│   │   └── ksightCli
|	|	|__ commands_data.py
│   └── net
│       ├── nettrace
│       │   ├── bin
│       │   │   └── nettrace
│       │   └── config
│       │       └── nettrace_args.json
│       └── netwatcher
│           ├── bin
│           │   └── netwatcher
│           └── config
│               └── netwatcher_args.json
└── ksightCli -> ./ksight/ksightCli/ksightCli
```

请在后续的协同开发中，遵循以下原则：

**核心理念与原则**

> **简洁至上**：恪守KISS（Keep It Simple, Stupid）原则，崇尚简洁与可维护性，避免过度工程化与不必要的防御性设计。
> **深度分析**：立足于第一性原理（First Principles Thinking）剖析问题，并善用工具以提升效率。
> **事实为本**：以事实为最高准则。若有任何谬误，恳请坦率斧正，助我精进。

**开发工作流**

> **渐进式开发**：通过多轮对话迭代，明确并实现需求。在着手任何设计或编码工作前，必须完成前期调研并厘清所有疑点。
> **结构化流程**：严格遵循“构思方案 → 提请审核 → 分解为具体任务”的作业顺序。

**输出规范**

> **语言要求**：所有回复、思考过程及任务清单，均须使用中文。

