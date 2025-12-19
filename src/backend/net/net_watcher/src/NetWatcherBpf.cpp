#include <iostream>
#include <iomanip>
#include <ctime>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>
#include <filesystem>
#include <fstream>
#include <vector>
#include <algorithm>
#include <cstring>

#include "NetWatcherBpf.h"
#include "net_watcher/include/dropreason.h"
#include "spdlog/spdlog.h"
#include "fmt/format.h"
#include "fmt/color.h"

using namespace net::netWatcher;
namespace fs = std::filesystem;

static const char *tcp_states[] = {
    [1] = "ESTABLISHED", [2] = "SYN_SENT",   [3] = "SYN_RECV",
    [4] = "FIN_WAIT1",   [5] = "FIN_WAIT2",  [6] = "TIME_WAIT",
    [7] = "CLOSE",       [8] = "CLOSE_WAIT", [9] = "LAST_ACK",
    [10] = "LISTEN",     [11] = "CLOSING",   [12] = "NEW_SYN_RECV",
    [13] = "UNKNOWN",
};

struct SymbolEntry {
    unsigned long addr;
    char name[64];
};

static std::vector<SymbolEntry> symbols;
static bool symbols_loaded = false;

static void readallsym() {
    if (symbols_loaded) return;
    std::ifstream file("/proc/kallsyms");
    if (!file.is_open()) {
        SPDLOG_ERROR("Error opening /proc/kallsyms");
        return;
    }
    std::string line;
    while (std::getline(file, line)) {
        unsigned long addr;
        char type;
        char name[64];
        if (sscanf(line.c_str(), "%lx %c %63s", &addr, &type, name) == 3) {
            symbols.push_back({addr, ""});
            strncpy(symbols.back().name, name, 63);
        }
    }
    std::sort(symbols.begin(), symbols.end(), [](const SymbolEntry& a, const SymbolEntry& b) {
        return a.addr < b.addr;
    });
    symbols_loaded = true;
}

static SymbolEntry findfunc(unsigned long addr) {
    if (!symbols_loaded) readallsym();
    auto it = std::upper_bound(symbols.begin(), symbols.end(), addr, [](unsigned long val, const SymbolEntry& e) {
        return val < e.addr;
    });
    if (it == symbols.begin()) return {0, "unknown"};
    return *(--it);
}

// EWMA logic
#define ALPHA 0.2
#define GRANULARITY 3
static float ewma_values[10] = {0};
static int count_values[10] = {0};

static float calculate_ewma(float new_value, float old_ewma) {
    return ALPHA * new_value + (1 - ALPHA) * old_ewma;
}

static bool process_delay(float layer_delay, int layer_index) {
    if (layer_delay == 0) return false;
    count_values[layer_index]++;
    if (ewma_values[layer_index] == 0) {
        ewma_values[layer_index] = layer_delay;
        return false;
    }
    ewma_values[layer_index] = calculate_ewma(layer_delay, ewma_values[layer_index]);
    float threshold = ewma_values[layer_index] * GRANULARITY;
    if (count_values[layer_index] > 30) {
        return layer_delay > threshold;
    }
    return false;
}

// Static callbacks
static int print_packet(void *ctx, void *data, size_t len) {
    auto self = static_cast<NetWatcherBpf*>(ctx);
    auto pack_info = static_cast<struct pack_t*>(data);
    if (self->getMonitorMode() != NetWatcherBpf::MonitorMode::MODE_DEFAULT) return 0;
    char s_str[INET_ADDRSTRLEN], d_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &pack_info->saddr, s_str, sizeof(s_str));
    inet_ntop(AF_INET, &pack_info->daddr, d_str, sizeof(d_str));
    std::string http_data = "-";
    if (strstr((char*)pack_info->data, "HTTP/1")) {
        char* r = strchr((char*)pack_info->data, '\r');
        if (r) http_data.assign((char*)pack_info->data, r - (char*)pack_info->data);
        else http_data.assign((char*)pack_info->data, sizeof(pack_info->data));
    }
    fmt::print("{:<22p} {:<20} {:<8} {:<20} {:<8} {:<15} {:<15} {:<15} {:<10} {:<15}\n",
               pack_info->sock, s_str, pack_info->sport, d_str, pack_info->dport,
               pack_info->mac_time, pack_info->ip_time, pack_info->tran_time,
               pack_info->rx, http_data);
    return 0;
}

