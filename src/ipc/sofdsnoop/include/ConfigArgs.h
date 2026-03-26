#ifndef IPC_SOFDSNOOP_CONFIG_ARGS_H
#define IPC_SOFDSNOOP_CONFIG_ARGS_H

#include <cstdint>
#include <string>

namespace ipc {
namespace sofdsnoop {

struct ConfigArgs {
    bool verbose {false};
    bool print_timestamp {false};
    uint32_t target_pid {0};
    uint32_t target_tid {0};
    std::string filter_name {};  // 进程名过滤
    uint32_t duration {0};       // 追踪时长 (秒), 0=无限
};

} // namespace sofdsnoop
} // namespace ipc

#endif // IPC_SOFDSNOOP_CONFIG_ARGS_H
