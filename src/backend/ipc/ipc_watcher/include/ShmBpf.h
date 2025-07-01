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

    int shmMonitorFd_;
    std::string shmMonitorPath_;
    int shmMonitorSize_;
    int* shmMonitorAddr_;

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
    /*!
     * \brief 轮询 cat /proc/{pid}/fd/{fd}的数据，这是共享内存写入与读取的payload
     *        识别到数据改变时，输出一次。
     * */
    static std::string readShmPayloadCycle(int pid, int fd);

    static void handleEvent(void *ctx, void *data, size_t len);
    static std::string pidToCommand(std::uint32_t pid);
    std::string findCommand(std::uint32_t pid);
    static void handleCommand(std::string& command);

    static std::string getShmPath(int pid, int fd);  /** 仅针对非匿名 文件共享映射 */
    static std::vector<unsigned long> getShmVmAddr(int pid, std::string& shmPath); /** 起始地址与结束地址 */
    static std::string getShmVmAddrString(int pid, std::string& shmPath);
    static std::string getShmProtString(unsigned long prot);
    static std::string getShmFlagString(unsigned long flag);

    static std::string toHex(const char* data, size_t len);
    static std::string hexToString(const char* data, size_t len);


    void setAndPrintHeader(FormatType type);
};

} // namespace ipc::ipcWatcher

#endif //IPC_IPC_WATCHER_SHM_BPF_H

