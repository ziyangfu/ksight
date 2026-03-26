#ifndef NET_TCPPKTLAT_ARG_PARSER_H
#define NET_TCPPKTLAT_ARG_PARSER_H

#include <string>
#include "argparse/argparse.hpp"
#include "ConfigArgs.h"
#include <arpa/inet.h>

namespace net {
namespace tcppktlat {

inline void cmdParser(argparse::ArgumentParser& parser, ConfigArgs& config) {
    parser.add_argument("-p", "--pid")
            .help("Process PID to trace")
            .scan<'i', uint32_t>()
            .default_value(uint32_t{0})
            .store_into(config.target_pid);

    parser.add_argument("-t", "--tid")
            .help("Thread TID to trace")
            .scan<'i', uint32_t>()
            .default_value(uint32_t{0})
            .store_into(config.target_tid);

    parser.add_argument("-T", "--timestamp")
            .help("Include timestamp on output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.print_timestamp);

    parser.add_argument("-l", "--lport")
            .help("filter for local port")
            .scan<'i', uint16_t>()
            .default_value(uint16_t{0})
            .action([&config](const std::string& value) {
                config.target_sport = htons(std::stoi(value));
            });

    parser.add_argument("-r", "--rport")
            .help("filter for remote port")
            .scan<'i', uint16_t>()
            .default_value(uint16_t{0})
            .action([&config](const std::string& value) {
                config.target_dport = htons(std::stoi(value));
            });

    parser.add_argument("-w", "--wide")
            .help("Wide column output (fits IPv6 addresses)")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.wide_output);

    parser.add_argument("--verbose")
            .help("Verbose debug output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.verbose);

    parser.add_argument("delay")
            .help("filter for latency higher than this delay (in us)")
            .scan<'i', uint64_t>()
            .default_value(uint64_t{0})
            .store_into(config.min_us);

    parser.add_description("Trace latency between TCP received pkt and picked up by userspace thread.\n"
                           "\n"
                           "EXAMPLES:\n"
                           "  tcppktlat             # Trace all TCP packet picked up latency\n"
                           "  tcppktlat -T          # summarize with timestamps\n"
                           "  tcppktlat -p 123      # filter for pid\n"
                           "  tcppktlat -t 123      # filter for tid\n"
                           "  tcppktlat -l 80       # filter for local port\n"
                           "  tcppktlat -r 80       # filter for remote port\n"
                           "  tcppktlat 1000        # filter for latency higher than 1000us");
}

} // namespace tcppktlat
} // namespace net

#endif // NET_TCPPKTLAT_ARG_PARSER_H
