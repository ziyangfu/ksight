import socket
import os
import subprocess
import json
import sys

def test():
    sock_path = "/tmp/test_ksight_agent.sock"
    if os.path.exists(sock_path):
        os.remove(sock_path)
    
    server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server.bind(sock_path)
    server.listen(1)
    server.settimeout(10.0)
    
    print(f"UDS Server listening on {sock_path}")
    
    # Run tcpnagle
    script_dir = os.path.dirname(os.path.abspath(__file__))
    bin_path = os.path.join(script_dir, "build_tmp/src/net/tcpnagle/tcpnagle")
    
    if not os.path.exists(bin_path):
        print(f"Error: {bin_path} not found")
        return

    env = os.environ.copy()
    env["KSIGHT_AGENT_SOCK"] = sock_path
    
    print("Running tcpnagle...")
    # 使用 sudo -E 推送环境变量
    try:
        p = subprocess.Popen(["sudo", "-E", bin_path, "--agent", "--ojson"], env=env)
        
        conn, addr = server.accept()
        with conn:
            print("Connection accepted")
            data = []
            while True:
                chunk = conn.recv(4096)
                if not chunk:
                    break
                data.append(chunk)
            
            raw_data = b"".join(data).decode('utf-8')
            if not raw_data:
                print("Error: Received empty data")
            else:
                result = json.loads(raw_data)
                print("Received JSON from tcpnagle:")
                print(json.dumps(result, indent=2))
                if isinstance(result, list):
                    print(f"Success: Received {len(result)} items")
                else:
                    print("Success: Received JSON object")
                    
        p.wait()
    except socket.timeout:
        print("Error: UDS Accept timeout")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        server.close()
        if os.path.exists(sock_path):
            os.remove(sock_path)

if __name__ == "__main__":
    test()
