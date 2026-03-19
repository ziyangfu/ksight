import os

try:
    from dotenv import load_dotenv
    load_dotenv()
except ImportError:
    # 如果没有安装 python-dotenv，也可以手动尝试从根目录读取 .env 文件
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
