#ifndef IPC_SOFDSNOOP_ARG_PARSER_H
#define IPC_SOFDSNOOP_ARG_PARSER_H

#include <string>
#include "argparse/argparse.hpp"
#include "ConfigArgs.h"

namespace ipc {
namespace sofdsnoop {

inline void cmdParser(argparse::ArgumentParser& parser, ConfigArgs& config) {
    parser.add_argument("-T", "--timestamp")
            .help("Include timestamp on output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.print_timestamp);

    parser.add_argument("-p", "--pid")
            .help("trace this PID only")
            .scan<'i', uint32_t>()
            .default_value(uint32_t{0})
            .store_into(config.target_pid);

    parser.add_argument("-t", "--tid")
            .help("trace this TID only")
            .scan<'i', uint32_t>()
            .default_value(uint32_t{0})
            .store_into(config.target_tid);

    parser.add_argument("-n", "--name")
            .help("only print process names containing this name")
            .default_value(std::string{})
            .store_into(config.filter_name);

    parser.add_argument("-d", "--duration")
            .help("total duration of trace in seconds")
            .scan<'i', uint32_t>()
            .default_value(uint32_t{0})
            .store_into(config.duration);

    parser.add_argument("--verbose")
            .help("Verbose debug output")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.verbose);

    parser.add_description("Trace file descriptors passed via Unix domain socket.\n"
                           "\n"
                           "EXAMPLES:\n"
                           "  sofdsnoop           # trace passed file descriptors\n"
                           "  sofdsnoop -T        # include timestamps\n"
                           "  sofdsnoop -p 181    # only trace PID 181\n"
                           "  sofdsnoop -t 123    # only trace TID 123\n"
                           "  sofdsnoop -d 10     # trace for 10 seconds only\n"
                           "  sofdsnoop -n main   # only print process names containing \"main\"");
}

} // namespace sofdsnoop
} // namespace ipc

#endif // IPC_SOFDSNOOP_ARG_PARSER_H
