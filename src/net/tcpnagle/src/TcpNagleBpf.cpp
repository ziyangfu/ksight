#include "TcpNagleBpf.h"
#include "spdlog/spdlog.h"
#include <arpa/inet.h>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <unistd.h>

namespace net::tcpNagle {

TcpNagleBpf::TcpNagleBpf(ConfigArgs &config)
    : config_(config), skel_(nullptr), ringBuffer_(nullptr), cgroupFd_(-1) {}

TcpNagleBpf::~TcpNagleBpf() { destroy(); }

void TcpNagleBpf::openAndLoad() {
  skel_ = tcpnagle_bpf__open_and_load();
  if (!skel_) {
    SPDLOG_ERROR("Failed to open and load BPF skeleton");
    return;
  }

  ringBuffer_ =
      ring_buffer__new(bpf_map__fd(skel_->maps.events), handleEvent, this, NULL);
  if (!ringBuffer_) {
    SPDLOG_ERROR("Failed to create ring buffer");
  }
}

void TcpNagleBpf::attach() {
  if (config_.cgroupPath.empty()) {
    return;
  }

  SPDLOG_INFO("正在挂载 sockops 程序到 cgroup: {}", config_.cgroupPath);
  cgroupFd_ = ::open(config_.cgroupPath.c_str(), O_RDONLY);
  if (cgroupFd_ < 0) {
    SPDLOG_ERROR("无法打开 cgroup 路径: {}. Error: {}",
                 config_.cgroupPath, strerror(errno));
    return;
  }

  int sockops_fd = bpf_program__fd(skel_->progs.bpf_disable_nagle);
  if (sockops_fd < 0) {
    SPDLOG_ERROR("无法获取 sockops 程序 FD");
    return;
  }

  int err = bpf_prog_attach(sockops_fd, cgroupFd_, BPF_CGROUP_SOCK_OPS, 0);
  if (err) {
    SPDLOG_ERROR("挂载 sockops 到 cgroup 失败: {}. Error: {}", err,
                 strerror(errno));
    return;
  }
  SPDLOG_INFO("成功挂载！该 cgroup 下的所有新 TCP 连接将强制禁用 Nagle 算法。");
}

void TcpNagleBpf::detach() {
  if (cgroupFd_ >= 0) {
    int sockops_fd = bpf_program__fd(skel_->progs.bpf_disable_nagle);
    if (sockops_fd >= 0) {
      bpf_prog_detach2(sockops_fd, cgroupFd_, BPF_CGROUP_SOCK_OPS);
    }
    ::close(cgroupFd_);
    cgroupFd_ = -1;
  }
}

void TcpNagleBpf::destroy() {
  detach();
  if (ringBuffer_) {
    ring_buffer__free(ringBuffer_);
    ringBuffer_ = nullptr;
  }
  if (skel_) {
    tcpnagle_bpf__destroy(skel_);
    skel_ = nullptr;
  }
}

void TcpNagleBpf::run() {
  buildProcMap();

  // 1. 创建迭代器 Link (针对现有 Socket 的快照探测)
  LIBBPF_OPTS(bpf_iter_attach_opts, opts);
  struct bpf_link *link =
      bpf_program__attach_iter(skel_->progs.tcpnagle_iter, &opts);
  if (!link) {
    SPDLOG_ERROR("Failed to attach BPF iterator");
    return;
  }

  // 2. 创建迭代器读取 FD 以触发运行
  int iter_fd = bpf_iter_create(bpf_link__fd(link));
  if (iter_fd < 0) {
    SPDLOG_ERROR("Failed to create iterator FD");
    bpf_link__destroy(link);
    return;
  }

  // 表头打印 (在此处打印，确保结果在中间)
  printf("\n%-10s %-25s %-25s %-20s %-15s\n", "类型", "本地地址:端口",
         "远端地址:端口", "程序(PID)", "Nagle状态");
  printf("%.110s\n", std::string(110, '-').c_str());

  // 3. 读取迭代器触发 BPF 程序
  char buf[64];
  while (::read(iter_fd, buf, sizeof(buf)) > 0) {
      // BPF 程序产生事件会通过 RingBuf 传输
  }

  // 4. 处理收集到的事件
  ring_buffer__consume(ringBuffer_);

  ::close(iter_fd);
  bpf_link__destroy(link);
  printf("%.110s\n\n", std::string(110, '-').c_str());
}

void TcpNagleBpf::buildProcMap() {
  connToProc_.clear();
  DIR *proc = opendir("/proc");
  if (!proc)
    return;

  struct dirent *entry;
  while ((entry = readdir(proc))) {
    int pid = atoi(entry->d_name);
    if (pid <= 0)
      continue;

    // 读取进程名
    std::string comm;
    std::ifstream comm_file("/proc/" + std::to_string(pid) + "/comm");
    std::getline(comm_file, comm);

    // 扫描 FD 目录寻找 Socket Inode
    DIR *fd_dir = opendir(("/proc/" + std::to_string(pid) + "/fd").c_str());
    if (!fd_dir)
      continue;

    struct dirent *fd_entry;
    while ((fd_entry = readdir(fd_dir))) {
      char link_path[256];
      ssize_t len = readlink(
          ("/proc/" + std::to_string(pid) + "/fd/" + fd_entry->d_name).c_str(),
          link_path, sizeof(link_path) - 1);
      if (len > 0) {
        link_path[len] = '\0';
        unsigned long long inode;
        if (sscanf(link_path, "socket:[%llu]", &inode) == 1) {
          connToProc_[std::to_string(inode)] = {pid, comm};
        }
      }
    }
    closedir(fd_dir);
  }
  closedir(proc);
}

int TcpNagleBpf::handleEvent(void *ctx, void *data, size_t len) {
  auto *self = static_cast<TcpNagleBpf *>(ctx);
  auto *event = static_cast<const struct tcpnagle_event *>(data);
  self->processEvent(event);
  return 0;
}

void TcpNagleBpf::processEvent(const struct tcpnagle_event *event) {
  // 过滤 PID
  std::string inode_str = std::to_string(event->inode);
  int pid = 0;
  std::string comm = "-";

  if (connToProc_.count(inode_str)) {
    pid = connToProc_[inode_str].pid;
    comm = connToProc_[inode_str].comm;
  }

  if (config_.pid != 0 && pid != config_.pid) {
      return;
  }

  // 过滤 DisabledOnly
  bool is_disabled = (event->nonagle == 1); // TCP_NAGLE_OFF
  if (config_.disabledOnly && !is_disabled) {
      return;
  }

  char s_addr_str[INET_ADDRSTRLEN], d_addr_str[INET_ADDRSTRLEN];
  struct in_addr s_addr = {event->saddr}, d_addr = {event->daddr};
  inet_ntop(AF_INET, &s_addr, s_addr_str, sizeof(s_addr_str));
  inet_ntop(AF_INET, &d_addr, d_addr_str, sizeof(d_addr_str));

  std::string local = std::string(s_addr_str) + ":" + std::to_string(event->sport);
  std::string remote = std::string(d_addr_str) + ":" + std::to_string(event->dport);
  std::string prog_info = (pid != 0) ? (comm + "(" + std::to_string(pid) + ")") : "unknown";
  
  std::string status;
  if (event->nonagle == 1) status = "✅ DISABLED";
  else if (event->nonagle == 2) status = "⚠ CORKED";
  else status = "❌ ENABLED";

  printf("%-10s %-25s %-25s %-20s %-15s\n", "连接", local.c_str(),
         remote.c_str(), prog_info.c_str(), status.c_str());
}

} // namespace net::tcpNagle
