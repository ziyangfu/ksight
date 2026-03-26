#ifndef NET_TCPTRACER_ARG_PARSER_H
#define NET_TCPTRACER_ARG_PARSER_H

#include "argparse/argparse.hpp"
#include "ConfigArgs.h"
#include "fmt/format.h"

namespace net {
namespace tcptracer {

inline void cmdParser(argparse::ArgumentParser& parser, ConfigArgs& config) {
    parser.add_argument("-t", "--timestamp")
            .help("Include timestamp on output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.print_timestamp);

    parser.add_argument("-U", "--print-uid")
            .help("Include UID on output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.print_uid);

    parser.add_argument("-p", "--pid")
            .help("Process PID to trace")
            .scan<'i', uint32_t>()
            .default_value(uint32_t{0})
            .store_into(config.target_pid);

    parser.add_argument("-u", "--uid")
            .help("Process UID to trace")
            .scan<'i', uint32_t>()
            .default_value(static_cast<uint32_t>(-1))
            .store_into(config.target_uid);

    parser.add_argument("--verbose")
            .help("Verbose debug output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.verbose);

    parser.add_description("tcptracer: Trace TCP connections\n"
                           "\n"
                           "EXAMPLES:\n"
                           "  tcptracer             # trace all TCP connections\n"
                           "  tcptracer -t          # include timestamps\n"
                           "  tcptracer -p 181      # only trace PID 181\n"
                           "  tcptracer -U          # include UID\n"
                           "  tcptracer -u 1000     # only trace UID 1000");
}

} // namespace tcptracer
} // namespace net

#endif // NET_TCPTRACER_ARG_PARSER_H
