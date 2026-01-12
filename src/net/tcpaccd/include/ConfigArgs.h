#ifndef NET_TCP_ACCD_CONFIG_ARGS_H
#define NET_TCP_ACCD_CONFIG_ARGS_H

#include <string>

namespace net::tcpAccd {

struct ConfigArgs {
  bool verbose = false;
  std::string cgroupPath = "/sys/fs/cgroup";
};

} // namespace net::tcpAccd

#endif // NET_TCP_ACCD_CONFIG_ARGS_H
