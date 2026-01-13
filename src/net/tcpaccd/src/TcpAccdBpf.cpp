#include "TcpAccdBpf.h"
#include "spdlog/spdlog.h"
#include <fcntl.h>
#include <iostream>
#include <unistd.h>

namespace net::tcpAccd {

TcpAccdBpf::TcpAccdBpf(ConfigArgs &config)
    : config_(config), skel_(nullptr), cgroupFd_(-1) {}

TcpAccdBpf::~TcpAccdBpf() { destroy(); }

void TcpAccdBpf::open() {
  skel_ = tcpaccd_bpf__open();
  if (!skel_) {
    SPDLOG_ERROR("Failed to open BPF skeleton");
  }
}

void TcpAccdBpf::load() {
  int err = tcpaccd_bpf__load(skel_);
  if (err) {
    SPDLOG_ERROR("Failed to load BPF skeleton: {}", err);
  }
}

void TcpAccdBpf::openAndLoad() {
  skel_ = tcpaccd_bpf__open_and_load();
  if (!skel_) {
    SPDLOG_ERROR("Failed to open and load BPF skeleton");
  }
}

void TcpAccdBpf::attach() {
  cgroupFd_ = ::open(config_.cgroupPath.c_str(), O_RDONLY);
  if (cgroupFd_ < 0) {
    SPDLOG_ERROR("Failed to open cgroup path: {}", config_.cgroupPath);
    return;
  }

  // Attach sockops program to cgroup
  int prog_fd = bpf_program__fd(skel_->progs.bpf_sockmap);
  int err = bpf_prog_attach(prog_fd, cgroupFd_, BPF_CGROUP_SOCK_OPS, 0);
  if (err) {
    SPDLOG_ERROR("Failed to attach sockops to cgroup: {}", err);
    return;
  }

  // Attach sk_msg program to sockmap
  err = tcpaccd_bpf__attach(skel_);
  if (err) {
    SPDLOG_ERROR("Failed to attach BPF skeleton: {}", err);
  }
}

void TcpAccdBpf::detach() {
  if (skel_) {
    tcpaccd_bpf__detach(skel_);
  }
  if (cgroupFd_ >= 0) {
    int prog_fd = bpf_program__fd(skel_->progs.bpf_sockmap);
    bpf_prog_detach2(prog_fd, cgroupFd_, BPF_CGROUP_SOCK_OPS);
    ::close(cgroupFd_);
    cgroupFd_ = -1;
  }
}

void TcpAccdBpf::destroy() {
  detach();
  if (skel_) {
    tcpaccd_bpf__destroy(skel_);
    skel_ = nullptr;
  }
}

void TcpAccdBpf::setRodataFlags() {}

void TcpAccdBpf::setBpfProgsLoadOpt() {}

void TcpAccdBpf::poll() { sleep(1); }

void TcpAccdBpf::printConns() {}

void TcpAccdBpf::printLogo() {
  std::cout << "TCP Acceleration Daemon (tcpaccd) starting..." << std::endl;
}

void TcpAccdBpf::handleEvent(void *ctx, void *data, size_t len) {}

void TcpAccdBpf::processEvent(void *data, size_t len) {}

} // namespace net::tcpAccd