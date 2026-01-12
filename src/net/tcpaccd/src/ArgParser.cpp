#include "ArgParser.h"

namespace net::tcpAccd {

void cmdParser(argparse::ArgumentParser &parser, ConfigArgs &config) {
  parser.add_argument("-v", "--verbose")
      .help("Enable verbose output")
      .default_value(false)
      .implicit_value(true)
      .store_into(config.verbose);

  parser.add_argument("-c", "--cgroup")
      .help("Cgroup v2 path")
      .default_value(std::string("/sys/fs/cgroup"))
      .store_into(config.cgroupPath);
}

} // namespace net::tcpAccd
