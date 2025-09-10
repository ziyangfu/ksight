/*!
 * \brief shm共享内存BPF生成代码的封装
 * */

#ifndef IPC_IPC_WATCHER_SHM_BPF_H
#define IPC_IPC_WATCHER_SHM_BPF_H

#include <string>
#include <unordered_map>
#include <memory>
#include <vector>
#include <optional>
#include "ConfigArgs.h"
#include "ipcwatcher.h"
extern "C" {
#include "ipc/ipcwatcher/shm.skel.h"
}

namespace ipc::ipcWatcher {

class ShmBpf final {
private:
    enum class FormatType : std::uint8_t {
        kMmapPrintNormal = 0,
        kPrintTest,
        kPrintTest2,
        kReserve,
    };
    enum class PrintType : std::uint8_t {
        kTerminal = 0,
        kPcap,
        kOther
    };
private:
    ConfigArgs& config_;
    shm_bpf *skel_;
    //perf_buffer *pb;
    ring_buffer *rb_;
    std::string data_;
    const int kPollPeriodMs {200};
    FormatType type_;
    PrintType printType_;

    std::string formatHeader;
    std::string formatHeaderVars;
    std::unique_ptr<std::unordered_map<std::uint32_t, std::string>> pidCommandHash_;

    int shmMonitorFd_{0};
    std::string shmMonitorPath_{};
    int shmMonitorSize_{0};
    int* shmMonitorAddr_{nullptr};

public:
    explicit ShmBpf(ConfigArgs& config);
    ~ShmBpf();
    void open();
    void load();
    void openAndLoad();
    void attach();
    void destroy();
    void setRodataFlags();
    void setBpfProgsLoadOpt();
    void poll();
private:
    void createMmapMonitor();
    static void handleEvent(void *ctx, void *data, size_t len);

    std::string findCommand(std::uint32_t pid);
    void setAndPrintHeader(FormatType type);

    /*!
     * \brief 展示共享内存连接信息
     * 例如：  (client) pid --> vaddr  -------->> pfn(physical addr) <<------------- vaddr --> pid (server)
     *  pid， fd， vm_addr...                              pid， fd， vm_addr
                   \                               /
                    \_______ page cache addr  ____/
                    /                             \
 pid， fd， vm_addr /                               \   pid， fd， vm_addr
     * */
    void printShmConnectInfo();
};

} // namespace ipc::ipcWatcher

#endif //IPC_IPC_WATCHER_SHM_BPF_H

