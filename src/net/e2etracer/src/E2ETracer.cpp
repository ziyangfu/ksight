#include "E2ETracer.h"
#include <cstdio>
#include <arpa/inet.h>
#include <unistd.h>
#include "SoaManager.h"

namespace ksight {
namespace net {

extern SoaManager g_soa_manager;

E2ETracer::E2ETracer() : skel_(nullptr), rb_(nullptr) {}

E2ETracer::~E2ETracer() {
    stop();
}

int E2ETracer::init() {
    skel_ = e2etracer_bpf__open();
    if (!skel_) {
        std::cerr << "Failed to open BPF skeleton" << std::endl;
        return -1;
    }

    if (skel_->rodata) {
        skel_->rodata->target_service_id = target_service_id_;
        skel_->rodata->target_instance_id = target_instance_id_;
    }

    // 禁用 uprobe 的自动挂载，因为它的二进制路径需要在运行时动态指定
    bpf_program__set_autoattach(skel_->progs.uprobe_middleware_send, false);

    if (e2etracer_bpf__load(skel_) != 0) {
        std::cerr << "Failed to load BPF skeleton" << std::endl;
        e2etracer_bpf__destroy(skel_);
        skel_ = nullptr;
        return -1;
    }

    rb_ = ring_buffer__new(bpf_map__fd(skel_->maps.rb), handle_event, this, nullptr);
    if (!rb_) {
        std::cerr << "Failed to create ring buffer" << std::endl;
        return -1;
    }

    return 0;
}

int E2ETracer::start() {
    int err = e2etracer_bpf__attach(skel_);
    if (err) {
        std::cerr << "Failed to attach BPF skeleton" << std::endl;
        return err;
    }

    std::cout << "Successfully attached fentry/fexit network probes." << std::endl;
    // uprobe 的手动挂载可以在此处或通过命令行暴露接口
    // example: bpf_program__attach_uprobe(skel_->progs.uprobe_middleware_send, false, pid, binary_path, func_offset);

    while (true) {
        err = ring_buffer__poll(rb_, 100);
        if (err == -EINTR) continue;
        if (err < 0) break;
    }
    return 0;
}

void E2ETracer::stop() {
    if (rb_) {
        ring_buffer__free(rb_);
        rb_ = nullptr;
    }
    if (skel_) {
        e2etracer_bpf__destroy(skel_);
        skel_ = nullptr;
    }
}

void E2ETracer::set_service_filter(uint32_t service_id, uint32_t instance_id) {
    target_service_id_ = service_id;
    target_instance_id_ = instance_id;
}

int E2ETracer::handle_event(void *ctx, void *data, size_t data_sz) {
    E2ETracer *tracer = (E2ETracer *)ctx;
    tracer->process_event((const struct e2e_packet_event *)data);
    return 0;
}

void E2ETracer::process_event(const struct e2e_packet_event *e) {
    char saddr_str[INET_ADDRSTRLEN], daddr_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &e->saddr, saddr_str, sizeof(saddr_str));
    inet_ntop(AF_INET, &e->daddr, daddr_str, sizeof(daddr_str));

    uint32_t service_id = e->service_id;
    uint32_t instance_id = e->instance_id;
    uint16_t method_id = 0;
    uint16_t session_id = 0;

    // 如果 BPF 没有通过染色获取 ID，且存在 Payload (UDP 场景)，则尝试解析
    if (service_id == 0 && e->protocol == IPPROTO_UDP) {
        service_id = (e->payload_raw[0] << 8) | e->payload_raw[1];
        method_id = (e->payload_raw[2] << 8) | e->payload_raw[3];
        session_id = (e->payload_raw[10] << 8) | e->payload_raw[11];
    }

    auto* service = (service_id != 0) 
                    ? g_soa_manager.find_service_by_id(service_id, instance_id)
                    : g_soa_manager.find_service_by_endpoint(daddr_str, ntohs(e->dport));
    
    std::string service_name = service ? service->name : "Unknown Service";

    printf("[%s] PID:%-6d TID:%-6d | SERVICE:%-15s (0x%04x:0x%04x)\n", 
           e->is_rx ? "RX" : "TX", e->pid, e->tid, service_name.c_str(), service_id, method_id);
    
    // 延迟详细分析
    if (!e->is_rx) { // TX 路径
        printf("  L- [App->Sys: %llu ns] [Net: %llu ns] [MAC: %llu ns]\n",
               e->ts_syscall - e->ts_app,
               e->ts_network - e->ts_syscall,
               e->ts_mac - e->ts_network);
    } else { // RX 路径
        printf("  L- [MAC->Net: %llu ns] [Net->Trans: %llu ns]\n",
               e->ts_network - e->ts_mac,
               e->ts_transport - e->ts_network);
    }
}

} // namespace net
} // namespace ksight
