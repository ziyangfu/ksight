# \copyright
# Copyright (c) 2024 by the ksight project authors. All Rights Reserved.

# ksight_add_tool(NAME name MODULE module BPF_FILES files...)
# 模块名称
# 所属子系统 (net, fs, memory, etc.)
# BPF 源文件列表
function(ksight_add_tool)
    set(options)
    set(oneValueArgs NAME MODULE)
    set(multiValueArgs BPF_FILES)
    cmake_parse_arguments(TOOL "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    set(TOOL_NAME ${TOOL_NAME})
    set(TOOL_BELONG_TO_MODULE ${TOOL_MODULE})

    # 使用绝对路径，确保在复杂的构建树中路径解析始终正确
    get_filename_component(CURRENT_SOURCE_DIR_ABS ${CMAKE_CURRENT_SOURCE_DIR} ABSOLUTE)
    
    # 设置生成目录在 CMAKE_BINARY_DIR 中
    set(SRC_GEN_TARGET_DIR ${CMAKE_BINARY_DIR}/src-gen/${TOOL_BELONG_TO_MODULE}/${TOOL_NAME})
    file(MAKE_DIRECTORY ${SRC_GEN_TARGET_DIR})

    # 查找 C++ 源文件
    file(GLOB_RECURSE TOOL_SRCS ${CURRENT_SOURCE_DIR_ABS}/src/*.cpp)
    if(NOT TOOL_SRCS)
        file(GLOB_RECURSE TOOL_SRCS ${CURRENT_SOURCE_DIR_ABS}/src/*.c)
    endif()

    # 处理 BPF 文件
    foreach(bpf_file ${TOOL_BPF_FILES})
        get_filename_component(app_stem ${bpf_file} NAME_WE)
        # 构造 BPF 文件的绝对路径
        get_filename_component(BPF_SRC_FULL_PATH "${CURRENT_SOURCE_DIR_ABS}/bpf/${bpf_file}" ABSOLUTE)
        
        # bpf_object 宏在 FindBpfObject.cmake 中定义
        # 现在传递绝对路径给 bpf_object
        bpf_object(${app_stem} ${BPF_SRC_FULL_PATH} ${SRC_GEN_TARGET_DIR})
        # 显式添加对 libbpf 和 bpftool 构建目标的依赖
        add_dependencies(${app_stem}_skel libbpf bpftool)
    endforeach()

    # 创建可执行文件
    add_executable(${TOOL_NAME} ${TOOL_SRCS})
    
    # 包含路径设置
    target_include_directories(${TOOL_NAME} PRIVATE 
        ${CURRENT_SOURCE_DIR_ABS}/include
        ${SRC_GEN_TARGET_DIR}
        ${BPF_COMMON_FILES_DIR}
        ${PROJECT_COMPONENT_FILES_DIR}
    )

    # 链接常用的库
    target_link_libraries(${TOOL_NAME} PRIVATE 
        Threads::Threads
        argparse::argparse
        fmt::fmt-header-only
        spdlog::spdlog_header_only
        nlohmann_json::nlohmann_json
    )

    foreach(bpf_file ${TOOL_BPF_FILES})
        get_filename_component(app_stem ${bpf_file} NAME_WE)
        target_link_libraries(${TOOL_NAME} PRIVATE ${app_stem}_skel)
    endforeach()

    # 安装配置
    set(INSTALL_DIR ${TOOL_BELONG_TO_MODULE}/${TOOL_NAME})
    install(TARGETS ${TOOL_NAME} RUNTIME DESTINATION ${INSTALL_DIR}/bin)
    
    if(EXISTS ${CURRENT_SOURCE_DIR_ABS}/config)
        install(DIRECTORY ${CURRENT_SOURCE_DIR_ABS}/config/ DESTINATION ${INSTALL_DIR}/config)
    endif()
    
    if(EXISTS ${CURRENT_SOURCE_DIR_ABS}/scripts)
        install(DIRECTORY ${CURRENT_SOURCE_DIR_ABS}/scripts/ DESTINATION ${INSTALL_DIR}/scripts)
    endif()
endfunction()
