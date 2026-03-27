非特权级网络工具
调用级别：1级（最常用调用级别）

网络部分，需要重点关注以下几个方面：
1. 带宽
2. 吞吐
3. 延迟
4. 抖动
5. 丢包

你需要进行连通性测试


链路层与网络层
ethtool
ethtool可以利用-i和-k选项检查网络接口的静态配置信息，也可利用-S选项打印驱动程序统计信息。
nicstat
nicstat可以打印网络接口的统计信息
netstat
netstat是一个用来汇报各种类型的网络统计信息的传统工具
ip
ip是一个管理路由、网络设备、接口以及隧道的工具，可用来打印各种对象的统计信息，如link、address、route等。如图为使用ip -s link打印link的统计信息
sar
系统报表活动工具sar可以打印出各种网络统计信息表
ss
ss是一个可以显示套接字信息的工具，可以用来打印各种类型的套接字信息，如tcp、udp、unix等。如图为使用ss -s打印套接字统计信息

netqtop
netqtop对指定网络接口的每个队列的传输和接收的数据包进行统计，帮助开发者检查其流量负载是否平衡。 结果显示为一个表格，列有PPS、BPS、平均大小和数据包计数。每隔给定的时间间隔以秒为单位进行打印

tcpdump
tcpdump可以用来抓取网络包进行分析。借助Wireshark GUI工具，可以利用tcpdump的输出文件来检查包头、跟踪某个TCP连接、进行包重组以及其他操作，如图所示。


iperf
iperf是一个用来测量网络吞吐量的工具，可以用来测量TCP和UDP的吞吐量，如图所示。






车载场景特有工具：
nagle算法检查工具：禁用nagle算法，查看是否禁用。
以太网协议配置检查工具：
0. TCP/IP协议栈参数检查工具：有没有禁用ICMP，有没有禁用广播，有没有禁用arp等等
    提供示例参数json文件。工具会读取json文件，并与系统中的实际配置进行比对，确认TCP/IP协议栈参数是否配置正确，然后给agent提示，请查看当前json文件，是否是期望的配置，如果json文件，例如新增了参数或者修改了格式，**让agent先重写这个参数检查工具**，以适配新的json文件。（这个需要agent能够理解json文件，并根据json文件生成对应的工具，在用户层，需要给出一个明确的option，用户确实是不是打开让agent去适配json文件的功能，如果为false，则这个工具直接读取固定的json文件，如果json文件，报错即可）
1. VLAN配置信息查看。查看网络VLAN，MAC绑定是否配置正确
配置信息以json信息给定，工具会读取json文件，并与系统中的实际配置进行比对，确认VLAN是否配置正确
```bash
root@tegra-ubuntu:~# ifconfig 
eth: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1466
        ether 48:b0:2d:63:4e:24  txqueuelen 1000  (Ethernet)
        RX packets 55897900  bytes 60271993009 (60.2 GB)
        RX errors 0  dropped 24  overruns 0  frame 0
        TX packets 53298260  bytes 52673121848 (52.6 GB)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

eth.7: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1466
        inet 172.31.7.9  netmask 255.255.255.0  broadcast 172.31.7.255
        ether 02:80:5e:1f:00:09  txqueuelen 1000  (Ethernet)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

eth.8: flags=4163<UP,BROADCAST,RUNNING,MULTICAST>  mtu 1466
        inet 172.31.8.9  netmask 255.255.255.0  broadcast 172.31.8.255
        ether 02:80:5e:1f:00:09  txqueuelen 1000  (Ethernet)
        RX packets 0  bytes 0 (0.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 0  bytes 0 (0.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0

lo: flags=73<UP,LOOPBACK,RUNNING>  mtu 65536
        inet 127.0.0.1  netmask 255.0.0.0
        loop  txqueuelen 1000  (Local Loopback)
        RX packets 4  bytes 448 (448.0 B)
        RX errors 0  dropped 0  overruns 0  frame 0
        TX packets 4  bytes 448 (448.0 B)
        TX errors 0  dropped 0 overruns 0  carrier 0  collisions 0
```
2. 本地端到端网络数据包跟踪
- 从内核能够抓取到特定的四元组（源IP、源端口、目的IP、目的端口）的数据包，然后在用户态
通过读取json配置文件，能够知道这个IP地址对应的程序名称、service ID，通信端双方都是谁。是走的转发路由还是直接通信。它属于什么功能（例如属于FOTA功能）
- 是不是就可以分析出这个数据包，在哪个程序产生的，每一层的耗时是多少，是不是被丢了。




