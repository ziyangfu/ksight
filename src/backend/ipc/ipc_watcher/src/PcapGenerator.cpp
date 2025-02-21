//
// Created by fzy on 2025/2/18.
//
#include "PcapGenerator.h"
#include "spdlog/spdlog.h"

#include <exception>
#include <vector>
#include "fmt/format.h"

#include <iostream>

using namespace ipc::ipcWatcher;

//template<class DataStruct>
PcapGenerator::PcapGenerator(std::string& path)
    : path_(path),
      handler_(pcap_open_dead(0, 65535)),
      dumper_(pcap_dump_open(handler_, path_.data()))
{
    //std::cout << "dumper_: " << dumper_ << std::endl;
    assert(handler_ != nullptr);
    try {
        assert(dumper_ != nullptr);
    }
    catch (const std::exception& e) {
        pcap_close(handler_);
        SPDLOG_ERROR("Failed to open pcap file: {}", e.what());
    }
}

PcapGenerator::~PcapGenerator() {
    if (dumper_ != nullptr) {
        pcap_dump_close(dumper_);
        dumper_ = nullptr;
    }
    if (handler_ != nullptr) {
        pcap_close(handler_);
        handler_ = nullptr;
    }
}

/*!
 * \brief 将数据写入pcap文件
 * \details
 *   1. 构造一个简单的数据包头部
 *   2. 分配内存来存储数据包
 *   3. 写入数据包到 pcap 文件
 * */
void PcapGenerator::WriteToPcap(uds_event& data) {
    //fmt::print("WriteToPcap: {}\n", data.timestamp);
    const std::uint8_t payload[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    //memcpy(data.payload, payload, sizeof(payload));
    if (dumper_ == nullptr) {
        SPDLOG_ERROR("dumper_ is nullptr");
        return;
    }
    struct pcap_pkthdr header {};
    header.ts.tv_sec = time(nullptr);  /** 可以用 data.event.timestamp? */
    header.ts.tv_usec = 0;
    header.caplen = sizeof(UDSData) + sizeof(payload);
    header.len = header.caplen;
    std::vector<std::uint8_t> packet(header.len);
    memcpy(packet.data(), &data, sizeof(UDSData));
    memcpy(packet.data() + sizeof(UDSData), payload, sizeof(payload));
    pcap_dump(reinterpret_cast<u_char*>(dumper_), &header, packet.data());
}

void PcapGenerator::WriteToPcap(uds_event* data) {
    //fmt::print("WriteToPcap: {}\n", data.timestamp);
    const std::uint8_t payload[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A};
    //memcpy(data.payload, payload, sizeof(payload));
    if (dumper_ == nullptr) {
        SPDLOG_ERROR("dumper_ is nullptr");
        return;
    }
    struct pcap_pkthdr header {};
    header.ts.tv_sec = time(nullptr);  /** 可以用 data.event.timestamp? */
    header.ts.tv_usec = 0;
    header.caplen = sizeof(UDSData) + sizeof(payload);
    header.len = header.caplen;
    std::vector<std::uint8_t> packet(header.len);
    memcpy(packet.data(), data, sizeof(UDSData));
    memcpy(packet.data() + sizeof(UDSData), payload, sizeof(payload));
    pcap_dump(reinterpret_cast<u_char*>(dumper_), &header, packet.data());
}
