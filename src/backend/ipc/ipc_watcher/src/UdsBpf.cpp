//
// Created by fzy on 2025/2/12.
//
#include <filesystem>
#include <fstream>

#include "UdsBpf.h"
#include "ipcwatcher.h"
#include "spdlog/spdlog.h"
#include "fmt/format.h"
using namespace ipc::ipcWatcher;
namespace fs = std::filesystem;

UdsBpf::UdsBpf(ConfigArgs& config)
    : config_(config),
      skel_(nullptr),
      //pb(nullptr),
      rb_(nullptr),
      formatHeader()
{
    setHeader(FormatType::kPrintNormal8);

}

UdsBpf::~UdsBpf() {
    destroy();
}

/*!
 * \brief 打开BPF程序
 * */
void UdsBpf::open() {
    skel_ = uds_bpf::open();
    if (!skel_) {
        SPDLOG_ERROR("Failed to open and load BPF skeleton");
    }

}

/*!
 * \brief 加载BPF程序
 * */
 void UdsBpf::load() {
    int err = uds_bpf::load(skel_);
    if (err) {
        SPDLOG_ERROR("Failed to load BPF program, err：{}", err);
        destroy();
    }
}

/*!
 * \brief 加载并验证BPF程序
 * */
void UdsBpf::openAndLoad() {
    skel_ = uds_bpf::open_and_load();
    if (!skel_) {
        SPDLOG_ERROR("Failed to load and verify BPF skeleton");
        //return 1;
    }
}

/*!
 * \brief 附加kprobe等事件
 * */
void UdsBpf::attach() {
    int err = uds_bpf::attach(skel_);
    if (err) {
        SPDLOG_ERROR("Failed to attach BPF program, err：{}", err);
        destroy();
    }
}

/*!
 * \brief 选择部分事件，独立挂载
 * */
void UdsBpf::setBpfProgsLoadOpt() {
    bpf_program__set_autoload(skel_->progs.unix_dgram_sendmsg, false);
    bpf_program__set_autoload(skel_->progs.unix_dgram_recvmsg, false);
    bpf_program__set_autoload(skel_->progs.unix_stream_sendmsg, true);
    bpf_program__set_autoload(skel_->progs.unix_stream_recvmsg, true);
}

/*!
 * \brief 根据选项要求，打印头部信息
 */
void UdsBpf::setHeader(FormatType type) {
    type_ = type;
    switch (type) {
        case FormatType::kPrintNormal8: {
            formatHeader = "{:<25} {:<15} {:<25} {:<15} {:<25} {:<10} {20} {30}\n";
            formatHeaderVars = R"("Timestamp", "sendPID", "sendComm"
                                     "recvPID", "recvComm", "Size", "Type", "Path")";
            break;
        }
        case FormatType::kPrintWithPayload9: {
            formatHeader = "{:<25} {:<15} {:<25} {:<15} {:<25} {:<10} {20} {30} {60}\n";
            formatHeaderVars = R"("Timestamp", "sendPID", "sendComm"
                                     "recvPID", "recvComm", "Size", "Type", "Path", "Payload")";
            break;
        }
        case FormatType::kReserve: { /** reserve */
            formatHeader = "{:<25} {:<15} {:<25} {:<15} {:<25}\n";
            formatHeaderVars = R"("Timestamp", "sendPID", "sendComm"
                                     "recvPID", "recvComm")";
            break;
        }
        default:
            break;
    }
}

/*!
 * \brief 接收BPF采集的数据，并调用处理函数进行处理
 * */
void UdsBpf::poll() {
    // 设置 ringbuffer 回调
//    rb_ = ring_buffer__new(bpf_map__fd(skel_->maps.uds_events),
//                          reinterpret_cast<ring_buffer_sample_fn>(UdsBpf::handleEvent),
//                          nullptr, nullptr);
    rb_ = ring_buffer__new(bpf_map__fd(skel_->maps.uds_events),
                           reinterpret_cast<ring_buffer_sample_fn>(UdsBpf::handleEvent),
                           this, nullptr);
    if (!rb_) {
        SPDLOG_ERROR("Failed to create uds ring buffer");
        destroy();
    }

    int err;
//    pb = perf_buffer__new(bpf_map__fd(skel->maps.events), 8,
//                          reinterpret_cast<perf_buffer_sample_fn>(UdsBpf::handleEvent),
//                          nullptr, nullptr, nullptr);
//    if (!pb) {
//        SPDLOG_ERROR("Failed to create perf buffer");
//        err = -1;
//        destroy();
//
//    }
    fmt::print("Tracing UDS send/recv events... Ctrl+C to exit\n");
    printHeader();
// 4. 轮询事件
    while (true) {
        //err = perf_buffer__poll(pb, 100 /* timeout_ms */);
        err = ring_buffer__poll(rb_, kPollPeriodMs);
        if (err == -EINTR) {
            err = 0;
            break;
        }
        if (err < 0) {
            printf("Error polling ring buffer: %d\n", err);
            break;
        }
    }
}

void UdsBpf::destroy() {
    ring_buffer__free(rb_);
    //perf_buffer__free(pb);
    uds_bpf::destroy(skel_);
}

/** static */ void UdsBpf::handleEvent(void *ctx, void *data, size_t len) {
    auto udsBpf = reinterpret_cast<UdsBpf*>(ctx);
    auto *e = reinterpret_cast<uds_event*>(data);
    if (udsBpf->type_ == FormatType::kPrintNormal8) {
        fmt::print(udsBpf->formatHeader,   e->timestamp,
                                           e->send_pid,
                                           udsBpf->pidToCommand(e->send_pid),
                                           e->recv_pid,
                                           udsBpf->pidToCommand(e->recv_pid),
                                           e->size,
                                           udsBpf->getUdsType(e->type),
                                           e->path);
    }
}

/*!
 * \brief 根据PID获取进程名
 * */
std::string UdsBpf::pidToCommand(std::uint32_t pid) {
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

/*!
 * \brief:   根据type获取UDS类型
 * \details
 *       enum sock_type {
                SOCK_STREAM = 1,
                SOCK_DGRAM = 2,
                SOCK_RAW = 3,
                SOCK_RDM = 4,
                SOCK_SEQPACKET = 5,
                SOCK_DCCP = 6,
                SOCK_PACKET = 10,
         };
 */
std::string UdsBpf::getUdsType(int enumId) {
std::string type {};
switch (enumId) {
    case 1:
        type = "SOCK_STREAM";
        break;
    case 2:
        type = "SOCK_DGRAM";
        break;
    case 3:
        type = "SOCK_RAW";
        break;
    case 4:
        type = "SOCK_RDM";
        break;
    case 5:
        type = "SOCK_SEQPACKET";
        break;
    case 6:
        type = "SOCK_DCCP";
        break;
    case 10:
        type = "SOCK_PACKET";
        break;
    default:
        type = "UNKNOWN";
        break;
    }
    return type;
}

void UdsBpf::printHeader() {
    fmt::print(formatHeader, formatHeaderVars);
}


std::string UdsBpf::findCommand(std::uint32_t pid) {
    return "";
}

