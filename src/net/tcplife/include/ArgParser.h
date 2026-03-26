#ifndef NET_TCPLIFE_ARG_PARSER_H
#define NET_TCPLIFE_ARG_PARSER_H

#include <string>
#include <sstream>
#include "argparse/argparse.hpp"
#include "ConfigArgs.h"

namespace net {
namespace tcplife {

inline void parsePorts(const std::string& arg, std::vector<uint16_t>& ports) {
    std::stringstream ss(arg);
    std::string token;
    while (std::getline(ss, token, ',')) {
        int port = std::stoi(token);
        if (port <= 0 || port > 65535) {
            throw std::runtime_error("Invalid port: " + token);
        }
        ports.push_back(static_cast<uint16_t>(port));
    }
}

inline void cmdParser(argparse::ArgumentParser& parser, ConfigArgs& config) {
    parser.add_argument("-p", "--pid")
            .help("Process ID to trace")
            .scan<'i', uint32_t>()
            .default_value(uint32_t{0})
            .store_into(config.target_pid);
            
    parser.add_argument("--verbose")
            .help("Verbose debug output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.verbose);

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

    parser.add_argument("-w", "--wide")
            .help("Wide column output (fits IPv6 addresses)")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.wide_output);

    parser.add_argument("-L", "--localport")
            .help("Comma-separated list of local ports to trace.")
            .default_value(std::string(""))
            .action([&config](const std::string& arg) {
                if (!arg.empty()) {
                    parsePorts(arg, config.target_sports);
                }
            });

    parser.add_argument("-D", "--remoteport")
            .help("Comma-separated list of remote ports to trace.")
            .default_value(std::string(""))
            .action([&config](const std::string& arg) {
                if (!arg.empty()) {
                    parsePorts(arg, config.target_dports);
                }
            });

    parser.add_description("Trace the lifespan of TCP sessions and summarize.\n"
                           "EXAMPLES:\n"
                           "  tcplife -p 1215            # only trace PID 1215\n"
                           "  tcplife -p 1215 -4         # trace IPv4 only");
}

} // namespace tcplife
} // namespace net

#endif // NET_TCPLIFE_ARG_PARSER_H
