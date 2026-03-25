#ifndef NET_TCP_TOP_BPF_H
#define NET_TCP_TOP_BPF_H

#include <string>
#include <memory>
#include <vector>
#include "ConfigArgs.h"
#include "tcptop.h"

// Forward declaration
struct tcptop_bpf;

namespace net::tcpTop {

struct Info {
    struct ip_key_t key;
    struct traffic_t value;
};

class TcptopBpf final {
public:
    explicit TcptopBpf(const ConfigArgs& config);
    ~TcptopBpf();

    TcptopBpf(const TcptopBpf&) = delete;
    TcptopBpf& operator=(const TcptopBpf&) = delete;

    void init();
    void load();
    void attach();
    void poll();
    void destroy();

private:
    void printStat();
    static int sortColumn(const void *obj1, const void *obj2, SortBy sort_by);

    const ConfigArgs& config_;
    struct tcptop_bpf *skel_ {nullptr};
    int cgfd_ {-1};
    uint32_t current_count_ {0};
};

} // namespace net::tcpTop

#endif // NET_TCP_TOP_BPF_H
