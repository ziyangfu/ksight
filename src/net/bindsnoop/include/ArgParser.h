#ifndef NET_BIND_SNOOP_ARG_PARSER_H
#define NET_BIND_SNOOP_ARG_PARSER_H

#include "argparse/argparse.hpp"
#include "ConfigArgs.h"

namespace net::bindSnoop {

inline void cmdParser(argparse::ArgumentParser& parser, ConfigArgs& config) {
    parser.add_argument("-t", "--timestamp")
            .help("Include timestamp on output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.emit_timestamp);

    parser.add_argument("-x", "--failed")
            .help("Include errors on output")
            .default_value(true) // Original ignore_errors was true, meaning NOT include errors? 
                                 // Wait, let's look at original again.
                                 // case 'x': ignore_errors = false; break;
                                 // So by default it ignores errors.
            .implicit_value(false)
            .store_into(config.ignore_errors);

    parser.add_argument("-p", "--pid")
            .help("Process ID to trace")
            .default_value(0u)
            .scan<'u', uint32_t>()
            .store_into(config.target_pid);

    parser.add_argument("-P", "--ports")
            .help("Comma-separated list of ports to trace")
            .default_value(std::string(""))
            .store_into(config.target_ports);

    parser.add_argument("-c", "--cgroup")
            .help("Trace process in cgroup path")
            .default_value(std::string(""))
            .store_into(config.cgroupspath);

    parser.add_argument("-v", "--verbose")
            .help("Verbose debug output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.verbose);

    parser.add_description("Trace bind syscalls");
}

} // namespace net::bindSnoop

#endif // NET_BIND_SNOOP_ARG_PARSER_H
