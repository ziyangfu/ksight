#include "TcpAccdBpf.h"
#include "spdlog/spdlog.h"
#include <cerrno>
#include <cstring>
#include <fcntl.h>
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
  SPDLOG_INFO("Opening cgroup path: {}", config_.cgroupPath);
  cgroupFd_ = ::open(config_.cgroupPath.c_str(), O_RDONLY);
  if (cgroupFd_ < 0) {
    SPDLOG_ERROR("Failed to open cgroup path: {}. Error: {}",
                 config_.cgroupPath, strerror(errno));
    return;
  }

  // Attach sockops program to cgroup
  int sockops_fd = bpf_program__fd(skel_->progs.bpf_sockmap);
  if (sockops_fd < 0) {
    SPDLOG_ERROR("Failed to get sockops program FD");
    return;
  }

  int err = bpf_prog_attach(sockops_fd, cgroupFd_, BPF_CGROUP_SOCK_OPS, 0);
  if (err) {
    SPDLOG_ERROR("Failed to attach sockops to cgroup: {}. Error: {}", err,
                 strerror(errno));
    return;
  }
  SPDLOG_INFO("Successfully attached sockops to cgroup");

  // Attach sk_msg program to sockmap
  int redir_fd = bpf_program__fd(skel_->progs.bpf_redir);
  int map_fd = bpf_map__fd(skel_->maps.sock_ops_map);
  if (redir_fd < 0 || map_fd < 0) {
    SPDLOG_ERROR("Failed to get redir prog FD or map FD");
    return;
  }

  err = bpf_prog_attach(redir_fd, map_fd, BPF_SK_MSG_VERDICT, 0);
  if (err) {
    SPDLOG_ERROR("Failed to attach sk_msg to sockmap: {}. Error: {}", err,
                 strerror(errno));
    return;
  }
  SPDLOG_INFO("Successfully attached sk_msg to sockmap");

  // Attach other programs if any (though usually not needed for this logic)
  err = tcpaccd_bpf__attach(skel_);
  if (err) {
    SPDLOG_ERROR("Failed to auto-attach BPF skeleton: {}", err);
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

void TcpAccdBpf::handleEvent(void *ctx, void *data, size_t len) {}

void TcpAccdBpf::processEvent(void *data, size_t len) {}

} // namespace net::tcpAccd