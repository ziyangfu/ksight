该文件夹存放不需要特权级的常用工具，如ping，ss等。
它会用Python进行开发。
主要的使用方是agent，碰到问题，它会首先调用这一层的工具进行排查。
如果这一层的工具无法解决问题，它会调用特权级层（eBPF）的工具进行排查。

net	TCP/UDP 连通性与响应时间检查
netif	网卡健康检查（链路状态、错误/丢包增量，Linux）
ntp	NTP 同步状态、时钟偏移、时间源层级检查（Linux）
ping	ICMP 可达性、丢包率、时延检查
sockstat	TCP listen 队列溢出检测（Linux）
tcpstate	TCP 连接状态监控（CLOSE_WAIT/TIME_WAIT，Netlink 采集，Linux）

ping、traceroute、DNS 解析、ARP 邻居表、TCP 连接状态、Socket 详情（RTT/cwnd）、重传率、连接延迟分布、Listen 队列溢出、TCP 内核调优检查、softnet 统计、路由表、IP 地址、网卡流量、防火墙规则



网络部分

应用层协议及上层应用的观测、智能诊断



SOME/IP 协议观测

DDS中间件观测