static int print_udp(void *ctx, void *data, size_t len) {
    auto pack_info = static_cast<struct udp_message*>(data);
    char s_str[INET_ADDRSTRLEN], d_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &pack_info->saddr, s_str, sizeof(s_str));
    inet_ntop(AF_INET, &pack_info->daddr, d_str, sizeof(d_str));
    fmt::print("{:<20} {:<20} {:<10} {:<10} {:<12} {:<10} {:<10}\n",
               s_str, d_str, pack_info->sport, pack_info->dport,
               pack_info->tran_time, pack_info->rx, pack_info->len);
    return 0;
}

static int print_netfilter(void *ctx, void *data, size_t len) {
    auto pack_info = static_cast<struct netfilter*>(data);
    char s_str[INET_ADDRSTRLEN], d_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &pack_info->saddr, s_str, sizeof(s_str));
    inet_ntop(AF_INET, &pack_info->daddr, d_str, sizeof(d_str));
    fmt::print("{:<20} {:<20} {:<10} {:<10} {:<10} {:<10} {:<10} {:<10} {:<10} {:<10}\n",
               s_str, d_str, pack_info->sport, pack_info->dport,
               pack_info->pre_routing_time, pack_info->local_input_time,
               pack_info->forward_time, pack_info->post_routing_time,
               pack_info->local_out_time, pack_info->rx);
    return 0;
}

static int print_kfree(void *ctx, void *data, size_t len) {
    auto pack_info = static_cast<struct reasonissue*>(data);
    char s_str[INET_ADDRSTRLEN], d_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &pack_info->saddr, s_str, sizeof(s_str));
    inet_ntop(AF_INET, &pack_info->daddr, d_str, sizeof(d_str));
    time_t now = time(nullptr);
    struct tm *lt = localtime(&now);
    std::string prot = (pack_info->protocol == 2048) ? "ipv4" : (pack_info->protocol == 34525 ? "ipv6" : "other");
    auto sym = findfunc(pack_info->location);
    std::string loc = fmt::format("{}+0x{:lx}", sym.name, pack_info->location - sym.addr);
    fmt::print("{:02}:{:02}:{:02} {:<17} {:<17} {:<10} {:<10} {:<9} {:<33} {:<30}\n",
               lt->tm_hour, lt->tm_min, lt->tm_sec, s_str, d_str,
               pack_info->sport, pack_info->dport, prot, loc,
               SKB_Drop_Reason_Strings[pack_info->drop_reason]);
    return 0;
}

static int print_tcpstate(void *ctx, void *data, size_t len) {
    auto pack_info = static_cast<struct tcp_state*>(data);
    char s_str[INET_ADDRSTRLEN], d_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &pack_info->saddr, s_str, sizeof(s_str));
    inet_ntop(AF_INET, &pack_info->daddr, d_str, sizeof(d_str));
    fmt::print("{:<20} {:<20} {:<10} {:<10} {:<15} {:<15} {:<10}\n",
               s_str, d_str, pack_info->sport, pack_info->dport,
               tcp_states[pack_info->oldstate], tcp_states[pack_info->newstate],
               pack_info->time);
    return 0;
}

static int print_icmptime(void *ctx, void *data, size_t len) {
    auto pack_info = static_cast<struct icmptime*>(data);
    char s_str[INET_ADDRSTRLEN], d_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &pack_info->saddr, s_str, sizeof(s_str));
    inet_ntop(AF_INET, &pack_info->daddr, d_str, sizeof(d_str));
    fmt::print("{:<20} {:<20} {:<20} {:<20}\n", s_str, d_str, pack_info->icmp_tran_time, pack_info->flag);
    return 0;
}

