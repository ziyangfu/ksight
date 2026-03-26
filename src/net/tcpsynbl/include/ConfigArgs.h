#ifndef NET_TCPSYNBL_CONFIG_ARGS_H
#define NET_TCPSYNBL_CONFIG_ARGS_H

#include <cstdint>

namespace net {
namespace tcpsynbl {

struct ConfigArgs {
    bool verbose {false};
    bool print_timestamp {false};
    bool ipv4_only {false};
    bool ipv6_only {false};
    int interval {99999999};
    int times {99999999};
};

} // namespace tcpsynbl
} // namespace net

#endif // NET_TCPSYNBL_CONFIG_ARGS_H
