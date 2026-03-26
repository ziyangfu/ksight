#ifndef NET_TCPPKTLAT_CONFIG_ARGS_H
#define NET_TCPPKTLAT_CONFIG_ARGS_H

#include <cstdint>

namespace net {
namespace tcppktlat {

struct ConfigArgs {
    bool verbose {false};
    bool print_timestamp {false};
    bool wide_output {false};
    uint32_t target_pid {0};
    uint32_t target_tid {0};
    uint16_t target_sport {0};
    uint16_t target_dport {0};
    uint64_t min_us {0};
};

} // namespace tcppktlat
} // namespace net

#endif // NET_TCPPKTLAT_CONFIG_ARGS_H
