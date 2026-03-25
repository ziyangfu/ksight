#ifndef NET_TCP_CONNECT_CONFIG_ARGS_H
#define NET_TCP_CONNECT_CONFIG_ARGS_H

#include <vector>
#include <cstdint>
#include "tcpconnect.h"

namespace net::tcpConnect {

struct ConfigArgs {
    bool verbose {false};
    bool count {false};
    bool print_timestamp {false};
    bool print_uid {false};
    uint32_t target_pid {0};
    uint32_t target_uid {static_cast<uint32_t>(-1)};
    std::vector<uint16_t> target_ports;
    bool source_port {false};
};

} // namespace net::tcpConnect

#endif // NET_TCP_CONNECT_CONFIG_ARGS_H
