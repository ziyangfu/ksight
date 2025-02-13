//
// Created by fzy on 2025/2/12.
//
#include "UdsBpf.h"
#include "ipcwatcher.h"
#include "spdlog/spdlog.h"
#include "fmt/format.h"
using namespace ipc::ipcWatcher;

/** timestamp send_pid [command] recv_pid [command] size [payload] Path */

/** PID Path Size Payload Direction */
/** static */ const std::string UdsBpf::formatHeader {"{:<10} {:<10} {:<25} {:<10} {:<85} {:<10}\n"};
/** PID Path Size Direction */
/** static */ const std::string UdsBpf::formatHeaderNotPayload
                                        {"{:<10} {:<10} {:<25} {:<10} {:<10}\n"};

UdsBpf::UdsBpf()
    : skel(nullptr),
      //pb(nullptr),
      rb(nullptr)
{

}

UdsBpf::~UdsBpf() {
    destroy();
}

/*!
 * \brief 打开BPF程序
 * */
void UdsBpf::open() {
    skel = uds_bpf::open();
    if (!skel) {
        SPDLOG_ERROR("Failed to open and load BPF skeleton");
    }

}

/*!
 * \brief 加载BPF程序
 * */
 void UdsBpf::load() {
    int err = uds_bpf::load(skel);
    if (err) {
        SPDLOG_ERROR("Failed to load BPF program, err：{}", err);
        destroy();
    }
}

/*!
 * \brief 加载并验证BPF程序
 * */
void UdsBpf::openAndLoad() {
    skel = uds_bpf::open_and_load();
    if (!skel) {
        SPDLOG_ERROR("Failed to load and verify BPF skeleton");
        //return 1;
    }
}

/*!
 * \brief 附加kprobe等事件
 * */
void UdsBpf::attach() {
    int err = uds_bpf::attach(skel);
    if (err) {
        SPDLOG_ERROR("Failed to attach BPF program, err：{}", err);
        destroy();
    }
}

/*!
 * \brief 选择部分事件，独立挂载
 * */
void UdsBpf::setBpfProgsLoadOpt() {
    bpf_program__set_autoload(skel->progs.unix_dgram_sendmsg, false);
    bpf_program__set_autoload(skel->progs.unix_dgram_recvmsg, false);
    bpf_program__set_autoload(skel->progs.unix_stream_sendmsg, true);
    bpf_program__set_autoload(skel->progs.unix_stream_recvmsg, true);
}

/*!
 * \brief 根据选项要求，打印头部信息
 * */
void UdsBpf::printHeader() {
    fmt::print(UdsBpf::formatHeader, "Timestamp", "PID", "Path", "Size", "Payload", "Direction");
}

/*!
 * \brief 接收BPF采集的数据，并调用处理函数进行处理
 * */
void UdsBpf::poll() {
    // 设置 ringbuffer 回调
    rb = ring_buffer__new(bpf_map__fd(skel->maps.uds_events),
                          reinterpret_cast<ring_buffer_sample_fn>(UdsBpf::handleEvent),
                          nullptr, nullptr);
    if (!rb) {
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
        err = ring_buffer__poll(rb, kPollPeriodMs);
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
    ring_buffer__free(rb);
    //perf_buffer__free(pb);
    uds_bpf::destroy(skel);
}

/** static */ void UdsBpf::handleEvent(void *ctx, void *data, size_t len) {
    uds_event *e = reinterpret_cast<uds_event*>(data);
    std::string dir = e->direction == 0 ? "Send" : "Recv";
    fmt::print(UdsBpf::formatHeader, e->timestamp, e->send_pid, e->path, e->size, e->payload, dir);
}

/*!
 * \brief 根据PID获取进程名
 * */
std::string UdsBpf::pidToCommand(pid_t pid) {
    std::string command {};
    return command;
}