static int print_dns(void *ctx, void *data, size_t len) {
    auto pack_info = static_cast<struct dns_information*>(data);
    char s_str[INET_ADDRSTRLEN], d_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &pack_info->saddr, s_str, sizeof(s_str));
    inet_ntop(AF_INET, &pack_info->daddr, d_str, sizeof(d_str));
    fmt::print("{:<20} {:<20} {:<10} {:<10} {:<5} {:<5} {:<5} {:<5} {:<40} {:<5} {:<5} {:<5}\n",
               s_str, d_str, pack_info->id, pack_info->flags, pack_info->qdcount,
               pack_info->ancount, pack_info->nscount, pack_info->arcount,
               "domain", pack_info->request_count, pack_info->response_count, pack_info->rx);
    return 0;
}

static int print_trace(void *ctx, void *data, size_t len) {
    auto event = static_cast<struct stacktrace_event*>(data);
    fmt::print("COMM: {} (pid={}) @ CPU {}\n", event->comm, event->pid, event->cpu_id);
    return 0;
}

static int print_mysql(void *ctx, void *data, size_t len) {
    auto pack_info = static_cast<struct mysql_query*>(data);
    fmt::print("{:<10} {:<10} {:<15} {:<10} {:<40} {:<15} {:<10}\n",
               pack_info->pid, pack_info->tid, pack_info->comm, pack_info->size,
               pack_info->msql, pack_info->duratime, pack_info->count);
    return 0;
}

static int print_redis(void *ctx, void *data, size_t len) {
    auto pack_info = static_cast<struct redis_query*>(data);
    fmt::print("{:<10} {:<15} {:<10} {:<20} {:<15}\n",
               pack_info->pid, pack_info->comm, pack_info->argc, "redis_cmd", pack_info->duratime);
    return 0;
}

static int print_rtt(void *ctx, void *data, size_t len) {
    auto rtt_tuple = static_cast<struct RTT*>(data);
    char s_str[INET_ADDRSTRLEN], d_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &rtt_tuple->saddr, s_str, sizeof(s_str));
    inet_ntop(AF_INET, &rtt_tuple->daddr, d_str, sizeof(d_str));
    fmt::print("Source: {}, Dest: {}, Avg RTT: {} ms\n", s_str, d_str, (double)rtt_tuple->latency / rtt_tuple->cnt / 1000.0);
    return 0;
}

static int print_rst(void *ctx, void *data, size_t len) {
    auto event = static_cast<struct reset_event_t*>(data);
    char s_str[INET_ADDRSTRLEN], d_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &event->saddr, s_str, sizeof(s_str));
    inet_ntop(AF_INET, &event->daddr, d_str, sizeof(d_str));
    fmt::print("{:<10} {:<15} {:<20} {:<20} {:<10} {:<10} {:<20}\n",
               event->pid, event->comm, s_str, d_str, event->sport, event->dport, event->timestamp);
    return 0;
}

NetWatcherBpf::NetWatcherBpf(ConfigArgs& config)
    : config_(config), skel_(nullptr), rb_(nullptr) {}

NetWatcherBpf::~NetWatcherBpf() {
    destroy();
}

void NetWatcherBpf::open() {
    skel_ = netwatcher_bpf__open();
    if (!skel_) SPDLOG_ERROR("Failed to open BPF skeleton");
}

void NetWatcherBpf::load() {
    int err = netwatcher_bpf__load(skel_);
    if (err) {
        SPDLOG_ERROR("Failed to load BPF skeleton: {}", err);
        destroy();
    }
}

void NetWatcherBpf::attach() {
    int err = netwatcher_bpf__attach(skel_);
    if (err) {
        SPDLOG_ERROR("Failed to attach BPF skeleton: {}", err);
        destroy();
        return;
    }

    if (config_.mysql_info) {
        attachUprobeMysql();
    }
    if (config_.redis_info) {
        attachUprobeRedis();
    }
}

