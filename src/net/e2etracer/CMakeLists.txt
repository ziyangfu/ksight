# \copyright
# Copyright (c) 2024 by the lmp/magic_eyes project authors. All Rights Reserved.
#
# This file is open source software, licensed to you under the terms
# of the Apache License, Version 2.0 (the "License").  See the NOTICE file
# distributed with this work for additional information regarding copyright
# ownership.  See the NOTICE file at http://www.apache.org/licenses/LICENSE-2.0
# -----------------------------------------------------------------------------------------
# \brief
#      e2etracer 工具 CMakeLists 文件
# -----------------------------------------------------------------------------------------

ksight_add_tool(
    NAME e2etracer
    MODULE net
    BPF_FILES 
        bpf/e2etracer.bpf.c
)

target_link_libraries(e2etracer PRIVATE pcap rt nlohmann_json::nlohmann_json)
