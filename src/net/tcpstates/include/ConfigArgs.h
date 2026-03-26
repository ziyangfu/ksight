#ifndef NET_TCP_STATES_CONFIG_ARGS_H
#define NET_TCP_STATES_CONFIG_ARGS_H

#include <string>
#include <vector>

namespace net::tcpStates {

struct ConfigArgs {
    bool verbose {false};
    bool print_timestamp {false};
    bool ipv4_only {false};
    bool ipv6_only {false};
    bool wide_output {false};
    std::vector<uint16_t> target_sports;
    std::vector<uint16_t> target_dports;
};

} // namespace net::tcpStates

#endif // NET_TCP_STATES_CONFIG_ARGS_H
