#ifndef NET_BIND_SNOOP_CONFIG_ARGS_H
#define NET_BIND_SNOOP_CONFIG_ARGS_H

#include <string>
#include <cstdint>

namespace net::bindSnoop {

struct ConfigArgs {
    bool emit_timestamp {false};
    uint32_t target_pid {0};
    bool ignore_errors {true};
    std::string target_ports;
    bool verbose {false};
    std::string cgroupspath;
};

} // namespace net::bindSnoop

#endif // NET_BIND_SNOOP_CONFIG_ARGS_H
