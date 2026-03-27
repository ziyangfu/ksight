#include <arpa/inet.h>
#include <csignal>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include <sys/socket.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "TcpretransBpf.h"
#include "fmt/format.h"
#include "spdlog/spdlog.h"

extern "C" {
#include "trace_helpers.h"
#include "btf_helpers.h"
}

extern "C" {
#include "tcpretrans.skel.h"
}

using namespace net::tcpretrans;

static volatile sig_atomic_t g_exiting = 0;

static void sig_handler(int signo) { g_exiting = 1; }

static const char *tcp_state_to_str(int state) {
    switch (state) {
        case 1:  return "ESTABLISHED";
        case 2:  return "SYN_SENT";
        case 3:  return "SYN_RECV";
        case 4:  return "FIN_WAIT1";
        case 5:  return "FIN_WAIT2";
        case 6:  case 12: return "TIME_WAIT";
        case 7:  return "CLOSE";
        case 8:  return "CLOSE_WAIT";
        case 9:  return "LAST_ACK";
        case 10: return "LISTEN";
        case 11: return "CLOSING";
        default: return "UNKNOWN";
    }
}

TcpretransBpf::TcpretransBpf(const ConfigArgs &config) : config_(config) {}

TcpretransBpf::~TcpretransBpf() { destroy(); }

void TcpretransBpf::destroy() {
  if (pb_) {
    perf_buffer__free(pb_);
    pb_ = nullptr;
  }
  if (skel_) {
    tcpretrans_bpf__destroy(skel_);
    skel_ = nullptr;
  }
}

void TcpretransBpf::init() {
  LIBBPF_OPTS(bpf_object_open_opts, open_opts);
  if (ensure_core_btf(&open_opts)) {
    SPDLOG_WARN("Failed to fetch necessary BTF for CO-RE, continuing anyway...");
  }
  skel_ = tcpretrans_bpf__open_opts(&open_opts);
  if (!skel_) {
    cleanup_core_btf(&open_opts);
    throw std::runtime_error("Failed to open BPF skeleton");
  }
}

void TcpretransBpf::load() {
  skel_->rodata->targ_lossprobe = config_.lossprobe;
  skel_->rodata->targ_count = config_.count;
  if (config_.ipv4) skel_->rodata->targ_family = AF_INET;
  else if (config_.ipv6) skel_->rodata->targ_family = AF_INET6;
  else skel_->rodata->targ_family = 0;

  int err = tcpretrans_bpf__load(skel_);
  if (err) {
    destroy();
    throw std::runtime_error(fmt::format("Failed to load BPF object: {}", err));
  }
}

void TcpretransBpf::attach() {
    bool has_tp = tracepoint_exists("tcp", "tcp_retransmit_skb");

    if (has_tp) {
        bpf_program__set_autoload(skel_->progs.tcp_retransmit_skb_kp, false);
    } else {
        bpf_program__set_autoload(skel_->progs.tcp_retransmit_skb_tp, false);
        SPDLOG_INFO("Kernel does not have tcp_retransmit_skb tracepoint, using kprobe fallback.");
    }

    if (!config_.lossprobe) {
        bpf_program__set_autoload(skel_->progs.tcp_send_loss_probe_kp, false);
    }

    int err = tcpretrans_bpf__attach(skel_);
    if (err) {
        destroy();
        throw std::runtime_error(fmt::format("Failed to attach BPF programs: {}", err));
    }
}

void TcpretransBpf::handleEvent(void *ctx, int cpu, void *data, unsigned int data_sz) {
  auto self = static_cast<TcpretransBpf *>(ctx);
  if (data_sz < sizeof(struct event))
    return;
  auto e = static_cast<const struct event *>(data);
  self->processEvent(e);
}

void TcpretransBpf::handleLostEvents(void *ctx, int cpu, unsigned long long lost_cnt) {
  SPDLOG_WARN("lost {} events on CPU #{}", lost_cnt, cpu);
}

void TcpretransBpf::processEvent(const struct event *e) {
  char saddr[INET6_ADDRSTRLEN], daddr[INET6_ADDRSTRLEN];
  char time_str[16];
  time_t now = time(nullptr);
  struct tm *t = localtime(&now);
  strftime(time_str, sizeof(time_str), "%H:%M:%S", t);

  inet_ntop(e->family, e->saddr, saddr, sizeof(saddr));
  inet_ntop(e->family, e->daddr, daddr, sizeof(daddr));

  const char type_char = (e->type == RETRANSMIT) ? 'R' : 'L';
  const char *state_str = tcp_state_to_str(e->state);

  std::string laddr_port = fmt::format("{}:{}", saddr, e->lport);
  std::string raddr_port = fmt::format("{}:{}", daddr, e->dport);

  fmt::print("{:<8} {:<7} {:<2} {:<20} {} {:<20} {:<12}",
             time_str, e->pid, e->ip, laddr_port, type_char, raddr_port, state_str);
  
  if (config_.sequence) {
      fmt::print(" {:<10}", e->seq);
  }
  fmt::print("\n");
}

void net::tcpretrans::TcpretransBpf::printCounts() {
    int fd = bpf_map__fd(skel_->maps.counts);
    struct flow_key key = {}, next_key;
    uint64_t value;

    fmt::print("\n{:<25} {:<25} {:<10}\n", "LADDR:LPORT", "RADDR:RPORT", "RETRANSMITS");

    while (bpf_map_get_next_key(fd, &key, &next_key) == 0) {
        if (bpf_map_lookup_elem(fd, &next_key, &value) == 0) {
            char saddr[INET6_ADDRSTRLEN], daddr[INET6_ADDRSTRLEN];
            inet_ntop(next_key.family, next_key.saddr, saddr, sizeof(saddr));
            inet_ntop(next_key.family, next_key.daddr, daddr, sizeof(daddr));

            std::string laddr_port = fmt::format("{}:{}", saddr, next_key.lport);
            std::string raddr_port = fmt::format("{}:{}", daddr, next_key.dport);

            fmt::print("{:<25} {:<25} {:<10}\n", laddr_port, raddr_port, value);
        }
        key = next_key;
    }
}

void TcpretransBpf::poll() {
  signal(SIGINT, sig_handler);
  signal(SIGTERM, sig_handler);

  if (!config_.count) {
    pb_ = perf_buffer__new(bpf_map__fd(skel_->maps.events), 64,
                           handleEvent, handleLostEvents, this, nullptr);
    if (!pb_) {
      throw std::runtime_error(fmt::format("Failed to open perf buffer: {}", -errno));
    }

    fmt::print("{:<8} {:<7} {:<2} {:<20} {:<2} {:<20} {:<12}",
               "TIME", "PID", "IP", "LADDR:LPORT", "T", "RADDR:RPORT", "STATE");
    if (config_.sequence) {
        fmt::print(" {:<10}", "SEQ");
    }
    fmt::print("\n");
  } else {
      fmt::print("Tracing retransmits ... Hit Ctrl-C to end\n");
  }

  while (!g_exiting) {
    if (!config_.count) {
        int err = perf_buffer__poll(pb_, 100);
        if (err < 0 && err != -EINTR) {
          throw std::runtime_error(fmt::format("Error polling perf buffer: {}", err));
        }
    } else {
        sleep(1);
    }
  }

  if (config_.count) {
      printCounts();
  }
}