void NetWatcherBpf::attachUprobeMysql() {
    // Note: binary_path should be configurable or detected. 
    // For now, we assume it's in a standard location or the user provides it.
    // The original code used a global binary_path[64] = ""; which is not very helpful.
    // Usually it's /usr/sbin/mysqld
    const char* mysql_path = "/usr/sbin/mysqld"; 
    if (!fs::exists(mysql_path)) return;

    LIBBPF_OPTS(bpf_uprobe_opts, uprobe_opts, 
                .func_name = "_Z16dispatch_commandP3THDPK8COM_DATA19enum_server_command",
                .retprobe = false);
    skel_->links.query__start = bpf_program__attach_uprobe_opts(
        skel_->progs.query__start, -1, mysql_path, 0, &uprobe_opts);
    
    uprobe_opts.retprobe = true;
    skel_->links.query__end = bpf_program__attach_uprobe_opts(
        skel_->progs.query__end, -1, mysql_path, 0, &uprobe_opts);
}

void NetWatcherBpf::attachUprobeRedis() {
    const char* redis_path = "/usr/bin/redis-server";
    if (!fs::exists(redis_path)) return;

    LIBBPF_OPTS(bpf_uprobe_opts, uprobe_opts, .func_name = "processCommand", .retprobe = false);
    skel_->links.query__start_redis_process = bpf_program__attach_uprobe_opts(
        skel_->progs.query__start_redis_process, -1, redis_path, 0, &uprobe_opts);

    uprobe_opts.func_name = "call";
    uprobe_opts.retprobe = true;
    skel_->links.query__end_redis = bpf_program__attach_uprobe_opts(
        skel_->progs.query__end_redis, -1, redis_path, 0, &uprobe_opts);
}

void NetWatcherBpf::destroy() {
    if (rb_) { ring_buffer__free(rb_); rb_ = nullptr; }
    if (skel_) { netwatcher_bpf__destroy(skel_); skel_ = nullptr; }
}

void NetWatcherBpf::setRodataFlags() {
    skel_->rodata->filter_dport = config_.dport;
    skel_->rodata->filter_sport = config_.sport;
    skel_->rodata->all_conn = config_.all_conn;
    skel_->rodata->err_packet = config_.err_packet;
    skel_->rodata->extra_conn_info = config_.extra_conn_info;
    skel_->rodata->layer_time = config_.layer_time;
    skel_->rodata->http_info = config_.http_info;
    skel_->rodata->retrans_info = config_.retrans_info;
    skel_->rodata->udp_info = config_.udp_info;
    skel_->rodata->net_filter = config_.net_filter;
    skel_->rodata->drop_reason = config_.drop_reason;
    skel_->rodata->tcp_info = config_.tcp_info;
    skel_->rodata->icmp_info = config_.icmp_info;
    skel_->rodata->dns_info = config_.dns_info;
    skel_->rodata->stack_info = config_.stack_info;
    skel_->rodata->mysql_info = config_.mysql_info;
    skel_->rodata->redis_info = config_.redis_info;
    skel_->rodata->rtt_info = config_.rtt_info;
    skel_->rodata->rst_info = config_.rst_info;
}

