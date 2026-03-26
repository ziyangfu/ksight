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

#include "TcptracerBpf.h"
#include "fmt/format.h"
#include "spdlog/spdlog.h"

extern "C" {
#include "btf_helpers.h"
#include "trace_helpers.h"
}

extern "C" {
#include "tcptracer.skel.h"
}

using namespace net::tcptracer;

static volatile sig_atomic_t g_exiting = 0;

static void sig_handler(int signo) { g_exiting = 1; }

#ifndef AF_INET
#define AF_INET 2
#endif
#ifndef AF_INET6
#define AF_INET6 10
#endif

TcptracerBpf::TcptracerBpf(const ConfigArgs &config) : config_(config) {}

TcptracerBpf::~TcptracerBpf() { destroy(); }

void TcptracerBpf::destroy() {
  if (pb_) {
    perf_buffer__free(pb_);
    pb_ = nullptr;
  }
  if (skel_) {
    tcptracer_bpf__destroy(skel_);
    skel_ = nullptr;
  }
}

void TcptracerBpf::init() {
  LIBBPF_OPTS(bpf_object_open_opts, open_opts);
  if (ensure_core_btf(&open_opts)) {
    SPDLOG_WARN(
        "Failed to fetch necessary BTF for CO-RE, continuing anyway...");
  }
  skel_ = tcptracer_bpf__open_opts(&open_opts);
  if (!skel_) {
    cleanup_core_btf(&open_opts);
    throw std::runtime_error("Failed to open BPF skeleton");
  }
}

void TcptracerBpf::load() {
  if (config_.target_pid != 0) {
    skel_->rodata->filter_pid = config_.target_pid;
  }
  if (config_.target_uid != static_cast<uint32_t>(-1)) {
    skel_->rodata->filter_uid = config_.target_uid;
  }

  int err = tcptracer_bpf__load(skel_);
  if (err) {
    destroy();
    throw std::runtime_error(
        fmt::format("Failed to load BPF skeleton: {}", err));
  }
}

void TcptracerBpf::attach() {
  int err = tcptracer_bpf__attach(skel_);
  if (err) {
    destroy();
    throw std::runtime_error(
        fmt::format("Failed to attach BPF programs: {}", err));
  }
}

void TcptracerBpf::handleEvent(void *ctx, int cpu, void *data,
                                unsigned int data_sz) {
  auto self = static_cast<TcptracerBpf *>(ctx);
  if (data_sz < sizeof(struct event))
    return;
  auto e = static_cast<const struct event *>(data);
  self->processEvent(e);
}

void TcptracerBpf::handleLostEvents(void *ctx, int cpu,
                                     unsigned long long lost_cnt) {
  SPDLOG_WARN("lost {} events on CPU #{}", lost_cnt, cpu);
}

void TcptracerBpf::processEvent(const struct event *e) {
  char src[INET6_ADDRSTRLEN];
  char dst[INET6_ADDRSTRLEN];
  union {
    struct in_addr x4;
    struct in6_addr x6;
  } s, d;

  if (e->af == AF_INET) {
    s.x4.s_addr = e->saddr_v4;
    d.x4.s_addr = e->daddr_v4;
  } else if (e->af == AF_INET6) {
    std::memcpy(&s.x6.s6_addr, &e->saddr_v6, sizeof(s.x6.s6_addr));
    std::memcpy(&d.x6.s6_addr, &e->daddr_v6, sizeof(d.x6.s6_addr));
  } else {
    SPDLOG_WARN("Broken event: event.af={}", e->af);
    return;
  }

  if (config_.print_timestamp) {
    if (start_ts_ == 0)
      start_ts_ = e->ts_us;
    fmt::print("{:<9.3f} ", (double)(e->ts_us - start_ts_) / 1000000.0);
  }

  if (config_.print_uid) {
    fmt::print("{:<6} ", e->uid);
  }

  char type = '-';
  switch (e->type) {
  case TCP_EVENT_TYPE_CONNECT:
    type = 'C';
    break;
  case TCP_EVENT_TYPE_ACCEPT:
    type = 'A';
    break;
  case TCP_EVENT_TYPE_CLOSE:
    type = 'X';
    break;
  }

  const char* p_src = inet_ntop(e->af, &s, src, sizeof(src));
  const char* p_dst = inet_ntop(e->af, &d, dst, sizeof(dst));

  fmt::print("{} {:<6} {:<12.12} {:<2} {:<16} {:<16} {:<4} {:<4}\n",
             type, e->pid, e->task,
             e->af == AF_INET ? 4 : 6,
             p_src ? p_src : "UNKNOWN",
             p_dst ? p_dst : "UNKNOWN",
             ntohs(e->sport), ntohs(e->dport));
}

void TcptracerBpf::poll() {
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  pb_ = perf_buffer__new(bpf_map__fd(skel_->maps.events), 128, handleEvent,
                         handleLostEvents, this, nullptr);
  if (!pb_) {
    throw std::runtime_error(
        fmt::format("Failed to open perf buffer: {}", -errno));
  }

  if (config_.print_timestamp)
    fmt::print("{:<9} ", "TIME(s)");
  if (config_.print_uid)
    fmt::print("{:<6} ", "UID");
  
  fmt::print("{} {:<6} {:<12} {:<2} {:<16} {:<16} {:<4} {:<4}\n",
             "T", "PID", "COMM", "IP", "SADDR", "DADDR", "SPORT", "DPORT");

  while (!g_exiting) {
    int err = perf_buffer__poll(pb_, 100);
    if (err < 0 && err != -EINTR) {
      throw std::runtime_error(
          fmt::format("Error polling perf buffer: {}", err));
    }
  }
}
