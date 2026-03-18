#!/usr/bin/python3
import socket
import os

# 定义Unix Domain Socket文件路径
SOCKET_PATH = '/tmp/uds_socket'

# 如果存在旧的socket文件，先删除它
if os.path.exists(SOCKET_PATH):
    os.unlink(SOCKET_PATH)

# 创建一个Unix Domain Socket
server_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
server_socket.bind(SOCKET_PATH)
server_socket.listen(1)

pid = os.getpid()
print(f"当前进程的 PID 是: {pid}")

print("服务端已启动，等待客户端连接...")

# 接受客户端连接
client_socket, addr = server_socket.accept()
print(f"客户端已连接: {addr}")

try:
    while True:
        # 接收客户端消息
        data = client_socket.recv(1024)
        if not data:
            break
        print(f"收到消息: {data.decode('utf-8')}")
finally:
    # 关闭连接
    client_socket.close()
    server_socket.close()
    os.unlink(SOCKET_PATH)
