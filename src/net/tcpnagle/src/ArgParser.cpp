#include "ArgParser.h"

namespace net::tcpNagle {

void cmdParser(argparse::ArgumentParser &parser, ConfigArgs &config) {
  parser.add_argument("-vvv", "--verbose")
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

  parser.add_argument("-c", "--cgroup")
      .help("Specify cgroup v2 path to forcefully disable Nagle algorithm for "
            "all apps within it (e.g., /sys/fs/cgroup)")
      .default_value(std::string(""))
      .store_into(config.cgroupPath);

  parser.add_argument("--agent")
      .help("Enable agent mode (output to UDS)")
      .default_value(false)
      .implicit_value(true)
      .store_into(config.agentMode);

  parser.add_argument("--ojson")
      .help("Output result in JSON format")
      .default_value(false)
      .implicit_value(true)
      .store_into(config.outputJson);
}

} // namespace net::tcpNagle
