#include <ctime>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <cstring>
#include <csignal>
#include <iostream>
#include <fcntl.h>
#include <sys/socket.h>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "SolistenBpf.h"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

extern "C" {
#include "trace_helpers.h"
}

extern "C" {
#include "solisten.skel.h"
}

using namespace net::soListen;

static volatile sig_atomic_t g_exiting = 0;

static void sig_handler(int signo) {
    g_exiting = 1;
}

SolistenBpf::SolistenBpf(const ConfigArgs& config)
    : config_(config) {}

SolistenBpf::~SolistenBpf() {
    destroy();
}

void SolistenBpf::destroy() {
    if (pb_) {
        perf_buffer__free(pb_);
        pb_ = nullptr;
    }
    if (skel_) {
        solisten_bpf__destroy(skel_);
        skel_ = nullptr;
    }
}

void SolistenBpf::init() {
    skel_ = solisten_bpf__open();
    if (!skel_) {
        throw std::runtime_error("Failed to open BPF skeleton");
    }

    if (fentry_can_attach("inet_listen", nullptr)) {
        bpf_program__set_autoload(skel_->progs.inet_listen_entry, false);
        bpf_program__set_autoload(skel_->progs.inet_listen_exit, false);
    } else {
        bpf_program__set_autoload(skel_->progs.inet_listen_fexit, false);
    }
}

void SolistenBpf::load() {
    skel_->rodata->target_pid = config_.target_pid;

    int err = solisten_bpf__load(skel_);
    if (err) {
        destroy();
        throw std::runtime_error(fmt::format("Failed to load BPF skeleton: {}", err));
    }
}

void SolistenBpf::attach() {
    int err = solisten_bpf__attach(skel_);
    if (err) {
        destroy();
        throw std::runtime_error(fmt::format("Failed to attach BPF programs: {}", err));
    }
}

void SolistenBpf::handleEvent(void *ctx, int cpu, void *data, unsigned int data_sz) {
    auto self = static_cast<SolistenBpf*>(ctx);
    if (data_sz < sizeof(struct event)) return;
    auto e = static_cast<const struct event*>(data);
    self->processEvent(e);
}

void SolistenBpf::handleLostEvents(void *ctx, int cpu, unsigned long long lost_cnt) {
    SPDLOG_WARN("lost {} events on CPU #{}", lost_cnt, cpu);
}

static std::string get_current_time_str() {
    time_t now = time(nullptr);
    struct tm ts;
    localtime_r(&now, &ts);
    char buf[32];
    strftime(buf, sizeof(buf), "%H:%M:%S", &ts);
    return std::string(buf);
}

void SolistenBpf::processEvent(const struct event *e) {
    char proto[16], addr[48] = {};
    uint16_t family = e->proto >> 16;
    uint16_t type = static_cast<uint16_t>(e->proto);
    const char *prot_base;

    if (type == SOCK_STREAM) prot_base = "TCP";
    else if (type == SOCK_DGRAM) prot_base = "UDP";
    else prot_base = "UNK";

    snprintf(proto, sizeof(proto), "%sv%d", prot_base, (family == AF_INET ? 4 : 6));
    inet_ntop(family, e->addr, addr, sizeof(addr));

    if (config_.emit_timestamp) {
        fmt::print("{:<8} ", get_current_time_str());
    }

    fmt::print("{:<7} {:<16} {:<3} {:<7} {:<5} {:<5} {:<32}\n",
               e->pid, e->task, e->ret, e->backlog, proto, e->port, addr);
}

void SolistenBpf::poll() {
    pb_ = perf_buffer__new(bpf_map__fd(skel_->maps.events), 16, handleEvent, handleLostEvents, this, nullptr);
    if (!pb_) {
        throw std::runtime_error(fmt::format("Failed to open perf buffer: {}", -errno));
    }

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    if (config_.emit_timestamp) {
        fmt::print("{:<8} ", "TIME(s)");
    }
    fmt::print("{:<7} {:<16} {:<3} {:<7} {:<5} {:<5} {:<32}\n",
               "PID", "COMM", "RET", "BACKLOG", "PROTO", "PORT", "ADDR");

    while (!g_exiting) {
        int err = perf_buffer__poll(pb_, 100);
        if (err < 0 && err != -EINTR) {
            throw std::runtime_error(fmt::format("Error polling perf buffer: {}", err));
        }
    }
}
