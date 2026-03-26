#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <string>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "SofdsnoopBpf.h"
#include "fmt/format.h"
#include "spdlog/spdlog.h"

extern "C" {
#include "trace_helpers.h"
}

extern "C" {
#include "sofdsnoop.skel.h"
}

using namespace ipc::sofdsnoop;

static volatile sig_atomic_t g_exiting = 0;

static void sig_handler(int signo) { g_exiting = 1; }

SofdsnoopBpf::SofdsnoopBpf(const ConfigArgs &config) : config_(config) {}

SofdsnoopBpf::~SofdsnoopBpf() { destroy(); }

void SofdsnoopBpf::destroy() {
  if (pb_) {
    perf_buffer__free(pb_);
    pb_ = nullptr;
  }
  if (skel_) {
    sofdsnoop_bpf__destroy(skel_);
    skel_ = nullptr;
  }
}

void SofdsnoopBpf::init() {
  skel_ = sofdsnoop_bpf__open();
  if (!skel_) {
    throw std::runtime_error("Failed to open BPF skeleton");
  }
}

void SofdsnoopBpf::load() {
  // 通过 rodata 配置过滤参数
  if (config_.target_pid)
    skel_->rodata->targ_pid = static_cast<pid_t>(config_.target_pid);
  if (config_.target_tid)
    skel_->rodata->targ_tid = static_cast<pid_t>(config_.target_tid);

  int err = sofdsnoop_bpf__load(skel_);
  if (err) {
    destroy();
    throw std::runtime_error(fmt::format("Failed to load BPF skeleton: {}", err));
  }
}

void SofdsnoopBpf::attach() {
  // sendmsg/recvmsg 的系统调用符号在不同内核可能带 __x64_ 前缀，
  // 尝试手动挂载两个变体，如果已有 SEC("kprobe/sys_sendmsg") 这类自动挂载可能不够，
  // 这里依赖 skeleton auto-attach 处理
  int err = sofdsnoop_bpf__attach(skel_);
  if (err) {
    destroy();
    throw std::runtime_error(fmt::format("Failed to attach BPF programs: {}", err));
  }
}

std::string SofdsnoopBpf::getFileName(uint32_t tid, int fd) {
  if (fd < 0) return "N/A";
  char path[256];
  char target[1024];
  snprintf(path, sizeof(path), "/proc/%u/fd/%d", tid, fd);
  ssize_t len = readlink(path, target, sizeof(target) - 1);
  if (len < 0) return "N/A";
  target[len] = '\0';
  return std::string(target);
}

void SofdsnoopBpf::handleEvent(void *ctx, int cpu, void *data, unsigned int data_sz) {
  auto self = static_cast<SofdsnoopBpf *>(ctx);
  if (data_sz < sizeof(struct event))
    return;
  auto e = static_cast<const struct event *>(data);
  self->processEvent(e);
}

void SofdsnoopBpf::handleLostEvents(void *ctx, int cpu, unsigned long long lost_cnt) {
  SPDLOG_WARN("lost {} events on CPU #{}", lost_cnt, cpu);
}

void SofdsnoopBpf::processEvent(const struct event *e) {
  uint32_t tid = static_cast<uint32_t>(e->id & 0xffffffff);
  int cnt = (e->fd_cnt < MAX_FD) ? e->fd_cnt : MAX_FD;

  // 过滤进程名
  if (!config_.filter_name.empty()) {
    std::string comm(e->comm, strnlen(e->comm, TASK_COMM_LEN));
    if (comm.find(config_.filter_name) == std::string::npos)
      return;
  }

  for (int i = 0; i < cnt; i++) {
    if (config_.print_timestamp) {
      if (start_ts_ == 0)
        start_ts_ = e->ts;
      double delta_s = (double)(e->ts - start_ts_) / 1e6;
      fmt::print("{:<14.9f} ", delta_s);
    }

    const char *action_str = (e->action == ACTION_SEND) ? "SEND" : "RECV";
    std::string sock_desc = fmt::format("{}:{}", e->sock_fd, getFileName(tid, e->sock_fd));

    int fd = e->fd[i];
    std::string fd_file = (e->action == ACTION_SEND) ? getFileName(tid, fd) : "";

    fmt::print("{:<6} {:<6} {:<16.16} {:<25} {:<5} {}\n",
               action_str, tid, e->comm,
               sock_desc, fd, fd_file);
  }
}

void SofdsnoopBpf::poll() {
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  pb_ = perf_buffer__new(bpf_map__fd(skel_->maps.events), 64,
                         handleEvent, handleLostEvents, this, nullptr);
  if (!pb_) {
    throw std::runtime_error(fmt::format("Failed to open perf buffer: {}", -errno));
  }

  // 打印表头
  if (config_.print_timestamp)
    fmt::print("{:<14} ", "TIME(s)");
  fmt::print("{:<6} {:<6} {:<16} {:<25} {:<5} {}\n",
             "ACTION", "TID", "COMM", "SOCKET", "FD", "NAME");

  auto start = std::chrono::steady_clock::now();

  while (!g_exiting) {
    int err = perf_buffer__poll(pb_, 100);
    if (err < 0 && err != -EINTR) {
      throw std::runtime_error(fmt::format("Error polling perf buffer: {}", err));
    }

    // 检查 duration 是否到期
    if (config_.duration > 0) {
      auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
          std::chrono::steady_clock::now() - start).count();
      if (static_cast<uint32_t>(elapsed) >= config_.duration)
        break;
    }
  }
}
