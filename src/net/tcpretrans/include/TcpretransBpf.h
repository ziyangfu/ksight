#ifndef NET_TCPRETRANS_BPF_H
#define NET_TCPRETRANS_BPF_H

#include "ConfigArgs.h"
#include "tcpretrans.h"

struct tcpretrans_bpf;
struct perf_buffer;

namespace net {
namespace tcpretrans {

class TcpretransBpf final {
public:
    explicit TcpretransBpf(const ConfigArgs& config);
    ~TcpretransBpf();

    TcpretransBpf(const TcpretransBpf&) = delete;
    TcpretransBpf& operator=(const TcpretransBpf&) = delete;

    void init();
    void load();
    void attach();
    void poll();
    void destroy();
    void printCounts();

private:
    static void handleEvent(void *ctx, int cpu, void *data, unsigned int data_sz);
    static void handleLostEvents(void *ctx, int cpu, unsigned long long lost_cnt);
    void processEvent(const struct event *e);

    const ConfigArgs& config_;
    struct tcpretrans_bpf *skel_ {nullptr};
    struct perf_buffer *pb_ {nullptr};
};

} // namespace tcpretrans
} // namespace net

#endif // NET_TCPRETRANS_BPF_H
