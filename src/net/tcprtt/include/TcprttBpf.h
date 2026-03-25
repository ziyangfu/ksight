#ifndef NET_TCP_RTT_BPF_H
#define NET_TCP_RTT_BPF_H

#include <string>
#include <memory>
#include <vector>
#include "ConfigArgs.h"
#include "tcprtt.h"

// Forward declaration
struct tcprtt_bpf;
struct bpf_map;

namespace net::tcpRtt {

class TcprttBpf final {
public:
    explicit TcprttBpf(const ConfigArgs& config);
    ~TcprttBpf();

    TcprttBpf(const TcprttBpf&) = delete;
    TcprttBpf& operator=(const TcprttBpf&) = delete;

    void init();
    void load();
    void attach();
    void poll();
    void destroy();

private:
    void printMap(struct bpf_map *map);

    const ConfigArgs& config_;
    struct tcprtt_bpf *skel_ {nullptr};
};

} // namespace net::tcpRtt

#endif // NET_TCP_RTT_BPF_H
