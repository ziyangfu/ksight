//
// Created by fzy on 2025/2/21.
//

/*!
 * \brief 为了解决libbpf与libpcap的bpf结构体冲突问题，使用中间文件隔离
 * */

#ifndef MAGICEYES_PCAPMIDDLE_H
#define MAGICEYES_PCAPMIDDLE_H

#include <memory>
#include <string>
#include "PcapGenerator.h"

namespace ipc::ipcWatcher {

class PcapMiddle {
public:
    explicit PcapMiddle(std::string& path);
    ~PcapMiddle();
    void startPcapStream();
private:
    std::unique_ptr<PcapGenerator> pcapGenerator_;


};
}  // namespace ipc::IpcWatcher {


#endif //MAGICEYES_PCAPMIDDLE_H
