#ifndef NET_TCPSYNBL_ARG_PARSER_H
#define NET_TCPSYNBL_ARG_PARSER_H

#include <string>
#include "argparse/argparse.hpp"
#include "ConfigArgs.h"

namespace net {
namespace tcpsynbl {

inline void cmdParser(argparse::ArgumentParser& parser, ConfigArgs& config) {
    parser.add_argument("-T", "--timestamp")
            .help("Include timestamp on output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.print_timestamp);

    parser.add_argument("-4", "--ipv4")
            .help("Trace IPv4 family only")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.ipv4_only);

    parser.add_argument("-6", "--ipv6")
            .help("Trace IPv6 family only")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.ipv6_only);

    parser.add_argument("--verbose")
            .help("Verbose debug output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.verbose);

    parser.add_argument("interval")
            .help("print interval (in seconds)")
            .scan<'i', int>()
            .default_value(99999999)
            .nargs(argparse::nargs_pattern::optional)
            .store_into(config.interval);

    parser.add_argument("count")
            .help("number of times to print")
            .scan<'i', int>()
            .default_value(99999999)
            .nargs(argparse::nargs_pattern::optional)
            .store_into(config.times);

    parser.add_description("Summarize TCP SYN backlog as a histogram.\n"
                           "\n"
                           "EXAMPLES:\n"
                           "  tcpsynbl              # summarize TCP SYN backlog as a histogram\n"
                           "  tcpsynbl 1 10         # print 1 second summaries, 10 times\n"
                           "  tcpsynbl -T 1         # 1s summaries with timestamps\n"
                           "  tcpsynbl -4           # trace IPv4 family only\n"
                           "  tcpsynbl -6           # trace IPv6 family only");
}

} // namespace tcpsynbl
} // namespace net

#endif // NET_TCPSYNBL_ARG_PARSER_H
