#ifndef NET_BIND_SNOOP_BPF_H
#define NET_BIND_SNOOP_BPF_H

#include <string>
#include <memory>
#include <vector>
#include "ConfigArgs.h"
#include "bindsnoop.h"

// Forward declaration of the skeleton
struct bindsnoop_bpf;
struct perf_buffer;

namespace net::bindSnoop {

class BindsnoopBpf final {
public:
    explicit BindsnoopBpf(const ConfigArgs& config);
    ~BindsnoopBpf();

    // Disable copy
    BindsnoopBpf(const BindsnoopBpf&) = delete;
    BindsnoopBpf& operator=(const BindsnoopBpf&) = delete;

    void init();
    void load();
    void attach();
    void poll();
    void destroy();

private:
    static void handleEvent(void *ctx, int cpu, void *data, unsigned int data_sz);
    static void handleLostEvents(void *ctx, int cpu, unsigned long long lost_cnt);
    void processEvent(const struct bind_event *e);

    const ConfigArgs& config_;
    struct bindsnoop_bpf *skel_ {nullptr};
    struct perf_buffer *pb_ {nullptr};
    volatile bool exiting_ {false};
};

} // namespace net::bindSnoop

#endif // NET_BIND_SNOOP_BPF_H
