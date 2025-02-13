/*!
\brief Linux kernel IPC 观测工具, 使用 Linux eBPF 技术
\TODO
    1. 将抓取到的数据，在终端输出
    2. 将抓取到的数据，存入pcap文件中，并可以使用wireshark进行分析
*/
/*!
 * 1. 命令行参数解析
 *      - -u --uds 追踪unix domain socket
 *      - -m --mmap 追踪mmap
 *      - --filter_path=/path/to/file 追踪指定路径下的文件
 *      - --payload 是否打印 payload， 为保证性能，仅支持过滤状态跟踪，可使用 --force 强制开启全局payload打印
 *      - --force 强制开启全局payload打印
 *      - --pcap_file=/path/to/file.pcap 将输出结果保存为pcap文件，可以使用wireshark进行分析
 *      - --vvv --verbose 输出更多信息
 *      - -v --version 输出版本信息
 *      - -h --help 输出帮助信息
 * */

#include <iostream>
#include <csignal>

#include "argparse/argparse.hpp"
#include "spdlog/spdlog.h"  /** 注意 spdlog与fmt的顺序 */

#if __cplusplus >= 202002L
#include <format>
#else
#include "fmt/format.h"
#endif

#include "UdsBpf.h"
#include "Version.h"

std::atomic<bool> g_interrupted(false);

void signalHandler(int signum) {
    if (signum == SIGINT) {
        SPDLOG_INFO("Received SIGINT, preparing to exit...");
        g_interrupted = true;
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


int cmdParser(argparse::ArgumentParser& parser) {
    parser.add_argument("-u", "--uds")
        .help("Trace unix domain socket")
        .default_value(false)
        .implicit_value(true);
    parser.add_argument("-m", "--mmap")
        .help("Trace mmap")
        .default_value(false)
        .implicit_value(true);
    parser.add_argument("--filter_path")
        .help("Filter path")
        .default_value("")
        .action([](const std::string& path) {
            /** --filter_path=/tmp/uds.socket
             * path: /tmp/uds.socket */
            std::cout << path << std::endl;
        });
    parser.add_argument("--filter_exist_path")
        .help("过滤存在路径的数据包")
        .default_value(false)
        .implicit_value(true);
    parser.add_argument("--payload")
        .help("Print payload")
        .default_value(false)
        .implicit_value(true);
    parser.add_argument("--force")
        .help("Force enable payload printing")
        .default_value(false)
        .implicit_value(true);
    parser.add_argument("--pcap_file")
        .help("Save output to pcap file")
        .default_value("")
        .action(
                [](const std::string& value) {

        });
    parser.add_argument("--vvv", "--verbose")
        .help("Output more information")
        .default_value(false)
        .implicit_value(true);
    parser.add_argument("-v", "--version")
        .help("Output version information")
        .default_value(false)
        .implicit_value(true)
        .action(
                [](const std::string& value) {
                    ipc::ipcWatcher::printVersion();
                    exit(0);
                }
                );
    return 0;
}

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::info);
    //initSignalHandling();
    argparse::ArgumentParser parser("ipc_watcher");
    cmdParser(parser);
    try {
        parser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        SPDLOG_ERROR("{}", err.what());
        return 1;
    }
    // 获取解析后的参数值
    bool traceUds = parser.get<bool>("--uds");
    bool traceMmap = parser.get<bool>("--mmap");
    std::string filter_path = parser.get<std::string>("--filter_path");
    bool print_payload = parser.get<bool>("--payload");
    bool force_payload = parser.get<bool>("--force");
    std::string pcap_file = parser.get<std::string>("--pcap_file");
    bool verbose = parser.get<bool>("--verbose");


    int udsSetValue = static_cast<int>(parser.get<bool>("--filter_exist_path"));
    ipc::ipcWatcher::UdsBpf udsBpf;
    udsBpf.open();
    //udsBpf.setRodataFlags(udsSetValue);
    udsBpf.load();
    udsBpf.attach();
    while (!g_interrupted) {
        udsBpf.poll();
    }

}
