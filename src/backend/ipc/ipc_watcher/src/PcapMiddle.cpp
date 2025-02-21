//
// Created by fzy on 2025/2/21.
//
#include "PcapMiddle.h"

using namespace ipc::ipcWatcher;

PcapMiddle::PcapMiddle(std::string& path)
    : pcapGenerator_(std::make_unique<PcapGenerator>(path))
{

}

PcapMiddle::~PcapMiddle() {

}

void PcapMiddle::startPcapStream() {
    //pcapGenerator_->WriteToPcap();

}