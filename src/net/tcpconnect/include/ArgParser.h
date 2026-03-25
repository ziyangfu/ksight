#ifndef NET_TCP_CONNECT_ARG_PARSER_H
#define NET_TCP_CONNECT_ARG_PARSER_H

#include <iostream>
#include <string>
#include <sstream>
#include "argparse/argparse.hpp"
#include "ConfigArgs.h"

namespace net::tcpConnect {

inline void cmdParser(argparse::ArgumentParser& parser, ConfigArgs& config) {
    parser.add_argument("-v", "--verbose")
            .help("Verbose debug output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.verbose);

    parser.add_argument("-t", "--timestamp")
            .help("Include timestamp on output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.print_timestamp);

    parser.add_argument("-c", "--count")
            .help("Count connects per src ip and dst ip/port")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.count);

    parser.add_argument("-U", "--print-uid")
            .help("Include UID on output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.print_uid);

    parser.add_argument("-p", "--pid")
            .help("Process PID to trace")
            .default_value(0u)
            .scan<'u', uint32_t>()
            .store_into(config.target_pid);

    parser.add_argument("-u", "--uid")
            .help("Process UID to trace")
            .default_value(static_cast<uint32_t>(-1))
            .scan<'u', uint32_t>()
            .store_into(config.target_uid);

    parser.add_argument("-s", "--source-port")
            .help("Consider source port when counting")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.source_port);

    parser.add_argument("-P", "--port")
            .help("Comma-separated list of destination ports to trace")
            .default_value(std::string(""))
            .action([&config](const std::string& value) {
                std::stringstream ss(value);
                std::string segment;
                while (std::getline(ss, segment, ',')) {
                    try {
                        config.target_ports.push_back(static_cast<uint16_t>(std::stoi(segment)));
                    } catch (...) {
                        throw std::runtime_error("Invalid port: " + segment);
                    }
                }
            });

    parser.add_description("Count/Trace active tcp connections");
}

} // namespace net::tcpConnect

#endif // NET_TCP_CONNECT_ARG_PARSER_H
