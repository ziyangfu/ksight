# \copyright
# Copyright (c) 2026 by the ksight project authors. All Rights Reserved.
#
# This file is open source software, licensed to you under the terms
# of the Apache License, Version 2.0 (the "License").  See the NOTICE file
# distributed with this work for additional information regarding copyright
# ownership.  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
# -----------------------------------------------------------------------------------------
# \brief
#      tcpnagle 工具 CMakeLists 文件
# -----------------------------------------------------------------------------------------

# 使用 ksight_add_tool 宏简化构建逻辑
ksight_add_tool(
    NAME tcpnagle
    MODULE net
    BPF_FILES
        tcpnagle.bpf.c
)
