#ifndef NET_SO_LISTEN_BPF_H
#define NET_SO_LISTEN_BPF_H

#include <string>
#include <memory>
#include <vector>
#include "ConfigArgs.h"
#include "solisten.h"

// Forward declaration
struct solisten_bpf;
struct perf_buffer;

namespace net::soListen {

class SolistenBpf final {
public:
    explicit SolistenBpf(const ConfigArgs& config);
    ~SolistenBpf();

    SolistenBpf(const SolistenBpf&) = delete;
    SolistenBpf& operator=(const SolistenBpf&) = delete;

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
    struct solisten_bpf *skel_ {nullptr};
    struct perf_buffer *pb_ {nullptr};
};

} // namespace net::soListen

#endif // NET_SO_LISTEN_BPF_H