中汽协操作系统性能测试团标：
系统需要为网络拓扑等网络配置信息维护状态，为网络连接建立多元组信息并管理协议状态，其处理效率与硬件配置、调度、内存管理、网络栈、设备驱动和网卡特性密切关系。网络性能可通过数据吞吐量、丢包率、延迟和网络抖动四个通用指标来衡量。
（1）数据吞吐量 packets/second 
根据套接字是否面向连接及是否有顺序要求，收发数据的大小、收发数据的内存布局、收发频率、网络状态、以及所有中间节点的软硬件处理能力都影响网络的最终吞吐量，即单位时间内通过本机的入站出站流量。
（2）丢包率 lost_packets/send_packets 
若接收流量超过节点处理能力或有异常，如并发连接超过上限、内存不足、网络拥塞、路由故障等，系统在尽最大努力交付整机流量原则下，将随机丢弃一部分网络包，因此使用丢包率来表示丢弃包在发送总量所占比例。丢包率达高时将影响服务可用性及用户体验。
（3）延迟 rx_time - tx_time
当网络包从发送到接收，经过本地处理、中间节点和对端处理后所经过的总时间。除了正常处理所耗时间外，可能由于本机协议状态发生变化，本机负载变化，本机资源不足导致的重试操作，中间节点的排队处理等各因素影响，会导致延迟在不同情况下发生变化。 当延迟过大时甚至会影响服务可用性。
除此之外，为了提高网络的实时性能，建议禁用TCP协议中的Nagle算法。为了加快网络数据包的处理速度，在保证安全性与可靠性的前提下，推荐使用DPDK与XDP等新兴内核旁路技术。
7.2.1 技术要求
测试应覆盖64B、128B、256B、512B、1KB、2KB、4KB、8KB、16KB和32KB等大小数据包，TCP、UDP传输的latency（延迟）、throughput（吞吐量）和丢包率等。硬件和测试场景等因素影响大，不对数值做具体要求。
7.2.2 测试方法
（1）数据吞吐量
分为TCP协议吞吐量以及UDP协议吞吐量。
使用测试工具或程序在指定的一段时间内，按照设定的速率（如每秒发送数据包的数量或速率比特/秒）连续不断地向网络发送数据包。
在另一端，使用测试工具或接收端设备会捕获这些数据包，并记录接收的数据量。
测试工具或程序会统计发送和接收的数据包数量以及它们的内容大小，并计算在测试时间段内成功传输的总数据量。
（2）丢包率
需要连续测试一小时。
通过测试工具向目标地址发送一定数量的数据包，在接收端记录接收到的数据包数量，用发送数据包总数减去接收成功的数据包数，再除以发送数据包总数，得到丢包率。为了得到更准确的评估结果，通常需要在一段时间内连续发送并监测数据包的接收情况，以获取在不同网络负载条件下的丢包率变化。
（3）延迟
网络延迟包括TCP连接延迟以及TCP往返时间。TCP连接延迟即发送端从三次握手的SYN数据包发出，到响应数据包的时间。TCP往返时间即发送端从发送一个TCP数据包到接收方，并接收到该数据包的确认信息所耗费的时间。
（4）网络抖动
初始测试环境网络负载50%；
通过发送连续的数据包，并在发送端和接收端分别记录每个数据包的发送和接收时间戳。对于每一个接收到的数据包，计算其相对于期望到达时间（基于理想情况下均匀间隔发送的假设）的时间偏差，也就是延迟的变动幅度。统计所有数据包的延迟变化值，计算抖动的平均值、最大值、最小值和方差等统计参数，从而评估网络的抖动特性。
推荐测试方案：
(1)选取两台配置相同设备直连；
(2)运行Iperf、Netperf、ping等网络性能测试工具运行测试，收集结果。
(3)为保证测试一致性，建议在下述测试环境中进行测试：
环境部分：
•环境温度：23℃±5℃（73.4℉±9℉）
•环境湿度：50±20%
设备连接部分：
准备一台运行Linux系统的测试设备Tester，将智能驾驶域控制器与Tester通过千兆以太网连接，两者处于同一局域网下，保证网络连通性功能正常。在域控制器与Tester上分别安装Iperf3、ping、BCC等性能测试工具。
（1）数据吞吐量
采用iperf3进行网络吞吐量测试，主要分为TCP协议吞吐量以及UDP协议吞吐量。具体方法如下：
•Tester作为服务端，运行如下指令：
iperf3 –s
其中，
-s:以Server模式运行
•在测试TCP协议吞吐量时，智能驾驶域控制器中运行如下指令
iperf3 –c <server_IP_address> -b <bitrate> -4 –O 5 –i 1 –t 600 -N
其中，
-c: 以Client模式运行
server_IP_address：Tester端的IP地址
-b：限制测试带宽,bitrate根据域控制器硬件参数进行定义
-4: 仅适用IPv4协议
-O：忽略前n秒的测试，上述命令为忽略前5秒
-i: 每次报告的时间间隔，上述命令为1秒
-t: 测试时间，上述命令为一小时
-N：屏蔽Nagle算法
•在测试UDP协议吞吐量时，智能驾驶域控制器中运行如下指令
iperf3 –c <server_IP_address> -u -b <bitrate> -4 –O 5 –i 1 –t 600
其中，
-c: 以Client模式运行
server_IP_address：Tester端的IP地址
-u：采用UDP协议进行传输
-b：限制测试带宽,bitrate根据域控制器硬件参数进行定义
-4: 仅适用IPv4协议
-O：忽略前n秒的测试，上述命令为忽略前5秒
-i: 每次报告的时间间隔，上述命令为1秒
-t: 测试时间，上述命令为一小时 
通过上述测试，获取TCP与UDP的吞吐量值，单次测试时间为一小时。
（2）网络抖动与丢包率
采用iperf3进行UDP协议网络抖动与丢包率，初始测试环境网络负载50%。采用命令如下：
iperf3 –c <server_IP_address> -u -b <value> -4 –O 5 –i 1 –t 600
其中，
-c: 以Client模式运行
server_IP_address：测试设备端的IP地址
-u: 采用UDP协议进行传输
-b：限制测试带宽,value值为用户自定义
-4: 仅适用IPv4协议
-O：忽略前n秒的测试，上述命令为忽略前5秒
-i: 每次报告的时间间隔，上述命令为1秒
-t: 测试时间，上述命令为一小时
通过查看输出结果中的Jitter了解网络抖动情况，查看域控制器中iperf3输出结果中的Lost/Total Datagrams了解丢包率。单次测试时间为一小时。
（3）网络延迟
网络延迟包括TCP连接延迟以及TCP往返时间。TCP连接延迟即发送端从三次握手的SYN数据包发出，到响应数据包的时间。TCP往返时间即发送端从发送一个TCP数据包到接收方，并接收到该数据包的确认信息所耗费的时间。
TCP连接延迟的具体测试方法如下：
•在智能驾驶域控制器上运行TCP连接建立测试脚本，测试时间一小时。
•在智能驾驶域控制器端进行连接延迟测量。智能驾驶域控制器端发送SYN包时记录时间为T1，域控制器接收到测试设备端发送的SYN-ANK时记录时间为T2，TCP连接延迟时间为delta = T2-T1
测试工具上，可以采用BCC工具中的tcpconnlat，如下：
Tcpconnlat –p <PID> -t
其中，
-p：测试脚本的PID
-t： 在输出数据中添加时间戳
查看工具输出数据中的LAT一栏，即可获取本次TCP连接的连接延迟。
TCP往返时间的具体测试方法如下：
•在智能驾驶域控制器与Tester之间创建持续的TCP数据流量。
•在智能驾驶域控制器端进行TCP往返时间测量。域控制器端发送TCP数据包时记录时间为T1，域控制器接收到该数据包的确认信息时记录时间为T2，TCP往返时间为delta = T2-T1
在具体的测试工具上，可以选择iperf3与BCC中的tcprtt工具，如下：
•在智能驾驶域控制器以及Tester上运行iperf3，创建大量的TCP数据包，操作如下：
Tester端指令如下：
iperf3 –s –p <port>
其中，
-s: 以Server模式运行
-p: 指定服务端端口
智能驾驶域控制器上运行如下指令。
iperf3 –c <server_IP_address> -p <port> -4 -t 600 -N
其中，
-c: 以Client模式运行
server_IP_address：Tester端的IP地址
-p: 服务端端口
-4: 仅适用IPv4协议
-t: 测试时间
-N：屏蔽Nagle算法
•在智能驾驶域控制器上运行BCC工具中的tcprtt，进行TCP往返时间的测量，如下：
Tcprtt  -4 -m –T –e –i 30 –d 5
其中，
-4：仅统计IPv4
-m：往返时间单位为毫秒
-T： 时间戳
-e： 输出平均值
-i： 单次直方图统计时间
-d:   统计次数
查看tcprtt输出数据，即可了解当前环境下的TCP往返时间的直方图分布情况。





