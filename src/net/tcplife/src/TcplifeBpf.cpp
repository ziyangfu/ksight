#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <ctime>
#include <fcntl.h>
#include <iostream>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "TcplifeBpf.h"
#include "fmt/format.h"
#include "spdlog/spdlog.h"

extern "C" {
#include "btf_helpers.h"
#include "trace_helpers.h"
}

extern "C" {
#include "tcplife.skel.h"
}

using namespace net::tcplife;

static volatile sig_atomic_t g_exiting = 0;

static void sig_handler(int signo) { g_exiting = 1; }

#ifndef AF_INET
#define AF_INET 2
#endif
#ifndef AF_INET6
#define AF_INET6 10
#endif

TcplifeBpf::TcplifeBpf(const ConfigArgs &config) : config_(config) {}

TcplifeBpf::~TcplifeBpf() { destroy(); }

void TcplifeBpf::destroy() {
  if (pb_) {
    perf_buffer__free(pb_);
    pb_ = nullptr;
  }
  if (skel_) {
    tcplife_bpf__destroy(skel_);
    skel_ = nullptr;
  }
}

void TcplifeBpf::init() {
  LIBBPF_OPTS(bpf_object_open_opts, open_opts);
  if (ensure_core_btf(&open_opts)) {
    SPDLOG_WARN(
        "Failed to fetch necessary BTF for CO-RE, continuing anyway...");
  }
  skel_ = tcplife_bpf__open_opts(&open_opts);
  if (!skel_) {
    cleanup_core_btf(&open_opts);
    throw std::runtime_error("Failed to open BPF skeleton");
  }
}

void TcplifeBpf::load() {
  uint16_t family = 0;
  if (config_.ipv4_only)
    family = AF_INET;
  if (config_.ipv6_only)
    family = AF_INET6;

  skel_->rodata->target_pid = config_.target_pid;
  skel_->rodata->target_family = family;

  if (!config_.target_sports.empty()) {
    size_t count = std::min<size_t>(config_.target_sports.size(), MAX_PORTS);
    for (size_t i = 0; i < count; i++) {
      skel_->rodata->target_sports[i] = config_.target_sports[i];
    }
    skel_->rodata->filter_sport = true;
  }

  if (!config_.target_dports.empty()) {
    size_t count = std::min<size_t>(config_.target_dports.size(), MAX_PORTS);
    for (size_t i = 0; i < count; i++) {
      skel_->rodata->target_dports[i] = config_.target_dports[i];
    }
    skel_->rodata->filter_dport = true;
  }

  int err = tcplife_bpf__load(skel_);
  if (err) {
    destroy();
    throw std::runtime_error(
        fmt::format("Failed to load BPF skeleton: {}", err));
  }
}

void TcplifeBpf::attach() {
  int err = tcplife_bpf__attach(skel_);
  if (err) {
    destroy();
    throw std::runtime_error(
        fmt::format("Failed to attach BPF programs: {}", err));
  }
}

void TcplifeBpf::handleEvent(void *ctx, int cpu, void *data,
                             unsigned int data_sz) {
  auto self = static_cast<TcplifeBpf *>(ctx);
  if (data_sz < sizeof(struct event))
    return;
  auto e = static_cast<const struct event *>(data);
  self->processEvent(e);
}

void TcplifeBpf::handleLostEvents(void *ctx, int cpu,
                                  unsigned long long lost_cnt) {
  SPDLOG_WARN("lost {} events on CPU #{}", lost_cnt, cpu);
}

void TcplifeBpf::processEvent(const struct event *e) {
  char ts[32], saddr[INET6_ADDRSTRLEN], daddr[INET6_ADDRSTRLEN];

  if (config_.print_timestamp) {
    str_timestamp("%H:%M:%S", ts, sizeof(ts));
    fmt::print("{:>8} ", ts);
  }

  inet_ntop(e->family, &e->saddr, saddr, sizeof(saddr));
  inet_ntop(e->family, &e->daddr, daddr, sizeof(daddr));

  int column_width = config_.wide_output ? 39 : 15;

  fmt::print(
      "{:<7} {:<16.16} {:<{}} {:<5} {:<{}} {:<5} {:<6.2f} {:<6.2f} {:.2f}\n",
      e->pid, e->comm, saddr, column_width, e->sport, daddr, column_width,
      e->dport, (double)e->tx_b / 1024.0, (double)e->rx_b / 1024.0,
      (double)e->span_us / 1000.0);
}

void TcplifeBpf::poll() {
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  pb_ = perf_buffer__new(bpf_map__fd(skel_->maps.events), 16, handleEvent,
                         handleLostEvents, this, nullptr);
  if (!pb_) {
    throw std::runtime_error(
        fmt::format("Failed to open perf buffer: {}", -errno));
  }

  if (config_.print_timestamp)
    fmt::print("{:<8} ", "TIME(s)");

  int column_width = config_.wide_output ? 39 : 15;
  fmt::print("{:<7} {:<16} {:<{}} {:<5} {:<{}} {:<5} {:<6} {:<6} {}\n", "PID",
             "COMM", "LADDR", column_width, "LPORT", "RADDR", column_width,
             "RPORT", "TX_KB", "RX_KB", "MS");

  while (!g_exiting) {
    int err = perf_buffer__poll(pb_, 100);
    if (err < 0 && err != -EINTR) {
      throw std::runtime_error(
          fmt::format("Error polling perf buffer: {}", err));
    }
  }
}
