#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <ctime>
#include <unistd.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "TcpsynblBpf.h"
#include "fmt/format.h"
#include "spdlog/spdlog.h"

extern "C" {
#include "btf_helpers.h"
#include "trace_helpers.h"
}

extern "C" {
#include "tcpsynbl.skel.h"
}

using namespace net::tcpsynbl;

static volatile sig_atomic_t g_exiting = 0;

static void sig_handler(int signo) { g_exiting = 1; }

TcpsynblBpf::TcpsynblBpf(const ConfigArgs &config) : config_(config) {}

TcpsynblBpf::~TcpsynblBpf() { destroy(); }

void TcpsynblBpf::destroy() {
  if (skel_) {
    tcpsynbl_bpf__destroy(skel_);
    skel_ = nullptr;
  }
}

void TcpsynblBpf::init() {
  LIBBPF_OPTS(bpf_object_open_opts, open_opts);
  if (ensure_core_btf(&open_opts)) {
    SPDLOG_WARN("Failed to fetch necessary BTF for CO-RE, continuing anyway...");
  }
  skel_ = tcpsynbl_bpf__open_opts(&open_opts);
  if (!skel_) {
    cleanup_core_btf(&open_opts);
    throw std::runtime_error("Failed to open BPF skeleton");
  }
}

void TcpsynblBpf::disableAllProgs() {
  bpf_program__set_autoload(skel_->progs.tcp_v4_syn_recv_kprobe, false);
  bpf_program__set_autoload(skel_->progs.tcp_v6_syn_recv_kprobe, false);
  bpf_program__set_autoload(skel_->progs.tcp_v4_syn_recv, false);
  bpf_program__set_autoload(skel_->progs.tcp_v6_syn_recv, false);
}

void TcpsynblBpf::setAutoloadProg(int version) {
  if (version == 4) {
    if (fentry_can_attach("tcp_v4_syn_recv_sock", NULL))
      bpf_program__set_autoload(skel_->progs.tcp_v4_syn_recv, true);
    else
      bpf_program__set_autoload(skel_->progs.tcp_v4_syn_recv_kprobe, true);
  }
  if (version == 6) {
    if (fentry_can_attach("tcp_v6_syn_recv_sock", NULL))
      bpf_program__set_autoload(skel_->progs.tcp_v6_syn_recv, true);
    else
      bpf_program__set_autoload(skel_->progs.tcp_v6_syn_recv_kprobe, true);
  }
}

void TcpsynblBpf::load() {
  disableAllProgs();

  if (config_.ipv4_only) {
    setAutoloadProg(4);
  } else if (config_.ipv6_only) {
    setAutoloadProg(6);
  } else {
    setAutoloadProg(4);
    setAutoloadProg(6);
  }

  int err = tcpsynbl_bpf__load(skel_);
  if (err) {
    destroy();
    throw std::runtime_error(fmt::format("Failed to load BPF object: {}", err));
  }
}

void TcpsynblBpf::attach() {
  int err = tcpsynbl_bpf__attach(skel_);
  if (err) {
    destroy();
    throw std::runtime_error(fmt::format("Failed to attach BPF programs: {}", err));
  }

  map_fd_ = bpf_map__fd(skel_->maps.hists);
}

void TcpsynblBpf::printHistograms() {
  uint64_t lookup_key = static_cast<uint64_t>(-1), next_key;
  struct hist hist;

  // 遍历并打印所有桶
  while (!bpf_map_get_next_key(map_fd_, &lookup_key, &next_key)) {
    int err = bpf_map_lookup_elem(map_fd_, &next_key, &hist);
    if (err < 0) {
      SPDLOG_ERROR("failed to lookup hist: {}", err);
      return;
    }
    fmt::print("backlog_max = {}\n", next_key);
    print_log2_hist(hist.slots, MAX_SLOTS, "backlog");
    lookup_key = next_key;
  }

  // 清理 map
  lookup_key = static_cast<uint64_t>(-1);
  while (!bpf_map_get_next_key(map_fd_, &lookup_key, &next_key)) {
    bpf_map_delete_elem(map_fd_, &next_key);
    lookup_key = next_key;
  }
}

void TcpsynblBpf::run() {
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  char ts[32];

  fmt::print("Tracing SYN backlog size. Ctrl-C to end.\n");

  for (int times = config_.times; !g_exiting && times > 0; times--) {
    sleep(config_.interval);
    fmt::print("\n");

    if (config_.print_timestamp) {
      str_timestamp("%H:%M:%S", ts, sizeof(ts));
      fmt::print("{:<8}\n", ts);
    }

    printHistograms();

    if (g_exiting)
      break;
  }
}
