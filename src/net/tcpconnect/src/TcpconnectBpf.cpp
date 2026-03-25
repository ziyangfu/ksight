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

#include "TcpconnectBpf.h"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

extern "C" {
#include "map_helpers.h"
}

extern "C" {
#include "tcpconnect.skel.h"
}

using namespace net::tcpConnect;

static volatile sig_atomic_t g_exiting = 0;

static void sig_handler(int signo) {
    g_exiting = 1;
}

TcpconnectBpf::TcpconnectBpf(const ConfigArgs& config)
    : config_(config) {}

TcpconnectBpf::~TcpconnectBpf() {
    destroy();
}

void TcpconnectBpf::destroy() {
    if (pb_) {
        perf_buffer__free(pb_);
        pb_ = nullptr;
    }
    if (skel_) {
        tcpconnect_bpf__destroy(skel_);
        skel_ = nullptr;
    }
}

void TcpconnectBpf::init() {
    skel_ = tcpconnect_bpf__open();
    if (!skel_) {
        throw std::runtime_error("Failed to open BPF skeleton");
    }
}

void TcpconnectBpf::load() {
    skel_->rodata->do_count = config_.count;
    skel_->rodata->filter_pid = config_.target_pid;
    skel_->rodata->filter_uid = config_.target_uid;
    skel_->rodata->source_port = config_.source_port;

    if (!config_.target_ports.empty()) {
        int n = static_cast<int>(config_.target_ports.size());
        if (n > MAX_PORTS) n = MAX_PORTS;
        skel_->rodata->filter_ports_len = n;
        for (int i = 0; i < n; i++) {
            skel_->rodata->filter_ports[i] = htons(config_.target_ports[i]);
        }
    }

    int err = tcpconnect_bpf__load(skel_);
    if (err) {
        destroy();
        throw std::runtime_error(fmt::format("Failed to load BPF skeleton: {}", err));
    }
}

void TcpconnectBpf::attach() {
    int err = tcpconnect_bpf__attach(skel_);
    if (err) {
        destroy();
        throw std::runtime_error(fmt::format("Failed to attach BPF programs: {}", err));
    }
}

void TcpconnectBpf::handleEvent(void *ctx, int cpu, void *data, unsigned int data_sz) {
    auto self = static_cast<TcpconnectBpf*>(ctx);
    if (data_sz < sizeof(struct event)) return;
    auto e = static_cast<const struct event*>(data);
    self->processEvent(e);
}

void TcpconnectBpf::handleLostEvents(void *ctx, int cpu, unsigned long long lost_cnt) {
    SPDLOG_WARN("lost {} events on CPU #{}", lost_cnt, cpu);
}

void TcpconnectBpf::processEvent(const struct event *e) {
    char src[INET6_ADDRSTRLEN], dst[INET6_ADDRSTRLEN];
    
    if (e->af == AF_INET) {
        struct in_addr s, d;
        s.s_addr = e->saddr_v4;
        d.s_addr = e->daddr_v4;
        inet_ntop(AF_INET, &s, src, sizeof(src));
        inet_ntop(AF_INET, &d, dst, sizeof(dst));
    } else {
        inet_ntop(AF_INET6, e->saddr_v6, src, sizeof(src));
        inet_ntop(AF_INET6, e->daddr_v6, dst, sizeof(dst));
    }

    if (config_.print_timestamp) {
        if (start_ts_ == 0) start_ts_ = e->ts_us;
        fmt::print("{:<9.3f} ", (e->ts_us - start_ts_) / 1000000.0);
    }
    
    if (config_.print_uid) {
        fmt::print("{:<6} ", e->uid);
    }

    fmt::print("{:<6} {:<16.16} {:<2} {:<16} {:<16}",
               e->pid, e->task, (e->af == AF_INET ? 4 : 6), src, dst);
    
    if (config_.source_port) {
        fmt::print(" {:<5}", e->sport);
    }
    fmt::print(" {:<5}\n", ntohs(e->dport));
}

