#include <ctime>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <cstring>
#include <csignal>
#include <iostream>
#include <fcntl.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "TcprttBpf.h"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

extern "C" {
#include "trace_helpers.h"
}

extern "C" {
#include "tcprtt.skel.h"
}

using namespace net::tcpRtt;

static volatile sig_atomic_t g_exiting = 0;

static void sig_handler(int signo) {
    g_exiting = 1;
}

TcprttBpf::TcprttBpf(const ConfigArgs& config)
    : config_(config) {}

TcprttBpf::~TcprttBpf() {
    destroy();
}

void TcprttBpf::destroy() {
    if (skel_) {
        tcprtt_bpf__destroy(skel_);
        skel_ = nullptr;
    }
}

void TcprttBpf::init() {
    skel_ = tcprtt_bpf__open();
    if (!skel_) {
        throw std::runtime_error("Failed to open BPF skeleton");
    }

    skel_->rodata->targ_laddr_hist = config_.laddr_hist;
    skel_->rodata->targ_raddr_hist = config_.raddr_hist;
    skel_->rodata->targ_show_ext = config_.extended;
    skel_->rodata->targ_sport = htons(config_.lport);
    skel_->rodata->targ_dport = htons(config_.rport);
    skel_->rodata->targ_saddr = config_.laddr;
    skel_->rodata->targ_daddr = config_.raddr;
    memcpy(skel_->rodata->targ_saddr_v6, config_.laddr_v6, sizeof(skel_->rodata->targ_saddr_v6));
    memcpy(skel_->rodata->targ_daddr_v6, config_.raddr_v6, sizeof(skel_->rodata->targ_daddr_v6));
    skel_->rodata->targ_ms = config_.milliseconds;

    if (fentry_can_attach("tcp_rcv_established", nullptr)) {
        bpf_program__set_autoload(skel_->progs.tcp_rcv_kprobe, false);
    } else {
        bpf_program__set_autoload(skel_->progs.tcp_rcv, false);
    }
}

void TcprttBpf::load() {
    int err = tcprtt_bpf__load(skel_);
    if (err) {
        destroy();
        throw std::runtime_error(fmt::format("Failed to load BPF skeleton: {}", err));
    }
}

void TcprttBpf::attach() {
    int err = tcprtt_bpf__attach(skel_);
    if (err) {
        destroy();
        throw std::runtime_error(fmt::format("Failed to attach BPF programs: {}", err));
    }
}

void TcprttBpf::printMap(struct bpf_map *map) {
    const char *units = config_.milliseconds ? "msecs" : "usecs";
    struct hist_key lookup_key = {};
    struct hist_key next_key;
    int fd = bpf_map__fd(map);
    struct hist hist;
    
    // First element
    void* p_lookup_key = nullptr;

    while (!bpf_map_get_next_key(fd, p_lookup_key, &next_key)) {
        int err = bpf_map_lookup_elem(fd, &next_key, &hist);
        if (err < 0) {
            SPDLOG_ERROR("failed to lookup infos: {}", err);
            return;
        }

        if (config_.laddr_hist)
            fmt::print("Local Address = ");
        else if (config_.raddr_hist)
            fmt::print("Remote Address = ");
        else
            fmt::print("All Addresses = ****** ");

        if (config_.laddr_hist || config_.raddr_hist) {
            char str[INET6_ADDRSTRLEN];
            if (!inet_ntop(next_key.family, next_key.addr, str, sizeof(str))) {
                SPDLOG_ERROR("converting IP to string failed");
                return;
            }
            fmt::print("{} ", str);
        }

        if (config_.extended && hist.cnt > 0)
            fmt::print("[AVG {}] ", hist.latency / hist.cnt);
        fmt::print("\n");
        
        print_log2_hist(hist.slots, MAX_SLOTS, units);
        
        lookup_key = next_key;
        p_lookup_key = &lookup_key;
    }

    // Cleanup map after printing
    p_lookup_key = nullptr;
    while (!bpf_map_get_next_key(fd, p_lookup_key, &next_key)) {
        bpf_map_delete_elem(fd, &next_key);
        lookup_key = next_key;
        p_lookup_key = &lookup_key;
    }
}

void TcprttBpf::poll() {
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    fmt::print("Tracing TCP RTT");
    if (config_.duration)
        fmt::print(" for {} secs.\n", config_.duration);
    else
        fmt::print("... Hit Ctrl-C to end.\n");

    uint64_t time_end = 0;
    if (config_.duration)
        time_end = get_ktime_ns() + (uint64_t)config_.duration * NSEC_PER_SEC;

    while (!g_exiting) {
        sleep(config_.interval);
        fmt::print("\n");

        if (config_.timestamp) {
            time_t now = time(nullptr);
            struct tm ts;
            localtime_r(&now, &ts);
            char buf[32];
            strftime(buf, sizeof(buf), "%H:%M:%S", &ts);
            fmt::print("{}\n", buf);
        }

        printMap(skel_->maps.hists);

        if (config_.duration && get_ktime_ns() > time_end)
            break;
    }
}
