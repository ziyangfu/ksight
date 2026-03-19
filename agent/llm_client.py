import os
import json
from abc import ABC, abstractmethod
from typing import List, Dict, Any, Optional
from .config import config

try:
    from openai import OpenAI
except ImportError:
    OpenAI = None

try:
    import ollama
except ImportError:
    ollama = None

class LLMClient(ABC):
    @abstractmethod
    def chat(self, messages: List[Dict[str, str]], tools: Optional[List[Dict[str, Any]]] = None) -> Any:
        pass

class QwenClient(LLMClient):
    def __init__(self):
        if OpenAI is None:
            raise ImportError("未安装 'openai' 库，请运行 'pip install openai'。")
        self.client = OpenAI(
            api_key=config.QWEN_API_KEY,
            base_url=config.QWEN_BASE_URL
        )
        self.model = config.QWEN_MODEL

    def chat(self, messages: List[Dict[str, str]], tools: Optional[List[Dict[str, Any]]] = None) -> Any:
        # 远程模型通常支持完备的 Tool Calling
        response = self.client.chat.completions.create(
            model=self.model,
            messages=messages,
            tools=tools,
            tool_choice="auto" if tools else None
        )
        return response.choices[0].message

class OllamaClient(LLMClient):
    def __init__(self, model_name: str):
        if ollama is None:
            raise ImportError("未安装 'ollama-python' 库，请运行 'pip install ollama'。")
        self.model = model_name

    def chat(self, messages: List[Dict[str, str]], tools: Optional[List[Dict[str, Any]]] = None) -> Any:
        # 使用 ollama-python 库的 chat 接口
        # 注意：本地小模型对 Tool Calling 的支持可能不如大模型，但 ollama 最新版已在尝试支持
        response = ollama.chat(
            model=self.model,
            messages=messages,
            tools=tools
        )
        return response['message']

def get_llm_client(client_type: str = "remote", model_name: Optional[str] = None) -> LLMClient:
    """工厂方法获取模型客户端"""
    if client_type == "remote":
        return QwenClient()
    elif client_type == "local":
        return OllamaClient(model_name or config.OLLAMA_MODEL_QWEN)
    else:
        raise ValueError(f"不支持的客户端类型: {client_type}")
