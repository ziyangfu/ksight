#ifndef NET_TCP_NAGLE_CONFIG_ARGS_H
#define NET_TCP_NAGLE_CONFIG_ARGS_H

#include <string>

namespace net::tcpNagle {

struct ConfigArgs {
  bool verbose = false;
  bool disabledOnly = false;
  bool agentMode = false;
  bool outputJson = false;
  int pid = 0;
  std::string cgroupPath = ""; // 如果不为空，则尝试在 cgroup 下强制禁用 Nagle
};

} // namespace net::tcpNagle

#endif // NET_TCP_NAGLE_CONFIG_ARGS_H
