#ifndef NET_GET_HOST_LATENCY_CONFIG_ARGS_H
#define NET_GET_HOST_LATENCY_CONFIG_ARGS_H

#include <string>
#include <cstdint>

namespace net::getHostLatency {

struct ConfigArgs {
    uint32_t target_pid {0};
    std::string libc_path;
    bool verbose {false};
};

} // namespace net::getHostLatency

#endif // NET_GET_HOST_LATENCY_CONFIG_ARGS_H
