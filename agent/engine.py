import json
import logging
from typing import List, Dict, Any, Optional
from .llm_client import get_llm_client
from .tools import KSIGHT_TOOLS, tool_executor
from .prompts import SYSTEM_PROMPT
from .config import config

# 配置日志
logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger("ksight.agent")

class DiagnoseEngine:
    def __init__(self, client_type: str = "remote", model_name: Optional[str] = None):
        self.llm = get_llm_client(client_type, model_name)
        self.max_rounds = config.MAX_ROUNDS
        self.history = [{"role": "system", "content": SYSTEM_PROMPT}]

    def chat(self, user_input: str):
        """交互式对话接口"""
        self.history.append({"role": "user", "content": user_input})
        
        for round_idx in range(self.max_rounds):
            logger.info(f"--- Round {round_idx + 1} ---")
            
            # 调用 LLM
            response_msg = self.llm.chat(self.history, tools=KSIGHT_TOOLS)
            
            # 处理响应 (兼容 OpenAI 消息对象或 Ollama 字典)
            content = getattr(response_msg, "content", None) or response_msg.get("content", "")
            tool_calls = getattr(response_msg, "tool_calls", None) or response_msg.get("tool_calls", [])
            
            # 将 AI 的回复加入历史
            self.history.append(response_msg)
            
            if not tool_calls:
                # 如果没有工具调用，说明 AI 已经给出结论或需要用户进一步输入
                return content

            # 处理工具调用
            for tool_call in tool_calls:
                # 兼容不同格式
                if hasattr(tool_call, "function"):
                    func_name = tool_call.function.name
                    func_args = json.loads(tool_call.function.arguments)
                    call_id = tool_call.id
                else:
                    func_name = tool_call["function"]["name"]
                    func_args = tool_call["function"]["arguments"]
                    call_id = tool_call.get("id")

                logger.info(f"AI 调用工具: {func_name}, 参数: {func_args}")
                
                # 执行工具
                result = tool_executor.execute(func_name, func_args)
                
                # 将工具执行结果返回给 AI
                self.history.append({
                    "role": "tool",
                    "tool_call_id": call_id,
                    "name": func_name,
                    "content": result
                })

        return "诊断轮次达到上限，请检查当前上下文。"

def run_agent_chat(client_type: str = "remote", model_name: Optional[str] = None):
    """启动交互式聊天"""
    engine = DiagnoseEngine(client_type, model_name)
    print("AI 智能诊断启动。您可以输入问题进行排查，如：'检查网络是否存在丢包？'")
    
    while True:
        try:
            user_input = input("\n> ")
            if user_input.lower() in ["exit", "quit", "退出"]:
                break
            
            print("AI 正在分析中...")
            response = engine.chat(user_input)
            print(f"\nAI: {response}")
            
        except KeyboardInterrupt:
            break
        except Exception as e:
            logger.error(f"Error: {e}")
            print(f"发生错误: {e}")

def run_agent_monitor():
    """启动监控模式 (待后续扩展需求：结合告警处理器)"""
    print("监控 Agent 模式已启动。此模式将监听系统关键指标并在异常时触发 AI 诊断。")
    # TODO: 实现自动触发逻辑
    pass