void TcpconnectBpf::printCountIpv4(int map_fd) {
    static struct ipv4_flow_key keys[MAX_ENTRIES];
    static uint64_t counts[MAX_ENTRIES];
    uint32_t n = MAX_ENTRIES;
    struct ipv4_flow_key zero = {};
    
    if (dump_hash(map_fd, keys, sizeof(keys[0]), counts, sizeof(counts[0]), &n, &zero, false) != 0) {
        SPDLOG_ERROR("Failed to dump IPv4 count map");
        return;
    }

    for (uint32_t i = 0; i < n; i++) {
        char s[INET_ADDRSTRLEN], d[INET_ADDRSTRLEN];
        struct in_addr src_addr = {keys[i].saddr};
        struct in_addr dst_addr = {keys[i].daddr};
        inet_ntop(AF_INET, &src_addr, s, sizeof(s));
        inet_ntop(AF_INET, &dst_addr, d, sizeof(d));

        fmt::print("{:<25} {:<25}", s, d);
        if (config_.source_port) {
            fmt::print(" {:<20}", keys[i].sport);
        }
        fmt::print(" {:<20} {:<10}\n", ntohs(keys[i].dport), counts[i]);
    }
}

void TcpconnectBpf::printCountIpv6(int map_fd) {
    static struct ipv6_flow_key keys[MAX_ENTRIES];
    static uint64_t counts[MAX_ENTRIES];
    uint32_t n = MAX_ENTRIES;
    struct ipv6_flow_key zero = {};

    if (dump_hash(map_fd, keys, sizeof(keys[0]), counts, sizeof(counts[0]), &n, &zero, false) != 0) {
        SPDLOG_ERROR("Failed to dump IPv6 count map");
        return;
    }

    for (uint32_t i = 0; i < n; i++) {
        char s[INET6_ADDRSTRLEN], d[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, keys[i].saddr, s, sizeof(s));
        inet_ntop(AF_INET6, keys[i].daddr, d, sizeof(d));

        fmt::print("{:<25} {:<25}", s, d);
        if (config_.source_port) {
            fmt::print(" {:<20}", keys[i].sport);
        }
        fmt::print(" {:<20} {:<10}\n", ntohs(keys[i].dport), counts[i]);
    }
}

void TcpconnectBpf::printCount() {
    fmt::print("\n{:<25} {:<25}", "LADDR", "RADDR");
    if (config_.source_port) {
        fmt::print(" {:<20}", "LPORT");
    }
    fmt::print(" {:<20} {:<10}\n", "RPORT", "CONNECTS");

    printCountIpv4(bpf_map__fd(skel_->maps.ipv4_count));
    printCountIpv6(bpf_map__fd(skel_->maps.ipv6_count));
}

void TcpconnectBpf::poll() {
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    if (config_.count) {
        SPDLOG_INFO("Counting. Press Ctrl+C to print results.");
        while (!g_exiting) pause();
        printCount();
        return;
    }

    pb_ = perf_buffer__new(bpf_map__fd(skel_->maps.events), 128, handleEvent, handleLostEvents, this, nullptr);
    if (!pb_) {
        throw std::runtime_error(fmt::format("Failed to open perf buffer: {}", -errno));
    }

    if (config_.print_timestamp) fmt::print("{:<9}", "TIME(s)");
    if (config_.print_uid) fmt::print("{:<6}", "UID");
    fmt::print("{:<6} {:<16} {:<2} {:<16} {:<16}", "PID", "COMM", "IP", "SADDR", "DADDR");
    if (config_.source_port) fmt::print(" {:<5}", "SPORT");
    fmt::print(" {:<5}\n", "DPORT");

    while (!g_exiting) {
        int err = perf_buffer__poll(pb_, 100);
        if (err < 0 && err != -EINTR) {
            throw std::runtime_error(fmt::format("Error polling perf buffer: {}", err));
        }
    }
}
