#!/bin/python3
"""
\brief 将vfsstat工具采集到的信息进行规范化处理，处理后数据为prometheus格式数据
"""


"""
    读取命令行参数，以sudo权限启动
    捕获后端工具的数据，规范化存储
"""