import os

from pathlib import Path

try:
    from dotenv import load_dotenv
    # 1. 优先尝试从本文件所在目录加载(安装目录)
    # 这确保了全局调用时能读取安装时的配置
    current_file_dir = Path(__file__).parent
    load_dotenv(dotenv_path=current_file_dir / ".env")
    
    # 2. 尝试从当前执行目录加载 (允许用户临时覆盖配置)
    load_dotenv(dotenv_path=Path.cwd() / ".env", override=True)
except ImportError:
    pass

class Config:
    # 远程 Qwen 配置
    QWEN_BASE_URL = os.getenv("QWEN_BASE_URL", "https://dashscope.aliyuncs.com/compatible-mode/v1")
    QWEN_API_KEY = os.getenv("QWEN_API_KEY")  # 从环境变量加载，避免硬编码
    QWEN_MODEL = "qwen-plus"
    
    # 本地 Ollama 配置
    OLLAMA_BASE_URL = os.getenv("OLLAMA_BASE_URL", "http://localhost:11434")
    OLLAMA_MODEL_DEEPSEEK = "deepseek-r1:7b"
    OLLAMA_MODEL_QWEN = "qwen3.5:4b"
    
    # 诊断引擎配置
    MAX_ROUNDS = 10
    CONTEXT_WINDOW = 128000
    
    @classmethod
    def validate(cls):
        """验证必要配置"""
        if not cls.QWEN_API_KEY:
            # 如果没有设置 API KEY，后续调用远程模型会报错，这里可以预警
            pass

config = Config()
