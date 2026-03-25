#include <ctime>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <cstring>
#include <csignal>
#include <fcntl.h>
#include <algorithm>
#include <vector>

#include <bpf/bpf.h>
#include <bpf/libbpf.h>

#include "TcptopBpf.h"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

extern "C" {
#include "trace_helpers.h"
}

extern "C" {
#include "tcptop.skel.h"
}

using namespace net::tcpTop;

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
#define OUTPUT_ROWS_LIMIT 10240

TcptopBpf::TcptopBpf(const ConfigArgs& config)
    : config_(config) {}

TcptopBpf::~TcptopBpf() {
    destroy();
}

void TcptopBpf::destroy() {
    if (cgfd_ != -1) {
        close(cgfd_);
        cgfd_ = -1;
    }
    if (skel_) {
        tcptop_bpf__destroy(skel_);
        skel_ = nullptr;
    }
}

void TcptopBpf::init() {
    skel_ = tcptop_bpf__open();
    if (!skel_) {
        throw std::runtime_error("Failed to open BPF skeleton");
    }
}

void TcptopBpf::load() {
    int family = -1;
    if (config_.ipv4_only) family = AF_INET;
    if (config_.ipv6_only) family = AF_INET6;

    skel_->rodata->target_pid = config_.target_pid;
    skel_->rodata->target_family = family;
    skel_->rodata->filter_cg = config_.cgroup_filtering;

    int err = tcptop_bpf__load(skel_);
    if (err) {
        destroy();
        throw std::runtime_error(fmt::format("Failed to load BPF skeleton: {}", err));
    }

    if (config_.cgroup_filtering) {
        int zero = 0;
        int cg_map_fd = bpf_map__fd(skel_->maps.cgroup_map);

        cgfd_ = open(config_.cgroup_path.c_str(), O_RDONLY);
        if (cgfd_ < 0) {
            throw std::runtime_error(fmt::format("Failed opening Cgroup path: {}", config_.cgroup_path));
        }

        if (bpf_map_update_elem(cg_map_fd, &zero, &cgfd_, BPF_ANY)) {
            throw std::runtime_error("Failed adding target cgroup to map");
        }
    }
}

void TcptopBpf::attach() {
    int err = tcptop_bpf__attach(skel_);
    if (err) {
        destroy();
        throw std::runtime_error(fmt::format("Failed to attach BPF programs: {}", err));
    }
}

int TcptopBpf::sortColumn(const void *obj1, const void *obj2, SortBy sort_by) {
    const auto* i1 = static_cast<const Info*>(obj1);
    const auto* i2 = static_cast<const Info*>(obj2);

    if (i1->key.family != i2->key.family)
        return i1->key.family - i2->key.family;

    if (sort_by == SortBy::SENT)
        return (i2->value.sent > i1->value.sent) ? 1 : -1;
    else if (sort_by == SortBy::RECEIVED)
        return (i2->value.received > i1->value.received) ? 1 : -1;
    else {
        size_t total1 = i1->value.sent + i1->value.received;
        size_t total2 = i2->value.sent + i2->value.received;
        return (total2 > total1) ? 1 : -1;
    }
}

void TcptopBpf::printStat() {
    char buf[256], ts[64];
    int fd = bpf_map__fd(skel_->maps.ip_map);
    std::vector<Info> infos;
    struct ip_key_t key, next_key;
    void* p_key = nullptr;

    if (!config_.no_summary) {
        if (str_loadavg(buf, sizeof(buf)) > 0 && str_timestamp("%H:%M:%S", ts, sizeof(ts)) > 0)
            fmt::print("{:>8} {}\n", ts, buf);
    }

    while (bpf_map_get_next_key(fd, p_key, &next_key) == 0) {
        struct traffic_t value;
        if (bpf_map_lookup_elem(fd, &next_key, &value) == 0) {
            infos.push_back({next_key, value});
        }
        key = next_key;
        p_key = &key;
    }

    int pid_max_fd = open("/proc/sys/kernel/pid_max", O_RDONLY);
    int pid_maxlen = 7;
    if (pid_max_fd >= 0) {
        char pbuf[32];
        ssize_t n = read(pid_max_fd, pbuf, sizeof(pbuf) - 1);
        if (n > 0) {
            pbuf[n] = '\0';
            pid_maxlen = static_cast<int>(n);
        }
        close(pid_max_fd);
    }
    if (pid_maxlen < 6) pid_maxlen = 6;

    fmt::print("{:<{}} {:<12} {:<21} {:<21} {:>6} {:>6}\n",
               "PID", pid_maxlen, "COMM", "LADDR", "RADDR", "RX_KB", "TX_KB");

    SortBy sb = config_.sort_by;
    std::sort(infos.begin(), infos.end(), [sb](const Info& a, const Info& b) {
        return sortColumn(&a, &b, sb) < 0;
    });

    uint32_t rows = std::min(static_cast<uint32_t>(infos.size()), config_.output_rows);
    bool ipv6_header_printed = false;

    for (uint32_t i = 0; i < rows; i++) {
        const auto& item = infos[i];
        int column_width = 21;
        if (item.key.family == AF_INET6) {
            column_width = 51;
            if (!ipv6_header_printed) {
                fmt::print("\n{:<{}} {:<12} {:<51} {:<51} {:>6} {:>6}\n",
                           "PID", pid_maxlen, "COMM", "LADDR6", "RADDR6", "RX_KB", "TX_KB");
                ipv6_header_printed = true;
            }
        }

        char saddr[INET6_ADDRSTRLEN], daddr[INET6_ADDRSTRLEN];
        inet_ntop(item.key.family, &item.key.saddr, saddr, sizeof(saddr));
        inet_ntop(item.key.family, &item.key.daddr, daddr, sizeof(daddr));

        std::string saddr_port = fmt::format("{}:{}", saddr, item.key.lport);
        std::string daddr_port = fmt::format("{}:{}", daddr, item.key.dport);

        fmt::print("{:<{}} {:<12.12} {:<{}} {:<{}} {:>6} {:>6}\n",
                   item.key.pid, pid_maxlen, item.key.name, 
                   saddr_port, column_width, daddr_port, column_width,
                   item.value.received / 1024, item.value.sent / 1024);
    }
    fmt::print("\n");

    // Cleanup map
    p_key = nullptr;
    while (bpf_map_get_next_key(fd, p_key, &next_key) == 0) {
        bpf_map_delete_elem(fd, &next_key);
        // Do not update p_key after delete, logic in original was a bit flawed or specialized.
        // Actually, bpf_map_get_next_key(fd, NULL, ...) gets the first.
        // If we delete the first, the next call with NULL might get the new first.
        // But the common way is to NOT delete while iterating next_key if possible, or use a specific pattern.
        // Original code used: prev_key = &key; after delete. This is strange because key was deleted.
        // I'll just use a safer way or follow original.
        // p_key = &key;
    }
    // Safer clear:
    p_key = nullptr;
    while (bpf_map_get_next_key(fd, p_key, &next_key) == 0) {
        bpf_map_delete_elem(fd, &next_key);
    }
}

void TcptopBpf::poll() {
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    while (!g_exiting && current_count_ < config_.count) {
        sleep(config_.interval);
        
        if (config_.clear_screen) {
            int ret = std::system("clear");
            (void)ret;
        }

        printStat();
        current_count_++;
    }
}
