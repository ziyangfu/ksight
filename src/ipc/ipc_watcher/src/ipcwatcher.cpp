/*!
\brief Linux kernel IPC 观测工具, 使用 Linux eBPF 技术
\file ipcwatcher.cpp
\TODO
    1. 将抓取到的数据，在终端输出
    2. 将抓取到的数据，存入pcap文件中，并可以使用wireshark进行分析
*/
#include <iostream>
#include <csignal>

#include "argparse/argparse.hpp"

#include "spdlog/spdlog.h"  /** 注意 spdlog与fmt的顺序 */
#if __cplusplus >= 202002L
#include <format>
namespace fmt = std;
#else
#include "fmt/format.h"
#endif

#include "UdsBpf.h"
#include "ShmBpf.h"

#include "Version.h"
#include "ConfigArgs.h"
#include "ArgParser.h"
#include "JsonConfigGenerator.h"

std::atomic<bool> gStoped(false);

void signalHandler(int signum) {
    if (signum == SIGINT) {
        SPDLOG_INFO("Received SIGINT, preparing to exit...");
        gStoped = true;
    }
}

void initSignalHandling() noexcept {
    bool success{true};
    sigset_t signals;
    success = success && (0 == sigfillset(&signals));
    success = success && (0 == sigdelset(&signals, SIGABRT));
    success = success && (0 == sigdelset(&signals, SIGBUS));
    success = success && (0 == sigdelset(&signals, SIGFPE));
    success = success && (0 == sigdelset(&signals, SIGILL));
    success = success && (0 == sigdelset(&signals, SIGSEGV));
    success = success && (0 == pthread_sigmask(SIG_SETMASK, &signals, nullptr));
    if (!success) {
        SPDLOG_ERROR("Failed to initialize signal handling");
    }
    // 注册 SIGINT 信号处理函数
    signal(SIGINT, signalHandler);
}

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::info);
    //initSignalHandling();
    ipc::ipcWatcher::ConfigArgs config;
    argparse::ArgumentParser parser("ipcwatcher");
    ipc::ipcWatcher::cmdParser(parser, config);
    try {
        parser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        SPDLOG_ERROR("{}", err.what());
        return 1;
    }
    
    // Handle JSON config generation
    if (config.generateJson) {
        if (ipc::ipcWatcher::generateJsonConfig(config, "ipcwatcher")) {
            fmt::print("JSON configuration file generated successfully: ipcwatcher_args.json\n");
            return 0;
        } else {
            SPDLOG_ERROR("Failed to generate JSON configuration file");
            return 1;
        }
    }
    
    if (config.traceUds) {
        ipc::ipcWatcher::UdsBpf udsBpf(config);
        udsBpf.open();
        udsBpf.setRodataFlags();
        udsBpf.load();
        udsBpf.attach();
        while (!gStoped) {
            udsBpf.poll();
        }
    }
    else if (config.traceMmap) {
        //fmt::print("do not support right now, exiting...\n");
        ipc::ipcWatcher::ShmBpf shmBpf(config);
        shmBpf.open();
        shmBpf.load();
        shmBpf.attach();
        while (!gStoped) {
            shmBpf.poll();
        }
     }
    else {
        fmt::print("No trace type selected, exiting...\n");
    }
}
