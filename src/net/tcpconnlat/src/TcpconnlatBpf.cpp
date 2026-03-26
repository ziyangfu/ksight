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

#include "TcpconnlatBpf.h"
#include "fmt/format.h"
#include "spdlog/spdlog.h"

extern "C" {
#include "btf_helpers.h"
#include "trace_helpers.h"
}

extern "C" {
#include "tcpconnlat.skel.h"
}

using namespace net::tcpconnlat;

static volatile sig_atomic_t g_exiting = 0;

static void sig_handler(int signo) { g_exiting = 1; }

#ifndef AF_INET
#define AF_INET 2
#endif
#ifndef AF_INET6
#define AF_INET6 10
#endif

TcpconnlatBpf::TcpconnlatBpf(const ConfigArgs &config) : config_(config) {}

TcpconnlatBpf::~TcpconnlatBpf() { destroy(); }

void TcpconnlatBpf::destroy() {
  if (pb_) {
    perf_buffer__free(pb_);
    pb_ = nullptr;
  }
  if (skel_) {
    tcpconnlat_bpf__destroy(skel_);
    skel_ = nullptr;
  }
}

void TcpconnlatBpf::init() {
  LIBBPF_OPTS(bpf_object_open_opts, open_opts);
  if (ensure_core_btf(&open_opts)) {
    SPDLOG_WARN(
        "Failed to fetch necessary BTF for CO-RE, continuing anyway...");
  }
  skel_ = tcpconnlat_bpf__open_opts(&open_opts);
  if (!skel_) {
    cleanup_core_btf(&open_opts);
    throw std::runtime_error("Failed to open BPF skeleton");
  }
}

void TcpconnlatBpf::load() {
  skel_->rodata->targ_min_us = static_cast<uint64_t>(config_.min_us);
  skel_->rodata->targ_tgid = config_.target_pid;

  if (fentry_can_attach("tcp_v4_connect", NULL)) {
    bpf_program__set_attach_target(skel_->progs.fentry_tcp_v4_connect, 0, "tcp_v4_connect");
    bpf_program__set_attach_target(skel_->progs.fentry_tcp_v6_connect, 0, "tcp_v6_connect");
    bpf_program__set_attach_target(skel_->progs.fentry_tcp_rcv_state_process, 0, "tcp_rcv_state_process");
    bpf_program__set_autoload(skel_->progs.tcp_v4_connect, false);
    bpf_program__set_autoload(skel_->progs.tcp_v6_connect, false);
    bpf_program__set_autoload(skel_->progs.tcp_rcv_state_process, false);
  } else {
    bpf_program__set_autoload(skel_->progs.fentry_tcp_v4_connect, false);
    bpf_program__set_autoload(skel_->progs.fentry_tcp_v6_connect, false);
    bpf_program__set_autoload(skel_->progs.fentry_tcp_rcv_state_process, false);
  }

  int err = tcpconnlat_bpf__load(skel_);
  if (err) {
    destroy();
    throw std::runtime_error(
        fmt::format("Failed to load BPF skeleton: {}", err));
  }
}

void TcpconnlatBpf::attach() {
  int err = tcpconnlat_bpf__attach(skel_);
  if (err) {
    destroy();
    throw std::runtime_error(
        fmt::format("Failed to attach BPF programs: {}", err));
  }
}

void TcpconnlatBpf::handleEvent(void *ctx, int cpu, void *data,
                                unsigned int data_sz) {
  auto self = static_cast<TcpconnlatBpf *>(ctx);
  if (data_sz < sizeof(struct event))
    return;
  auto e = static_cast<const struct event *>(data);
  self->processEvent(e);
}

void TcpconnlatBpf::handleLostEvents(void *ctx, int cpu,
                                     unsigned long long lost_cnt) {
  SPDLOG_WARN("lost {} events on CPU #{}", lost_cnt, cpu);
}

void TcpconnlatBpf::processEvent(const struct event *e) {
  char src[INET6_ADDRSTRLEN];
  char dst[INET6_ADDRSTRLEN];
  union {
    struct in_addr x4;
    struct in6_addr x6;
  } s, d;

  if (config_.print_timestamp) {
    if (start_ts_ == 0)
      start_ts_ = e->ts_us;
    fmt::print("{:<9.3f} ", (double)(e->ts_us - start_ts_) / 1000000.0);
  }

  if (e->af == AF_INET) {
    s.x4.s_addr = e->saddr_v4;
    d.x4.s_addr = e->daddr_v4;
  } else if (e->af == AF_INET6) {
    std::memcpy(&s.x6.s6_addr, e->saddr_v6, sizeof(s.x6.s6_addr));
    std::memcpy(&d.x6.s6_addr, e->daddr_v6, sizeof(d.x6.s6_addr));
  } else {
    SPDLOG_WARN("Broken event: event->af={}", e->af);
    return;
  }

  int family_val = (e->af == AF_INET) ? 4 : 6;
  const char* p_src = inet_ntop(e->af, &s, src, sizeof(src));
  const char* p_dst = inet_ntop(e->af, &d, dst, sizeof(dst));

  if (config_.lport) {
    fmt::print("{:<6} {:<12.12} {:<2} {:<16} {:<6} {:<16} {:<5} {:.2f}\n",
               e->tgid, e->comm, family_val, p_src ? p_src : "UNKNOWN", e->lport,
               p_dst ? p_dst : "UNKNOWN", ntohs(e->dport),
               (double)e->delta_us / 1000.0);
  } else {
    fmt::print("{:<6} {:<12.12} {:<2} {:<16} {:<16} {:<5} {:.2f}\n",
               e->tgid, e->comm, family_val, p_src ? p_src : "UNKNOWN",
               p_dst ? p_dst : "UNKNOWN", ntohs(e->dport),
               (double)e->delta_us / 1000.0);
  }
}

void TcpconnlatBpf::poll() {
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  pb_ = perf_buffer__new(bpf_map__fd(skel_->maps.events), 16, handleEvent,
                         handleLostEvents, this, nullptr);
  if (!pb_) {
    throw std::runtime_error(
        fmt::format("Failed to open perf buffer: {}", -errno));
  }

  if (config_.print_timestamp)
    fmt::print("{:<9} ", "TIME(s)");

  if (config_.lport) {
    fmt::print("{:<6} {:<12} {:<2} {:<16} {:<6} {:<16} {:<5} {}\n", "PID",
               "COMM", "IP", "SADDR", "LPORT", "DADDR", "DPORT", "LAT(ms)");
  } else {
    fmt::print("{:<6} {:<12} {:<2} {:<16} {:<16} {:<5} {}\n", "PID", "COMM",
               "IP", "SADDR", "DADDR", "DPORT", "LAT(ms)");
  }

  while (!g_exiting) {
    int err = perf_buffer__poll(pb_, 100);
    if (err < 0 && err != -EINTR) {
      throw std::runtime_error(
          fmt::format("Error polling perf buffer: {}", err));
    }
  }
}
