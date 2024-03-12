# Linux内存工具监控开发
file(GLOB apps *.bpf.c)
# 若不用Rust，则排除 profile.bpf.c
if(NOT CARGO_EXISTS)
    list(REMOVE_ITEM apps ${CMAKE_CURRENT_SOURCE_DIR}/profile.bpf.c)
endif()
# 设置并创建  <>/src-gen/fs文件夹
set(SRC_GEN_FS_DIR ${CONFIG_SRC_GEN_DIR}/fs)
if (NOT EXISTS ${SRC_GEN_FS_DIR})
    file(MAKE_DIRECTORY ${SRC_GEN_FS_DIR})
endif ()
# 遍历文件夹内所有的bpf.c
foreach(app ${apps})
    get_filename_component(app_stem ${app} NAME_WE)

    # Build object skeleton and depend skeleton on libbpf build
    bpf_object(${app_stem} ${app_stem}.bpf.c ${SRC_GEN_FS_DIR})
    add_dependencies(${app_stem}_skel libbpf-build bpftool-build)

    add_executable(${app_stem} ${app_stem}.c)
    target_link_libraries(${app_stem} ${app_stem}_skel)
    if(${app_stem} STREQUAL profile)
        target_include_directories(${app_stem} PRIVATE
                ${CMAKE_SOURCE_DIR}/blazesym/include)
        target_link_libraries(${app_stem}
                ${CMAKE_SOURCE_DIR}/blazesym/target/release/libblazesym.a -lpthread -lrt -ldl)
    endif()

    # 安装
   # install(TARGETS ${app_stem}
    #        RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/install/backend/fs/bin)

endforeach()

