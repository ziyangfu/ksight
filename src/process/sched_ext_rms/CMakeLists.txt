# \copyright
# Copyright (c) 2026 by the ksight project authors. All Rights Reserved.
#
# This file is open source software, licensed to you under the terms
# of the Apache License, Version 2.0 (the "License").  See the NOTICE file
# distributed with this work for additional information regarding copyright
# ownership.  You may not use this file except in compliance with the License.
# You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
# -----------------------------------------------------------------------------------------
# \brief
#      sched_ext_rms 工具 CMakeLists 文件
# \details
# scx says:
# Dependencies:
#       clang: >=16 required, >=17 recommended
#       libbpf: >=1.2.2 required, >=1.3 recommended
#       bpftool: Usually available in linux-tools-common or similar packages
#       libelf, libz, libzstd: For linking against libbpf
#       pkg-config: For finding system libraries
#       Rust toolchain: >=1.82
# The kernel has to be built with the following configuration:
#       CONFIG_BPF=y
#       CONFIG_BPF_SYSCALL=y
#       CONFIG_BPF_JIT=y
#       CONFIG_DEBUG_INFO_BTF=y
#       CONFIG_BPF_JIT_ALWAYS_ON=y
#       CONFIG_BPF_JIT_DEFAULT_ON=y
#       CONFIG_SCHED_CLASS_EXT=y
# -----------------------------------------------------------------------------------------
set(TOOL_NAME sched_ext_rms)
set(TOOL_BELONG_TO_MODULE process)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 第三方库路径
set(ARG_PARSE_DIR ${CMAKE_SOURCE_DIR}/third_party/argparse/include)
set(FMT_DIR ${CMAKE_SOURCE_DIR}/third_party/fmt/include)
set(SPDLOG_DIR ${CMAKE_SOURCE_DIR}/third_party/spdlog/include)
set(SCX_INCLUDE_DIR ${CMAKE_SOURCE_DIR}/third_party/scx/scheds/include)

if (NOT EXISTS ${ARG_PARSE_DIR} OR NOT EXISTS ${FMT_DIR} OR NOT EXISTS ${SPDLOG_DIR} OR NOT EXISTS ${SCX_INCLUDE_DIR})
    message(FATAL_ERROR "third party include directory does not exist.")
endif ()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${ARG_PARSE_DIR})
include_directories(${FMT_DIR})
include_directories(${SPDLOG_DIR})
include_directories(${SCX_INCLUDE_DIR})

# 设置 BPF 额外的包含路径，以便 FindBpfObject.cmake 中的 bpf_object 宏能找到 scx 头文件
# 优先包含 scx 提供的 vmlinux.h 以支持 sched_ext 成员
set(BPF_EXTRA_INCLUDES "-I${SCX_INCLUDE_DIR}" "-I${CMAKE_SOURCE_DIR}/third_party/scx/scheds/vmlinux/arch/${ARCH}")

file(GLOB_RECURSE ${TOOL_NAME}_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)

find_package(Threads REQUIRED)

file(GLOB apps ${CMAKE_CURRENT_SOURCE_DIR}/bpf/*.bpf.c)

# 设置并创建代码生成目录
set(SRC_GEN_TARGET_DIR ${CONFIG_SRC_GEN_DIR}/${TOOL_BELONG_TO_MODULE}/${TOOL_NAME})
if (NOT EXISTS ${SRC_GEN_TARGET_DIR})
    file(MAKE_DIRECTORY ${SRC_GEN_TARGET_DIR})
endif ()

# 编译 BPF 代码并生成 Skeleton
foreach(app ${apps})
    get_filename_component(app_stem ${app} NAME_WE)
    bpf_object(${app_stem} ${app_stem}.bpf.c ${SRC_GEN_TARGET_DIR})
    add_dependencies(${app_stem}_skel libbpf-build bpftool-build)
endforeach()

add_executable(${TOOL_NAME} ${${TOOL_NAME}_SRCS})
target_link_libraries(${TOOL_NAME} ${CMAKE_THREAD_LIBS_INIT} rt m)

foreach (app ${apps})
    get_filename_component(app_stem ${app} NAME_WE)
    target_link_libraries(${TOOL_NAME} ${app_stem}_skel)
endforeach ()

# 安装规则
set(SCHE_EXT_RMS_INSTALL_DIR ${TOOL_BELONG_TO_MODULE}/${TOOL_NAME})
install(TARGETS ${TOOL_NAME}
        RUNTIME DESTINATION ${SCHE_EXT_RMS_INSTALL_DIR}/bin)
