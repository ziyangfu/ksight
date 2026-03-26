#ifndef NET_TCPTRACER_BPF_H
#define NET_TCPTRACER_BPF_H

#include "ConfigArgs.h"
#include "tcptracer.h"

struct tcptracer_bpf;
struct perf_buffer;

namespace net {
namespace tcptracer {

class TcptracerBpf final {
public:
    explicit TcptracerBpf(const ConfigArgs& config);
    ~TcptracerBpf();

    TcptracerBpf(const TcptracerBpf&) = delete;
    TcptracerBpf& operator=(const TcptracerBpf&) = delete;

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
    struct tcptracer_bpf *skel_ {nullptr};
    struct perf_buffer *pb_ {nullptr};
    unsigned long long start_ts_ {0};
};

} // namespace tcptracer
} // namespace net

#endif // NET_TCPTRACER_BPF_H
