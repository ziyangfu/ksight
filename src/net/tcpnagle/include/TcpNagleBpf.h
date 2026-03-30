#ifndef NET_TCP_NAGLE_BPF_H
#define NET_TCP_NAGLE_BPF_H

#include "ConfigArgs.h"
#include "tcpnagle_common.h"
#include <map>
#include <string>

extern "C" {
#include "tcpnagle.skel.h"
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
}

namespace net::tcpNagle {

class TcpNagleBpf final {
private:
  ConfigArgs &config_;
  struct tcpnagle_bpf *skel_;
  struct ring_buffer *ringBuffer_;

  struct ProcInfo {
    int pid;
    std::string comm;
  };
  // Key: "src_ip:src_port" 字符串，用于匹配迭代器发现的 Socket
  std::map<std::string, ProcInfo> connToProc_;

public:
  explicit TcpNagleBpf(ConfigArgs &config);
  ~TcpNagleBpf();

  void open();
  void load();
  void openAndLoad();
  void attach();
  void destroy();

  void run(); // 开始迭代并读取结果

private:
  static int handleEvent(void *ctx, void *data, size_t len);
  void processEvent(const struct tcpnagle_event *event);
  void buildProcMap();
  std::string addr2str(uint32_t addr, uint16_t port);
};

} // namespace net::tcpNagle

#endif // NET_TCP_NAGLE_BPF_H
