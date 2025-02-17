/*!
 * \brief ipc_watcher工具环境参数，接收自命令行参数，或配置文件
 * \file  env.h
 * */

#ifndef IPC_IPC_WATCHER_ENV_H
#define IPC_IPC_WATCHER_ENV_H

#include <string>

namespace ipc::ipcWatcher {

struct ConfigArgs {
    bool traceUds;
    bool traceMmap;
    bool print_payload;
    bool force_payload;
    bool verbose;
    std::string filter_path;
    std::string pcap_file;

    ConfigArgs()
    : traceUds(false),
      traceMmap(false),
      print_payload(false),
      force_payload(false),
      verbose(false)
    {
    }

    ~ConfigArgs() = default;
};

}

#endif //IPC_IPC_WATCHER_ENV_H
