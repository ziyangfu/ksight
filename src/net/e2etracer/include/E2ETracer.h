#ifndef E2E_TRACER_USER_H
#define E2E_TRACER_USER_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include "../bpf/e2etracer.h"
#include "E2ETracer.skel.h"

namespace ksight {
namespace net {

class E2ETracer {
public:
    E2ETracer();
    ~E2ETracer();

    int init();
    int start();
    void stop();

    // 设置过滤条件
    void set_service_filter(uint32_t service_id, uint32_t instance_id);

    // 动态挂载用户态探测点
    int attach_middleware_uprobe(int target_pid, const std::string& lib_name, const std::string& symbol);
    
private:
    static int handle_event(void *ctx, void *data, size_t data_sz);
    void process_event(const struct e2e_packet_event *e);

    std::string find_library_path(int pid, const std::string& lib_name);

    struct e2etracer_bpf *skel_;
    struct ring_buffer *rb_;
    std::vector<struct bpf_link*> uprobe_links_;

    uint32_t target_service_id_ = 0;
    uint32_t target_instance_id_ = 0;
};

} // namespace net
} // namespace ksight

#endif // E2E_TRACER_USER_H
