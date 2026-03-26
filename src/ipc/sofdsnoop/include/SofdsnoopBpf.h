#ifndef IPC_SOFDSNOOP_BPF_H
#define IPC_SOFDSNOOP_BPF_H

#include "ConfigArgs.h"
#include "sofdsnoop.h"

struct sofdsnoop_bpf;
struct perf_buffer;

namespace ipc {
namespace sofdsnoop {

class SofdsnoopBpf final {
public:
    explicit SofdsnoopBpf(const ConfigArgs& config);
    ~SofdsnoopBpf();

    SofdsnoopBpf(const SofdsnoopBpf&) = delete;
    SofdsnoopBpf& operator=(const SofdsnoopBpf&) = delete;

    void init();
    void load();
    void attach();
    void poll();
    void destroy();

private:
    static void handleEvent(void *ctx, int cpu, void *data, unsigned int data_sz);
    static void handleLostEvents(void *ctx, int cpu, unsigned long long lost_cnt);
    void processEvent(const struct event *e);

    // 通过 /proc/<tid>/fd/<fd> 解析文件名
    static std::string getFileName(uint32_t tid, int fd);

    const ConfigArgs& config_;
    struct sofdsnoop_bpf *skel_ {nullptr};
    struct perf_buffer *pb_ {nullptr};
    unsigned long long start_ts_ {0};
};

} // namespace sofdsnoop
} // namespace ipc

#endif // IPC_SOFDSNOOP_BPF_H
