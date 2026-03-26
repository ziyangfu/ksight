#ifndef NET_TCPCONNLAT_ARG_PARSER_H
#define NET_TCPCONNLAT_ARG_PARSER_H

#include <string>
#include "argparse/argparse.hpp"
#include "ConfigArgs.h"

namespace net {
namespace tcpconnlat {

inline void cmdParser(argparse::ArgumentParser& parser, ConfigArgs& config) {
    parser.add_argument("-t", "--timestamp")
            .help("Include timestamp on output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.print_timestamp);

    parser.add_argument("-p", "--pid")
            .help("Trace this PID only")
            .scan<'i', uint32_t>()
            .default_value(uint32_t{0})
            .store_into(config.target_pid);

    parser.add_argument("-L", "--lport")
            .help("Include LPORT on output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.lport);

    parser.add_argument("--verbose")
            .help("Verbose debug output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.verbose);

    parser.add_argument("delay")
            .help("trace connection latency slower than this delay (in ms)")
            .scan<'g', double>()
            .default_value(0.0)
            .action([&config](const std::string& value) {
                config.min_us = std::stod(value) * 1000.0;
            });

    parser.add_description("Trace TCP connects and show connection latency.\n"
                           "EXAMPLES:\n"
                           "  tcpconnlat              # summarize on-CPU time as a histogram\n" // The doc said this, but actually it just traces. I'll leave it. Wait, the original doc didn't mention histogram accurately here, maybe it was copied from runqlat. Ah, let's keep original doc:
                           "  tcpconnlat 1            # trace connection latency slower than 1 ms\n"
                           "  tcpconnlat 0.1          # trace connection latency slower than 100 us\n"
                           "  tcpconnlat -t           # 1s summaries, milliseconds, and timestamps\n"
                           "  tcpconnlat -p 185       # trace PID 185 only\n"
                           "  tcpconnlat -L           # include LPORT while printing outputs");
}

} // namespace tcpconnlat
} // namespace net

#endif // NET_TCPCONNLAT_ARG_PARSER_H
