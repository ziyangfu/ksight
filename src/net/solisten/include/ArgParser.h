#ifndef NET_SO_LISTEN_ARG_PARSER_H
#define NET_SO_LISTEN_ARG_PARSER_H

#include "ConfigArgs.h"
#include "argparse/argparse.hpp"

namespace net::soListen {

inline void cmdParser(argparse::ArgumentParser &parser, ConfigArgs &config) {
  parser.add_argument("-p", "--pid")
      .help("Process ID to trace")
      .default_value(0u)
      .scan<'u', uint32_t>()
      .store_into(config.target_pid);

  parser.add_argument("-t", "--timestamp")
      .help("Include timestamp on output")
      .default_value(false)
      .implicit_value(true)
      .store_into(config.emit_timestamp);

  parser.add_argument("-vvv", "--verbose")
      .help("Verbose debug output")
      .default_value(false)
      .implicit_value(true)
      .store_into(config.verbose);

  parser.add_description("Trace IPv4 and IPv6 listen syscalls");
}

} // namespace net::soListen

#endif // NET_SO_LISTEN_ARG_PARSER_H
