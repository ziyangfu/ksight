#ifndef NET_TCPRETRANS_CONFIG_ARGS_H
#define NET_TCPRETRANS_CONFIG_ARGS_H

#include <cstdint>

namespace net {
namespace tcpretrans {

struct ConfigArgs {
    bool verbose {false};
    bool sequence {false};
    bool lossprobe {false};
    bool count {false};
    bool ipv4 {false};
    bool ipv6 {false};
};

} // namespace tcpretrans
} // namespace net

#endif // NET_TCPRETRANS_CONFIG_ARGS_H
