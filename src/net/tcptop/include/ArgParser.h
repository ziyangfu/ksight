#ifndef NET_TCP_TOP_ARG_PARSER_H
#define NET_TCP_TOP_ARG_PARSER_H

#include <iostream>
#include <string>
#include "argparse/argparse.hpp"
#include "ConfigArgs.h"

namespace net::tcpTop {

inline void cmdParser(argparse::ArgumentParser& parser, ConfigArgs& config) {
    parser.add_argument("-p", "--pid")
            .help("Process ID to trace")
            .default_value(static_cast<uint32_t>(-1))
            .scan<'u', uint32_t>()
            .store_into(config.target_pid);

    parser.add_argument("-c", "--cgroup")
            .help("Trace process in cgroup path")
            .default_value(std::string(""))
            .action([&config](const std::string& value) {
                config.cgroup_path = value;
                config.cgroup_filtering = true;
            });

    parser.add_argument("-4", "--ipv4")
            .help("trace IPv4 family only")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.ipv4_only);

    parser.add_argument("-6", "--ipv6")
            .help("trace IPv6 family only")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.ipv6_only);

    parser.add_argument("-S", "--nosummary")
            .help("Skip system summary line")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.no_summary);

    parser.add_argument("-C", "--noclear")
            .help("Don't clear the screen")
            .default_value(false)
            .implicit_value(true)
            .action([&config](const std::string& /*value*/) {
                config.clear_screen = false;
            });

    parser.add_argument("-s", "--sort")
            .help("Sort columns, default all [all, sent, received]")
            .default_value(std::string("all"))
            .action([&config](const std::string& value) {
                if (value == "all") config.sort_by = SortBy::ALL;
                else if (value == "sent") config.sort_by = SortBy::SENT;
                else if (value == "received") config.sort_by = SortBy::RECEIVED;
                else throw std::runtime_error("invalid sort method: " + value);
            });

    parser.add_argument("-r", "--rows")
            .help("Maximum rows to print, default 20")
            .default_value(20u)
            .scan<'u', uint32_t>()
            .store_into(config.output_rows);

    parser.add_argument("-v", "--verbose")
            .help("Verbose debug output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.verbose);

    parser.add_argument("interval")
            .help("Refresh interval in seconds")
            .default_value(1u)
            .scan<'u', uint32_t>()
            .nargs(0, 1)
            .action([&config](const std::string& value) {
                config.interval = static_cast<uint32_t>(std::stoul(value));
            });

    parser.add_argument("count")
            .help("Number of summaries")
            .default_value(99999999u)
            .scan<'u', uint32_t>()
            .nargs(0, 1)
            .action([&config](const std::string& value) {
                config.count = static_cast<uint32_t>(std::stoul(value));
            });

    parser.add_description("Summarize the top active TCP sessions - like top, but for TCP");
}

} // namespace net::tcpTop

#endif // NET_TCP_TOP_ARG_PARSER_H
