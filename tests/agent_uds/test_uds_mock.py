import socket
import os
import json
import time
from agent.ipc import TempUdsServer

def mock_client(sock_path):
    time.sleep(0.5) # Wait for server to start
    client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    try:
        client.connect(sock_path)
        data = [
            {"type": "connection", "local": "127.0.0.1:1234", "remote": "127.0.0.1:80", "status": "DISABLED"},
            {"type": "connection", "local": "127.0.0.1:5678", "remote": "127.0.0.1:443", "status": "ENABLED"}
        ]
        client.sendall(json.dumps(data).encode('utf-8'))
        print("Mock client sent data")
    except Exception as e:
        print(f"Mock client error: {e}")
    finally:
        client.close()

def test_uds_server():
    sock_path = "/tmp/test_agent_mock.sock"
    with TempUdsServer(sock_path) as server:
        print(f"Server started on {sock_path}")
        
        import threading
        t = threading.Thread(target=mock_client, args=(sock_path,))
        t.start()
        
        print("Waiting for JSON...")
        res = server.receive_json(timeout=5.0)
        print(f"Received result: {res}")
        
        t.join()
        
        if res and len(res) == 2:
            print("UDS Server test PASSED")
        else:
            print("UDS Server test FAILED")

if __name__ == "__main__":
    test_uds_server()
