/*!
 * \brief tcpaccd 版本信息
 * \file Version.h
 * */

#ifndef NET_TCP_ACCD_VERSION_H
#define NET_TCP_ACCD_VERSION_H

#include <string>
#if __cplusplus >= 202002L
#include <format>
namespace fmt = std;
#else
#include "fmt/format.h"
#endif

namespace net::tcpAccd {

const std::string kTcpAccdVersion{"0.0.1"};
int printVersion() {
  fmt::print("tcpaccd version: {}\n", kTcpAccdVersion);
  return 0;
}
} // namespace net::tcpAccd

#endif // NET_TCP_ACCD_VERSION_H
