#ifndef NET_NET_WATCHER_BPF_H
#define NET_NET_WATCHER_BPF_H

#include <string>
#include <memory>
#include "ConfigArgs.h"
#include "netwatcher/include/netwatcher.h"

extern "C" {
#include "net/netwatcher/netwatcher.skel.h"
}

namespace net::netWatcher {

class NetWatcherBpf final {
public:
    enum class MonitorMode {
        MODE_UDP,
        MODE_NET_FILTER,
        MODE_DROP_REASON,
        MODE_ICMP,
        MODE_TCP,
        MODE_DNS,
        MODE_RTT,
        MODE_RST,
        MODE_DEFAULT
    };

private:
    ConfigArgs& config_;
    struct netwatcher_bpf *skel_;
    struct ring_buffer *rb_;
    const int kPollPeriodMs {200};

public:
    explicit NetWatcherBpf(ConfigArgs& config);
    ~NetWatcherBpf();

    void open();
    void load();
    void attach();
    void destroy();

    void setRodataFlags();
    void setBpfProgsLoadOpt();
    void poll();
    void printConns();
    void printLogo();

    MonitorMode getMonitorMode() const;
    void printHeader(MonitorMode mode) const;

private:
    static void handleEvent(void *ctx, void *data, size_t len);
    void processEvent(void *data, size_t len);
    void attachUprobeMysql();
    void attachUprobeRedis();
};

} // namespace net::netWatcher

#endif // NET_NET_WATCHER_BPF_H
