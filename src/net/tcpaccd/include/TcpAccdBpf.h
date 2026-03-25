#ifndef NET_TCP_ACCD_BPF_H
#define NET_TCP_ACCD_BPF_H

#include "ConfigArgs.h"
extern "C" {
#include "tcpaccd.skel.h"
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
}

namespace net::tcpAccd {

class TcpAccdBpf final {
private:
  ConfigArgs &config_;
  struct ::tcpaccd_bpf *skel_;
  int cgroupFd_;
  const int kPollPeriodMs{200};

public:
  explicit TcpAccdBpf(ConfigArgs &config);
  ~TcpAccdBpf();

  void open();
  void load();
  void openAndLoad();
  void attach();
  void detach();
  void destroy();

  void setRodataFlags();
  void setBpfProgsLoadOpt();
  void poll();
  void printConns();
  void printLogo();

private:
  static void handleEvent(void *ctx, void *data, size_t len);
  void processEvent(void *data, size_t len);
};

} // namespace net::tcpAccd

#endif // NET_TCP_ACCD_BPF_H