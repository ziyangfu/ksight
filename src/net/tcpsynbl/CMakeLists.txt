# \copyright
# Copyright (c) 2024 by the ksight project authors. All Rights Reserved.
#
# This file is open source software, licensed to you under the terms
# of the Apache License, Version 2.0 (the "License").  See the NOTICE file
# distributed with this work for additional information regarding copyright
# ownership.  You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
# -----------------------------------------------------------------------------------------
# \brief
#      tcpsynbl 工具 CMakeLists 文件
# -----------------------------------------------------------------------------------------

ksight_add_tool(
    NAME tcpsynbl
    MODULE net
    BPF_FILES
        tcpsynbl.bpf.c
)

target_sources(tcpsynbl PRIVATE
    ${PROJECT_SOURCE_DIR}/src/bpf_common_helper/trace_helpers.c
    ${PROJECT_SOURCE_DIR}/src/bpf_common_helper/btf_helpers.c
    ${PROJECT_SOURCE_DIR}/src/bpf_common_helper/uprobe_helpers.c
)