void NetWatcherBpf::setBpfProgsLoadOpt() {
    bool tcp_related = config_.all_conn || config_.err_packet || config_.extra_conn_info ||
                       config_.retrans_info || config_.layer_time || config_.http_info ||
                       config_.rtt_info;
    bpf_program__set_autoload(skel_->progs.inet_csk_accept_exit, tcp_related);
    bpf_program__set_autoload(skel_->progs.tcp_v4_connect, tcp_related);
    bpf_program__set_autoload(skel_->progs.tcp_v4_connect_exit, tcp_related);
    bpf_program__set_autoload(skel_->progs.tcp_v6_connect, tcp_related);
    bpf_program__set_autoload(skel_->progs.tcp_v6_connect_exit, tcp_related);
    bpf_program__set_autoload(skel_->progs.tcp_set_state, tcp_related);
    bpf_program__set_autoload(skel_->progs.eth_type_trans, tcp_related);
    bpf_program__set_autoload(skel_->progs.ip_rcv_core, tcp_related);
    bpf_program__set_autoload(skel_->progs.ip6_rcv_core, tcp_related);
    bpf_program__set_autoload(skel_->progs.tcp_v4_rcv, tcp_related);
    bpf_program__set_autoload(skel_->progs.tcp_v6_rcv, tcp_related);
    bpf_program__set_autoload(skel_->progs.tcp_v4_do_rcv, tcp_related);
    bpf_program__set_autoload(skel_->progs.tcp_v6_do_rcv, tcp_related);
    bpf_program__set_autoload(skel_->progs.skb_copy_datagram_iter, tcp_related);
    bpf_program__set_autoload(skel_->progs.tcp_validate_incoming, config_.err_packet);
    bpf_program__set_autoload(skel_->progs.__skb_checksum_complete_exit, config_.err_packet);
    bpf_program__set_autoload(skel_->progs.tcp_sendmsg, tcp_related);
    bpf_program__set_autoload(skel_->progs.ip_queue_xmit, tcp_related);
    bpf_program__set_autoload(skel_->progs.inet6_csk_xmit, tcp_related);
    bpf_program__set_autoload(skel_->progs.__dev_queue_xmit, tcp_related);
    bpf_program__set_autoload(skel_->progs.dev_hard_start_xmit, tcp_related);
    bpf_program__set_autoload(skel_->progs.tcp_enter_recovery, config_.retrans_info);
    bpf_program__set_autoload(skel_->progs.tcp_enter_loss, config_.retrans_info);
    bpf_program__set_autoload(skel_->progs.udp_rcv, config_.udp_info || config_.dns_info);
    bpf_program__set_autoload(skel_->progs.__udp_enqueue_schedule_skb, config_.udp_info || config_.dns_info);
    bpf_program__set_autoload(skel_->progs.udp_send_skb, config_.udp_info || config_.dns_info);
    bpf_program__set_autoload(skel_->progs.ip_send_skb, config_.udp_info || config_.dns_info);
    bpf_program__set_autoload(skel_->progs.ip_rcv, config_.net_filter);
    bpf_program__set_autoload(skel_->progs.ip_local_deliver, config_.net_filter);
    bpf_program__set_autoload(skel_->progs.ip_local_deliver_finish, config_.net_filter);
    bpf_program__set_autoload(skel_->progs.ip_local_out, config_.net_filter);
    bpf_program__set_autoload(skel_->progs.ip_output, config_.net_filter);
    bpf_program__set_autoload(skel_->progs.__ip_finish_output, config_.net_filter);
    bpf_program__set_autoload(skel_->progs.ip_forward, config_.net_filter);
    bpf_program__set_autoload(skel_->progs.tp_kfree, config_.drop_reason);
    bpf_program__set_autoload(skel_->progs.icmp_rcv, config_.icmp_info);
    bpf_program__set_autoload(skel_->progs.__sock_queue_rcv_skb, config_.icmp_info);
    bpf_program__set_autoload(skel_->progs.icmp_reply, config_.icmp_info);
    bpf_program__set_autoload(skel_->progs.handle_set_state, config_.tcp_info);
    bpf_program__set_autoload(skel_->progs.query__start, config_.mysql_info);
    bpf_program__set_autoload(skel_->progs.query__end, config_.mysql_info);
    bpf_program__set_autoload(skel_->progs.query__end_redis, config_.redis_info);
    bpf_program__set_autoload(skel_->progs.query__start_redis_process, config_.redis_info);
    bpf_program__set_autoload(skel_->progs.tcp_rcv_established, tcp_related);
    bpf_program__set_autoload(skel_->progs.handle_send_reset, config_.rst_info);
    bpf_program__set_autoload(skel_->progs.handle_receive_reset, config_.rst_info);
}

