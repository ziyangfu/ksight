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
  int cgroupFd_; // 用于挂载 sockops 的 cgroup 文件描述符

  struct ProcInfo {
    int pid;
    std::string comm;
  };
  std::map<std::string, ProcInfo> connToProc_;

public:
  explicit TcpNagleBpf(ConfigArgs &config);
  ~TcpNagleBpf();

  void open();
  void load();
  void openAndLoad();
  void attach();
  void detach(); // 新增侦听解绑
  void destroy();

  void run(); // 对现有连接快照探测

private:
  static int handleEvent(void *ctx, void *data, size_t len);
  void processEvent(const struct tcpnagle_event *event);
  void buildProcMap();
  std::string addr2str(uint32_t addr, uint16_t port);
};

} // namespace net::tcpNagle

#endif // NET_TCP_NAGLE_BPF_H
