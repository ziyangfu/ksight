#!/usr/local/bin/ksight/venv/bin/python3
"""
ksightCli - ksight 内核可观测性工具集的统一命令行界面。

该脚本动态加载工具元数据，并利用 Click 框架提供参数解析和自动补全功能。
执行时，它会根据用户输入调用相应的后端二进制工具。
"""

import os
import sys
import subprocess
import click

# 使用 realpath 跟随符号链接，确保在任意位置调用时都能正确找到模块
# os.path.abspath() 不解析符号链接，当入口是 /usr/local/bin/ksightCli 软链接时会算出错误路径
script_dir = os.path.dirname(os.path.realpath(__file__))
project_root = os.path.dirname(script_dir)
for p in [script_dir, project_root]:
    if p not in sys.path:
        sys.path.insert(0, p)

try:
    from commands_data import COMMANDS
except ImportError:
    COMMANDS = {}

def get_install_root():
    """
    智能获取 ksight 的安装根目录。
    """
    # 1. 检查是否在标准安装路径下运行
    if os.path.basename(script_dir) == "ksightCli":
        return os.path.dirname(script_dir)
    
    # 2. 检查默认安装路径
    default_path = "/usr/local/bin/ksight"
    if os.path.exists(default_path):
        return default_path
    
    # 3. 检查开发环境下的构建目录
    base_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    dev_path = os.path.join(base_dir, "build", "install")
    if os.path.exists(dev_path):
        return dev_path
        
    return default_path

@click.group(context_settings=dict(help_option_names=['-h', '--help']))
def cli():
    """ksightCli: ksight 工具集的统一入口。"""
    pass

def create_command_callback(tool_name, tool_info):
    """
    为指定的工具创建一个 Click 命令回调函数。
    """
    def callback(**kwargs):
        install_root = get_install_root()
        bin_path = os.path.join(install_root, tool_info['bin_path'])
        
        if not os.path.exists(bin_path):
            click.secho(f"错误: 找不到后端工具二进制文件: {bin_path}", fg='red', err=True)
            sys.exit(1)
            
        # 组装命令行参数
        # 我们从 sys.argv 中提取原始参数，以确保传递给后端工具的格式完全正确
        try:
            # 找到工具名在参数列表中的位置，取其后的所有内容
            idx = sys.argv.index(tool_name)
            cmd_args = sys.argv[idx + 1:]
        except ValueError:
            cmd_args = []
            
        full_cmd = [bin_path] + cmd_args
        
        try:
            # 执行后端工具，保持交互性
            # 透传退出码
            result = subprocess.run(full_cmd)
            sys.exit(result.returncode)
        except KeyboardInterrupt:
            # 优雅处理 Ctrl+C
            sys.exit(130)
        except Exception as e:
            click.secho(f"执行工具时出错: {e}", fg='red', err=True)
            sys.exit(1)
            
    return callback

def build_cli():
    """
    根据 COMMANDS 元数据动态构建 CLI 结构。
    """
    for tool_name, tool_info in COMMANDS.items():
        desc = tool_info.get('description', f"运行 {tool_name} 工具")
        options = tool_info.get('options', [])
        
        # 创建命令对象
        # 关键点：设置 help_option_names=[] 禁用 Click 对该子命令的帮助拦截。
        # 这样 -h 和 --help 就会被视为普通参数透传给后端工具。
        cmd = click.Command(
            name=tool_name,
            help=desc,
            callback=create_command_callback(tool_name, tool_info),
            context_settings=dict(
                ignore_unknown_options=True,
                allow_extra_args=True,
                help_option_names=[] 
            )
        )
        
        # 注册选项以支持自动补全
        seen_flags = set()
        for opt_str in options:
            if opt_str.startswith('-') and opt_str not in seen_flags:
                # 注册为选项，让 Click 的补全机制能够识别这些 flag。
                # 使用 expose_value=False 避免污染回调参数。
                option = click.Option([opt_str], is_flag=True, expose_value=False, help=f"传递 {opt_str} 给后端工具")
                cmd.params.append(option)
                seen_flags.add(opt_str)
        
        cli.add_command(cmd)

    # --- 注册 AI Agent 命令 ---
    @cli.group()
    def agent():
        """ksight AI 智能诊断：具备 AI 根因分析及自动工具调用能力的运维 Agent。"""
        pass

    @agent.command()
    @click.option('--client', default='remote', type=click.Choice(['remote', 'local']), help='大模型客户端类型 (remote: 阿里云 Qwen, local: 本地 Ollama)')
    @click.option('--model', default=None, help='要使用的模型名称 (本地模式下，默认为 qwen2.5:4b)')
    def chat(client, model):
        """交互式故障诊断：启动 AI 会话，AI 会根据对话内容自动调动工具排查问题。"""
        try:
            from agent import run_agent_chat
            run_agent_chat(client_type=client, model_name=model)
        except Exception as e:
            click.echo(f"启动 Agent 失败: {e}", err=True)

    @agent.command()
    def run():
        """启动后台监控 Agent：自动监听系统运行状态。"""
        try:
            from agent import run_agent_monitor
            run_agent_monitor()
        except Exception as e:
            click.echo(f"运行 Agent 失败: {e}", err=True)

if __name__ == "__main__":
    build_cli()
    # 设置程序名称，方便帮助信息显示
    cli(prog_name="ksightCli")