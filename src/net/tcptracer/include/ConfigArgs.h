#ifndef NET_TCPTRACER_CONFIG_ARGS_H
#define NET_TCPTRACER_CONFIG_ARGS_H

#include <cstdint>

namespace net {
namespace tcptracer {

struct ConfigArgs {
    bool verbose {false};
    bool print_timestamp {false};
    bool print_uid {false};
    uint32_t target_pid {0};
    uint32_t target_uid {static_cast<uint32_t>(-1)};
};

} // namespace tcptracer
} // namespace net

#endif // NET_TCPTRACER_CONFIG_ARGS_H
