#ifndef NET_TCPRETRANS_ARG_PARSER_H
#define NET_TCPRETRANS_ARG_PARSER_H

#include "argparse/argparse.hpp"
#include "ConfigArgs.h"

namespace net {
namespace tcpretrans {

inline void cmdParser(argparse::ArgumentParser& parser, ConfigArgs& config) {
    parser.add_argument("-s", "--sequence")
            .help("display TCP sequence numbers")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.sequence);

    parser.add_argument("-l", "--lossprobe")
            .help("include tail loss probe attempts")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.lossprobe);

    parser.add_argument("-c", "--count")
            .help("count occurred retransmits per flow")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.count);

    parser.add_argument("-4", "--ipv4")
            .help("trace IPv4 family only")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.ipv4);

    parser.add_argument("-6", "--ipv6")
            .help("trace IPv6 family only")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.ipv6);

    parser.add_argument("--verbose")
            .help("Verbose debug output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.verbose);

    parser.add_description("Trace TCP retransmits and TLPs.");

    parser.add_epilog("examples:\n"
                       "  tcpretrans           # trace TCP retransmits\n"
                       "  tcpretrans -l        # include TLP attempts\n"
                       "  tcpretrans -4        # trace IPv4 family only\n"
                       "  tcpretrans -6        # trace IPv6 family only\n"
                       "  tcpretrans -c        # count retransmits per flow");
}

} // namespace tcpretrans
} // namespace net

#endif // NET_TCPRETRANS_ARG_PARSER_H
