#ifndef NET_GET_HOST_LATENCY_ARG_PARSER_H
#define NET_GET_HOST_LATENCY_ARG_PARSER_H

#include "ConfigArgs.h"
#include "argparse/argparse.hpp"

namespace net::getHostLatency {

inline void cmdParser(argparse::ArgumentParser &parser, ConfigArgs &config) {
  parser.add_argument("-p", "--pid")
      .help("Process ID to trace")
      .default_value(0u)
      .scan<'u', uint32_t>()
      .store_into(config.target_pid);

  parser.add_argument("-l", "--libc")
      .help("Specify which libc.so to use")
      .default_value(std::string(""))
      .store_into(config.libc_path);

  parser.add_argument("-vvv", "--verbose")
      .help("Verbose debug output")
      .default_value(false)
      .implicit_value(true)
      .store_into(config.verbose);

  parser.add_description("Show latency for getaddrinfo/gethostbyname[2] calls");
}

} // namespace net::getHostLatency

#endif // NET_GET_HOST_LATENCY_ARG_PARSER_H
