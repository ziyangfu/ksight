#ifndef NET_TCPSYNBL_BPF_H
#define NET_TCPSYNBL_BPF_H

#include "ConfigArgs.h"
#include "tcpsynbl.h"

struct tcpsynbl_bpf;

namespace net {
namespace tcpsynbl {

class TcpsynblBpf final {
public:
    explicit TcpsynblBpf(const ConfigArgs& config);
    ~TcpsynblBpf();

    TcpsynblBpf(const TcpsynblBpf&) = delete;
    TcpsynblBpf& operator=(const TcpsynblBpf&) = delete;

    void init();
    void load();
    void attach();
    void run();
    void destroy();

private:
    void disableAllProgs();
    void setAutoloadProg(int version);
    void printHistograms();

    const ConfigArgs& config_;
    struct tcpsynbl_bpf *skel_ {nullptr};
    int map_fd_ {-1};
};

} // namespace tcpsynbl
} // namespace net

#endif // NET_TCPSYNBL_BPF_H
