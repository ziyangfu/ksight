#ifndef NET_TCPLIFE_BPF_H
#define NET_TCPLIFE_BPF_H

#include "ConfigArgs.h"
#include "tcplife.h"

struct tcplife_bpf;
struct perf_buffer;

namespace net {
namespace tcplife {

class TcplifeBpf final {
public:
    explicit TcplifeBpf(const ConfigArgs& config);
    ~TcplifeBpf();

    TcplifeBpf(const TcplifeBpf&) = delete;
    TcplifeBpf& operator=(const TcplifeBpf&) = delete;

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
    struct tcplife_bpf *skel_ {nullptr};
    struct perf_buffer *pb_ {nullptr};
};

} // namespace tcplife
} // namespace net

#endif // NET_TCPLIFE_BPF_H
