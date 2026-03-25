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

#include "BindsnoopBpf.h"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

extern "C" {
#include "bindsnoop.skel.h"
}

using namespace net::bindSnoop;

static volatile sig_atomic_t g_exiting = 0;

static void sig_handler(int signo) {
    g_exiting = 1;
}

BindsnoopBpf::BindsnoopBpf(const ConfigArgs& config)
    : config_(config) {}

BindsnoopBpf::~BindsnoopBpf() {
    destroy();
}

void BindsnoopBpf::destroy() {
    if (pb_) {
        perf_buffer__free(pb_);
        pb_ = nullptr;
    }
    if (skel_) {
        bindsnoop_bpf__destroy(skel_);
        skel_ = nullptr;
    }
}

void BindsnoopBpf::init() {
    skel_ = bindsnoop_bpf__open();
    if (!skel_) {
        throw std::runtime_error("Failed to open BPF skeleton");
    }
}

void BindsnoopBpf::load() {
    // Set rodata
    skel_->rodata->filter_cg = !config_.cgroupspath.empty();
    skel_->rodata->target_pid = config_.target_pid;
    skel_->rodata->ignore_errors = config_.ignore_errors;
    skel_->rodata->filter_by_port = !config_.target_ports.empty();

    int err = bindsnoop_bpf__load(skel_);
    if (err) {
        destroy();
        throw std::runtime_error(fmt::format("Failed to load BPF skeleton: {}", err));
    }
}

void BindsnoopBpf::attach() {
    // Update cgroup map if needed
    if (!config_.cgroupspath.empty()) {
        int idx = 0;
        int cg_map_fd = bpf_map__fd(skel_->maps.cgroup_map);
        int cgfd = open(config_.cgroupspath.c_str(), O_RDONLY);
        if (cgfd < 0) {
            throw std::runtime_error(fmt::format("Failed to open cgroup path: {}", config_.cgroupspath));
        }
        if (bpf_map_update_elem(cg_map_fd, &idx, &cgfd, BPF_ANY)) {
            close(cgfd);
            throw std::runtime_error("Failed to update cgroup map");
        }
        close(cgfd);
    }

    // Update ports map if needed
    if (!config_.target_ports.empty()) {
        int port_map_fd = bpf_map__fd(skel_->maps.ports);
        char *ports_str = strdup(config_.target_ports.c_str());
        char *port = strtok(ports_str, ",");
        while (port) {
            uint16_t port_num = static_cast<uint16_t>(strtol(port, nullptr, 10));
            bpf_map_update_elem(port_map_fd, &port_num, &port_num, BPF_ANY);
            port = strtok(nullptr, ",");
        }
        free(ports_str);
    }

    int err = bindsnoop_bpf__attach(skel_);
    if (err) {
        destroy();
        throw std::runtime_error(fmt::format("Failed to attach BPF programs: {}", err));
    }
}

void BindsnoopBpf::handleEvent(void *ctx, int cpu, void *data, unsigned int data_sz) {
    auto self = static_cast<BindsnoopBpf*>(ctx);
    auto e = static_cast<const struct bind_event*>(data);
    self->processEvent(e);
}

void BindsnoopBpf::handleLostEvents(void *ctx, int cpu, unsigned long long lost_cnt) {
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

void BindsnoopBpf::processEvent(const struct bind_event *e) {
    char addr[INET6_ADDRSTRLEN];
    char opts_str[] = {'F', 'T', 'N', 'R', 'r', '\0'};
    const char *proto;

    if (e->proto == IPPROTO_TCP) proto = "TCP";
    else if (e->proto == IPPROTO_UDP) proto = "UDP";
    else proto = "UNK";

    for (int i = 0; i < 5; ++i) {
        if (!((1 << i) & e->opts)) {
            opts_str[i] = '.';
        }
    }

    if (e->ver == 4) {
        inet_ntop(AF_INET, e->addr, addr, sizeof(addr));
    } else {
        inet_ntop(AF_INET6, e->addr, addr, sizeof(addr));
    }

    if (config_.emit_timestamp) {
        fmt::print("{:<8} ", get_current_time_str());
    }

    fmt::print("{:<7} {:<16} {:<3} {:<5} {:<5} {:<4} {:<5} {:<48}\n",
               e->pid, e->task, e->ret, proto, opts_str, e->bound_dev_if, e->port, addr);
}

void BindsnoopBpf::poll() {
    pb_ = perf_buffer__new(bpf_map__fd(skel_->maps.events), 16, handleEvent, handleLostEvents, this, nullptr);
    if (!pb_) {
        throw std::runtime_error(fmt::format("Failed to open perf buffer: {}", -errno));
    }

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    if (config_.emit_timestamp) {
        fmt::print("{:<8} ", "TIME(s)");
    }
    fmt::print("{:<7} {:<16} {:<3} {:<5} {:<5} {:<4} {:<5} {:<48}\n",
               "PID", "COMM", "RET", "PROTO", "OPTS", "IF", "PORT", "ADDR");

    while (!g_exiting) {
        int err = perf_buffer__poll(pb_, 100);
        if (err < 0 && err != -EINTR) {
            throw std::runtime_error(fmt::format("Error polling perf buffer: {}", err));
        }
    }
}
