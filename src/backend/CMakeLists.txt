include_directories(${CMAKE_CURRENT_SOURCE_DIR})
# -------------------------------------------------------------------
# tips: 重新修改选项时，需要清除 <build>/CMakeCache.txt
option(BUILD_ALL "is build all tools? " ON)
# fs部分
option(BUILD_FS_WATCHER "is build fs_watcher? " OFF)
# memory部分
option(BUILD_MEM_WATCHER "is build mem_watcher? " OFF)
# net部分
option(BUILD_NET_WATCHER "is build net_watcher? " OFF)
# process部分
option(BUILD_CPU_WATCHER " is build cpu_watcher? " ON)
option(BUILD_PROC_IMAGE "is build proc_image? " OFF)
# 未命名部分
option(BUILD_KVM_WATCHER "is build kvm_watcher? " OFF)
option(BUILD_STACK_ANALYZER "is build stack_analyzer? " OFF)
# -------------------------------------------------------------------
message(STATUS "build fs_watcher? ${BUILD_FS_WATCHER}")
if (BUILD_FS_WATCHER)
    add_subdirectory(fs/fs_watcher)
endif ()

message(STATUS "build mem_watcher? ${BUILD_MEM_WATCHER}")
if (BUILD_MEM_WATCHER)
    add_subdirectory(memory/mem_watcher)
endif ()

message(STATUS "build net_watcher? ${BUILD_NET_WATCHER}")
if (BUILD_NET_WATCHER)
    add_subdirectory(net/net_watcher)
endif ()

message(STATUS "build cpu_watcher? ${BUILD_CPU_WATCHER}")
if (BUILD_CPU_WATCHER)
    add_subdirectory(process/cpu_watcher)
endif ()

message(STATUS "build proc_image? ${BUILD_PROC_IMAGE}")
if (BUILD_PROC_IMAGE)
    add_subdirectory(process/proc_image)
endif ()

message(STATUS "build kvm_watcher? ${BUILD_KVM_WATCHER}")
message(STATUS "build stack_analyzer? ${BUILD_STACK_ANALYZER}")
