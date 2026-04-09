import os
import subprocess
import json
from typing import Dict, Any, List, Optional
from .config import config
from .ipc import TempUdsServer

def get_install_root():
    """获取 ksight 的安装根目录（复用 ksightCli 的逻辑）"""
    script_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    # 模拟 ksightCli 的查找逻辑
    if os.path.exists(os.path.join(script_dir, "build", "install")):
        return os.path.join(script_dir, "build", "install")
    return "/usr/local/bin/ksight"  # 默认路径

class ToolResult:
    def __init__(self, output: str, error: str = "", exit_code: int = 0):
        self.output = output
        self.error = error
        self.exit_code = exit_code

    def __str__(self):
        if self.error:
            return f"Error: {self.error}\nOutput: {self.output}"
        return self.output

def run_command(cmd: List[str], timeout: int = 30, sudo: bool = False, env: Optional[Dict[str, str]] = None) -> ToolResult:
    """运行外部命令并返回结果"""
    if sudo and os.getuid() != 0:
        cmd = ["sudo", "-n"] + cmd  # 使用 -n 避免交互式输入密码
    try:
        process = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=timeout,
            env=env
        )
        return ToolResult(process.stdout, process.stderr, process.returncode)
    except subprocess.TimeoutExpired:
        return ToolResult("", "Command timeout", -1)
    except Exception as e:
        return ToolResult("", str(e), -1)