1. 协议栈确定性/实时性检查工具
车载以太网（SOME/IP, DoIP）最忌讳不可控的延迟。

网络抖动（Jitter）分析工具： 定期发送小包，计算最大延迟与最小延迟的差值。如果抖动超过 1ms，自动检查 CPU 调度压力或中断绑定情况。

中继/转发延迟测量： 在多网段（如网关从 Ethernet 转发到 CAN-FD）场景下，测量报文在不同协议间转换的时间开销。

中断聚合（Interrupt Coalescence）检查： 检查网卡驱动是否开启了中断聚合。虽然这能降低 CPU 负载，但会增加延迟。在车载场景下通常需要禁用（类似于 Nagle 算法的逻辑）。

2. 物理层与二层链路诊断工具 (针对 100/1000Base-T1)
车载以太网不同于普通以太网，它是双绞线单对传输。

T1 链路质量监控： 通过读取网卡寄存器（通过 ethtool），获取 SQI (Signal Quality Indicator) 信号质量指示。如果信号质量下降，预警线束老化或电磁干扰。

CRC 错误帧追踪： 实时监控统计 rx_crc_errors。在车辆行驶（振动、加速）时，记录错误帧与时间的对应关系。

端口映射校验工具： 自动导出当前所有物理端口的 Master/Slave 状态（车载 T1 链路必须一端为 Master，一端为 Slave 才能 link up）。

