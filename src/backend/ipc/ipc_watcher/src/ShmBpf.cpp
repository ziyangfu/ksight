#include <filesystem>
#include <fstream>

#include <sys/mman.h>

#include "ShmBpf.h"
#include "ipcwatcher.h"
#include "ShmUtils.h"

#include "spdlog/spdlog.h"
#include "fmt/format.h"

using namespace ipc::ipcWatcher;

ShmBpf::ShmBpf(ConfigArgs& config)
        : config_(config),
          skel_(nullptr),
        //pb(nullptr),
          rb_(nullptr),
          formatHeader(),
          pidCommandHash_(std::make_unique<std::unordered_map<std::uint32_t, std::string>>()),
          type_(FormatType::kPrintTest),
          printType_(PrintType::kTerminal)
{
    //config_.printPayloadHex = true;
}

ShmBpf::~ShmBpf() {
    destroy();
}

/*!
 * \brief 打开BPF程序
 * */
void ShmBpf::open() {
    skel_ = shm_bpf::open();
    if (!skel_) {
        SPDLOG_ERROR("Failed to open and load BPF skeleton");
    }

}

/*!
 * \brief 加载BPF程序
 * */
void ShmBpf::load() {
    int err = shm_bpf::load(skel_);
    if (err) {
        SPDLOG_ERROR("Failed to load BPF program, err：{}", err);
        destroy();
    }
}

/*!
 * \brief 加载并验证BPF程序
 * */
void ShmBpf::openAndLoad() {
    skel_ = shm_bpf::open_and_load();
    if (!skel_) {
        SPDLOG_ERROR("Failed to load and verify BPF skeleton");
        //return 1;
    }
}

/*!
 * \brief 附加kprobe等事件
 * */
void ShmBpf::attach() {
    int err = shm_bpf::attach(skel_);
    if (err) {
        SPDLOG_ERROR("Failed to attach BPF program, err：{}", err);
        destroy();
    }
}

/*!
 * \brief 选择部分事件，独立挂载
 * \details 是否挂载，依据传递的ConfigArgs
 * */
void ShmBpf::setBpfProgsLoadOpt() {
    bpf_program__set_autoload(skel_->progs.handle_syscall_enter_mmap, true);
    bpf_program__set_autoload(skel_->progs.handle_syscall_exit_mmap, true);
}

void ShmBpf::setRodataFlags() {
    fmt::print("config_.pid val = {}", config_.pid);
    skel_->rodata->send_pid = config_.pid;
}



/*!
 * \brief 接收BPF采集的数据，并调用处理函数进行处理
 * */
void ShmBpf::poll() {
    // 设置 ringbuffer 回调
    rb_ = ring_buffer__new(bpf_map__fd(skel_->maps.shm_events),
                           reinterpret_cast<ring_buffer_sample_fn>(ShmBpf::handleEvent),
                           this, nullptr);
    if (!rb_) {
        SPDLOG_ERROR("Failed to create uds ring buffer");
        destroy();
    }

    int err;
    fmt::print("Tracing POSIX shared memory com ... Ctrl+C to exit\n");
    setAndPrintHeader(type_);
// 4. 轮询事件
    while (true) {
        //err = perf_buffer__poll(pb, 100 /* timeout_ms */);
        err = ring_buffer__poll(rb_, kPollPeriodMs);
        if (err == -EINTR) {
            err = 0;
            break;
        }
        if (err < 0) {
            SPDLOG_ERROR("Error polling ring buffer: {}", err);
            break;
        }
    }
}

/*!
 * \brief 清理释放资源
 * */
void ShmBpf::destroy() {
    ring_buffer__free(rb_);
    //perf_buffer__free(pb);
    shm_bpf::destroy(skel_);
}

/*!
 * \brief 根据选项要求，设置并打印头部信息
 * 货架信息： ts, PID, command, fd, shm_flag, shm_prot, shm_path, shm_size,
 *          shm_ret_addr, shm_vm_addr_area, shm_phy_addr_area, phy_mem_count(物理内存进程引用计数)
 *          （shm_name，从shm_open或者open，或者memfd_create中的）
 * 基本信息打印：
 * 物理内存映射的多进程图表展示：
 * 共享内存的内存泄露检测
 * 共享内存的运行脉络打印
 */