void NetWatcherBpf::printLogo() {
    fmt::print(fmt::fg(fmt::color::cyan), 
        "              __                          __           __               \n"
        "             /\\ \\__                      /\\ \\__       /\\ \\              \n"
        "  ___      __\\ \\  _\\  __  __  __     __  \\ \\  _\\   ___\\ \\ \\___      __   _ __   \n"
        " /  _  \\  / __ \\ \\ \\/ /\\ \\/\\ \\/\\ \\  / __ \\ \\ \\ \\/  / ___\\ \\  _  \\  / __ \\/\\  __\\ \n"
        "/\\ \\/\\ \\/\\  __/\\ \\ \\_\\ \\ \\_/ \\_/ \\/\\ \\_\\ \\_\\ \\ \\_/\\ \\__/\\ \\ \\ \\ \\/\\  __/\\ \\ \\/  \n"
        "\\ \\_\\ \\_\\ \\____\\ \\__\\ \\_______ / /\\ \\__/\\ \\_\\ \\__\\ \\____/\\ \\_\\ \\_\\ \\____ \\ \\_\\  \n"
        " \\/_/\\/_/\\/____/ \\/__/ \\/__//__ /  \\/_/  \\/_/\\/__/\\/____/ \\/_/\\/_/\\/____/ \\/_/  \n\n");
}

void NetWatcherBpf::poll() {
    printLogo();
    rb_ = ring_buffer__new(bpf_map__fd(skel_->maps.rb), print_packet, this, nullptr);
    if (!rb_) { SPDLOG_ERROR("Failed to create ring buffer"); return; }
    ring_buffer__add(rb_, bpf_map__fd(skel_->maps.udp_rb), print_udp, this);
    ring_buffer__add(rb_, bpf_map__fd(skel_->maps.netfilter_rb), print_netfilter, this);
    ring_buffer__add(rb_, bpf_map__fd(skel_->maps.kfree_rb), print_kfree, this);
    ring_buffer__add(rb_, bpf_map__fd(skel_->maps.tcp_rb), print_tcpstate, this);
    ring_buffer__add(rb_, bpf_map__fd(skel_->maps.icmp_rb), print_icmptime, this);
    ring_buffer__add(rb_, bpf_map__fd(skel_->maps.dns_rb), print_dns, this);
    ring_buffer__add(rb_, bpf_map__fd(skel_->maps.trace_rb), print_trace, this);
    ring_buffer__add(rb_, bpf_map__fd(skel_->maps.mysql_rb), print_mysql, this);
    ring_buffer__add(rb_, bpf_map__fd(skel_->maps.redis_rb), print_redis, this);
    ring_buffer__add(rb_, bpf_map__fd(skel_->maps.rtt_rb), print_rtt, this);
    ring_buffer__add(rb_, bpf_map__fd(skel_->maps.events), print_rst, this);
    printHeader(getMonitorMode());
    struct timeval last_conn_print, now;
    gettimeofday(&last_conn_print, nullptr);
    while (true) {
        int err = ring_buffer__poll(rb_, kPollPeriodMs);
        if (err == -EINTR) break;
        if (err < 0) { SPDLOG_ERROR("Error polling ring buffer: {}", err); break; }
        gettimeofday(&now, nullptr);
        if (now.tv_sec - last_conn_print.tv_sec >= 1) { printConns(); last_conn_print = now; }
    }
}

void NetWatcherBpf::printConns() {
    int map_fd = bpf_map__fd(skel_->maps.conns_info);
    void *sk = nullptr, *next_sk = nullptr;
    while (bpf_map_get_next_key(map_fd, sk, &next_sk) == 0) {
        struct conn_t d = {};
        if (bpf_map_lookup_elem(map_fd, &next_sk, &d) == 0) { /* write to file or print */ }
        sk = next_sk;
    }
}

NetWatcherBpf::MonitorMode NetWatcherBpf::getMonitorMode() const {
    if (config_.udp_info) return MonitorMode::MODE_UDP;
    if (config_.net_filter) return MonitorMode::MODE_NET_FILTER;
    if (config_.drop_reason) return MonitorMode::MODE_DROP_REASON;
    if (config_.icmp_info) return MonitorMode::MODE_ICMP;
    if (config_.tcp_info) return MonitorMode::MODE_TCP;
    if (config_.dns_info) return MonitorMode::MODE_DNS;
    if (config_.mysql_info) return MonitorMode::MODE_MYSQL;
    if (config_.redis_info) return MonitorMode::MODE_REDIS;
    if (config_.rtt_info) return MonitorMode::MODE_RTT;
    if (config_.rst_info) return MonitorMode::MODE_RST;
    return MonitorMode::MODE_DEFAULT;
}

