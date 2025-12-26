## ksight & ksightCli

## 1. 简述

ksight：采用eBPF技术的后端数据采集器

ksightCli：将所有的后端工具统一到一个命令行前端，并且具备Tab自动补全功能

## 2. 编译与安装

直接运行`sudo run.sh`。将会完成所有事情。结束后，直接使用ksightCli就可以了。

options:

- --install_dir <dir>: 安装目录。默认为 /usr/local/bin/ksight下 

功能：用户通过运行这个脚本，实现以下功能：

- 前置条件检测，包括Python、argcomplete等
- 在当前文件夹新建build文件夹，cd到build文件夹，并通过cmake与make编译各个工具
- 运行tool_config_generator.py，为每个工具，生成yaml文件
- 执行make install 将自有工具及其配置文件安装到安装目录。包括ksightCli。
- 对于第三方工具。给定一个bool量，用于确定是否从源码编译安装，还是直接cp二进制文件。默认为直接cp二进制文件安装。
  - 通过cp将所有的第三方工具安装到安装目录对应的位置，这里需要查看当前平台的架构，并选择将架构特定的工具安装。
- 在/usr/local/bin下创建一个ksightCli的软链接，命名为ksightCli





安装在/usr/local/bin下，与ksight有关的布局如下，当前只有2个自己开发工具ipcwatcher与netwatcher，以及一个第三方工具nettrace。后续会集成更多。

```bash
├── ksight
│   ├── ipc
│   │   └── ipcwatcher
│   │       ├── bin
│   │       │   └── ipcwatcher
│   │       └── config
│   │           └── config_ipcwatcher.yaml
│   ├── ksightCli
│   │   └── ksightCli
│   └── net
│       ├── nettrace
│       │   ├── bin
│       │   │   └── nettrace
│       │   └── config
│       │       └── config_nettrace.yaml
│       └── netwatcher
│           ├── bin
│           │   └── netwatcher
│           └── config
│               └── config_netwatcher.yaml
└── ksightCli -> ./ksight/ksightCli/ksightCli
```

这个ksightCli是一个python文件。所具有的功能在后面描述。





tool_config_generator：

功能：为每个自己开发的工具，生成一个专属的config_<tool_name>.yaml 文件，这个文件，定义了当前工具的所有参数。这个文件，会在编译时，自动调用tool_config_generator生成对应的yaml文件。

这个tool_name从哪里来？

硬性规定，tool_name与文件夹的名字一致，那么tool_name直接读取src下的对应子系统的文件夹名即可。

对于每个第三方工具，目前是手动为其添加yaml文件。



### 1. 简述

。

Tips：**记得Tab**

### 2. 使用之前

```bash
mkdir build && cd build
cmake .. && make && make install
cd ./install/magic_eyes_cli
# 运行前置条件脚本
source ./before_running.sh
```

### 3. 使用

```bash
(venv) $ ./magic_eyes_cli -h
/home/fzy/Downloads/04_bcc_ebpf/MagicEyes
usage: magic_eyes_cli [-h] [-l | -c] {net,memory,system_diagnosis,process} ...

magic_eyes_cli: command tools for Linux kernel diagnosis and optimization

positional arguments:
  {net,memory,system_diagnosis,process}
    net                 tool for Linux net subsystem
    memory              tool for Linux memory subsystem
    system_diagnosis    tool for Linux system_diagnosis subsystem
    process             tool for Linux process subsystem

optional arguments:
  -h, --help            show this help message and exit

all of common options:
  -l                    list all avaliable tools
  -c                    check all tools dependency, and whether it can be run in current platform

eg: magic_eyes_cli -l
```

**固定命令**

magic_eyes_cli具有2个固定命令， 即

```bash
-l : 即list， 列出所有可用的后端命令
-c : 即check， 检查所有运行依赖项（暂未实现）
```

**动态命令**

{net,memory,system_diagnosis,process}为动态命令，会根据backend文件夹下的情况动态调整。

### 







实现：后端工具采用标准的argparse实现， 然后会通过脚本读取每个后端工具的argparse信息，自动生成ksightCli的自动补全信息。
然后使用complete来进行自动补全。
