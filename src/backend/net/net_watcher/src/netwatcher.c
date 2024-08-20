// Copyright 2023 The LMP Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// https://github.com/linuxkerneltravel/lmp/blob/develop/LICENSE
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// author: blown.away@qq.com
//
// netwatcher libbpf 用户态代码

#include <argp.h>
#include <arpa/inet.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>

#include "net_watcher/include/netwatcher.h"     // 适配 ksight
#include "net_watcher/include/dropreason.h"
#include "net/netwatcher/netwatcher.skel.h"

static volatile bool exiting = false;

static char connects_file_path[1024];
static char err_file_path[1024];
static char packets_file_path[1024];
static char udp_file_path[1024];

static int sport = 0, dport = 0; // for filter
static int all_conn = 0, err_packet = 0, extra_conn_info = 0, layer_time = 0,
           http_info = 0, retrans_info = 0, udp_info = 0, net_filter = 0,
           drop_reason = 0, addr_to_func = 0, icmp_info = 0, tcp_info = 0,
           time_load = 0, dns_info = 0, stack_info = 0, mysql_info = 0,
           redis_info = 0, count_info = 0, rtt_info = 0, rst_info = 0; // flag

static const char *tcp_states[] = {
    [1] = "ESTABLISHED", [2] = "SYN_SENT",   [3] = "SYN_RECV",
    [4] = "FIN_WAIT1",   [5] = "FIN_WAIT2",  [6] = "TIME_WAIT",
    [7] = "CLOSE",       [8] = "CLOSE_WAIT", [9] = "LAST_ACK",
    [10] = "LISTEN",     [11] = "CLOSING",   [12] = "NEW_SYN_RECV",
    [13] = "UNKNOWN",
};

static const char argp_program_doc[] = "Watch tcp/ip in network subsystem \n";
static const struct argp_option opts[] = {
    {"all", 'a', 0, 0, "set to trace CLOSED connection"},
    {"err", 'e', 0, 0, "set to trace TCP error packets"},
    {"extra", 'x', 0, 0, "set to trace extra conn info"},
    {"retrans", 'r', 0, 0, "set to trace extra retrans info"},
    {"time", 't', 0, 0, "set to trace layer time of each packet"},
    {"http", 'i', 0, 0, "set to trace http info"},
    {"sport", 's', "SPORT", 0, "trace this source port only"},
    {"dport", 'd', "DPORT", 0, "trace this destination port only"},
    {"udp", 'u', 0, 0, "trace the udp message"},
    {"net_filter", 'n', 0, 0, "trace ipv4 packget filter "},
    {"drop_reason", 'k', 0, 0, "trace kfree "},
    {"addr_to_func", 'F', 0, 0, "translation addr to func and offset"},
    {"icmptime", 'I', 0, 0, "set to trace layer time of icmp"},
    {"tcpstate", 'S', 0, 0, "set to trace tcpstate"},
    {"timeload", 'L', 0, 0, "analysis time load"},
    {"dns", 'D', 0, 0,
     "set to trace dns information info include Id 事务ID、Flags 标志字段、Qd "
     "问题部分计数、An 应答记录计数、Ns 授权记录计数、Ar 附加记录计数、Qr "
     "域名、rx 收发包 、Qc请求数、Sc响应数"},
    {"stack", 'A', 0, 0, "set to trace of stack "},
    {"mysql", 'M', 0, 0,
     "set to trace mysql information info include Pid 进程id、Comm "
     "进程名、Size sql语句字节大小、Sql 语句"},
    {"redis", 'R', 0, 0},
    {"count", 'C', "NUMBER", 0,
     "specify the time to count the number of requests"},
    {"rtt", 'T', 0, 0, "set to trace rtt"},
    {"rst_counters", 'U', 0, 0, "set to trace rst"},
    {}};

