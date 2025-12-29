#ifndef NET_NET_WATCHER_ARG_PARSER_H
#define NET_NET_WATCHER_ARG_PARSER_H

#include "argparse/argparse.hpp"
#include "ConfigArgs.h"
#include "ArgMetadata.h"
#include <string>

namespace net::netWatcher {

inline int cmdParser(argparse::ArgumentParser& parser, net::netWatcher::ConfigArgs& config) {
    // Get argument metadata
    auto argsMetadata = getArgsMetadata();
    
    // Configure parser using metadata
    for (const auto& meta : argsMetadata) {
        // Create argument with all flags
        argparse::Argument* arg_ptr;
        if (meta.flags.size() == 1) {
            arg_ptr = &parser.add_argument(meta.flags[0]);
        } else {
            arg_ptr = &parser.add_argument(meta.flags[0], meta.flags[1]);
        }
        auto& arg = *arg_ptr;
        
        // Set help text
        arg.help(meta.help);
        
        // Configure based on type
        if (meta.type == "bool") {
            bool defaultVal = std::any_cast<bool>(meta.default_value);
            arg.default_value(defaultVal)
               .implicit_value(true);
            
            // Store into appropriate config field
            if (meta.flags[0] == "-a" || meta.flags[0] == "--all") {
                arg.store_into(config.all_conn);
            } else if (meta.flags[0] == "-e" || meta.flags[0] == "--err") {
                arg.store_into(config.err_packet);
            } else if (meta.flags[0] == "-x" || meta.flags[0] == "--extra") {
                arg.store_into(config.extra_conn_info);
            } else if (meta.flags[0] == "-r" || meta.flags[0] == "--retrans") {
                arg.store_into(config.retrans_info);
            } else if (meta.flags[0] == "-t" || meta.flags[0] == "--time") {
                arg.store_into(config.layer_time);
            } else if (meta.flags[0] == "-i" || meta.flags[0] == "--http") {
                arg.store_into(config.http_info);
            } else if (meta.flags[0] == "-u" || meta.flags[0] == "--udp") {
                arg.store_into(config.udp_info);
            } else if (meta.flags[0] == "-n" || meta.flags[0] == "--net_filter") {
                arg.store_into(config.net_filter);
            } else if (meta.flags[0] == "-k" || meta.flags[0] == "--drop_reason") {
                arg.store_into(config.drop_reason);
            } else if (meta.flags[0] == "-F" || meta.flags[0] == "--addr_to_func") {
                arg.store_into(config.addr_to_func);
            } else if (meta.flags[0] == "-I" || meta.flags[0] == "--icmptime") {
                arg.store_into(config.icmp_info);
            } else if (meta.flags[0] == "-S" || meta.flags[0] == "--tcpstate") {
                arg.store_into(config.tcp_info);
            } else if (meta.flags[0] == "-L" || meta.flags[0] == "--timeload") {
                arg.store_into(config.time_load);
            } else if (meta.flags[0] == "-D" || meta.flags[0] == "--dns") {
                arg.store_into(config.dns_info);
            } else if (meta.flags[0] == "-A" || meta.flags[0] == "--stack") {
                arg.store_into(config.stack_info);
            } else if (meta.flags[0] == "-M" || meta.flags[0] == "--mysql") {
                arg.store_into(config.mysql_info);
            } else if (meta.flags[0] == "-R" || meta.flags[0] == "--redis") {
                arg.store_into(config.redis_info);
            } else if (meta.flags[0] == "-T" || meta.flags[0] == "--rtt") {
                arg.store_into(config.rtt_info);
            } else if (meta.flags[0] == "-U" || meta.flags[0] == "--rst_counters") {
                arg.store_into(config.rst_info);
            } else if (meta.flags[0] == "--generateConfigJson") {
                arg.store_into(config.generateConfigJson);
            }
        } else if (meta.type == "int") {
            int defaultVal = std::any_cast<int>(meta.default_value);
            arg.default_value(defaultVal).scan<'i', int>();
            
            if (meta.flags[0] == "-s" || meta.flags[0] == "--sport") {
                arg.store_into(config.sport);
            } else if (meta.flags[0] == "-d" || meta.flags[0] == "--dport") {
                arg.store_into(config.dport);
            } else if (meta.flags[0] == "-C" || meta.flags[0] == "--count") {
                arg.store_into(config.count_info);
            }
        }
    }

    parser.add_description("Watch tcp/ip in network subsystem");
    return 0;
}

} // namespace net::netWatcher

#endif // NET_NET_WATCHER_ARG_PARSER_H
