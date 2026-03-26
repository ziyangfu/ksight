#ifndef NET_TCPCONNLAT_BPF_H
#define NET_TCPCONNLAT_BPF_H

#include "ConfigArgs.h"
#include "tcpconnlat.h"

struct tcpconnlat_bpf;
struct perf_buffer;

namespace net {
namespace tcpconnlat {

class TcpconnlatBpf final {
public:
    explicit TcpconnlatBpf(const ConfigArgs& config);
    ~TcpconnlatBpf();

    TcpconnlatBpf(const TcpconnlatBpf&) = delete;
    TcpconnlatBpf& operator=(const TcpconnlatBpf&) = delete;

    void init();
    void load();
    void attach();
    void poll();
    void destroy();

private:
    static void handleEvent(void *ctx, int cpu, void *data, unsigned int data_sz);
    static void handleLostEvents(void *ctx, int cpu, unsigned long long lost_cnt);
    void processEvent(const struct event *e);

    const ConfigArgs& config_;
    struct tcpconnlat_bpf *skel_ {nullptr};
    struct perf_buffer *pb_ {nullptr};
    unsigned long long start_ts_ {0};
};

} // namespace tcpconnlat
} // namespace net

#endif // NET_TCPCONNLAT_BPF_H
