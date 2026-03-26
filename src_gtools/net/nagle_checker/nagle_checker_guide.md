延迟敏感的场景，例如车载云通信，需要禁用nagle算法
Nagle 算法的控制级别是 Per-Socket（每个套接字独立控制）。
Nagle 算法的设计初衷是为了解决“小包风暴”问题，提高带宽利用率。由于 Linux 是一个通用操作系统，不同的应用程序对延迟的敏感度完全不同

2. 应用程序如何控制？
程序员必须在代码中显式调用 setsockopt 来控制 Nagle 算法（即设置 TCP_NODELAY 选项）：

禁用 Nagle (立即发送):
```c
int opt = 1;
setsockopt(sock_fd, IPPROTO_TCP, TCP_NODELAY, &opt, sizeof(opt));
```
启用 Nagle (默认状态):
将 opt 设置为 0 即可。

方案 A：使用 ss 命令（推荐）
ss (Socket Statistics) 可以读取内核的 tcp_info 结构体。
这需要检查，当前系统中，是否有ss命令，如果没有，则需要安装ss命令。，如果安装失败，则不能执行方案A

Bash

# -t (tcp) -i (internal info)
ss -ti
在输出结果中，观察是否有 nodelay 字样：

如果看到 nodelay，说明该连接禁用了 Nagle 算法。

如果没有，说明 Nagle 算法正在运行。