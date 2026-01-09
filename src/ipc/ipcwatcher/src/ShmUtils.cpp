#include "ShmUtils.h"

#include "spdlog/spdlog.h"  // FIXME: fmt存在链接问题，在fmt加上spdlog即可编译成功，待修复
#include "fmt/format.h"
#include <filesystem>
#include <fstream>
#include <algorithm>
#include <sys/mman.h>

namespace ipc::ipcWatcher {

namespace fs = std::filesystem;

std::string shmUtils::toHex(const char *data, size_t len) {
    std::string result;
    for (size_t i = 0; i < len; ++i) {
        result += fmt::format("{:02x}", static_cast<unsigned char>(data[i]));
        if (i < len - 1) result += " ";
    }
    return result;
}

std::string shmUtils::hexToString(const char *data, size_t len) {
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
/**
 * \bug \fixme terminate called after throwing an instance of 'std::filesystem::__cxx11::filesystem_error'
 what():  filesystem error: read_symlink: No such file or directory [/proc/9463/fd/62]

 * */
std::string shmUtils::getShmPath(int pid, int fd) {
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
std::vector<unsigned long> shmUtils::getShmVmAddr(int pid, std::string &shmPath) {
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

std::string shmUtils::getShmVmAddrString(int pid, std::string &shmPath) {
    auto val = getShmVmAddr(pid, shmPath);
    if (val.empty()) {
        return "None";
    }
    return fmt::format("{:x}-{:x}", val.at(0), val.at(1));
}


std::string shmUtils::getShmProtString(unsigned long prot) {
    std::string protStr {};
    switch (prot) {
        case PROT_READ:
            protStr = "READ";
            break;
        case PROT_WRITE:
            protStr = "WRITE";
            break;
        case PROT_READ | PROT_WRITE:
            protStr = "READ | WRITE";
            break;
        case PROT_EXEC:
            protStr = "EXEC";
            break;
        case PROT_NONE:
            protStr = "NONE";
            break;
        default:
            protStr = "Unknown";
            break;
    }
    return protStr;
}

std::string shmUtils::getShmFlagString(unsigned long flag) {
    std::string flagStr {};
    switch (flag) {
        case MAP_SHARED:
            flagStr = "SHARED";
            break;
        case MAP_PRIVATE:
            flagStr = "PRIVATE";
            break;
        case MAP_SHARED | MAP_PRIVATE:
            flagStr = "SHARED | PRIVATE";
            break;
        default:
            flagStr = "Unknown";
            break;
    }
    return flagStr;
}


/*!
 * \brief 处理command，删除命令后带的一系列参数，便于在终端展示
 * \details 可以进一步考虑限制输出大小，例如 {：<25} 强制截取前面的命令
 * */
void shmUtils::handleCommand(std::string &command) {
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
std::string shmUtils::pidToCommand(std::uint32_t pid) {
    std::string cmdFormatPath {"/proc/{}/cmdline"};
    std::string cmdlinePath = fmt::vformat(cmdFormatPath, fmt::make_format_args(pid));
    //std::string cmdlinePath = fmt::format("/proc/{}/cmdline", pid);
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

/*!
 * \brief 根据程序名获取PID，与程序名同名的可能有多个
 * */
std::vector<int> shmUtils::commandToPid(const std::string &processName) {
    std::vector<int> pids;

    // 遍历/proc目录
    for (const auto& entry : fs::directory_iterator("/proc")) {
        // 检查是否为目录且目录名为数字
        if (entry.is_directory()) {
            std::string dirName = entry.path().filename().string();
            if (std::all_of(dirName.begin(), dirName.end(), ::isdigit)) {
                // 构造cmdline文件路径
                std::string cmdlinePath = entry.path() / "cmdline";

                try {
                    // 读取cmdline文件内容
                    std::ifstream cmdlineFile(cmdlinePath);
                    if (cmdlineFile.is_open()) {
                        std::string cmdline;
                        std::getline(cmdlineFile, cmdline, '\0'); // cmdline以null字符分隔
                        cmdlineFile.close();

                        // cmdline中的第一个参数通常是程序名
                        if (!cmdline.empty()) {
                            // 提取程序名部分（处理路径情况）
                            std::string progName = cmdline;
                            size_t lastSlash = progName.find_last_of('/');
                            if (lastSlash != std::string::npos) {
                                progName = progName.substr(lastSlash + 1);
                            }

                            // 检查是否匹配
                            if (progName == processName) {
                                pids.push_back(std::stoi(dirName));
                            }
                        }
                    }
                } catch (...) {
                    // 忽略无法访问的进程
                    continue;
                }
            }
        }
    }
    return pids;
}


/*!
* \details 通过读取 /proc/<pid>/pagemap 文件，获取虚拟地址对应的物理地址
*          pagemap 在 Linux kernel 2.6.25中引入
*          要注意 swap的影响，如果物理页帧被交换到 swap 中，则物理页是不对的
*          这时要检查 pread读取的uint64位数据中的第63位，如果为1，则表示该页帧被交换到 swap 中
* */
uintptr_t shmUtils::vaddrToPhysicalAddr(pid_t pid, unsigned long vaddr) {
    static size_t PAGE_SIZE = getpagesize(); // 获取页大小（通常为4096字节）
    // 构造pagemap路径
    char pagemap_path[256];
    snprintf(pagemap_path, sizeof(pagemap_path), "/proc/%d/pagemap", pid);

    // 打开pagemap文件
    const int fd = open(pagemap_path, O_RDONLY);
    if (fd == -1) {
        spdlog::error("Failed to open pagemap: {}", strerror(errno));
        return 0;
    }

    // 计算页索引和偏移
    const uintptr_t vpage = vaddr / PAGE_SIZE;
    const off_t file_offset = vpage * sizeof(uint64_t);
    const uintptr_t page_offset = vaddr % PAGE_SIZE;

    // 读取pagemap条目
    uint64_t entry;
    if (pread(fd, &entry, sizeof(entry), file_offset) != sizeof(entry)) {
        spdlog::error("Failed to read pagemap entry: {}", strerror(errno));
        close(fd);
        return 0;
    }
    close(fd);

    // 检查页面是否存在于物理内存
    if (!(entry & (1ULL << 63))) {
        spdlog::warn("Page not present in physical memory");
        return 0;
    }

    // 提取物理页帧号（PFN）
    const uint64_t pfn = entry & 0x7FFFFFFFFFFFFF;

    // 计算物理地址
    const uintptr_t phys_addr = (pfn << 12) | page_offset;

    return phys_addr;
}

std::string shmUtils::vaddrToPhysicalAddrString(pid_t pid, unsigned long vaddr) {
    return fmt::format("{:x}", vaddrToPhysicalAddr(pid, vaddr));
}



/*!
 * \brief 轮询 cat /proc/{pid}/fd/{fd}的数据，这是共享内存写入与读取的payload
 *        识别到数据改变时，输出一次。
 * */
std::string shmUtils::readShmPayloadCycle(int pid, int fd) {
    return "";
}


/*!
 * \brief 将十进制数转换为十六进制字符串表示
 * \param decimal 要转换的十进制数
 * \param withPrefix 是否添加"0x"前缀，默认为true
 * \return 十六进制字符串
 */
std::string shmUtils::decimalToHex(long long decimal, bool withPrefix) {
    if (withPrefix) {
        return fmt::format("0x{:x}", decimal);
    } else {
        return fmt::format("{:x}", decimal);
    }
}
// 内存
// CPU
// TCP/IP
// IPC


// 问题设想：






// 这个工具是什么？
// 解决了什么问题？
// 采用了什么样的技术，碰到了什么样的问题？
// 实践中使用怎样？效果如何？





// wirefisher 网络限流





// 问题是什么？








// star法则

// vscode 插件一体化，观测开发，作为辅助调试工具

// 集成nettrace来追踪网络问题，追踪wirefisher来分析网络

// 基于SchedExt的用户态调度器（based on星环RMS）
// 1. 平台开发

/*
NVIDIA Orin平台下基于eBPF的OS性能观测与诊断软件开发
针对Linux内核域控高负载场景下的问题定位难点，开发一套无侵入、低开销的系统级观测工具


负责软件架构设计，搭建可扩展、前后端解耦的软件架构。软件平台BSP OK
负责针对TCP/IP协议栈与IPC进程间通信的软件开发，POSIX共享内存的跟踪分析
Unix Domain Socket的元数据抓取与全链路跟踪
AUTOSAR AP中间件中的共享内存通信透明化。
eBPF 内核观测 (C/C++)： 基于 eBPF 技术，深入 Linux 内核网络通过挂载 kprobe/tracepoint，实现了对 TCP/IP 协议栈的分层耗时统计（MAC/IP/TCP层）及端到端延迟分析，将网络抖动定位精度提升至微秒级。



基于sched_ext的用户态调度器，实时调度，域控-区控背景下的算法周期性调度精度提升15%
*/
/*!
 * \details
 * 在用户空间，通过读取 /proc/kpagecount 文件，获取共享内存映射的数量
 * kpagecount，它的输出是二进制的，文本不可读，而且很大，20多MB的大小。
 * 这个功能是Linux kernel 2.6.25引入的。它可以通过物理页地址，返回一个uint64的数，
 * 表示有多少个虚拟映射（页表条目）指向同一个物理页面，也就是page中的 `_mapcount`值
 *
 * 如果要在内核中读取的话，需要去page结构体下读
 *
 * `pfn`（Page Frame Number，页框编号）
 *      定义：`pfn` 是操作系统用来唯一标识物理内存页（页框）的编号。
 *      本质：它是一个索引或者编号，表示“第几个物理页面”。
 *      表示范围：在Linux中，物理内存被抽象成一个由连续的“页框”组成的集合，每个页框有唯一的编号（pfn）。
 * */
int shmUtils::getShmMapCount(unsigned long pfn) {
    const char* kpagecount_path = "/proc/kpagecount";
    int fd = open(kpagecount_path, O_RDONLY);
    if (fd == -1) {
        spdlog::error("Failed to open {}: {}", kpagecount_path, strerror(errno));
        return -1;
    }

    // 每个计数是64位(8字节)，按PFN索引
    off_t offset = pfn * sizeof(uint64_t);
    uint64_t count = 0;

    ssize_t bytes_read = pread(fd, &count, sizeof(count), offset);
    close(fd);

    if (bytes_read != sizeof(count)) {
        spdlog::error("Failed to read map count from {}: {}", kpagecount_path, strerror(errno));
        return -1;
    }

    return static_cast<int>(count);
}

} // namespace ipc::ipcWatcher