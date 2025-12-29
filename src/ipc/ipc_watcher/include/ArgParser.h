/*!
 * \brief ipc watcher arg parser
 * */

#ifndef IPC_IPC_WATCHER_ARG_PARSER_H
#define IPC_IPC_WATCHER_ARG_PARSER_H

#include "argparse/argparse.hpp"
#include "ConfigArgs.h"
#include <string>

namespace ipc::ipcWatcher {

inline int cmdParser(argparse::ArgumentParser& parser, ipc::ipcWatcher::ConfigArgs& config) {
    parser.add_argument("-u", "--uds")
            .help("Trace unix domain socket")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.traceUds);
    parser.add_argument("-m", "--mmap")
            .help("Trace mmap")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.traceMmap);
    parser.add_argument("-p", "--pid")
            .help("filter via send pid")
            .default_value(0)
            .store_into(config.pid);
    parser.add_argument("--filterPath")
            .help("Filter path")
            .default_value("")
            .action([&config](const std::string& path) {
                /** --filter_path=/tmp/uds.socket
                 * path: /tmp/uds.socket */
                config.filterPath = path;
            });
    parser.add_argument("--traceNoAnonUds")
            .help("only trace no anon uds like /tmp/sample.uds")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.traceNoAnonUds);
    parser.add_argument("--payload")
            .help("Print payload")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.printPayload);
    parser.add_argument("--force")
            .help("Force enable payload printing")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.forcePayload);
    parser.add_argument("--pcapFile")
            .help("Save output to pcap file")
            .default_value("")
            .store_into(config.pcapFile);
    parser.add_argument("--fromJson")
            .help("read config args from json file")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.readFromJson);
    parser.add_argument("--vvv", "--verbose")
            .help("Output more information")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.verbose);
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
    parser.add_argument("--generateJson")
            .help("Generate json config file")
            .default_value(false)
            .implicit_value(true)
            .store_into(config.generateJson);
//    parser.add_argument("reserve_sample_int")
//        .help("Positional Arguments sample like: <...>/ipcwatcher 10")
//        .scan<'i', int>();
    //config.reserve = parser.get<int>("reserve_int");
    parser.add_description("Linux IPC Watcher 工具 - 用于监控 Unix 域套接字通信及 mmap 活动。\n"
                           "                   \"支持过滤、打印载荷、保存到 pcap 文件等功能");
    parser.add_epilog("使用示例:\n"
                      "root, using sudo in ubuntu\n"
                      "  ipc_watcher --uds --pid=1234 --payload            # 跟踪指定 PID 的 UDS 通信并打印数据\n"
                      "  ipc_watcher --uds --filterPath=/tmp/test.sock     # 只跟踪路径为 /tmp/test.sock 的 UDS\n"
                      "  ipc_watcher --uds --pcapFile=output.pcap          # 将抓取的数据保存到 pcap 文件\n"
                      "  ipc_watcher --version                             # 显示版本信息"
                      " "
                      );
    return 0;
}


}  // namespace ipc::ipcWatcher

#endif //IPC_IPC_WATCHER_ARG_PARSER_H
