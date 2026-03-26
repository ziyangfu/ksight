#ifndef NET_TCP_STATES_BPF_H
#define NET_TCP_STATES_BPF_H

#include "ConfigArgs.h"
#include "tcpstates.h"

struct tcpstates_bpf;
struct perf_buffer;

namespace net {
namespace tcpStates {

class TcpstatesBpf final {
public:
    explicit TcpstatesBpf(const ConfigArgs& config);
    ~TcpstatesBpf();

    TcpstatesBpf(const TcpstatesBpf&) = delete;
    TcpstatesBpf& operator=(const TcpstatesBpf&) = delete;

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
    struct tcpstates_bpf *skel_ {nullptr};
    struct perf_buffer *pb_ {nullptr};
};

} // namespace tcpStates
} // namespace net

#endif // NET_TCP_STATES_BPF_H