void ShmBpf::setAndPrintHeader(FormatType type) {
    type_ = type;
    switch (type) {
        case FormatType::kMmapPrintNormal: {
            formatHeader = "{:<15} {:<14} {:<20} {:<20} {:<20} {:<20} {:<20} {:<20}\n";
            fmt::print(formatHeader, "timestamp", "PID", "command", "shm_addr",
                       "shm_size", "shm_flag", "shm_prot", "shm_path");
            break;
        }
        /** 物理内存引用计数输出 */
        case FormatType::kPhyAddrPrint: {
            formatHeader = "{:<25} {:<10} {:<35} {:<35}\n";
            fmt::print(formatHeader, "physical_addr", "map_count", "pids", "command");
            break;
        }
        case FormatType::kPhyAddrPrint2: {
            formatHeader = "{:<25} {:<10} {:<35} {:<10} {:<35}\n";
            fmt::print(formatHeader, "timestamp", "PID", "vm_addr", "len", "phy_addr");
            break;
        }
        /** 物理内存引用计数在命令行中类图形化输出 */
        case FormatType::kPhyAddrPrintGui: {
            fmt::print("print with CLI GUI\n");
            break;
        }
        /** 内存泄露检测 */
        case FormatType::kShmLeakCheck: {
            fmt::print("print shared memory leak in system\n");
            break;
        }
        case FormatType::kPrintTest: {
            formatHeader = "{:<15} {:<10} {:<12} {:<5} {:<8} {:<12} {:<25} {:<25} {:<35}\n";
            fmt::print(formatHeader, "PID", "command","len", "prot", "flags",
                       "fd", "shm_path", "mmap_addr","shm_vm_addr");
            break;
        }
        case FormatType::kPrintTest2: {
            formatHeader = "{:<15} {:<6} {:<10} {:<20} {:<20} {:<12} {:<20} {:<25}\n";
            fmt::print(formatHeader, "timestamp", "PID", "len", "prot", "flags",
                       "fd", "off", "mmap_addr");
            break;
        }
        default:
            break;
    }
}

/** static */ void ShmBpf::handleEvent(void *ctx, void *data, size_t len) {
    auto shmBpf = reinterpret_cast<ShmBpf*>(ctx);
    auto *e = reinterpret_cast<shm_transfer_basic_data*>(data);

    std::string shmPath = shmUtils::getShmPath(e->pid, e->fd);
    std::string shmAddr = shmUtils::getShmVmAddrString(e->pid, shmPath);
    std::string command = shmBpf->findCommand(e->pid);
    if (shmBpf->type_ == FormatType::kMmapPrintNormal) {
        //fmt::print(shmBpf->formatHeader,   e->pid, );
    }
//    fmt::print(formatHeader, "PID", "command","len", "prot", "flags",
//               "fd", "shm_path", "mmap_addr","shm_vm_addr");
    else if (shmBpf->type_ == FormatType::kPrintTest) {
        fmt::print(shmBpf->formatHeader,    e->pid,
                                            command,
                                            e->len,
                                            shmUtils::getShmFlagString(e->prot),
                                            shmUtils::getShmProtString(e->flags),
                                            e->fd,
                                            shmPath,
                                            e->mmap_addr,
                                            shmAddr
                   );
    }

    else if (shmBpf->type_ == FormatType::kPrintTest2) {
        // 验证 fd 值是否在有效范围内 (0-1023)
        std::string fd_str = (e->fd < 1024) ? std::to_string(e->fd) : "BB";
        fmt::print(shmBpf->formatHeader,    e->timestamp,
                                            e->pid,
                                            e->len,
                                            shmUtils::getShmProtString(e->prot),
                                            shmUtils::getShmFlagString(e->flags),
                                            fd_str,
                                            e->off,
                                            e->mmap_addr
        );
     }
}

/*!
 * \brief 根据pid查找进程名
 * \details 为了加速，避免频繁读取，若哈希表中有，则直接从哈希表中获取，若没有，则从文件系统中获取，并存入哈希表
 * */
std::string ShmBpf::findCommand(std::uint32_t pid) {
    auto it = pidCommandHash_->find(pid);
    if (it != pidCommandHash_->end()) {
        return it->second;
    }
    else {
        std::string cmd = shmUtils::pidToCommand(pid);
        shmUtils::handleCommand(cmd);
        pidCommandHash_->emplace(pid, cmd);
        return cmd;
    }
    /*
     * 使用 try_emplace:
            try_emplace 会在插入时直接构造元素，并返回一个 std::pair，指示插入是否成功以及元素的位置。
            如果元素已经存在，inserted 为 false，it 指向已存在的元素。
            如果元素不存在，inserted 为 true，it 指向新插入的元素
      但不合适，还是会每次调用pidToCommand， 不符合加速要求
    auto [it, inserted] = pidCommandHash_->try_emplace(pid, pidToCommand(pid));
    if (inserted) {
        SPDLOG_DEBUG("Inserted new command for PID {}: {}", pid, it->second);
    }
    return it->second;
    */
}


void ShmBpf::createMmapMonitor() {
    int fd = ::shm_open(shmMonitorPath_.c_str(), O_CREAT | O_RDWR, 0666);
    if (fd == -1) {
        SPDLOG_ERROR("Failed to create shared memory: {}", strerror(errno));
    }
    if (::ftruncate(fd, shmMonitorSize_) == -1) {
        SPDLOG_ERROR("Failed to truncate shared memory: {}", strerror(errno));
    }
    shmMonitorAddr_ = static_cast<int*>(
            ::mmap(nullptr, shmMonitorSize_, PROT_READ, MAP_SHARED, shmMonitorFd_, 0));
}


void ShmBpf::printShmConnectInfo() {

}
