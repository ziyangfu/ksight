#!/bin/python3
# Prometheus可视化的核心逻辑，实现将规范化的数据加载到Prometheus的metrics中，
# 并启动http服务，供Prometheus-Service提取