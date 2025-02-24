/*!
 * \brief pcap文件生成器，基于libpcap
 * */

/** FIXME: The capture file appears to have been cut short in the middle of a packet
 * ctrl+c 异常退出问题
 * https://blog.csdn.net/weixin_43869898/article/details/116056580
 * https://stackoverflow.com/questions/13563523/the-capture-file-appears-to-have-been-cut-short-in-the-middle-of-a-packet-how
 * */

#ifndef IPC_IPC_WATCHER_PCAP_GENERATOR_H
#define IPC_IPC_WATCHER_PCAP_GENERATOR_H

#include <string>
/** 不需要用到libpcap的BPF功能，注意，libpcap的BPF与libbpf的eBPF有冲突，会有BPF重复定义 */
#define PCAP_DONT_INCLUDE_PCAP_BPF_H
#include <pcap/pcap.h>
#include "ipcwatcher.h"

namespace ipc::ipcWatcher {

/** 可以是单例模式？ */
//template<class DataStruct>
class PcapGenerator {
public:
    using UDSData = uds_event;
    explicit PcapGenerator(std::string& path);
    ~PcapGenerator();

    void WriteToPcap(uds_event& data);
    void WriteToPcap(uds_event* data);

private:
    std::string path_;
    pcap_t* handler_;
    pcap_dumper* dumper_;
};

}   // namespace ipc::ipcWatcher

#endif //IPC_IPC_WATCHER_PCAP_GENERATOR_H
