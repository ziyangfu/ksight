#include <filesystem>
#include <fstream>

#include <sys/mman.h>

#include "ShmBpf.h"
#include "ipcwatcher.h"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

using namespace ipc::ipcWatcher;
namespace fs = std::filesystem;

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
 * \brief 根据选项要求，设置并打印头部信息
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
        case FormatType::kPrintTest: {
            formatHeader = "{:<15} {:<10} {:<12} {:<5} {:<8} {:<12} {:<25} {:<25} {:<35}\n";
            fmt::print(formatHeader, "timestamp", "PID", "command", "fd",
                       "shm_size", "shm_flag", "shm_prot", "shm_path", "shm_vm_addr");
            break;
        }
        default:
            break;
    }
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

/** static */ void ShmBpf::handleEvent(void *ctx, void *data, size_t len) {
    auto shmBpf = reinterpret_cast<ShmBpf*>(ctx);
    auto *e = reinterpret_cast<shm_basic_info_event*>(data);

    std::string shmPath = getShmPath(e->pid, e->fd);
    std::string shmAddr = getShmVmAddrString(e->pid, shmPath);
    if (shmBpf->type_ == FormatType::kMmapPrintNormal) {
        //fmt::print(shmBpf->formatHeader,   e->pid, );
    }
//    "timestamp", "PID", "command", "fd",
//            "shm_size", "shm_flag", "shm_prot"
    else if (shmBpf->type_ == FormatType::kPrintTest) {
        fmt::print(shmBpf->formatHeader,    e->timestamp,
                                            e->pid,
                                            e->comm,
                                            e->fd,
                                            e->len,
                                            getShmFlagString(e->flag),
                                            getShmProtString(e->prot),
                                            shmPath,
                                            shmAddr
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
        std::string cmd = pidToCommand(pid);
        handleCommand(cmd);
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

/*!
 * \brief 处理command，删除命令后带的一系列参数，便于在终端展示
 * \details 可以进一步考虑限制输出大小，例如 {：<25} 强制截取前面的命令
 * */
void ShmBpf::handleCommand(std::string &command) {
    /** 找到第一个空格的位置 */
    size_t spacePos = command.find(' ');
    /** 如果找到了空格，则截取空格之前的部分 */
    if (spacePos != std::string::npos) {
        command = command.substr(0, spacePos);
    }
    /** 如果截取后的字符串长度大于30个字符，则截取前30个字符 */
    if (command.length() > 30) {
        command = command.substr(0, 30);
    }
}

/*!
 * \brief 根据PID获取进程名
 * */
std::string ShmBpf::pidToCommand(std::uint32_t pid) {
    std::string cmdFormatPath {"/proc/{}/cmdline"};
    std::string cmdlinePath = fmt::vformat(cmdFormatPath, fmt::make_format_args(pid));
    fs::path path(cmdlinePath);
    if (!fs::exists(path)) {
        return "";
    }
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }
    std::string command;
    std::getline(file, command, '\0');
    return command;
}


std::string ShmBpf::toHex(const char *data, size_t len) {
    std::string result;
    for (size_t i = 0; i < len; ++i) {
        result += fmt::format("{:02x}", static_cast<unsigned char>(data[i]));
        if (i < len - 1) result += " ";
    }
    return result;
}

std::string ShmBpf::hexToString(const char *data, size_t len) {
    std::string result;
    result.reserve(len); // 预分配内存以提高效率

    for (size_t i = 0; i < len; ++i) {
        char c = data[i];
        if (c >= 0x20 && c <= 0x7E) {
            result += static_cast<char>(c); // 可打印字符直接添加
        } else {
            result += '.'; // 不可打印字符用 '.' 替代
        }
    }

    return result;
}

/*!
 * \brief  proc/{pid}/fd/{fd}
 * */
std::string ShmBpf::getShmPath(int pid, int fd) {
    // 构造 /proc/{pid}/fd/{fd} 路径
    fs::path fd_path = fmt::format("/proc/{}/fd/{}", pid, fd);

    // 读取符号链接指向的实际路径
    if (fs::exists(fd_path) && fs::is_symlink(fd_path)) {
        fs::path target_path = fs::read_symlink(fd_path);

        // 将路径转换为字符串
        std::string path_str = target_path.string();

        // 如果路径以 "/dev/shm" 开头，则去除该前缀
        if (path_str.rfind("/dev/shm", 0) == 0) {
            return path_str.substr(strlen("/dev/shm"));
        }
        return path_str;
    }

    // 如果路径不存在或不是符号链接，返回空字符串或错误信息
    return "None";
}

/*!
 * \brief  proc/{pid}/maps
 * */
std::vector<unsigned long> ShmBpf::getShmVmAddr(int pid, std::string &shmPath) {
    std::vector<unsigned long> result;

    // 构造 /proc/{pid}/maps 路径
    std::string maps_path = fmt::format("/proc/{}/maps", pid);

    // 打开 maps 文件
    std::ifstream maps_file(maps_path);
    if (!maps_file.is_open()) {
        return result;  // 如果无法打开文件，返回空结果
    }

    std::string line;
    while (std::getline(maps_file, line)) {
        // 检查行是否包含指定的共享内存名称
        if (line.find(shmPath) != std::string::npos) {
            // 解析地址范围，例如：7f29418ae000-7f29418af000
            std::istringstream iss(line);
            std::string addr_range;
            iss >> addr_range;

            // 提取 '-' 分隔前后的地址
            size_t dash_pos = addr_range.find('-');
            if (dash_pos != std::string::npos) {
                std::string start_addr_str = addr_range.substr(0, dash_pos);
                std::string end_addr_str = addr_range.substr(dash_pos + 1);

                unsigned long start_addr = std::stoul(start_addr_str, nullptr, 16);
                unsigned long end_addr = std::stoul(end_addr_str, nullptr, 16);

                result.push_back(start_addr);
                result.push_back(end_addr);
            }
        }
    }

    maps_file.close();
    return result;
}

std::string ShmBpf::getShmVmAddrString(int pid, std::string &shmPath) {
    auto val = getShmVmAddr(pid, shmPath);
    if (val.empty()) {
        return "None";
    }
    return fmt::format("{:x}-{:x}", val.at(0), val.at(1));
}


std::string ShmBpf::getShmProtString(unsigned long prot) {
    std::string protStr {};
    switch (prot) {
        case PROT_READ:
            protStr = "PROT_READ";
            break;
        case PROT_WRITE:
            protStr = "PROT_WRITE";
            break;
        case PROT_READ | PROT_WRITE:
            protStr = "PROT_READ | PROT_WRITE";
            break;
        case PROT_EXEC:
            protStr = "PROT_EXEC";
            break;
        case PROT_NONE:
            protStr = "PROT_NONE";
            break;
        default:
            protStr = "Unknown";
            break;
    }
    return protStr;
}

std::string ShmBpf::getShmFlagString(unsigned long flag) {
    std::string flagStr {};
    switch (flag) {
        case MAP_SHARED:
            flagStr = "MAP_SHARED";
            break;
        case MAP_PRIVATE:
            flagStr = "MAP_PRIVATE";
            break;
        case MAP_SHARED | MAP_PRIVATE:
            flagStr = "MAP_SHARED | MAP_PRIVATE";
            break;
        default:
            flagStr = "Unknown";
            break;
    }
    return flagStr;
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
