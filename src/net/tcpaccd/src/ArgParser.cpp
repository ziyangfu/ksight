#include "ArgParser.h"
#include "Version.h"

namespace net::tcpAccd {

void cmdParser(argparse::ArgumentParser &parser, ConfigArgs &config) {
  parser.add_argument("-V", "--verbose")
      .help("Enable verbose output")
      .default_value(false)
      .implicit_value(true)
      .store_into(config.verbose);

  parser.add_argument("-c", "--cgroup")
      .help("Cgroup v2 path")
      .default_value(std::string("/sys/fs/cgroup"))
      .store_into(config.cgroupPath);
  parser.add_argument("-v", "--version")
      .help("Output version information")
      .default_value(false)
      .implicit_value(true)
      .action([](const std::string &value) {
        net::tcpAccd::printVersion();
        exit(0);
      });
}

} // namespace net::tcpAccd
