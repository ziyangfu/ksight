#ifndef NET_GET_HOST_LATENCY_BPF_H
#define NET_GET_HOST_LATENCY_BPF_H

#include <string>
#include <memory>
#include <vector>
#include "ConfigArgs.h"
#include "gethostlatency.h"

// Forward declaration
struct gethostlatency_bpf;
struct perf_buffer;
struct bpf_link;

namespace net::getHostLatency {

class GethostlatencyBpf final {
public:
    explicit GethostlatencyBpf(const ConfigArgs& config);
    ~GethostlatencyBpf();

    GethostlatencyBpf(const GethostlatencyBpf&) = delete;
    GethostlatencyBpf& operator=(const GethostlatencyBpf&) = delete;

    void init();
    void load();
    void attach();
    void poll();
    void destroy();

private:
    static void handleEvent(void *ctx, int cpu, void *data, unsigned int data_sz);
    static void handleLostEvents(void *ctx, int cpu, unsigned long long lost_cnt);
    void processEvent(const struct event *e);

    int getLibcPath(char *path, size_t path_sz);
    void attachUprobes();

    const ConfigArgs& config_;
    struct gethostlatency_bpf *skel_ {nullptr};
    struct perf_buffer *pb_ {nullptr};
    std::vector<struct bpf_link*> links_;
};

} // namespace net::getHostLatency

#endif // NET_GET_HOST_LATENCY_BPF_H
