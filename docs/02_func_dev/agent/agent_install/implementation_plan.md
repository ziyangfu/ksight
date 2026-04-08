# ksight Agent 安装与调用实现方案

本方案旨在解决 ksight Agent 在全局环境下的安装、依赖管理及调用问题，确保用户可以在任何目录下通过 `ksightCli agent chat` 正常使用。

## 用户审核事项

> [!IMPORTANT]
> 1. **权限要求**：安装过程需要 root 权限（以写入 `/usr/local/bin/ksight`）。
> 2. **环境变量 (.env)**：安装脚本会尝试将当前目录的 `.env` 复制到安装目录。如果稍后更改了 API Key，需手动更新 `/usr/local/bin/ksight/agent/.env` 或再次运行安装程序。
> 3. **Shell 包装器**：`/usr/local/bin/ksightCli` 将从软链接改为 Shell 脚本，以便在执行前自动激活虚拟环境并设置 `PYTHONPATH`。

## 提议的变更

### [Component] Agent 模块

#### [NEW] [requirements.txt](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/agent/requirements.txt)
- 列出 Agent 运行所需的 Python 库：`openai`, `python-dotenv`, `click`, `spdlog` (如果使用其 Python 绑定)。

#### [NEW] [agent_install.sh](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/agent/agent_install.sh)
- 负责 Agent 源文件的分发。
- 在安装目录创建 Python 虚拟环境 (`venv`)。
- 安装 `requirements.txt` 中的依赖。
- 处理 `.env` 文件的初始化。

#### [MODIFY] [config.py](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/agent/config.py)
- 优化 `load_dotenv` 逻辑，除了当前目录外，额外尝试加载安装目录下的 `.env` 文件，确保全局调用时能读取到配置。

---

### [Component] ksightCli 核心

#### [MODIFY] [ksightCli](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/ksightCli/ksightCli)
- 重命名为 `ksightCli.py` (更符合 Python 脚本命名)，或由安装脚本处理其执行权限。

---

### [Component] 编排与安装 (Orchestration)

#### [MODIFY] [run.sh](file:///home/fzy/Downloads/02_Personal_Dev/02_ksight_family/ksight/run.sh)
- 集成对 `agent_install.sh` 的调用。
- 修改 `setup_ksight_cli` 逻辑：不再创建软链接，而是生成一个 Shell 包装器脚本到 `/usr/local/bin/ksightCli`。

```bash
#!/bin/bash
# 自动生成的 ksightCli 包装器
export PYTHONPATH="/usr/local/bin/ksight"
/usr/local/bin/ksight/venv/bin/python3 "/usr/local/bin/ksight/ksightCli/ksightCli.py" "$@"
```

---

## 验证计划

### 自动化测试
- 编写简单的测试脚本验证在 "/tmp" 目录下调用 `ksightCli agent -h` 是否报错。
- 确认虚拟环境中的库是否能被正确导入。

### 手动验证
1. 运行 `sudo ./run.sh`。
2. 在非项目目录运行 `ksightCli agent chat`。
3. 检查控制台是否提示 "AI 智能诊断启动"，或显示大模型连接状态。
