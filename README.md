# ksight

## 1. 简介

一款用于Linux内核的可观测性定制命令行工具，覆盖CPU、内存、网络、IPC、文件、虚拟化等子系统。
名称释义：ksight 洞见内核

ksight family：
- [ksight](https://github.com/ziyangfu/ksight)：Linux内核可观测性定制工具。命令行，后端，可单独使用，正在开发中
- [ksight-lite](https://github.com/ziyangfu/ksight-lite)：针对RTOS(AUTOSAR OS?)的可观测性定制工具。命令行，后端，可单独使用，计划开发中
- [ksight-ui](https://github.com/ziyangfu/ksight-ui)： 跨平台应用软件（可能支持Windows），时间序列图表可视化，可交互。前端，与后端配套，计划开发中。

母项目直达：[lmp](https://github.com/linuxkerneltravel/lmp)

## 2. 架构

![basic_arch](./docs/00_introduction/images_dir/basic_arch.png)

## 3. 安装
### 3.1 一键编译安装
```bash
sh ./run.sh
```
### 3.1 单独安装ksight
```bash
git clone --recurse-submodules <ksight_github_address>
# eg：git clone --recurse-submodules https://github.com/ziyangfu/ksight.git
mkdir build && cd build
# -------------------------------------------------------
# 若想要编译所有工具
cmake ..
# or
cmake -DBUILD_ALL=ON -DCMAKE_INSTALL_PREFIX=<install_dir> ..
# or
# 若想要编译单独某个工具，如 fs_watcher
cmake -DBUILD_FS_WATCHER=ON ..
# or
# 若想在x64平台交叉编译出arm64平台的程序（TARCH 即 target arch）
cmake -DBUILD_ALL=ON -DTARCH=arm64 ..
# -------------------------------------------------------
make
make install
```
### 3.2 安装ksight family
使用repo安装多仓库，TODO


## 4. 使用
ksight编译安装后，会存在多个可执行文件，用户如果想单独使用某个工具，也可以直接使用。
最推荐的方式是使用ksightCli，这是一个聚合所有工具的命令行前端，具有Tab自动补全的功能。更方便使用。
例如：
```bash
ksightCli netwatcher -h

Usage: net_watcher [--help] [--version] [--all] [--err] [--extra] [--retrans] [--time] [--http] [--sport VAR] [--dport VAR] [--udp] [--net_filter] [--drop_reason] [--addr_to_func] [--icmptime] [--tcpstate] [--timeload] [--dns] [--stack] [--count VAR] [--rtt] [--rst_counters]

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
  -C, --count         specify the time to count the number of requests [nargs=0..1] [default: 0]
  -T, --rtt           set to trace rtt 
  -U, --rst_counters  set to trace rst 
```