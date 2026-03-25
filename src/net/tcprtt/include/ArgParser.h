#ifndef NET_TCP_RTT_ARG_PARSER_H
#define NET_TCP_RTT_ARG_PARSER_H

#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include "argparse/argparse.hpp"
#include "ConfigArgs.h"

namespace net::tcpRtt {

inline void cmdParser(argparse::ArgumentParser& parser, ConfigArgs& config) {
    parser.add_argument("-i", "--interval")
            .help("summary interval, seconds")
            .default_value(static_cast<uint32_t>(99999999))
            .scan<'u', uint32_t>()
            .store_into(config.interval);

    parser.add_argument("-d", "--duration")
            .help("total duration of trace, seconds")
            .default_value(static_cast<uint32_t>(0))
            .scan<'u', uint32_t>()
            .store_into(config.duration);

    parser.add_argument("-T", "--timestamp")
            .help("include timestamp on output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.timestamp);

    parser.add_argument("-m", "--millisecond")
            .help("millisecond histogram")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.milliseconds);

    parser.add_argument("-p", "--lport")
            .help("filter for local port")
            .default_value(static_cast<uint16_t>(0))
            .scan<'u', uint16_t>()
            .store_into(config.lport);

    parser.add_argument("-P", "--rport")
            .help("filter for remote port")
            .default_value(static_cast<uint16_t>(0))
            .scan<'u', uint16_t>()
            .store_into(config.rport);

    parser.add_argument("-a", "--laddr")
            .help("filter for local address")
            .default_value(std::string(""))
            .action([&config](const std::string& arg) {
                if (arg.find(':') != std::string::npos) {
                    struct in6_addr addr_v6;
                    if (inet_pton(AF_INET6, arg.c_str(), &addr_v6) < 1) {
                        throw std::runtime_error("invalid local IPv6 address: " + arg);
                    }
                    memcpy(config.laddr_v6, &addr_v6, sizeof(config.laddr_v6));
                } else {
                    struct in_addr addr;
                    if (inet_pton(AF_INET, arg.c_str(), &addr) < 1) {
                        throw std::runtime_error("invalid local address: " + arg);
                    }
                    config.laddr = addr.s_addr;
                }
            });

    parser.add_argument("-A", "--raddr")
            .help("filter for remote address")
            .default_value(std::string(""))
            .action([&config](const std::string& arg) {
                if (arg.find(':') != std::string::npos) {
                    struct in6_addr addr_v6;
                    if (inet_pton(AF_INET6, arg.c_str(), &addr_v6) < 1) {
                        throw std::runtime_error("invalid remote IPv6 address: " + arg);
                    }
                    memcpy(config.raddr_v6, &addr_v6, sizeof(config.raddr_v6));
                } else {
                    struct in_addr addr;
                    if (inet_pton(AF_INET, arg.c_str(), &addr) < 1) {
                        throw std::runtime_error("invalid remote address: " + arg);
                    }
                    config.raddr = addr.s_addr;
                }
            });

    parser.add_argument("-b", "--byladdr")
            .help("show sockets histogram by local address")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.laddr_hist);

    parser.add_argument("-B", "--byraddr")
            .help("show sockets histogram by remote address")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.raddr_hist);

    parser.add_argument("-e", "--extension")
            .help("show extension summary(average)")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.extended);

    parser.add_argument("-v", "--verbose")
            .help("Verbose debug output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.verbose);

    parser.add_description("Summarize TCP RTT as a histogram");
}

} // namespace net::tcpRtt

#endif // NET_TCP_RTT_ARG_PARSER_H
