import os
import subprocess
from typing import Dict, Any, List, Optional
from .config import config

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

def run_command(cmd: List[str], timeout: int = 30) -> ToolResult:
    """运行外部命令并返回结果"""
    try:
        process = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
            timeout=timeout
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
            return str(run_command(cmd))
        
        elif name == "ksight_ipcwatcher":
            bin_path = os.path.join(self.install_root, "ipc/ipcwatcher/bin/ipcwatcher")
            cmd = [bin_path] + args.get("args", "").split()
            return str(run_command(cmd))

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

        else:
            return f"Error: Unknown tool {name}"

tool_executor = ToolExecutor()
