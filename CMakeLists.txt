cmake_minimum_required(VERSION 3.16)
# 使用clang编译器
set(CMAKE_C_COMPILER /usr/bin/clang)
set(CMAKE_CXX_COMPILER /usr/bin/clang)

set(PROJECT_NAME MagicEyes)
project(${PROJECT_NAME})

# 创建代码生成目录，后面生成的skel代码均放在这里
set(CONFIG_SRC_GEN_DIR ${CMAKE_SOURCE_DIR}/src-gen)
if (NOT EXISTS ${CONFIG_SRC_GEN_DIR})
    message(STATUS "src-gen文件夹不存在，准备创建")
    file(MAKE_DIRECTORY ${CONFIG_SRC_GEN_DIR})
endif ()
include_directories(${CONFIG_SRC_GEN_DIR})

# 该项目仅支持Linux
if (NOT DEFINED OS_PLATFORM)
    string(TOLOWER ${CMAKE_SYSTEM_NAME} OS_PLATFORM)
endif ()
if (OS_PLATFORM MATCHES "linux")
    include(GNUInstallDirs)  # for CMAKE_INSTALL_INCLUDEDIR ......
    INCLUDE(CheckFunctionExists) # for command check_function_exists ......
    message(STATUS "${PROJECT_NAME} running in OS platform: ${OS_PLATFORM}")
else ()
    message(SEND_ERROR "${PROJECT_NAME} only support Linux platform now")
endif ()

# Tell cmake where to find BpfObject module
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/tools/cmake)

# Build vendored libbpf
include(ExternalProject)
# ExternalProject_Add()函数创建一个外部工程可以驱动下载、更新/补丁、配置、构建、安装和测试流程的自定义目标
# 编译libbpf
ExternalProject_Add(libbpf
        PREFIX libbpf
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/libbpf/src
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make
        BUILD_STATIC_ONLY=1
        OBJDIR=${CMAKE_CURRENT_BINARY_DIR}/libbpf/libbpf
        DESTDIR=${CMAKE_CURRENT_BINARY_DIR}/libbpf
        INCLUDEDIR=
        LIBDIR=
        UAPIDIR=
        install install_uapi_headers
        BUILD_IN_SOURCE TRUE
        INSTALL_COMMAND ""
        STEP_TARGETS build
)
# 编译bpftool
ExternalProject_Add(bpftool
        PREFIX bpftool
        SOURCE_DIR ${CMAKE_SOURCE_DIR}/bpftool/src
        CONFIGURE_COMMAND ""
        BUILD_COMMAND make bootstrap
        OUTPUT=${CMAKE_CURRENT_BINARY_DIR}/bpftool/
        BUILD_IN_SOURCE TRUE
        INSTALL_COMMAND ""
        STEP_TARGETS build
)
# Rust语言用，Rust需要编译 blazesym
find_program(CARGO_EXISTS cargo)
if(CARGO_EXISTS)
    ExternalProject_Add(blazesym
            PREFIX blazesym
            SOURCE_DIR ${CMAKE_SOURCE_DIR}/blazesym
            CONFIGURE_COMMAND ""
            BUILD_COMMAND cargo build --release
            BUILD_IN_SOURCE TRUE
            INSTALL_COMMAND ""
            STEP_TARGETS build
    )
endif()
# Set BpfObject input parameters -- note this is usually not necessary unless
# you're in a highly vendored environment (like libbpf-bootstrap)
# CPU架构
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
# 判断CPU架构
# if (NOT DEFINED OS_ARCH)
#    string(TOLOWER ${CMAKE_HOST_SYSTEM_PROCESSOR} OS_ARCH)
#    message(STATUS "${PROJECT_NAME} running in OS ARCH： ${OS_ARCH}")
#else ()
#    message(STATUS "${PROJECT_NAME} will running in OS ARCH： ${OS_ARCH}")
# endif ()

set(BPFOBJECT_BPFTOOL_EXE ${CMAKE_CURRENT_BINARY_DIR}/bpftool/bootstrap/bpftool)
set(BPFOBJECT_VMLINUX_H ${CMAKE_SOURCE_DIR}/vmlinux/${ARCH}/vmlinux.h)
set(LIBBPF_INCLUDE_DIRS ${CMAKE_CURRENT_BINARY_DIR}/libbpf)
set(LIBBPF_LIBRARIES ${CMAKE_CURRENT_BINARY_DIR}/libbpf/libbpf.a)

find_package(BpfObject REQUIRED)
if (NOT BpfObject_FOUND)
    message(STATUS "BpfObject not found, please check")
endif ()

add_subdirectory(src)