#ifndef SOA_MANAGER_H
#define SOA_MANAGER_H

#include <string>
#include <map>
#include <vector>
#include <nlohmann/json.hpp>

namespace ksight {
namespace net {

struct SoaService {
    uint32_t service_id;
    uint32_t instance_id;
    std::string name;
    std::string ip;
    uint16_t port;
    std::string protocol; // TCP/UDP
    std::string description;
};

class SoaManager {
public:
    SoaManager();
    ~SoaManager();

    // 从 JSON 文件加载配置
    bool load_config(const std::string& config_path);

    // 根据五元组查找服务信息 (初步过滤)
    const SoaService* find_service_by_endpoint(const std::string& ip, uint16_t port);

    // 根据 ServiceID 和 InstanceID 过滤
    const SoaService* find_service_by_id(uint32_t service_id, uint32_t instance_id);

private:
    std::vector<SoaService> services_;
    // 建立索引以加速查找
    std::map<std::pair<std::string, uint16_t>, size_t> endpoint_index_;
};

} // namespace net
} // namespace ksight

#endif // SOA_MANAGER_H
