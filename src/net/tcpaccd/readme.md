tcpaccd守护进程—使用eBPF的TCP/IP本地通信加速功能

- 本地与远程通信上层API接口统一
- 本地通信自动加速，实现类unix domain socket的效果


fasttcp 共享内存 + TCP接口的本地IPC通信

用socket，为什么不用unix domain socket

用原始的TCP，在本地IPC通信不行？

业务层不需要考虑远程和本地的问题。直接使用TCP协议栈

底层在IPC场景下，自动加速
