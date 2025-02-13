//
// Created by fzy on 2025/2/11.
//

#ifndef IPC_IPC_WATCHER_UDS_BPF_H
#define IPC_IPC_WATCHER_UDS_BPF_H

#include <string>
extern "C" {
#include "ipc/ipcwatcher/uds.skel.h"
}

namespace ipc::ipcWatcher {

class UdsBpf final {
public:
    UdsBpf();
    ~UdsBpf();
    void open();
    void load();
    void openAndLoad();
    void attach();
    void destroy();

    void setRodataFlags(int value) {
        skel->rodata->filter_is_exist_path = value;
    }
    void setBpfProgsLoadOpt();

    void printHeader();
    void poll();
private:
    static void handleEvent(void *ctx, void *data, size_t len);
    std::string pidToCommand(pid_t pid);
private:
    uds_bpf *skel;
    //perf_buffer *pb;
    ring_buffer *rb;
    std::string data;
    const int kPollPeriodMs {200};
    static const std::string formatHeader;
    static const std::string formatHeaderNotPayload;
    /** struct ArgsOpt 从构造函数中传入命令行的所有参数， 供类内使用 */

};

} // namespace ipc::ipcWatcher

#endif //IPC_IPC_WATCHER_UDS_BPF_H
