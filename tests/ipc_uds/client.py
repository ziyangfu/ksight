#!/usr/bin/python3
import socket
import time
import os

# 定义Unix Domain Socket文件路径
SOCKET_PATH = '/tmp/uds_socket'

# 创建一个Unix Domain Socket
client_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
client_socket.connect(SOCKET_PATH)
pid = os.getpid()
print(f"当前进程的 PID 是: {pid}")

try:
    while True:
        # 发送消息到服务端
        message = "Hello, Server!"
        client_socket.sendall(message.encode('utf-8'))
        print(f"发送消息: {message}")
        time.sleep(1)  # 每隔1秒发送一次消息
finally:
    # 关闭连接
    client_socket.close()