# 定义工具列表供 AI 选择
KSIGHT_TOOLS = [
    {
        "type": "function",
        "function": {
            "name": "ksight_netwatcher",
            "description": "ksight 网络监控工具，用于追踪 TCP/IP 栈事件、RTT、丢包等。",
            "parameters": {
                "type": "object",
                "properties": {
                    "args": {
                        "type": "string",
                        "description": "传递给 netwatcher 的参数，如 '-a' 查看所有事件，'-e' 查看错误，'-r' 查看重传。"
                    }
                }
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "ksight_ipcwatcher",
            "description": "ksight IPC 监控工具，用于追踪 Unix Domain Socket 和 共享内存 IPC 行为。",
            "parameters": {
                "type": "object",
                "properties": {
                    "args": {
                        "type": "string",
                        "description": "传递给 ipcwatcher 的参数，如 '-u' 追踪 UDS，'-m' 追踪共享内存。"
                    }
                }
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "system_top",
            "description": "查看系统 CPU 和内存实时负载。",
            "parameters": {
                "type": "object",
                "properties": {
                    "n": {"type": "integer", "description": "更新次数，默认 1", "default": 1}
                }
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "system_free",
            "description": "查看系统当前内存使用情况。",
            "parameters": {
                "type": "object",
                "properties": {}
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "system_df",
            "description": "查看磁盘空间占用情况。",
            "parameters": {
                "type": "object",
                "properties": {}
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "ksight_nettrace",
            "description": "ksight 网络追踪工具，用于追踪内核中的 skb 并诊断网络问题。",
            "parameters": {
                "type": "object",
                "properties": {
                    "args": {
                        "type": "string",
                        "description": "传递给 nettrace 的参数，如 '-s 1.1.1.1' 指定源地址，'-d 2.2.2.2' 指定目的地址。"
                    }
                }
            }
        }
    },
    {
        "type": "function",
        "function": {
            "name": "ksight_tcpnagle",
            "description": "ksight TCP Nagle 算法诊断工具，用于检查连接是否禁用 Nagle (TCP_NODELAY)。",
            "parameters": {
                "type": "object",
                "properties": {
                    "args": {
                        "type": "string",
                        "description": "传递给 tcpnagle 的参数，如 '-p <pid>' 过滤进程。"
                    }
                }
            }
        }
    }
]

# 工具执行处理器
class ToolExecutor:
    def __init__(self):
        self.install_root = get_install_root()

    def execute(self, name: str, args: Dict[str, Any]) -> str:
        if name == "ksight_netwatcher":
            bin_path = os.path.join(self.install_root, "net/netwatcher/bin/netwatcher")
            cmd = [bin_path] + args.get("args", "").split()
            return self._run_with_uds(cmd)
        
        elif name == "ksight_ipcwatcher":
            bin_path = os.path.join(self.install_root, "ipc/ipcwatcher/bin/ipcwatcher")
            cmd = [bin_path] + args.get("args", "").split()
            return self._run_with_uds(cmd)

        elif name == "system_top":
            # 简化 top 调用，直接用 -b -n 1
            n = args.get("n", 1)
            cmd = ["top", "-b", "-n", str(n)]
            return str(run_command(cmd))

        elif name == "system_free":
            cmd = ["free", "-h"]
            return str(run_command(cmd))

        elif name == "system_df":
            cmd = ["df", "-h"]
            return str(run_command(cmd))

        elif name == "ksight_nettrace":
            bin_path = os.path.join(self.install_root, "net/nettrace/bin/nettrace")
            cmd = [bin_path] + args.get("args", "").split()
            return self._run_with_uds(cmd)

        elif name == "ksight_tcpnagle":
            # 如果是在开发环境，可能在 build 目录下
            bin_path = os.path.join(self.install_root, "net/tcpnagle/bin/tcpnagle")
            if not os.path.exists(bin_path):
                # 兼容 build_tmp 目录 (用于当前测试)
                script_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
                bin_path = os.path.join(script_dir, "build_tmp/src/net/tcpnagle/tcpnagle")
            
            cmd = [bin_path] + args.get("args", "").split()
            return self._run_with_uds(cmd)

        else:
            return f"Error: Unknown tool {name}"

    def _run_with_uds(self, cmd: List[str]) -> str:
        """带 UDS 通信的工具执行逻辑"""
        with TempUdsServer() as server:
            env = os.environ.copy()
            env["KSIGHT_AGENT_SOCK"] = server.socket_path
            # 增加 agent 模式标志
            if "--agent" not in cmd:
                cmd.append("--agent")
            if "--ojson" not in cmd:
                cmd.append("--ojson")
            
            # 由于 ksight 工具需要 root 权限，必须提权
            if os.getuid() != 0:
                cmd = ["sudo", "-n", "-E"] + cmd
            
            try:
                # 异步启动进程，使得 UDS Server 可以并发 Accept
                process = subprocess.Popen(
                    cmd,
                    stdout=subprocess.PIPE,
                    stderr=subprocess.PIPE,
                    text=True,
                    env=env
                )
                
                # 尝试从 UDS 获取结构化数据，超时时间给足够短（失败说明工具本身没进正确逻辑或退出太快）
                uds_res = server.receive_json(timeout=5.0)
                
                # 获取命令执行的最终标准输出和错误（设置合理超时）
                try:
                    stdout, stderr = process.communicate(timeout=30.0)
                except subprocess.TimeoutExpired:
                    process.kill()
                    stdout, stderr = process.communicate()
                
                # 拦截特定的 sudo 权限错误，给 AI 明确的指导，以便让 AI 告诉用户
                if process.returncode != 0 and "sudo: a password is required" in stderr:
                    return (
                        "权限错误：调用底层系统分析工具需要 root(sudo) 权限。\n"
                        "请建议用户停止当前会话，使用 `sudo ksightCli agent chat` 命令重新进入智能系统诊断。"
                    )
                
                if uds_res is not None:
                    return json.dumps(uds_res, indent=2, ensure_ascii=False)
                
                # 如果 UDS 失败，回退到 stderr / stdout 结合
                if process.returncode != 0 and stderr:
                    return f"Error: {stderr}\nOutput: {stdout}"
                return stdout or "Command returned empty output and no UDS data."
            except Exception as e:
                return f"Error spawning command: {e}"

tool_executor = ToolExecutor()
