import socket
import os
import json
import logging
import time

logger = logging.getLogger("ksight.agent.ipc")

class UdsServer:
    def __init__(self, socket_path: str = "/tmp/ksight_agent.sock"):
        self.socket_path = socket_path
        self.server = None

    def start(self):
        if os.path.exists(self.socket_path):
            try:
                os.remove(self.socket_path)
            except OSError:
                pass
            
        self.server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.server.bind(self.socket_path)
        self.server.listen(1)
        # 设置超时，防止永久阻塞。由调用方决定等待多久。
        self.server.settimeout(5.0) 
        logger.info(f"UDS Server started on {self.socket_path}")

    def receive_json(self, timeout: float = 10.0) :
        """
        接收并解析一次 JSON 输出。
        注意：此方法是同步阻塞的。
        """
        self.server.settimeout(timeout)
        try:
            conn, addr = self.server.accept()
            with conn:
                logger.info("UDS Connection accepted")
                chunks = []
                while True:
                    chunk = conn.recv(4096)
                    if not chunk:
                        break
                    chunks.append(chunk)
                
                data_str = b"".join(chunks).decode('utf-8')
                if not data_str:
                    logger.warning("Received empty data from UDS")
                    return None
                return json.loads(data_str)
        except socket.timeout:
            logger.error(f"UDS Accept timeout after {timeout}s")
            return None
        except Exception as e:
            logger.error(f"Error receiving data from UDS: {e}")
            return None

    def stop(self):
        if self.server:
            self.server.close()
            self.server = None
        if os.path.exists(self.socket_path):
            try:
                os.remove(self.socket_path)
            except OSError:
                pass
        logger.info("UDS Server stopped")

class TempUdsServer:
    """Context manager for UDS server"""
    def __init__(self, socket_path: str = "/tmp/ksight_agent.sock"):
        self.socket_path = socket_path
        self.server = UdsServer(socket_path)

    def __enter__(self):
        self.server.start()
        return self.server

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.server.stop()
