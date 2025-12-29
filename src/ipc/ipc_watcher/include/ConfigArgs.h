/*!
 * \brief ipc_watcher工具环境参数，接收自命令行参数，或配置文件
 * \file  ConfigArgs.h
 * */

#ifndef IPC_IPC_WATCHER_CONFIG_ARGS_H
#define IPC_IPC_WATCHER_CONFIG_ARGS_H

#include <string>

namespace ipc::ipcWatcher {
/*!
 * 1. 命令行参数解析
 * 必选参数
 *      - -u --uds 追踪unix domain socket
 *      - -m --mmap 追踪mmap
 * 可选参数
 *      - -x 显示UDS基本连接信息
 *      - -p --pid=<val> 追踪指定进程的UDS/SHM信息
 *      - --filter_path=/path/to/file 追踪指定路径下的文件
 *      - --payload 是否打印 payload， 为保证性能，仅支持过滤状态跟踪，可使用 --force 强制开启全局payload打印
 *      - --force 强制开启全局payload打印
 *      - --pcap_file=/path/to/file.pcap 将输出结果保存为pcap文件，可以使用wireshark进行分析
 *      - --vvv --verbose 输出更多信息
 *      - -v --version 输出版本信息
 *      - -h --help 输出帮助信息
 * */
struct ConfigArgs {
    bool traceUds               {false};
    bool traceMmap              {false};
    bool traceNoAnonUds         {false};  /** 非匿名UDS，例如 /tmp/sample.uds */
    int pid                     {0};
    bool printPayload           {false};
    bool printPayloadHex        {false};
    bool forcePayload           {false};  /** payload输出一般仅支持pid过滤后输出，不推荐全量输出 */
    bool readFromJson           {false};
    bool generateConfigJson     {false};
    bool verbose                {false};
    std::string filterPath;
    std::string pcapFile;
};

}  // namespace ipc::ipcWatcher

#endif //IPC_IPC_WATCHER_CONFIG_ARGS_H
