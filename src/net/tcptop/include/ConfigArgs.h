#ifndef NET_TCP_TOP_CONFIG_ARGS_H
#define NET_TCP_TOP_CONFIG_ARGS_H

#include <cstdint>
#include <string>
#include "tcptop.h"

namespace net::tcpTop {

enum class SortBy {
    ALL,
    SENT,
    RECEIVED
};

struct ConfigArgs {
    uint32_t target_pid {static_cast<uint32_t>(-1)};
    std::string cgroup_path;
    bool cgroup_filtering {false};
    bool clear_screen {true};
    bool no_summary {false};
    bool ipv4_only {false};
    bool ipv6_only {false};
    uint32_t output_rows {20};
    SortBy sort_by {SortBy::ALL};
    uint32_t interval {1};
    uint32_t count {99999999};
    bool verbose {false};
};

} // namespace net::tcpTop

#endif // NET_TCP_TOP_CONFIG_ARGS_H
