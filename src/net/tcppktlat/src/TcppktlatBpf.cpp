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

#include "TcppktlatBpf.h"
#include "fmt/format.h"
#include "spdlog/spdlog.h"

extern "C" {
#include "trace_helpers.h"
#include "compat.h"
}

extern "C" {
#include "tcppktlat.skel.h"
}

using namespace net::tcppktlat;

static volatile sig_atomic_t g_exiting = 0;

static void sig_handler(int signo) { g_exiting = 1; }

TcppktlatBpf::TcppktlatBpf(const ConfigArgs &config) : config_(config) {}

TcppktlatBpf::~TcppktlatBpf() { destroy(); }

void TcppktlatBpf::destroy() {
  if (buf_) {
    bpf_buffer__free(buf_);
    buf_ = nullptr;
  }
  if (skel_) {
    tcppktlat_bpf__destroy(skel_);
    skel_ = nullptr;
  }
}

void TcppktlatBpf::init() {
  skel_ = tcppktlat_bpf__open();
  if (!skel_) {
    throw std::runtime_error("Failed to open BPF skeleton");
  }
}

void TcppktlatBpf::load() {
  skel_->rodata->targ_pid = config_.target_pid;
  skel_->rodata->targ_tid = config_.target_tid;
  skel_->rodata->targ_sport = config_.target_sport;
  skel_->rodata->targ_dport = config_.target_dport;
  skel_->rodata->targ_min_us = config_.min_us;

  buf_ = bpf_buffer__new(skel_->maps.events, skel_->maps.heap);
  if (!buf_) {
    throw std::runtime_error(fmt::format("Failed to create ring/perf buffer: {}", -errno));
  }

  if (probe_tp_btf("tcp_probe")) {
    bpf_program__set_autoload(skel_->progs.tcp_probe, false);
    bpf_program__set_autoload(skel_->progs.tcp_rcv_space_adjust, false);
    bpf_program__set_autoload(skel_->progs.tcp_destroy_sock, false);
  } else {
    bpf_program__set_autoload(skel_->progs.tcp_probe_btf, false);
    bpf_program__set_autoload(skel_->progs.tcp_rcv_space_adjust_btf, false);
    bpf_program__set_autoload(skel_->progs.tcp_destroy_sock_btf, false);
  }

  int err = tcppktlat_bpf__load(skel_);
  if (err) {
    destroy();
    throw std::runtime_error(
        fmt::format("Failed to load BPF object: {}, maybe your kernel doesn't support bpf_get_socket_cookie", err));
  }
}

void TcppktlatBpf::attach() {
  int err = tcppktlat_bpf__attach(skel_);
  if (err) {
    destroy();
    throw std::runtime_error(
        fmt::format("Failed to attach BPF programs: {}", err));
  }
}

int TcppktlatBpf::handleEvent(void *ctx, void *data, size_t data_sz) {
  auto self = static_cast<TcppktlatBpf *>(ctx);
  if (data_sz < sizeof(struct event))
    return 0;
  auto e = static_cast<const struct event *>(data);
  self->processEvent(e);
  return 0;
}

void TcppktlatBpf::handleLostEvents(void *ctx, int cpu, unsigned long long lost_cnt) {
  SPDLOG_WARN("lost {} events on CPU #{}", lost_cnt, cpu);
}

void TcppktlatBpf::processEvent(const struct event *e) {
  char ts[32];
  char saddr[INET6_ADDRSTRLEN], daddr[INET6_ADDRSTRLEN];

  if (config_.print_timestamp) {
    str_timestamp("%H:%M:%S", ts, sizeof(ts));
    fmt::print("{:<8} ", ts);
  }
  
  inet_ntop(e->family, e->saddr, saddr, sizeof(saddr));
  inet_ntop(e->family, e->daddr, daddr, sizeof(daddr));

  int column_width = config_.wide_output ? 26 : 15;

  fmt::print("{:<7} {:<7} {:<16.16} {:<{}} {:<5} {:<{}} {:<5} {:.2f}\n",
             e->pid, e->tid, e->comm, saddr, column_width, ntohs(e->sport),
             daddr, column_width, ntohs(e->dport), e->delta_us / 1000.0);
}

void TcppktlatBpf::poll() {
  int err = bpf_buffer__open(buf_, handleEvent, handleLostEvents, this);
  if (err) {
    throw std::runtime_error(fmt::format("Failed to open ring/perf buffer: {}", err));
  }

  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  if (config_.print_timestamp)
    fmt::print("{:<8} ", "TIME(s)");
  
  int column_width = config_.wide_output ? 26 : 15;
  fmt::print("{:<7} {:<7} {:<16} {:<{}} {:<5} {:<{}} {:<5} {}\n",
             "PID", "TID", "COMM", "LADDR", column_width, "LPORT",
             "RADDR", column_width, "RPORT", "MS");

  while (!g_exiting) {
    err = bpf_buffer__poll(buf_, 100);
    if (err < 0 && err != -EINTR) {
      throw std::runtime_error(
          fmt::format("Error polling ring/perf buffer: {}", err));
    }
  }
}