void NetWatcherBpf::printHeader(MonitorMode mode) const {
    switch (mode) {
    case MonitorMode::MODE_UDP:
        fmt::print("{:=^100}\n", "UDP INFORMATION");
        fmt::print("{:<20} {:<20} {:<10} {:<10} {:<12} {:<10} {:<10}\n", "Saddr", "Daddr", "Sport", "Dport", "udp_time/μs", "RX", "len/byte");
        break;
    case MonitorMode::MODE_NET_FILTER:
        fmt::print("{:=^100}\n", "NETFILTER INFORMATION");
        fmt::print("{:<20} {:<20} {:<10} {:<10} {:<10} {:<10} {:<10} {:<10} {:<10} {:<10}\n", "Saddr", "Daddr", "Sport", "Dport", "PreRT/μs", "L_IN/μs", "FW/μs", "PostRT/μs", "L_OUT/μs", "RX");
        break;
    case MonitorMode::MODE_DROP_REASON:
        fmt::print("{:=^100}\n", "DROP INFORMATION");
        fmt::print("{:<13} {:<17} {:<17} {:<10} {:<10} {:<9} {:<33} {:<30}\n", "Time", "Saddr", "Daddr", "Sport", "Dport", "prot", "addr", "reason");
        break;
    case MonitorMode::MODE_ICMP:
        fmt::print("{:=^100}\n", "ICMP INFORMATION");
        fmt::print("{:<20} {:<20} {:<20} {:<20}\n", "Saddr", "Daddr", "icmp_time/μs", "RX");
        break;
    case MonitorMode::MODE_TCP:
        fmt::print("{:=^100}\n", "TCP STATE INFORMATION");
        fmt::print("{:<20} {:<20} {:<10} {:<10} {:<15} {:<15} {:<10}\n", "Saddr", "Daddr", "Sport", "Dport", "oldstate", "newstate", "time/μs");
        break;
    case MonitorMode::MODE_DNS:
        fmt::print("{:=^100}\n", "DNS INFORMATION");
        fmt::print("{:<20} {:<20} {:<10} {:<10} {:<5} {:<5} {:<5} {:<5} {:<40} {:<5} {:<5} {:<5}\n", "Saddr", "Daddr", "Id", "Flags", "Qd", "An", "Ns", "Ar", "Qr", "Qc", "Sc", "RX");
        break;
    case MonitorMode::MODE_MYSQL:
        fmt::print("{:=^100}\n", "MYSQL INFORMATION");
        fmt::print("{:<10} {:<10} {:<15} {:<10} {:<40} {:<15} {:<10}\n", "Pid", "Tid", "Comm", "Size", "Sql", "Duration/μs", "Request");
        break;
    case MonitorMode::MODE_REDIS:
        fmt::print("{:=^100}\n", "REDIS INFORMATION");
        fmt::print("{:<10} {:<15} {:<10} {:<20} {:<15}\n", "Pid", "Comm", "Size", "Redis", "duration/μs");
        break;
    case MonitorMode::MODE_RTT:
        fmt::print("{:=^100}\n", "RTT INFORMATION");
        break;
    case MonitorMode::MODE_RST:
        fmt::print("{:=^100}\n", "RST INFORMATION");
        fmt::print("{:<10} {:<15} {:<20} {:<20} {:<10} {:<10} {:<20}\n", "Pid", "Comm", "Saddr", "Daddr", "Sport", "Dport", "Time");
        break;
    case MonitorMode::MODE_DEFAULT:
        fmt::print("{:=^100}\n", "INFORMATION");
        fmt::print("{:<22} {:<20} {:<8} {:<20} {:<8} {:<15} {:<15} {:<15} {:<10} {:<15}\n", "SOCK", "Saddr", "Sport", "Daddr", "Dport", "MAC_TIME/μs", "IP_TIME/μs", "TRAN_TIME/μs", "RX", "HTTP");
        break;
    }
}

void NetWatcherBpf::handleEvent(void *ctx, void *data, size_t len) {}
void NetWatcherBpf::processEvent(void *data, size_t len) {}
