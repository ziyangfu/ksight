#include "ArgParser.h"
#include <iostream>

namespace net::tcpNagle {

void cmdParser(argparse::ArgumentParser &parser, ConfigArgs &config) {
  parser.add_argument("-V", "--verbose")
      .help("Enable verbose output")
      .default_value(false)
      .implicit_value(true)
      .store_into(config.verbose);

  parser.add_argument("-d", "--disabled-only")
      .help("Only show connections with Nagle disabled (TCP_NODELAY)")
      .default_value(false)
      .implicit_value(true)
      .store_into(config.disabledOnly);

  parser.add_argument("-p", "--pid")
      .help("Filter by process ID")
      .scan<'i', int>()
      .default_value(0)
      .store_into(config.pid);
}

} // namespace net::tcpNagle
