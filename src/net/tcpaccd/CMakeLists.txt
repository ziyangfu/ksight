# \copyright
# Copyright (c) 2026 by the ksight project authors. All Rights Reserved.
#
# This file is open source software, licensed to you under the terms
# of the Apache License, Version 2.0 (the "License").  See the NOTICE file
# distributed with this work for additional information regarding copyright
# ownership.  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
# -----------------------------------------------------------------------------------------
# \brief
#      tcpaccd 工具 CMakeLists 文件
# -----------------------------------------------------------------------------------------

# 使用 ksight_add_tool 宏简化构建逻辑
# 参数说明：
#   NAME: 工具的可执行文件名称
#   MODULE: 所属子系统（用于安装路径路由）
#   BPF_FILES: 所有的 .bpf.c 文件
ksight_add_tool(
    NAME tcpaccd
    MODULE net
    BPF_FILES
        tcpaccd.bpf.c
)

# 如果有额外的特定库需要链接，可以在宏调用后补充
target_link_libraries(tcpaccd PRIVATE pcap rt)