3. 车载协议一致性与安全审计
SOME/IP 服务发现可视化： 监听 224.0.0.1:30490 等组播端口，列出当前整车网络中活跃的 Service ID、Instance ID 以及它们的 Offer 状态。

DoIP (Diagnostic over IP) 状态机检查： 模拟诊断仪发送 Vehicle Identification Request，确认各 ECU 的响应速度和逻辑是否符合 ISO 13400 标准。

异常报文检测 (IDS 基础版)： * 报文频率检查： 如果某个周期性报文（如 10ms 一发的雷达数据）突然变成了 5ms 或 50ms，工具应报错。

非法 MAC/IP 准入： 检查是否存在未在 JSON 白名单中的 MAC 地址接入网络。

4. 自动化配置与克隆工具
静态 ARP 绑定工具： 为了防止 ARP 欺骗和减少查询延迟，车载环境常使用静态 ARP。你的工具可以读取 JSON，一键将所有 ECU 的 IP/MAC 写入系统的 ARP 表。

PTP (Precision Time Protocol, IEEE 802.1AS) 同步状态监控：

车载传感器融合（如摄像头和激光雷达对齐）高度依赖时间同步。

功能： 读取 pmc (PTP Management Client) 数据，监控当前系统的 Master Offset。如果偏差超过 1us，立即告警。

5. 资源压力与流控检查 (TSN 基础)
队列策略 (QoS/Egress Shaping) 检查： 检查 tc (Traffic Control) 的配置。确认高优先级的报文（如控制指令）是否进入了优先队列，而视频流是否被限制在低优先级队列。

网卡环形缓冲区 (Ring Buffer) 监控： 监控 ethtool -g。如果发现 rx_dropped 增加且缓存区已满，说明处理速度跟不上，需要扩容缓冲区或优化业务代码。

推荐工具优先级：

PTP 时钟同步检查（感知融合的基础）。

VLAN & 静态 ARP 校验（通信架构的基础）。

SQI 信号质量监控（硬件可靠性的保障）。