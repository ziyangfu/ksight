#ifndef NET_TCP_NAGLE_CONFIG_ARGS_H
#define NET_TCP_NAGLE_CONFIG_ARGS_H

#include <string>

namespace net::tcpNagle {

struct ConfigArgs {
  bool verbose = false;
  bool disabledOnly = false;
  int pid = 0;
};

} // namespace net::tcpNagle

#endif // NET_TCP_NAGLE_CONFIG_ARGS_H
