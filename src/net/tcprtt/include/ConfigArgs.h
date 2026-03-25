#ifndef NET_TCP_RTT_CONFIG_ARGS_H
#define NET_TCP_RTT_CONFIG_ARGS_H

#include <cstdint>
#include <string>
#include "tcprtt.h"

namespace net::tcpRtt {

struct ConfigArgs {
    uint16_t lport {0};
    uint16_t rport {0};
    uint32_t laddr {0};
    uint32_t raddr {0};
    uint8_t laddr_v6[IPV6_LEN] {0};
    uint8_t raddr_v6[IPV6_LEN] {0};
    bool milliseconds {false};
    uint32_t duration {0};
    uint32_t interval {99999999};
    bool timestamp {false};
    bool laddr_hist {false};
    bool raddr_hist {false};
    bool extended {false};
    bool verbose {false};
};

} // namespace net::tcpRtt

#endif // NET_TCP_RTT_CONFIG_ARGS_H
