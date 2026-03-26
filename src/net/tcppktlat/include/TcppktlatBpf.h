#ifndef NET_TCPPKTLAT_BPF_H
#define NET_TCPPKTLAT_BPF_H

#include "ConfigArgs.h"
#include "tcppktlat.h"

struct tcppktlat_bpf;
struct bpf_buffer;

namespace net {
namespace tcppktlat {

class TcppktlatBpf final {
public:
    explicit TcppktlatBpf(const ConfigArgs& config);
    ~TcppktlatBpf();

    TcppktlatBpf(const TcppktlatBpf&) = delete;
    TcppktlatBpf& operator=(const TcppktlatBpf&) = delete;

    void init();
    void load();
    void attach();
    void poll();
    void destroy();

private:
    static int handleEvent(void *ctx, void *data, size_t data_sz);
    static void handleLostEvents(void *ctx, int cpu, unsigned long long lost_cnt);
    void processEvent(const struct event *e);

    const ConfigArgs& config_;
    struct tcppktlat_bpf *skel_ {nullptr};
    struct bpf_buffer *buf_ {nullptr};
};

} // namespace tcppktlat
} // namespace net

#endif // NET_TCPPKTLAT_BPF_H
