#ifndef NET_TCPCONNLAT_CONFIG_ARGS_H
#define NET_TCPCONNLAT_CONFIG_ARGS_H

#include <string>

namespace net {
namespace tcpconnlat {

struct ConfigArgs {
    bool verbose {false};
    bool print_timestamp {false};
    bool lport {false};
    uint32_t target_pid {0};
    double min_us {0.0};
};

} // namespace tcpconnlat
} // namespace net

#endif // NET_TCPCONNLAT_CONFIG_ARGS_H
