"""
    后端工具描述文件
        1. 工具名，所属子系统
        2. 是否具有运行依赖项，依赖项是什么？
    后续：
        是否直接读取文件夹名以及bin文件夹的工具名?
"""

class CpuWatcher():
    def __init__(self):
        self.tool_name = "cpu_watcher"
        self.belong_to_subsystem = "process"
        self.dependencies = {
            """ 工具运行的依赖项 """
        }
