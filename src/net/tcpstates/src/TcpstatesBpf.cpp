#include <ctime>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <cstring>
#include <csignal>
#include <fcntl.h>
#include <netinet/in.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "TcpstatesBpf.h"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

extern "C" {
#include "trace_helpers.h"
#include "btf_helpers.h"
}

extern "C" {
#include "tcpstates.skel.h"
}

using namespace net::tcpStates;

static volatile sig_atomic_t g_exiting = 0;

static void sig_handler(int signo) {
    g_exiting = 1;
}

#ifndef AF_INET
#define AF_INET		2
#endif
#ifndef AF_INET6
#define AF_INET6	10
#endif

static const char *tcp_states[] = {
    "UNKNOWN",      // 0
    "ESTABLISHED",  // 1
    "SYN_SENT",     // 2
    "SYN_RECV",     // 3
    "FIN_WAIT1",    // 4
    "FIN_WAIT2",    // 5
    "TIME_WAIT",    // 6
    "CLOSE",        // 7
    "CLOSE_WAIT",   // 8
    "LAST_ACK",     // 9
    "LISTEN",       // 10
    "CLOSING",      // 11
    "NEW_SYN_RECV", // 12
    "UNKNOWN"       // 13
};

TcpstatesBpf::TcpstatesBpf(const ConfigArgs& config)
    : config_(config) {}

TcpstatesBpf::~TcpstatesBpf() {
    destroy();
}

void TcpstatesBpf::destroy() {
    if (pb_) {
        perf_buffer__free(pb_);
        pb_ = nullptr;
    }
    if (skel_) {
        tcpstates_bpf__destroy(skel_);
        skel_ = nullptr;
    }
}

void TcpstatesBpf::init() {
    skel_ = tcpstates_bpf__open();
    if (!skel_) {
        throw std::runtime_error("Failed to open BPF skeleton");
    }
}

void TcpstatesBpf::load() {
    int family = 0;
    if (config_.ipv4_only) family = AF_INET;
    if (config_.ipv6_only) family = AF_INET6;

    skel_->rodata->target_family = family;
    skel_->rodata->filter_by_sport = !config_.target_sports.empty();
    skel_->rodata->filter_by_dport = !config_.target_dports.empty();

    int err = tcpstates_bpf__load(skel_);
    if (err) {
        destroy();
        throw std::runtime_error(fmt::format("Failed to load BPF skeleton: {}", err));
    }

    if (!config_.target_sports.empty()) {
        int port_map_fd = bpf_map__fd(skel_->maps.sports);
        for (uint16_t port : config_.target_sports) {
            bpf_map_update_elem(port_map_fd, &port, &port, BPF_ANY);
        }
    }

    if (!config_.target_dports.empty()) {
        int port_map_fd = bpf_map__fd(skel_->maps.dports);
        for (uint16_t port : config_.target_dports) {
            bpf_map_update_elem(port_map_fd, &port, &port, BPF_ANY);
        }
    }
}

void TcpstatesBpf::attach() {
    int err = tcpstates_bpf__attach(skel_);
    if (err) {
        destroy();
        throw std::runtime_error(fmt::format("Failed to attach BPF programs: {}", err));
    }
}

void TcpstatesBpf::handleEvent(void *ctx, int cpu, void *data, unsigned int data_sz) {
    auto self = static_cast<TcpstatesBpf*>(ctx);
    if (data_sz < sizeof(struct event)) return;
    auto e = static_cast<const struct event*>(data);
    self->processEvent(e);
}

void TcpstatesBpf::handleLostEvents(void *ctx, int cpu, unsigned long long lost_cnt) {
    SPDLOG_WARN("lost {} events on CPU #{}", lost_cnt, cpu);
}

void TcpstatesBpf::processEvent(const struct event *e) {
    char ts[32], saddr[INET6_ADDRSTRLEN], daddr[INET6_ADDRSTRLEN];

    if (config_.print_timestamp) {
        str_timestamp("%H:%M:%S", ts, sizeof(ts));
        fmt::print("{:>8} ", ts);
    }

    inet_ntop(e->family, &e->saddr, saddr, sizeof(saddr));
    inet_ntop(e->family, &e->daddr, daddr, sizeof(daddr));

    int family_val = (e->family == AF_INET) ? 4 : 6;
    
    const char *oldstate = (e->oldstate >= 1 && e->oldstate <= 13) ? tcp_states[e->oldstate] : "UNKNOWN";
    const char *newstate = (e->newstate >= 1 && e->newstate <= 13) ? tcp_states[e->newstate] : "UNKNOWN";

    if (config_.wide_output) {
        fmt::print("{:<16x} {:<7} {:<16.16} {:<2} {:<39} {:<5} {:<39} {:<5} {:<11} -> {:<11} {:.3f}\n",
                   e->skaddr, e->pid, e->task, family_val, saddr, e->sport, daddr, e->dport,
                   oldstate, newstate, (double)e->delta_us / 1000.0);
    } else {
        fmt::print("{:<16x} {:<7} {:<10.10} {:<15} {:<5} {:<15} {:<5} {:<11} -> {:<11} {:.3f}\n",
                   e->skaddr, e->pid, e->task, saddr, e->sport, daddr, e->dport,
                   oldstate, newstate, (double)e->delta_us / 1000.0);
    }
}

void TcpstatesBpf::poll() {
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    pb_ = perf_buffer__new(bpf_map__fd(skel_->maps.events), 16, handleEvent, handleLostEvents, this, nullptr);
    if (!pb_) {
        throw std::runtime_error(fmt::format("Failed to open perf buffer: {}", -errno));
    }

    if (config_.print_timestamp) {
        fmt::print("{:<8} ", "TIME(s)");
    }
    
    if (config_.wide_output) {
        fmt::print("{:<16} {:<7} {:<16} {:<2} {:<39} {:<5} {:<39} {:<5} {:<11} -> {:<11} {}\n",
                   "SKADDR", "PID", "COMM", "IP", "LADDR", "LPORT", "RADDR", "RPORT", "OLDSTATE", "NEWSTATE", "MS");
    } else {
        fmt::print("{:<16} {:<7} {:<10} {:<15} {:<5} {:<15} {:<5} {:<11} -> {:<11} {}\n",
                   "SKADDR", "PID", "COMM", "LADDR", "LPORT", "RADDR", "RPORT", "OLDSTATE", "NEWSTATE", "MS");
    }

    while (!g_exiting) {
        int err = perf_buffer__poll(pb_, 100);
        if (err < 0 && err != -EINTR) {
            throw std::runtime_error(fmt::format("Error polling perf buffer: {}", err));
        }
    }
}