static error_t parse_arg(int key, char *arg, struct argp_state *state) {
    char *end;
    switch (key) {
    case 'a':
        all_conn = 1;
        break;
    case 'e':
        err_packet = 1;
        break;
    case 'x':
        extra_conn_info = 1;
        break;
    case 'r':
        retrans_info = 1;
        break;
    case 't':
        layer_time = 1;
        break;
    case 'i':
        http_info = 1;
        break;
    case 's':
        sport = strtoul(arg, &end, 10);
        break;
    case 'd':
        dport = strtoul(arg, &end, 10);
        break;
    case 'u':
        udp_info = 1;
        break;
    case 'n':
        net_filter = 1;
        break;
    case 'k':
        drop_reason = 1;
        break;
    case 'F':
        addr_to_func = 1;
        break;
    case 'I':
        icmp_info = 1;
        break;
    case 'S':
        tcp_info = 1;
        break;
    case 'L':
        time_load = 1;
        break;
    case 'D':
        dns_info = 1;
        break;
    case 'A':
        stack_info = 1;
        break;
    case 'M':
        mysql_info = 1;
        break;
    case 'R':
        redis_info = 1;
        break;
    case 'T':
        rtt_info = 1;
        break;
    case 'U':
        rst_info = 1;
        break;
    case 'C':
        count_info = strtoul(arg, &end, 10);
        break;
    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

static const struct argp argp = {
    .options = opts,
    .parser = parse_arg,
    .doc = argp_program_doc,
};

struct SymbolEntry {
    unsigned long addr;
    char name[30];
};

enum MonitorMode {
    MODE_UDP,
    MODE_NET_FILTER,
    MODE_DROP_REASON,
    MODE_ICMP,
    MODE_TCP,
    MODE_DNS,
    MODE_MYSQL,
    MODE_REDIS,
    MODE_RTT,
    MODE_RST,
    MODE_DEFAULT
};

enum MonitorMode get_monitor_mode() {
    if (udp_info) {
        return MODE_UDP;
    } else if (net_filter) {
        return MODE_NET_FILTER;
    } else if (drop_reason) {
        return MODE_DROP_REASON;
    } else if (icmp_info) {
        return MODE_ICMP;
    } else if (tcp_info) {
        return MODE_TCP;
    } else if (dns_info) {
        return MODE_DNS;
    } else if (mysql_info) {
        return MODE_MYSQL;
    } else if (redis_info) {
        return MODE_REDIS;
    } else if (rtt_info) {
        return MODE_RTT;
    } else if (rst_info) {
        return MODE_RST;
    } else {
        return MODE_DEFAULT;
    }
}
#define LOGO_STRING                                                            \
    " "                                                                        \
    "              __                          __           __               " \
    "        \n"                                                               \
    "             /\\ \\__                      /\\ \\__       /\\ \\        " \
    "              \n"                                                         \
    "  ___      __\\ \\  _\\  __  __  __     __  \\ \\  _\\   ___\\ \\ \\___ " \
    "     __   _ __   \n"                                                      \
    "/  _  \\  / __ \\ \\ \\/ /\\ \\/\\ \\/\\ \\  / __ \\ \\ \\ \\/  / ___\\ " \
    "\\  _  \\  / __ \\/\\  __\\ \n"                                           \
    "/\\ \\/\\ \\/\\  __/\\ \\ \\_\\ \\ \\_/ \\_/ \\/\\ \\_\\ \\_\\ \\ "       \
    "\\_/\\ \\__/\\ \\ \\ \\ \\/\\  __/\\ \\ \\/  \n"                          \
    "\\ \\_\\ \\_\\ \\____\\ \\__\\ \\_______ / /\\ \\__/\\ \\_\\ \\__\\ "     \
    "\\____/\\ \\_\\ \\_\\ \\____ \\ \\_\\  \n"                                \
    " \\/_/\\/_/\\/____/ \\/__/ \\/__//__ /  \\/_/  \\/_/\\/__/\\/____/ "      \
    "\\/_/\\/_/\\/____/ \\/_/  \n\n"

// 通过lolcat命令彩色处理
void print_logo() {
    char *logo = LOGO_STRING;
    int i = 0;
    FILE *lolcat_pipe = popen("/usr/games/lolcat", "w");
    if (lolcat_pipe == NULL) {
        printf("Error: Unable to execute lolcat command.\n");
        return;
    }
    // 像lolcat管道逐个字符写入字符串
    while (logo[i] != '\0') {
        fputc(logo[i], lolcat_pipe);
        fflush(lolcat_pipe); // 刷新管道，确保字符被立即发送给lolcat
        usleep(150);
        i++;
    }

    pclose(lolcat_pipe);
}
static char binary_path[64] = "";
#define __ATTACH_UPROBE(skel, sym_name, prog_name, is_retprobe)                \
    do {                                                                       \
        LIBBPF_OPTS(bpf_uprobe_opts, uprobe_opts, .func_name = #sym_name,      \
                    .retprobe = is_retprobe);                                  \
        skel->links.prog_name = bpf_program__attach_uprobe_opts(               \
            skel->progs.prog_name, -1, binary_path, 0, &uprobe_opts);          \
    } while (false)

#define __CHECK_PROGRAM(skel, prog_name)                                       \
    do {                                                                       \
        if (!skel->links.prog_name) {                                          \
            perror("no program attached for " #prog_name);                     \
            return -errno;                                                     \
        }                                                                      \
    } while (false)

#define __ATTACH_UPROBE_CHECKED(skel, sym_name, prog_name, is_retprobe)        \
    do {                                                                       \
        __ATTACH_UPROBE(skel, sym_name, prog_name, is_retprobe);               \
        __CHECK_PROGRAM(skel, prog_name);                                      \
    } while (false)

#define ATTACH_UPROBE(skel, sym_name, prog_name)                               \
    __ATTACH_UPROBE(skel, sym_name, prog_name, false)
#define ATTACH_URETPROBE(skel, sym_name, prog_name)                            \
    __ATTACH_UPROBE(skel, sym_name, prog_name, true)

#define ATTACH_UPROBE_CHECKED(skel, sym_name, prog_name)                       \
    __ATTACH_UPROBE_CHECKED(skel, sym_name, prog_name, false)
#define ATTACH_URETPROBE_CHECKED(skel, sym_name, prog_name)                    \
    __ATTACH_UPROBE_CHECKED(skel, sym_name, prog_name, true)
struct SymbolEntry symbols[300000];
int num_symbols = 0;
// 定义快表
#define CACHEMAXSIZE 5
struct SymbolEntry cache[CACHEMAXSIZE];
int cache_size = 0;
// LRU算法查找函数
struct SymbolEntry find_in_cache(unsigned long int addr) {
    // 查找地址是否在快表中
    for (int i = 0; i < cache_size; i++) {
        if (cache[i].addr == addr) {
            // 更新访问时间
            struct SymbolEntry temp = cache[i];
            // 将访问的元素移动到快表的最前面，即最近使用的位置
            for (int j = i; j > 0; j--) {
                cache[j] = cache[j - 1];
            }
            cache[0] = temp;
            return temp;
        }
    }
    // 如果地址不在快表中，则返回空
    struct SymbolEntry empty_entry;
    empty_entry.addr = 0;
    return empty_entry;
}

// 将新的符号条目加入快表
void add_to_cache(struct SymbolEntry entry) {
    // 如果快表已满，则移除最久未使用的条目
    if (cache_size == CACHEMAXSIZE) {
        for (int i = cache_size - 1; i > 0; i--) {
            cache[i] = cache[i - 1];
        }
        cache[0] = entry;
    } else {
        // 否则，直接加入快表
        for (int i = cache_size; i > 0; i--) {
            cache[i] = cache[i - 1];
        }
        cache[0] = entry;
        cache_size++;
    }
}

struct SymbolEntry findfunc(unsigned long int addr) {
    // 先在快表中查找
    struct SymbolEntry entry = find_in_cache(addr);
    if (entry.addr != 0) {
        return entry;
    }
    unsigned long long low = 0, high = num_symbols - 1;
    unsigned long long result = -1;

    while (low <= high) {
        int mid = low + (high - low) / 2;
        if (symbols[mid].addr < addr) {
            result = mid;
            low = mid + 1;
        } else {
            high = mid - 1;
        }
    }
    add_to_cache(symbols[result]);
    return symbols[result];
};
void readallsym() {
    FILE *file = fopen("/proc/kallsyms", "r");
    if (!file) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        unsigned long addr;
        char type, name[30];
        int ret = sscanf(line, "%lx %c %s", &addr, &type, name);
        if (ret == 3) {
            symbols[num_symbols].addr = addr;
            strncpy(symbols[num_symbols].name, name, 30);
            num_symbols++;
        }
    }

    fclose(file);
}
/*
    指数加权移动平均算法（EWMA）
    1.使用指数加权移动平均算法（EWMA）来计算每层的指数加权移动平均值，
    公式EWMA_new = alpha * new_value + (1 - alpha) * old_ewma ,alpha
   指数加权系数，表示新数据点的权重，new_value 当前时延，old_ewma
   旧的指数加权移动平均值
    2.根据当前时延和指数加权移动平均值*预先设定的粒度阈值（GRANULARITY）对比，来判断时延是否异常
    3.可以快速适应数据的变化，并能够有效地检测异常时延

*/
struct LayerDelayInfo {
    float delay;     // 时延数据
    int layer_index; // 层索引
};
#define GRANULARITY 3
#define ALPHA 0.2 // 衰减因子
#define MAXTIME 10000
#define SLOW_QUERY_THRESHOLD 10000 //
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_RESET "\x1b[0m"

// 全局变量用于存储每层的移动平均值
float ewma_values[NUM_LAYERS] = {0};
int count[NUM_LAYERS] = {0};

// 指数加权移动平均算法
float calculate_ewma(float new_value, float old_ewma) {
    return ALPHA * new_value + (1 - ALPHA) * old_ewma;
}

// 收集时延数据并检测异常
int process_delay(float layer_delay, int layer_index) {

    if (layer_delay == 0)
        return 0;
    count[layer_index]++;
    if (ewma_values[layer_index] == 0) {
        ewma_values[layer_index] = layer_delay;
        return 0;
    }
    // 计算阈值,指数加权移动平均值乘以粒度因子
    ewma_values[layer_index] =
        calculate_ewma(layer_delay, ewma_values[layer_index]);
    float threshold = ewma_values[layer_index] * GRANULARITY;
    if (count[layer_index] > 30) {
        // 判断当前时延是否超过阈值
        //   printf("%d %d:%f %f
        //   ",layer_index,count[layer_index]++,threshold,layer_delay);
        if (layer_delay > threshold) { // 异常
            return 1;
        } else {
            return 0;
        }
    }
    return 0;
}

static void set_rodata_flags(struct netwatcher_bpf *skel) {
    skel->rodata->filter_dport = dport;
    skel->rodata->filter_sport = sport;
    skel->rodata->all_conn = all_conn;
    skel->rodata->err_packet = err_packet;
    skel->rodata->extra_conn_info = extra_conn_info;
    skel->rodata->layer_time = layer_time;
    skel->rodata->http_info = http_info;
    skel->rodata->retrans_info = retrans_info;
    skel->rodata->udp_info = udp_info;
    skel->rodata->net_filter = net_filter;
    skel->rodata->drop_reason = drop_reason;
    skel->rodata->tcp_info = tcp_info;
    skel->rodata->icmp_info = icmp_info;
    skel->rodata->dns_info = dns_info;
    skel->rodata->stack_info = stack_info;
    skel->rodata->mysql_info = mysql_info;
    skel->rodata->redis_info = redis_info;
    skel->rodata->rtt_info = rtt_info;
    skel->rodata->rst_info = rst_info;
}
static void set_disable_load(struct netwatcher_bpf *skel) {

    bpf_program__set_autoload(skel->progs.inet_csk_accept_exit,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.tcp_v4_connect,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.tcp_v4_connect_exit,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.tcp_v6_connect,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.tcp_v6_connect_exit,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.tcp_set_state,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.eth_type_trans,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.ip_rcv_core,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.ip6_rcv_core,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.tcp_v4_rcv,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.tcp_v6_rcv,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.tcp_v4_do_rcv,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.tcp_v6_do_rcv,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.skb_copy_datagram_iter,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.tcp_validate_incoming,
                              err_packet ? true : false);
    bpf_program__set_autoload(skel->progs.__skb_checksum_complete_exit,
                              err_packet ? true : false);
    bpf_program__set_autoload(skel->progs.tcp_sendmsg,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.ip_queue_xmit,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.inet6_csk_xmit,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.__dev_queue_xmit,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.dev_hard_start_xmit,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.tcp_enter_recovery,
                              retrans_info ? true : false);
    bpf_program__set_autoload(skel->progs.tcp_enter_loss,
                              retrans_info ? true : false);
    bpf_program__set_autoload(skel->progs.udp_rcv,
                              udp_info || dns_info ? true : false);
    bpf_program__set_autoload(skel->progs.__udp_enqueue_schedule_skb,
                              udp_info || dns_info ? true : false);
    bpf_program__set_autoload(skel->progs.udp_send_skb,
                              udp_info || dns_info ? true : false);
    bpf_program__set_autoload(skel->progs.ip_send_skb,
                              udp_info || dns_info ? true : false);
    bpf_program__set_autoload(skel->progs.ip_rcv, net_filter ? true : false);
    bpf_program__set_autoload(skel->progs.ip_local_deliver,
                              net_filter ? true : false);
    bpf_program__set_autoload(skel->progs.ip_local_deliver_finish,
                              net_filter ? true : false);
    bpf_program__set_autoload(skel->progs.ip_local_out,
                              net_filter ? true : false);
    bpf_program__set_autoload(skel->progs.ip_output, net_filter ? true : false);
    bpf_program__set_autoload(skel->progs.__ip_finish_output,
                              net_filter ? true : false);
    bpf_program__set_autoload(skel->progs.ip_forward,
                              net_filter ? true : false);
    bpf_program__set_autoload(skel->progs.tp_kfree, drop_reason ? true : false);
    bpf_program__set_autoload(skel->progs.icmp_rcv, icmp_info ? true : false);
    bpf_program__set_autoload(skel->progs.__sock_queue_rcv_skb,
                              icmp_info ? true : false);
    bpf_program__set_autoload(skel->progs.icmp_reply, icmp_info ? true : false);
    bpf_program__set_autoload(skel->progs.handle_set_state,
                              tcp_info ? true : false);
    bpf_program__set_autoload(skel->progs.query__start,
                              mysql_info ? true : false);
    bpf_program__set_autoload(skel->progs.query__end,
                              mysql_info ? true : false);
    bpf_program__set_autoload(skel->progs.query__end_redis,
                              redis_info ? true : false);
    bpf_program__set_autoload(skel->progs.query__start_redis_process,
                              redis_info ? true : false);
    bpf_program__set_autoload(skel->progs.tcp_rcv_established,
                              (all_conn || err_packet || extra_conn_info ||
                               retrans_info || layer_time || http_info ||
                               rtt_info)
                                  ? true
                                  : false);
    bpf_program__set_autoload(skel->progs.handle_send_reset,
                              rst_info ? true : false);
    bpf_program__set_autoload(skel->progs.handle_receive_reset,
                              rst_info ? true : false);
}

static void print_header(enum MonitorMode mode) {
    switch (mode) {
    case MODE_UDP:
        printf("==============================================================="
               "UDP "
               "INFORMATION===================================================="
               "====\n");
        printf("%-20s %-20s %-20s %-20s %-20s %-20s %-20s\n", "Saddr", "Daddr",
               "Sprot", "Dprot", "udp_time/μs", "RX/direction", "len/byte");
        break;
    case MODE_NET_FILTER:
        printf("==============================================================="
               "===NETFILTER "
               "INFORMATION===================================================="
               "=======\n");
        printf("%-20s %-20s %-12s %-12s %-8s %-8s %-7s %-8s %-8s %-8s\n",
               "Saddr", "Daddr", "Sprot", "Dprot", "PreRT/μs", "L_IN/μs",
               "FW/μs", "PostRT/μs", "L_OUT/μs", "RX/direction");
        break;
    case MODE_DROP_REASON:
        printf("==============================================================="
               "DROP "
               "INFORMATION===================================================="
               "====\n");
        printf("%-13s %-17s %-17s %-10s %-10s %-9s %-33s %-30s\n", "Time",
               "Saddr", "Daddr", "Sprot", "Dprot", "prot", "addr", "reason");
        break;
    case MODE_ICMP:
        printf("=================================================ICMP "
               "INFORMATION==============================================\n");
        printf("%-20s %-20s %-20s %-20s\n", "Saddr", "Daddr", "icmp_time/μs",
               "RX/direction");
        break;
    case MODE_TCP:
        printf("==============================================================="
               "TCP STATE "
               "INFORMATION===================================================="
               "====\n");
        printf("%-20s %-20s %-20s %-20s %-20s %-20s %-20s \n", "Saddr", "Daddr",
               "Sport", "Dport", "oldstate", "newstate", "time/μs");
        break;
    case MODE_DNS:
        printf("==============================================================="
               "====================DNS "
               "INFORMATION===================================================="
               "============================\n");
        printf("%-20s %-20s %-12s %-12s %-5s %-5s %-5s %-5s %-47s %-10s %-10s "
               "%-10s \n",
               "Saddr", "Daddr", "Id", "Flags", "Qd", "An", "Ns", "Ar", "Qr",
               "Qc", "Sc", "RX/direction");
        break;
    case MODE_MYSQL:
        printf("==============================================================="
               "====================MYSQL "
               "INFORMATION===================================================="
               "============================\n");
        printf("%-20s %-20s %-20s %-20s %-40s %-20s %-20s  \n", "Pid", "Tid",
               "Comm", "Size", "Sql", "Duration/μs", "Request");
        break;
    case MODE_REDIS:
        printf("==============================================================="
               "====================REDIS "
               "INFORMATION===================================================="
               "============================\n");
        printf("%-20s %-20s %-20s %-20s %-20s \n", "Pid", "Comm", "Size",
               "Redis", "duration/μs");
        break;
    case MODE_RTT:
        printf("==============================================================="
               "====================RTT "
               "INFORMATION===================================================="
               "============================\n");
        break;
    case MODE_RST:
        printf("==============================================================="
               "====================RST "
               "INFORMATION===================================================="
               "============================\n");
        printf("%-20s %-20s %-20s %-20s %-20s  %-20s %-20s \n", "Pid", "Comm",
               "Saddr", "Daddr", "Sport", "Dport", "Time");
        break;
    case MODE_DEFAULT:
        printf("==============================================================="
               "=INFORMATION==================================================="
               "======================\n");
        printf("%-22s %-20s %-8s %-20s %-8s %-15s %-15s %-15s %-15s %-15s \n",
               "SOCK", "Saddr", "Sport", "Daddr", "Dport", "MAC_TIME/μs",
               "IP_TIME/μs", "TRAN_TIME/μs", "RX/direction", "HTTP");
        break;
    }
}

static void open_log_files() {
    FILE *connect_file = fopen(connects_file_path, "w+");
    if (connect_file == NULL) {
        fprintf(stderr, "Failed to open connect.log: (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    fclose(connect_file);

    FILE *err_file = fopen(err_file_path, "w+");
    if (err_file == NULL) {
        fprintf(stderr, "Failed to open err.log: (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    fclose(err_file);

    FILE *packet_file = fopen(packets_file_path, "w+");
    if (packet_file == NULL) {
        fprintf(stderr, "Failed to open packets.log: (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    fclose(packet_file);

    FILE *udp_file = fopen(udp_file_path, "w+");
    if (udp_file == NULL) {
        fprintf(stderr, "Failed to open udp.log: (%s)\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    fclose(udp_file);
}

static void sig_handler(int signo) { exiting = true; }

static void bytes_to_str(char *str, unsigned long long num) {
    if (num > 1e9) {
        sprintf(str, "%.8lfG", (double)num / 1e9);
    } else if (num > 1e6) {
        sprintf(str, "%.6lfM", (double)num / 1e6);
    } else if (num > 1e3) {
        sprintf(str, "%.3lfK", (double)num / 1e3);
    } else {
        sprintf(str, "%llu", num);
    }
}

static int print_conns(struct netwatcher_bpf *skel) {

    FILE *file = fopen(connects_file_path, "w");
    if (file == NULL) {
        fprintf(stderr, "Failed to open connects.log: (%s)\n", strerror(errno));
        return 0;
    }

    int map_fd = bpf_map__fd(skel->maps.conns_info);
    struct sock *sk = NULL;

    while (bpf_map_get_next_key(map_fd, &sk, &sk) == 0) {
        // fprintf(stdout, "next_sk: (%p)\n", sk);
        struct conn_t d = {};
        int err = bpf_map_lookup_elem(map_fd, &sk, &d);
        if (err) {
            fprintf(stderr, "Failed to read value from the conns map: (%s)\n",
                    strerror(errno));
            return 0;
        }
        char s_str[INET_ADDRSTRLEN];
        char d_str[INET_ADDRSTRLEN];

        char s_str_v6[INET6_ADDRSTRLEN];
        char d_str_v6[INET6_ADDRSTRLEN];

        char s_ip_port_str[INET6_ADDRSTRLEN + 6];
        char d_ip_port_str[INET6_ADDRSTRLEN + 6];
        if ((d.saddr & 0x0000FFFF) == 0x0000007F ||
            (d.daddr & 0x0000FFFF) == 0x0000007F)
            return 0;
        if (d.family == AF_INET) {
            sprintf(s_ip_port_str, "%s:%d",
                    inet_ntop(AF_INET, &d.saddr, s_str, sizeof(s_str)),
                    d.sport);
            sprintf(d_ip_port_str, "%s:%d",
                    inet_ntop(AF_INET, &d.daddr, d_str, sizeof(d_str)),
                    d.dport);
        } else { // AF_INET6
            sprintf(
                s_ip_port_str, "%s:%d",
                inet_ntop(AF_INET6, &d.saddr_v6, s_str_v6, sizeof(s_str_v6)),
                d.sport);
            sprintf(
                d_ip_port_str, "%s:%d",
                inet_ntop(AF_INET6, &d.daddr_v6, d_str_v6, sizeof(d_str_v6)),
                d.dport);
        }
        char received_bytes[11], acked_bytes[11];
        bytes_to_str(received_bytes, d.bytes_received);
        bytes_to_str(acked_bytes, d.bytes_acked);
        fprintf(file,
                "connection{pid=\"%d\",sock=\"%p\",src=\"%s\",dst=\"%s\","
                "is_server=\"%d\"",
                d.pid, d.sock, s_ip_port_str, d_ip_port_str, d.is_server);
        if (extra_conn_info) {
            fprintf(file,
                    ",backlog=\"%u\""
                    ",maxbacklog=\"%u\""
                    ",rwnd=\"%u\""
                    ",cwnd=\"%u\""
                    ",ssthresh=\"%u\""
                    ",sndbuf=\"%u\""
                    ",wmem_queued=\"%u\""
                    ",rx_bytes=\"%s\""
                    ",tx_bytes=\"%s\""
                    ",srtt=\"%u\""
                    ",duration=\"%llu\""
                    ",total_retrans=\"%u\"",
                    d.tcp_backlog, d.max_tcp_backlog, d.rcv_wnd, d.snd_cwnd,
                    d.snd_ssthresh, d.sndbuf, d.sk_wmem_queued, received_bytes,
                    acked_bytes, d.srtt, d.duration, d.total_retrans);
        } else {
            fprintf(file,
                    ",backlog=\"-\",maxbacklog=\"-\",cwnd=\"-\",ssthresh=\"-\","
                    "sndbuf=\"-\",wmem_queued=\"-\",rx_bytes=\"-\",tx_bytes=\"-"
                    "\",srtt=\"-\",duration=\"-\",total_retrans=\"-\"");
        }
        if (retrans_info) {
            fprintf(file, ",fast_retrans=\"%u\",timeout_retrans=\"%u\"",
                    d.fastRe, d.timeout);
        } else {
            fprintf(file, ",fast_retrans=\"-\",timeout_retrans=\"-\"");
        }
        fprintf(file, "}\n");
    }
    fflush(file);
    fclose(file);
    return 0;
}

static int print_packet(void *ctx, void *packet_info, size_t size) {
    if (udp_info || net_filter || drop_reason || icmp_info || tcp_info ||
        dns_info || mysql_info || redis_info || rtt_info)
        return 0;
    const struct pack_t *pack_info = packet_info;
    if (pack_info->mac_time > MAXTIME || pack_info->ip_time > MAXTIME ||
        pack_info->tran_time > MAXTIME) {
        return 0;
    }
    char d_str[INET_ADDRSTRLEN];
    char s_str[INET_ADDRSTRLEN];
    unsigned int saddr = pack_info->saddr;
    unsigned int daddr = pack_info->daddr;
    if ((daddr & 0x0000FFFF) == 0x0000007F ||
        (saddr & 0x0000FFFF) == 0x0000007F)
        return 0;
    if (dport)
        if (pack_info->dport != dport)
            return 0;

    if (sport)
        if (pack_info->sport != sport)
            return 0;

    if (pack_info->err) {
        FILE *file = fopen(err_file_path, "a");
        char reason[20];
        if (pack_info->err == 1) {
            printf("[X] invalid SEQ: sock = %p,seq= %u,ack = %u\n",
                   pack_info->sock, pack_info->seq, pack_info->ack);
            sprintf(reason, "Invalid SEQ");
        } else if (pack_info->err == 2) {
            printf("[X] invalid checksum: sock = %p\n", pack_info->sock);
            sprintf(reason, "Invalid checksum");
        } else {
            printf("UNEXPECTED packet error %d.\n", pack_info->err);
            sprintf(reason, "Unkonwn");
        }
        fprintf(file,
                "error{sock=\"%p\",seq=\"%u\",ack=\"%u\","
                "reason=\"%s\"} \n",
                pack_info->sock, pack_info->seq, pack_info->ack, reason);
        fclose(file);
    } else {
        FILE *file = fopen(packets_file_path, "a");
        char http_data[256];

        if (strstr((char *)pack_info->data, "HTTP/1")) {

            for (int i = 0; i < sizeof(pack_info->data); ++i) {
                if (pack_info->data[i] == '\r') {
                    http_data[i] = '\0';
                    break;
                }
                http_data[i] = pack_info->data[i];
            }
        } else {

            sprintf(http_data, "-");
        }
        if (layer_time) {
            printf("%-22p %-20s %-8d %-20s %-8d %-14llu %-14llu %-14llu %-15d "
                   "%-16s",
                   pack_info->sock,
                   inet_ntop(AF_INET, &saddr, s_str, sizeof(s_str)),
                   pack_info->sport,
                   inet_ntop(AF_INET, &daddr, d_str, sizeof(d_str)),
                   pack_info->dport, pack_info->mac_time, pack_info->ip_time,
                   pack_info->tran_time, pack_info->rx, http_data);
            fprintf(
                file,
                "packet{sock=\"%p\",saddr=\"%s\",sport=\"%d\",daddr=\"%s\","
                "dport=\"%d\",seq=\"%u\",ack=\"%u\","
                "mac_time=\"%llu\",ip_time=\"%llu\",tran_time=\"%llu\",http_"
                "info=\"%s\",rx=\"%d\"} \n",
                pack_info->sock,
                inet_ntop(AF_INET, &saddr, s_str, sizeof(s_str)),
                pack_info->sport,
                inet_ntop(AF_INET, &daddr, d_str, sizeof(d_str)),
                pack_info->dport, pack_info->seq, pack_info->ack,
                pack_info->mac_time, pack_info->ip_time, pack_info->tran_time,
                http_data, pack_info->rx);
        } else {
            printf("%-22p %-20s %-8d %-20s %-8d %-10d %-10d %-10d %-5d %-10s",
                   pack_info->sock,
                   inet_ntop(AF_INET, &saddr, s_str, sizeof(s_str)),
                   pack_info->sport,
                   inet_ntop(AF_INET, &daddr, d_str, sizeof(d_str)),
                   pack_info->dport, 0, 0, 0, pack_info->rx, http_data);
            fprintf(file,
                    "packet{sock=\"%p\",saddr=\"%s\",sport=\"%d\",daddr=\"%s\","
                    "dport=\"%d\",seq=\"%u\",ack=\"%u\","
                    "mac_time=\"%d\",ip_time=\"%d\",tran_time=\"%d\",http_"
                    "info=\"%s\",rx=\"%d\"} \n",
                    pack_info->sock,
                    inet_ntop(AF_INET, &saddr, s_str, sizeof(s_str)),
                    pack_info->sport,
                    inet_ntop(AF_INET, &daddr, d_str, sizeof(d_str)),
                    pack_info->dport, pack_info->seq, pack_info->ack, 0, 0, 0,
                    http_data, pack_info->rx);
        }
        fclose(file);
    }
    if (time_load) {
        int mac = process_delay(pack_info->mac_time, 0);
        int ip = process_delay(pack_info->ip_time, 1);
        int tran = process_delay(pack_info->tran_time, 2);
        if (mac || ip || tran) {
            printf("%-15s", "abnormal data");
        }
    }
    printf("\n");
    return 0;
}
static int print_udp(void *ctx, void *packet_info, size_t size) {
    if (!udp_info)
        return 0;
    FILE *file = fopen(udp_file_path, "a+"); // 追加
    if (file == NULL) {
        fprintf(stderr, "Failed to open udp.log: (%s)\n", strerror(errno));
        return 0;
    }
    char d_str[INET_ADDRSTRLEN];
    char s_str[INET_ADDRSTRLEN];
    const struct udp_message *pack_info = packet_info;
    unsigned int saddr = pack_info->saddr;
    unsigned int daddr = pack_info->daddr;
    if (pack_info->tran_time > MAXTIME || (daddr & 0x0000FFFF) == 0x0000007F ||
        (saddr & 0x0000FFFF) == 0x0000007F)
        return 0;
    printf("%-20s %-20s %-20u %-20u %-20llu %-20d %-20d",
           inet_ntop(AF_INET, &saddr, s_str, sizeof(s_str)),
           inet_ntop(AF_INET, &daddr, d_str, sizeof(d_str)), pack_info->sport,
           pack_info->dport, pack_info->tran_time, pack_info->rx,
           pack_info->len);
    fprintf(file,
            "packet{saddr=\"%s\",daddr=\"%s\",sport=\"%u\","
            "dport=\"%u\",udp_time=\"%llu\",rx=\"%d\",len=\"%d\"} \n",
            inet_ntop(AF_INET, &saddr, s_str, sizeof(s_str)),
            inet_ntop(AF_INET, &daddr, d_str, sizeof(d_str)), pack_info->sport,
            pack_info->dport, pack_info->tran_time, pack_info->rx,
            pack_info->len);
    fclose(file);
    if (time_load) {
        int flag = process_delay(pack_info->tran_time, 3);
        if (flag)
            printf("%-15s", "abnormal data");
    }
    printf("\n");

    return 0;
}

static int print_netfilter(void *ctx, void *packet_info, size_t size) {
    if (!net_filter)
        return 0;
    char d_str[INET_ADDRSTRLEN];
    char s_str[INET_ADDRSTRLEN];
    const struct netfilter *pack_info = packet_info;
    if (pack_info->local_input_time > MAXTIME ||
        pack_info->forward_time > MAXTIME ||
        pack_info->local_out_time > MAXTIME ||
        pack_info->post_routing_time > MAXTIME ||
        pack_info->pre_routing_time > MAXTIME)
        return 0;
    unsigned int saddr = pack_info->saddr;
    unsigned int daddr = pack_info->daddr;
    // if ((daddr & 0x0000FFFF) == 0x0000007F ||
    //     (saddr & 0x0000FFFF) == 0x0000007F)
    //     return 0;
    printf("%-20s %-20s %-12d %-12d %-8lld %-8lld% -8lld %-8lld %-8lld %-8d",
           inet_ntop(AF_INET, &saddr, s_str, sizeof(s_str)),
           inet_ntop(AF_INET, &daddr, d_str, sizeof(d_str)), pack_info->sport,
           pack_info->dport, pack_info->pre_routing_time,
           pack_info->local_input_time, pack_info->forward_time,
           pack_info->post_routing_time, pack_info->local_out_time,
           pack_info->rx);
    // 定义一个数组用于存储需要检测的时延数据和对应的层索引
    struct LayerDelayInfo layer_delay_infos[] = {
        {pack_info->pre_routing_time, 4},
        {pack_info->local_input_time, 5},
        {pack_info->forward_time, 6},
        {pack_info->post_routing_time, 7},
        {pack_info->local_out_time, 8}};
    if (time_load) {
        // 循环遍历数组
        for (int i = 0; i < 5; i++) {
            // 数组的总字节数除以第一个元素的字节数得到元素的个数
            float delay = layer_delay_infos[i].delay;
            int layer_net = layer_delay_infos[i].layer_index;
            int flag = process_delay(delay, layer_net);
            if (flag)
                printf("%-15s", "abnormal data");
        }
    }
    printf("\n");

    return 0;
}

static int print_tcpstate(void *ctx, void *packet_info, size_t size) {
    if (!tcp_info)
        return 0;
    char d_str[INET_ADDRSTRLEN];
    char s_str[INET_ADDRSTRLEN];
    const struct tcp_state *pack_info = packet_info;
    unsigned int saddr = pack_info->saddr;
    unsigned int daddr = pack_info->daddr;
    printf("%-20s %-20s %-20d %-20d %-20s %-20s  %-20lld\n",
           inet_ntop(AF_INET, &saddr, s_str, sizeof(s_str)),
           inet_ntop(AF_INET, &daddr, d_str, sizeof(d_str)), pack_info->sport,
           pack_info->dport, tcp_states[pack_info->oldstate],
           tcp_states[pack_info->newstate], pack_info->time);

    return 0;
}

static int print_kfree(void *ctx, void *packet_info, size_t size) {
    if (!drop_reason)
        return 0;
    char d_str[INET_ADDRSTRLEN];
    char s_str[INET_ADDRSTRLEN];
    const struct reasonissue *pack_info = packet_info;
    unsigned int saddr = pack_info->saddr;
    unsigned int daddr = pack_info->daddr;
    if (saddr == 0 && daddr == 0) {
        return 0;
    }
    char prot[6];
    if (pack_info->protocol == 2048) {
        strcpy(prot, "ipv4");
    } else if (pack_info->protocol == 34525) {
        strcpy(prot, "ipv6");
    } else {
        // 其他协议
        strcpy(prot, "other");
    }
    time_t now = time(NULL);
    struct tm *localTime = localtime(&now);
    printf("%02d:%02d:%02d      %-17s %-17s %-10u %-10u %-10s",
           localTime->tm_hour, localTime->tm_min, localTime->tm_sec,
           inet_ntop(AF_INET, &saddr, s_str, sizeof(s_str)),
           inet_ntop(AF_INET, &daddr, d_str, sizeof(d_str)), pack_info->sport,
           pack_info->dport, prot);
    if (!addr_to_func)
        printf("%-34lx", pack_info->location);
    else {
        struct SymbolEntry data = findfunc(pack_info->location);
        char result[40];
        sprintf(result, "%s+0x%lx", data.name, pack_info->location - data.addr);
        printf("%-34s", result);
    }
    printf("%s\n", SKB_Drop_Reason_Strings[pack_info->drop_reason]);
    return 0;
}

static int print_icmptime(void *ctx, void *packet_info, size_t size) {
    if (!icmp_info)
        return 0;
    char d_str[INET_ADDRSTRLEN];
    char s_str[INET_ADDRSTRLEN];
    const struct icmptime *pack_info = packet_info;
    if (pack_info->icmp_tran_time > MAXTIME) {
        return 0;
    }
    unsigned int saddr = pack_info->saddr;
    unsigned int daddr = pack_info->daddr;
    printf("%-20s %-20s %-20lld %-20d",
           inet_ntop(AF_INET, &saddr, s_str, sizeof(s_str)),
           inet_ntop(AF_INET, &daddr, d_str, sizeof(d_str)),
           pack_info->icmp_tran_time, pack_info->flag);
    if (time_load) {
        int icmp_data = process_delay(pack_info->icmp_tran_time, 9);
        if (icmp_data) {
            printf("%-15s\n", "abnormal data");
        }
    }
    printf("\n");
    return 0;
}
#define MAX_EVENTS 1024

static __u64 rst_count = 0;
static struct reset_event_t event_store[MAX_EVENTS];
static int event_count = 0;

static int print_rst(void *ctx, void *packet_info, size_t size) {
    struct reset_event_t *event = packet_info;

    // 将事件存储到全局存储中
    if (event_count < MAX_EVENTS) {
        memcpy(&event_store[event_count], event, sizeof(struct reset_event_t));
        event_count++;
    }

    rst_count++;
    return 0;
}
void print_stored_events() {
    char s_str[INET_ADDRSTRLEN];
    char d_str[INET_ADDRSTRLEN];

    for (int i = 0; i < event_count; i++) {
        struct reset_event_t *event = &event_store[i];
        unsigned int saddr = event->saddr;
        unsigned int daddr = event->daddr;

        if (event->family == AF_INET) {
            inet_ntop(AF_INET, &saddr, s_str, sizeof(s_str));
            inet_ntop(AF_INET, &daddr, d_str, sizeof(d_str));
            printf("%-20llu %-20s %-20s %-20s %-20u %-20u %-20llu\n",
                   (unsigned long long)event->pid, event->comm, s_str, d_str,
                   event->sport, event->dport,
                   (unsigned long long)event->timestamp);
        } else if (event->family == AF_INET6) {
            char saddr_v6[INET6_ADDRSTRLEN];
            char daddr_v6[INET6_ADDRSTRLEN];
            inet_ntop(AF_INET6, &event->saddr_v6, saddr_v6, sizeof(saddr_v6));
            inet_ntop(AF_INET6, &event->daddr_v6, daddr_v6, sizeof(daddr_v6));
            printf("%-10llu %-16s %-16s %-16s %-8u %-8u %-20llu\n",
                   (unsigned long long)event->pid, event->comm, saddr_v6,
                   daddr_v6, event->sport, event->dport,
                   (unsigned long long)event->timestamp);
        }
    }
}
// 从DNS数据包中提取并打印域名
static void print_domain_name(const unsigned char *data, char *output) {
    const unsigned char *next = data;
    int pos = 0, first = 1;
    // 循环到尾部，标志0
    while (*next != 0) {
        if (!first) {
            output[pos++] = '.'; // 在每个段之前添加点号
        } else {
            first = 0; // 第一个段后清除标志
        }
        int len = *next++; // 下一个段长度

        for (int i = 0; i < len; ++i) {
            output[pos++] = *next++;
        }
    }
    output[pos] = '\0'; // 确保字符串正确结束
}

static int print_dns(void *ctx, void *packet_info, size_t size) {
    if (!packet_info)
        return 0;
    char d_str[INET_ADDRSTRLEN];
    char s_str[INET_ADDRSTRLEN];
    const struct dns_information *pack_info =
        (const struct dns_information *)packet_info; // 强制类型转换
    unsigned int saddr = pack_info->saddr;
    unsigned int daddr = pack_info->daddr;
    char domain_name[256]; // 用于存储输出的域名

    inet_ntop(AF_INET, &saddr, s_str, sizeof(s_str));
    inet_ntop(AF_INET, &daddr, d_str, sizeof(d_str));

    print_domain_name((const unsigned char *)pack_info->data, domain_name);
    if (pack_info->daddr == 0) {
        return 0;
    }
    printf("%-20s %-20s %-#12x %-#12x %-5x %-5x %-5x %-5x %-47s %-10d %-10d "
           "%-10d \n",
           s_str, d_str, pack_info->id, pack_info->flags, pack_info->qdcount,
           pack_info->ancount, pack_info->nscount, pack_info->arcount,
           domain_name, pack_info->request_count, pack_info->response_count,
           pack_info->rx);
    return 0;
}

static int print_mysql(void *ctx, void *packet_info, size_t size) {
    if (!mysql_info) {
        return 0;
    }

    const mysql_query *pack_info = packet_info;
    printf("%-20d %-20d %-20s %-20u %-41s", pack_info->pid, pack_info->tid,
           pack_info->comm, pack_info->size, pack_info->msql);
    // 当 duratime 大于 count_info 时，才打印 duratime
    if (pack_info->duratime > count_info) {
        printf("%-21llu", pack_info->duratime);
    } else {
        printf("%-21s", "");
    }
    printf("%-20d\n", pack_info->count);
    return 0;
}

static int print_redis(void *ctx, void *packet_info, size_t size) {
    const struct redis_query *pack_info = packet_info;
    int i = 0;
    char redis[64];
    for (i = 0; i < pack_info->argc; i++) {
        strcat(redis, pack_info->redis[i]);
        strcat(redis, " ");
    }
    printf("%-20d %-20s %-20d %-20s %-21llu\n", pack_info->pid, pack_info->comm,
           pack_info->argc, redis, pack_info->duratime);
    strcpy(redis, "");
    return 0;
}
static int libbpf_print_fn(enum libbpf_print_level level, const char *format,
                           va_list args) {
    return vfprintf(stderr, format, args);
}
static void show_stack_trace(__u64 *stack, int stack_sz, pid_t pid) {
    int i;
    printf("-----------------------------------\n");
    for (i = 1; i < stack_sz; i++) {
        if (addr_to_func) {
            struct SymbolEntry data = findfunc(stack[i]);
            char result[40];
            sprintf(result, "%s+0x%llx", data.name, stack[i] - data.addr);
            printf("%-10d [<%016llx>]=%s\n", i, stack[i], result);
        } else {
            printf("%-10d [<%016llx>]\n", i, stack[i]);
        }
    }
    printf("-----------------------------------\n");
}
static int print_trace(void *_ctx, void *data, size_t size) {
    struct stacktrace_event *event = data;

    if (event->kstack_sz <= 0 && event->ustack_sz <= 0)
        return 1;

    printf("COMM: %s (pid=%d) @ CPU %d\n", event->comm, event->pid,
           event->cpu_id);

    if (event->kstack_sz > 0) {
        printf("Kernel:\n");
        show_stack_trace(event->kstack, event->kstack_sz / sizeof(__u64), 0);
    } else {
        printf("No Kernel Stack\n");
    }
    printf("\n");
    return 0;
}

static int print_rtt(void *ctx, void *data, size_t size) {
    if (!rtt_info)
        return 0;
    struct RTT *rtt_tuple = data;
    unsigned long long total_latency = 0;
    unsigned long long total_count = 0;
    char d_str[INET_ADDRSTRLEN];
    char s_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &rtt_tuple->saddr, s_str, sizeof(s_str));
    inet_ntop(AF_INET, &rtt_tuple->daddr, d_str, sizeof(d_str));
    if ((rtt_tuple->saddr & 0x0000FFFF) == 0x0000007F ||
        (rtt_tuple->daddr & 0x0000FFFF) == 0x0000007F ||
        rtt_tuple->saddr == htonl(0xC0A83C01) ||
        rtt_tuple->daddr == htonl(0xC0A83C01)) {
        return 0; // 如果匹配任一过滤条件，放弃处理这些数据包
    }
    // 打印源地址和目的地址
    printf("Source Address: %s\n", s_str);
    printf("Destination Address: %s\n", d_str);
    // 更新总延迟和计数
    total_latency += rtt_tuple->latency;
    total_count += rtt_tuple->cnt;

    // 打印总延迟和平均RTT
    double average_rtt =
        (total_count > 0) ? (double)total_latency / total_count : 0;
    printf("Total Latency: %llu μs\n", total_latency);
    printf("Average RTT: %.2f ms\n", average_rtt / 1000.0);

    // 计算和打印RTT分布图
    printf(" usecs               : count     distribution\n");
    int bucket_size = 1;
    for (int i = 0; i < MAX_SLOTS; i++) {
        int start_range = bucket_size == 1 ? 0 : bucket_size;
        int end_range = bucket_size * 2 - 1;
        printf("%8d -> %-8d : %-8llu |", start_range, end_range,
               rtt_tuple->slots[i]);
        int bar_length =
            rtt_tuple->slots[i] /
            10; //计算该延迟范围内的计数对应的直方图条形长度,每个'*'
                //表示 10 个计数
        for (int j = 0; j < bar_length; j++) {
            printf("*");
        }
        printf("\n");
        bucket_size *= 2; //以对数方式扩展
    }

    printf("===============================================================\n");

    return 0;
}

int attach_uprobe_mysql(struct netwatcher_bpf *skel) {

    ATTACH_UPROBE_CHECKED(
        skel, _Z16dispatch_commandP3THDPK8COM_DATA19enum_server_command,
        query__start);
    ATTACH_URETPROBE_CHECKED(
        skel, _Z16dispatch_commandP3THDPK8COM_DATA19enum_server_command,
        query__end);
    return 0;
}
int attach_uprobe_redis(struct netwatcher_bpf *skel) {
    ATTACH_UPROBE_CHECKED(skel, call, query__end_redis);
    ATTACH_UPROBE_CHECKED(skel, processCommand, query__start_redis_process);
    return 0;
}
int main(int argc, char **argv) {
    char *last_slash = strrchr(argv[0], '/');
    if (last_slash) {
        *(last_slash + 1) = '\0';
    }
    strcpy(connects_file_path, argv[0]);
    strcpy(err_file_path, argv[0]);
    strcpy(packets_file_path, argv[0]);
    strcpy(udp_file_path, argv[0]);
    strcat(connects_file_path, "data/connects.log");
    strcat(err_file_path, "data/err.log");
    strcat(packets_file_path, "data/packets.log");
    strcat(udp_file_path, "data/udp.log");
    struct ring_buffer *rb = NULL;
    struct ring_buffer *udp_rb = NULL;
    struct ring_buffer *netfilter_rb = NULL;
    struct ring_buffer *kfree_rb = NULL;
    struct ring_buffer *icmp_rb = NULL;
    struct ring_buffer *tcp_rb = NULL;
    struct ring_buffer *dns_rb = NULL;
    struct ring_buffer *trace_rb = NULL;
    struct ring_buffer *mysql_rb = NULL;
    struct ring_buffer *redis_rb = NULL;
    struct ring_buffer *rtt_rb = NULL;
    struct ring_buffer *events = NULL;
    struct netwatcher_bpf *skel;
    int err;
    /* Parse command line arguments */
    if (argc > 1) {
        err = argp_parse(&argp, argc, argv, 0, NULL, NULL);
        if (err)
            return err;
    }

    libbpf_set_print(libbpf_print_fn);
    /* Cleaner handling of Ctrl-C */
    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    /* Open load and verify BPF application */
    skel = netwatcher_bpf__open();
    if (!skel) {
        fprintf(stderr, "Failed to open BPF skeleton\n");
        return 1;
    }
    /* Parameterize BPF code */
    set_rodata_flags(skel);
    set_disable_load(skel);

    if (addr_to_func)
        readallsym();

    err = netwatcher_bpf__load(skel);
    if (err) {
        fprintf(stderr, "Failed to load and verify BPF skeleton\n");
        goto cleanup;
    }

    /* Attach tracepoint handler */
    if (mysql_info) {
        strcpy(binary_path, "/usr/sbin/mysqld");
        err = attach_uprobe_mysql(skel);
        if (err) {
            fprintf(stderr, "failed to attach uprobes\n");

            goto cleanup;
        }
    } else if (redis_info) {
        strcpy(binary_path, "/usr/bin/redis-server");
        err = attach_uprobe_redis(skel);
        if (err) {
            fprintf(stderr, "failed to attach uprobes\n");

            goto cleanup;
        }
    } else {
        err = netwatcher_bpf__attach(skel);
        if (err) {
            fprintf(stderr, "Failed to attach BPF skeleton\n");
            goto cleanup;
        }
    }
    enum MonitorMode mode = get_monitor_mode();

    // print_logo();

    print_header(mode);

    udp_rb =
        ring_buffer__new(bpf_map__fd(skel->maps.udp_rb), print_udp, NULL, NULL);
    if (!udp_rb) {
        err = -1;
        fprintf(stderr, "Failed to create ring buffer(udp)\n");
        goto cleanup;
    }
    netfilter_rb = ring_buffer__new(bpf_map__fd(skel->maps.netfilter_rb),
                                    print_netfilter, NULL, NULL);
    if (!netfilter_rb) {
        err = -1;
        fprintf(stderr, "Failed to create ring buffer(netfilter)\n");
        goto cleanup;
    }
    kfree_rb = ring_buffer__new(bpf_map__fd(skel->maps.kfree_rb), print_kfree,
                                NULL, NULL);
    if (!kfree_rb) {
        err = -1;
        fprintf(stderr, "Failed to create ring buffer(kfree)\n");
        goto cleanup;
    }
    icmp_rb = ring_buffer__new(bpf_map__fd(skel->maps.icmp_rb), print_icmptime,
                               NULL, NULL);
    if (!icmp_rb) {
        err = -1;
        fprintf(stderr, "Failed to create ring buffer(icmp)\n");
        goto cleanup;
    }
    tcp_rb = ring_buffer__new(bpf_map__fd(skel->maps.tcp_rb), print_tcpstate,
                              NULL, NULL);
    if (!tcp_rb) {
        err = -1;
        fprintf(stderr, "Failed to create ring buffer(tcp)\n");
        goto cleanup;
    }
    dns_rb =
        ring_buffer__new(bpf_map__fd(skel->maps.dns_rb), print_dns, NULL, NULL);
    if (!dns_rb) {
        err = -1;
        fprintf(stderr, "Failed to create ring buffer(dns)\n");
        goto cleanup;
    }
    trace_rb = ring_buffer__new(bpf_map__fd(skel->maps.trace_rb), print_trace,
                                NULL, NULL);
    if (!trace_rb) {
        err = -1;
        fprintf(stderr, "Failed to create ring buffer(trace)\n");
        goto cleanup;
    }
    mysql_rb = ring_buffer__new(bpf_map__fd(skel->maps.mysql_rb), print_mysql,
                                NULL, NULL);
    if (!mysql_rb) {
        err = -1;
        fprintf(stderr, "Failed to create ring buffer(trace)\n");
        goto cleanup;
    }
    redis_rb = ring_buffer__new(bpf_map__fd(skel->maps.redis_rb), print_redis,
                                NULL, NULL);
    if (!redis_rb) {
        err = -1;
        fprintf(stderr, "Failed to create ring buffer(trace)\n");
        goto cleanup;
    }
    rtt_rb =
        ring_buffer__new(bpf_map__fd(skel->maps.rtt_rb), print_rtt, NULL, NULL);
    if (!rtt_rb) {
        err = -1;
        fprintf(stderr, "Failed to create ring buffer(connect_rb)\n");
        goto cleanup;
    }
    events =
        ring_buffer__new(bpf_map__fd(skel->maps.events), print_rst, NULL, NULL);
    if (!events) {
        err = -1;
        fprintf(stderr, "Failed to create ring buffer(rst_rb)\n");
        goto cleanup;
    }
    /* Set up ring buffer polling */
    rb = ring_buffer__new(bpf_map__fd(skel->maps.rb), print_packet, NULL, NULL);
    if (!rb) {
        err = -1;
        fprintf(stderr, "Failed to create ring buffer(packet)\n");
        goto cleanup;
    }

    open_log_files();
    struct timeval start, end;
    gettimeofday(&start, NULL);
    /* Process events */
    while (!exiting) {
        err = ring_buffer__poll(rb, 100 /* timeout, ms */);
        err = ring_buffer__poll(udp_rb, 100 /* timeout, ms */);
        err = ring_buffer__poll(netfilter_rb, 100 /* timeout, ms */);
        err = ring_buffer__poll(kfree_rb, 100 /* timeout, ms */);
        err = ring_buffer__poll(icmp_rb, 100 /* timeout, ms */);
        err = ring_buffer__poll(tcp_rb, 100 /* timeout, ms */);
        err = ring_buffer__poll(dns_rb, 100 /* timeout, ms */);
        err = ring_buffer__poll(trace_rb, 100 /* timeout, ms */);
        err = ring_buffer__poll(mysql_rb, 100 /* timeout, ms */);
        err = ring_buffer__poll(redis_rb, 100 /* timeout, ms */);
        err = ring_buffer__poll(rtt_rb, 100 /* timeout, ms */);
        err = ring_buffer__poll(events, 100 /* timeout, ms */);
        print_conns(skel);
        sleep(1);
        /* Ctrl-C will cause -EINTR */
        if (err == -EINTR) {
            err = 0;
            break;
        }
        if (err < 0) {
            printf("Error polling perf buffer: %d\n", err);
            break;
        }

        gettimeofday(&end, NULL);
        if ((end.tv_sec - start.tv_sec) >= 5) {
            print_stored_events();
            printf("Total RSTs in the last 5 seconds: %llu\n\n", rst_count);

            // 重置计数器和事件存储
            rst_count = 0;
            event_count = 0;
            gettimeofday(&start, NULL);
        }
    }

cleanup:
    netwatcher_bpf__destroy(skel);
    return err < 0 ? -err : 0;
}
