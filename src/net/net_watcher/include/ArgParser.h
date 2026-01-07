#ifndef NET_NET_WATCHER_ARG_PARSER_H
#define NET_NET_WATCHER_ARG_PARSER_H

#include "argparse/argparse.hpp"
#include "ConfigArgs.h"
#include <string>

namespace net::netWatcher {

inline int cmdParser(argparse::ArgumentParser& parser, net::netWatcher::ConfigArgs& config) {
    parser.add_argument("-a", "--all")
            .help("set to trace CLOSED connection")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.all_conn);
    parser.add_argument("-e", "--err")
            .help("set to trace TCP error packets")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.err_packet);
    parser.add_argument("-x", "--extra")
            .help("set to trace extra conn info")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.extra_conn_info);
    parser.add_argument("-r", "--retrans")
            .help("set to trace extra retrans info")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.retrans_info);
    parser.add_argument("-t", "--time")
            .help("set to trace layer time of each packet")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.layer_time);
    parser.add_argument("-i", "--http")
            .help("set to trace http info")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.http_info);
    parser.add_argument("-s", "--sport")
            .help("trace this source port only")
            .default_value(0)
            .scan<'i', int>()
            .store_into(config.sport);
    parser.add_argument("-d", "--dport")
            .help("trace this destination port only")
            .default_value(0)
            .scan<'i', int>()
            .store_into(config.dport);
    parser.add_argument("-u", "--udp")
            .help("trace the udp message")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.udp_info);
    parser.add_argument("-n", "--net_filter")
            .help("trace ipv4 packget filter")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.net_filter);
    parser.add_argument("-k", "--drop_reason")
            .help("trace kfree")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.drop_reason);
    parser.add_argument("-F", "--addr_to_func")
            .help("translation addr to func and offset")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.addr_to_func);
    parser.add_argument("-I", "--icmptime")
            .help("set to trace layer time of icmp")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.icmp_info);
    parser.add_argument("-S", "--tcpstate")
            .help("set to trace tcpstate")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.tcp_info);
    parser.add_argument("-L", "--timeload")
            .help("analysis time load")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.time_load);
    parser.add_argument("-D", "--dns")
            .help("set to trace dns information")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.dns_info);
    parser.add_argument("-A", "--stack")
            .help("set to trace of stack")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.stack_info);
    parser.add_argument("-C", "--count")
            .help("specify the time to count the number of requests")
            .default_value(0)
            .scan<'i', int>()
            .store_into(config.count_info);
    parser.add_argument("-T", "--rtt")
            .help("set to trace rtt")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.rtt_info);
    parser.add_argument("-U", "--rst_counters")
            .help("set to trace rst")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.rst_info);

    parser.add_description("Watch tcp/ip in network subsystem");
    return 0;
}

} // namespace net::netWatcher

#endif // NET_NET_WATCHER_ARG_PARSER_H