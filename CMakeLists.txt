# \copyright
# Copyright (c) 2024 by the ksight project authors. All Rights Reserved.
#
# This file is open source software, licensed to you under the terms
# of the Apache License, Version 2.0 (the "License").  See the NOTICE file
# distributed with this work for additional information regarding copyright
# ownership.  You may not use this file except in compliance with the License.
# You may obtain a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
# -----------------------------------------------------------------------------------------
# \brief
#      ksight 顶层 CMakeLists 文件
# -----------------------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.10)

# Project setup
set(PROJECT_NAME ksight)
project(${PROJECT_NAME})

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 查看目标系统，目前仅支持Linux
if (NOT DEFINED OS_PLATFORM)
    string(TOLOWER ${CMAKE_SYSTEM_NAME} OS_PLATFORM)
endif ()
if (OS_PLATFORM MATCHES "linux")
    include(GNUInstallDirs)
    include(CheckFunctionExists)
    message(STATUS "${PROJECT_NAME} running in OS platform: ${OS_PLATFORM}")
else ()
    message(SEND_ERROR "${PROJECT_NAME} only support Linux platform now")
endif ()

# 查看CPU架构
if(${CMAKE_SYSTEM_PROCESSOR} MATCHES "x86_64")
    set(ARCH "x86")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "arm")
    set(ARCH "arm")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "aarch64")
    set(ARCH "arm64")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "ppc64le")
    set(ARCH "powerpc")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "mips")
    set(ARCH "mips")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "riscv64")
    set(ARCH "riscv")
elseif(${CMAKE_SYSTEM_PROCESSOR} MATCHES "loongarch64")
    set(ARCH "loongarch")
endif()

# 交叉编译设置
set(CC ${CMAKE_C_COMPILER})
set(CROSS_COMPILE "")
if(DEFINED TARCH AND (TARCH STREQUAL arm64 OR TARCH STREQUAL aarch64))
    message(STATUS "Cross-compilation for arm64 is enabled.")
    set(ARCH "arm64")
    include(${PROJECT_SOURCE_DIR}/platforms/aarch64/arm64_linux_setup.cmake)
endif()

# 生成目录设置：放在 build 文件夹内
set(CONFIG_SRC_GEN_DIR ${CMAKE_BINARY_DIR}/src-gen)
file(MAKE_DIRECTORY ${CONFIG_SRC_GEN_DIR})

# 模块搜索路径
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/tools/cmake)
include(ksight_helpers)

# 第三方依赖管理 (使用 Target 替代手动设置路径)
add_subdirectory(third_party)

# 依赖库探测
find_package(Threads REQUIRED)

# Build vendored libbpf and bpftool
include(ExternalProject)

ExternalProject_Add(libbpf
    PREFIX libbpf
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/libbpf/src
    CONFIGURE_COMMAND ""
    BUILD_COMMAND make
        ARCH=${ARCH}
        CC=${CMAKE_C_COMPILER}
        BUILD_STATIC_ONLY=1
        OBJDIR=${CMAKE_CURRENT_BINARY_DIR}/libbpf/libbpf
        DESTDIR=${CMAKE_CURRENT_BINARY_DIR}/libbpf
        INCLUDEDIR=
        LIBDIR=
        UAPIDIR=
        install install_uapi_headers
    BUILD_IN_SOURCE TRUE
    INSTALL_COMMAND ""
)

ExternalProject_Add(bpftool
    PREFIX bpftool
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/bpftool/src
    # 显式创建输出目录，解决 Makefile.include 检查失败的问题
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_BINARY_DIR}/bpftool/
    BUILD_COMMAND make 
        ARCH=${ARCH}
        CROSS_COMPILE=${CROSS_COMPILE}
        bootstrap
        OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/bpftool/
    BUILD_IN_SOURCE TRUE
    INSTALL_COMMAND ""
)

# blazesym build if needed
find_program(CARGO_EXISTS cargo)
if(CARGO_EXISTS)
    ExternalProject_Add(blazesym
        PREFIX blazesym
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/blazesym
        CONFIGURE_COMMAND ""
        BUILD_COMMAND cargo build --release
        BUILD_IN_SOURCE TRUE
        INSTALL_COMMAND ""
    )
endif()

# 配置 BpfObject 参数 (恢复原始路径以保证兼容性)
set(BPFOBJECT_BPFTOOL_EXE ${CMAKE_CURRENT_BINARY_DIR}/bpftool/bootstrap/bpftool)
set(BPFOBJECT_VMLINUX_H ${CMAKE_SOURCE_DIR}/vmlinux/${ARCH}/vmlinux.h)
set(LIBBPF_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/libbpf)
set(LIBBPF_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/libbpf/libbpf.a)
set(BPFOBJECT_CLANG_EXE clang)

# 包含 BPF 系统搜索路径
set(BPF_COMMON_FILES_DIR ${CMAKE_SOURCE_DIR}/src/bpf_common_helper)
set(PROJECT_COMPONENT_FILES_DIR ${CMAKE_SOURCE_DIR}/src/component_helper)

# 加载 BpfObject 模块
find_package(BpfObject REQUIRED)

add_subdirectory(src)
