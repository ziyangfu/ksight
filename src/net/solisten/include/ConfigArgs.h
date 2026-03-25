#ifndef NET_SO_LISTEN_CONFIG_ARGS_H
#define NET_SO_LISTEN_CONFIG_ARGS_H

#include <cstdint>

namespace net::soListen {

struct ConfigArgs {
    uint32_t target_pid {0};
    bool emit_timestamp {false};
    bool verbose {false};
};

} // namespace net::soListen

#endif // NET_SO_LISTEN_CONFIG_ARGS_H
