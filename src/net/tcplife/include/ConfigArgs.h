#ifndef NET_TCPLIFE_CONFIG_ARGS_H
#define NET_TCPLIFE_CONFIG_ARGS_H

#include <string>
#include <vector>

namespace net {
namespace tcplife {

struct ConfigArgs {
    bool verbose {false};
    bool print_timestamp {false};
    bool ipv4_only {false};
    bool ipv6_only {false};
    bool wide_output {false};
    uint32_t target_pid {0};
    std::vector<uint16_t> target_sports;
    std::vector<uint16_t> target_dports;
};

} // namespace tcplife
} // namespace net

#endif // NET_TCPLIFE_CONFIG_ARGS_H
