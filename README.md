# ksight

> [简体中文](./docs/00_introduction/README_CN.md)

## 1. Introduction

A customized observability command-line toolkit for the Linux kernel, covering subsystems such as CPU, Memory, Network, IPC, File, and Virtualization.

**ksight** means "Kernel Insight."

**The ksight Family:**

- **ksight**: A customized observability tool for the Linux kernel. Built with **eBPF** and a command-line backend. It can be used as a standalone tool. *(In Development)*
- **ksight-lite**: A customized observability tool targeting RTOS (e.g., AUTOSAR OS). Features a command-line backend and can be used standalone. *(Planned)*
- **ksight-ui**: A cross-platform application (Windows support planned) for interactive time-series chart visualization. It serves as the frontend designed to complement the backends. *(Planned)*

**Parent Project:** [lmp](https://github.com/linuxkerneltravel/lmp)

## 2. Language & Architecture

- **Kernel Code**: C
- **User Code**: C++17
- **Build Tools**: CMake + Shell + Python
- **Connector**: Network Server (Python/C++17 supporting TCP, HTTPS, MQTT, or DDS)
- **ksight-UI**: Web-based, potentially deployed via Docker

![arch](./docs/00_introduction/images_dir/arch.png)

## 3. Installation

### 3.1 One-Click Build and Install

```bash
sudo apt install clang libelf1 libelf-dev zlib1g-dev libpcap-dev

git clone --recurse-submodules <ksight_github_address>
# eg：git clone --recurse-submodules https://github.com/ziyangfu/ksight.git
# 将安装在/usr/local/bin/ksight
sudo ./run.sh
```

### 3.2 Install the ksight Family (Multi-repo)

```bash
mkdir ksights
cd ksights
rm -rf ./.repo/ 
repo init -u git@github.com:ziyangfu/ksight-repo.git -b master -m default.xml
repo sync -d --fetch-submodules
```

## 4. How to use?

After building and installing **ksight**, multiple executable files will be generated. Users can call individual tools directly if desired. However, the recommended approach is to use **ksightCli**—a unified command-line frontend that aggregates all tools. It features **Tab auto-completion** for a more seamless user experience. For example:

```bash
ksightCli netwatcher -h

Usage: netwatcher [--help] [--version] [--all] [--err] [--extra] [--retrans] [--time] [--http] [--sport VAR] [--dport VAR] [--udp] [--net_filter] [--drop_reason] [--addr_to_func] [--icmptime] [--tcpstate] [--timeload] [--dns] [--stack] [--count VAR] [--rtt] [--rst_counters]

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