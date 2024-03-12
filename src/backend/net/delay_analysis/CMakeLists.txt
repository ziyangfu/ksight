# 遍历文件夹内所有的 .bpf.c
file(GLOB apps *.bpf.c)
if(NOT CARGO_EXISTS)
    list(REMOVE_ITEM apps ${CMAKE_CURRENT_SOURCE_DIR}/profile.bpf.c)   # 排除掉profile.bpf.c
endif()
# 设置并创建  <>/src-gen/net文件夹
set(NET_SRC_GEN_DIR ${CONFIG_SRC_GEN_DIR}/net)
if (NOT EXISTS ${NET_SRC_GEN_DIR})
    file(MAKE_DIRECTORY ${NET_SRC_GEN_DIR})
endif ()

foreach(app ${apps})
    get_filename_component(app_stem ${app} NAME_WE)

    # Build object skeleton and depend skeleton on libbpf build
    # 调用 FindBpfObject.cmake 中的宏 bpf_object， 生成 *.skel.h
    bpf_object(${app_stem} ${app_stem}.bpf.c ${NET_SRC_GEN_DIR})
    add_dependencies(${app_stem}_skel libbpf-build bpftool-build)

    add_executable(${app_stem} ${app_stem}.c)
    target_link_libraries(${app_stem} ${app_stem}_skel)  # ${app_stem}_skel 即依赖项 libbpf-build bpftool-build
    if(${app_stem} STREQUAL profile)  # 名字相同,Rust才执行
        target_include_directories(${app_stem} PRIVATE
                ${CMAKE_SOURCE_DIR}/blazesym/include)
        target_link_libraries(${app_stem}
                ${CMAKE_SOURCE_DIR}/blazesym/target/release/libblazesym.a -lpthread -lrt -ldl)
    endif()

    # 安装
    install(TARGETS ${app_stem}
            RUNTIME DESTINATION ${CMAKE_BINARY_DIR}/install/backend/net/bin)
endforeach()

install(FILES ../nettrace.yaml
        PERMISSIONS OWNER_READ OWNER_WRITE GROUP_READ GROUP_WRITE
        DESTINATION ${CMAKE_BINARY_DIR}/install/backend/net/etc/
)
