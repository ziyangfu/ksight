#ifndef NET_NET_WATCHER_CONFIG_ARGS_H
#define NET_NET_WATCHER_CONFIG_ARGS_H

#include <string>

namespace net::netWatcher {

struct ConfigArgs {
    bool all_conn           {false};
    bool err_packet         {false};
    bool extra_conn_info    {false};
    bool retrans_info       {false};
    bool layer_time         {false};
    bool http_info          {false};
    int sport               {0};
    int dport               {0};
    bool udp_info           {false};
    bool net_filter         {false};
    bool drop_reason        {false};
    bool addr_to_func       {false};
    bool icmp_info          {false};
    bool tcp_info           {false};
    bool time_load          {false};
    bool dns_info           {false};
    bool stack_info         {false};
    int count_info          {0};
    bool rtt_info           {false};
    bool rst_info           {false};
};

} // namespace net::netWatcher

#endif // NET_NET_WATCHER_CONFIG_ARGS_H
