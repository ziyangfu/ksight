/*!
    \FIXME: 
当前由工具生成json文件，然后在ksightCli中读取json文件，方案不合适。
不合适的原因是，当前的cmdParser，不直观，并且如果要修改，依然需要修改2处
考虑更换为：
每个工具自己写一个bash-complete脚本。然后有一个工具，读取bash-complete，
生成ksightCli的command_data.py
同时，如果工具想单独使用，也可以有自动补全功能。

*/

#ifndef IPC_IPC_WATCHER_ARG_PARSER_H
#define IPC_IPC_WATCHER_ARG_PARSER_H

#include "argparse/argparse.hpp"
#include "ConfigArgs.h"
#include "ArgMetadata.h"
#include <string>

namespace ipc::ipcWatcher {

inline int cmdParser(argparse::ArgumentParser& parser, ipc::ipcWatcher::ConfigArgs& config) {
    // Get argument metadata
    auto argsMetadata = getArgsMetadata();
    
    // Configure parser using metadata
    for (const auto& meta : argsMetadata) {
        // Create argument with all flags
        argparse::Argument* arg_ptr;
        if (meta.flags.size() == 1) {
            arg_ptr = &parser.add_argument(meta.flags[0]);
        } else {
            arg_ptr = &parser.add_argument(meta.flags[0], meta.flags[1]);
        }
        auto& arg = *arg_ptr;
        
        // Set help text
        arg.help(meta.help);
        
        // Configure based on type
        if (meta.type == "bool") {
            bool defaultVal = std::any_cast<bool>(meta.default_value);
            arg.default_value(defaultVal)
               .implicit_value(true);
            
            // Store into appropriate config field
            if (meta.flags[0] == "-u" || meta.flags[0] == "--uds") {
                arg.store_into(config.traceUds);
            } else if (meta.flags[0] == "-m" || meta.flags[0] == "--mmap") {
                arg.store_into(config.traceMmap);
            } else if (meta.flags[0] == "--traceNoAnonUds") {
                arg.store_into(config.traceNoAnonUds);
            } else if (meta.flags[0] == "--payload") {
                arg.store_into(config.printPayload);
            } else if (meta.flags[0] == "--force") {
                arg.store_into(config.forcePayload);
            } else if (meta.flags[0] == "--fromJson") {
                arg.store_into(config.readFromJson);
            } else if (meta.flags[0] == "--vvv" || meta.flags[0] == "--verbose") {
                arg.store_into(config.verbose);
            } else if (meta.flags[0] == "--generateConfigJson") {
                arg.store_into(config.generateConfigJson);
            }
        } else if (meta.type == "int") {
            int defaultVal = std::any_cast<int>(meta.default_value);
            arg.default_value(defaultVal).scan<'i', int>();
            
            if (meta.flags[0] == "-p" || meta.flags[0] == "--pid") {
                arg.store_into(config.pid);
            }
        } else if (meta.type == "string") {
            std::string defaultVal = std::any_cast<std::string>(meta.default_value);
            arg.default_value(defaultVal);
            
            if (meta.flags[0] == "--filterPath") {
                arg.action([&config](const std::string& path) {
                    config.filterPath = path;
                });
            } else if (meta.flags[0] == "--pcapFile") {
                arg.store_into(config.pcapFile);
            }
        }
    }

    parser.add_description("Linux IPC Watcher 工具 - 用于监控 Unix 域套接字通信及 mmap 活动。\n"
                           "                   \"支持过滤、打印载荷、保存到 pcap 文件等功能");
    parser.add_epilog("使用示例:\n"
                      "root, using sudo in ubuntu\n"
                      "  ipcwatcher --uds --pid=1234 --payload            # 跟踪指定 PID 的 UDS 通信并打印数据\n"
                      "  ipcwatcher --uds --filterPath=/tmp/test.sock     # 只跟踪路径为 /tmp/test.sock 的 UDS\n"
                      "  ipcwatcher --uds --pcapFile=output.pcap          # 将抓取的数据保存到 pcap 文件\n"
                      "  ipcwatcher --version                             # 显示版本信息");
    return 0;
}

} // namespace ipc::ipcWatcher

#endif // IPC_IPC_WATCHER_ARG_PARSER_H
