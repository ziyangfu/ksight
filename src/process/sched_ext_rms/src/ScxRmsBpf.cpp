#include <iostream>
#include <unistd.h>
#include <bpf/bpf.h>
#include <bpf/libbpf.h>
#include "ScxRmsBpf.h"
#include "scx/common.h"
#include "spdlog/spdlog.h"

namespace process::scheExtRms {

ScxRmsBpf::ScxRmsBpf() : skel_(nullptr), link_(nullptr) {}

ScxRmsBpf::~ScxRmsBpf() {
    destroy();
}

void ScxRmsBpf::open() {
    skel_ = scx_rms__open();
    if (!skel_) {
        SPDLOG_ERROR("Failed to open BPF skeleton");
        throw std::runtime_error("Failed to open BPF skeleton");
    }
}

void ScxRmsBpf::load() {
    skel_->rodata->usersched_pid = getpid();
    int err = scx_rms__load(skel_);
    if (err) {
        SPDLOG_ERROR("Failed to load BPF skeleton: {}", err);
        destroy();
        throw std::runtime_error("Failed to load BPF skeleton");
    }
}

void ScxRmsBpf::attach() {
    link_ = bpf_map__attach_struct_ops(skel_->maps.rms_ops);
    if (!link_) {
        SPDLOG_ERROR("Failed to attach struct_ops");
        destroy();
        throw std::runtime_error("Failed to attach struct_ops");
    }
    pinMaps();
}

void ScxRmsBpf::destroy() {
    unpinMaps();
    if (link_) {
        bpf_link__destroy(link_);
        link_ = nullptr;
    }
    if (skel_) {
        scx_rms__destroy(skel_);
        skel_ = nullptr;
    }
}

void ScxRmsBpf::pinMaps() {
    if (bpf_map__pin(skel_->maps.task_attr_hmap, "/sys/fs/bpf/task_attr_hmap")) {
        SPDLOG_ERROR("Failed to pin task_attr_hmap");
    }
    if (bpf_map__pin(skel_->maps.rms_entry_map, "/sys/fs/bpf/rms_entry_map")) {
        SPDLOG_ERROR("Failed to pin rms_entry_map");
    }
}

void ScxRmsBpf::unpinMaps() {
    if (skel_) {
        bpf_map__unpin(skel_->maps.task_attr_hmap, "/sys/fs/bpf/task_attr_hmap");
        bpf_map__unpin(skel_->maps.rms_entry_map, "/sys/fs/bpf/rms_entry_map");
    }
}

bool ScxRmsBpf::shouldExit() const {
    return UEI_EXITED(skel_, uei);
}

void ScxRmsBpf::reportExit() {
    UEI_REPORT(skel_, uei);
}

} // namespace process::scheExtRms
