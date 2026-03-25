#ifndef NET_TCP_CONNECT_BPF_H
#define NET_TCP_CONNECT_BPF_H

#include <string>
#include <memory>
#include <vector>
#include "ConfigArgs.h"
#include "tcpconnect.h"

// Forward declaration
struct tcpconnect_bpf;
struct perf_buffer;

namespace net::tcpConnect {

class TcpconnectBpf final {
public:
    explicit TcpconnectBpf(const ConfigArgs& config);
    ~TcpconnectBpf();

    TcpconnectBpf(const TcpconnectBpf&) = delete;
    TcpconnectBpf& operator=(const TcpconnectBpf&) = delete;

    void init();
    void load();
    void attach();
    void poll();
    void destroy();

private:
    static void handleEvent(void *ctx, int cpu, void *data, unsigned int data_sz);
    static void handleLostEvents(void *ctx, int cpu, unsigned long long lost_cnt);
    void processEvent(const struct event *e);

    void printCount();
    void printCountIpv4(int map_fd);
    void printCountIpv6(int map_fd);

    const ConfigArgs& config_;
    struct tcpconnect_bpf *skel_ {nullptr};
    struct perf_buffer *pb_ {nullptr};
    uint64_t start_ts_ {0};
};

} // namespace net::tcpConnect

#endif // NET_TCP_CONNECT_BPF_H
