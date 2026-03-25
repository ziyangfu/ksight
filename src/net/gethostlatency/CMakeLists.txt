# \copyright
# Copyright (c) 2024 by the ksight project authors. All Rights Reserved.
#
# This file is open source software, licensed to you under the terms
# of the Apache License, Version 2.0 (the "License").  See the NOTICE file
# distributed with this work for additional information regarding copyright
# ownership.  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
# -----------------------------------------------------------------------------------------
# \brief
#      gethostlatency 工具 CMakeLists 文件
# -----------------------------------------------------------------------------------------

ksight_add_tool(
    NAME gethostlatency
    MODULE net
    BPF_FILES 
        gethostlatency.bpf.c
)

# 显式包含共享助手源代码
target_sources(gethostlatency PRIVATE
    ${PROJECT_SOURCE_DIR}/src/bpf_common_helper/uprobe_helpers.c
)

# 链接额外的库
target_link_libraries(gethostlatency PRIVATE elf)
