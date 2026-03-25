# \copyright
# Copyright (c) 2024 by the lmp/magic_eyes project authors. All Rights Reserved.
#
# This file is open source software, licensed to you under the terms
# of the Apache License, Version 2.0 (the "License").  See the NOTICE file
# distributed with this work for additional information regarding copyright
# ownership.  You may not use this file except in compliance with the License.
# You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
# -----------------------------------------------------------------------------------------
# \brief
#      ipcwatcher 工具 CMakeLists 文件
# -----------------------------------------------------------------------------------------

ksight_add_tool(
    NAME ipcwatcher
    MODULE ipc
    BPF_FILES 
        ipcwatcher.bpf.c
)

target_link_libraries(ipcwatcher PRIVATE pcap rt)
