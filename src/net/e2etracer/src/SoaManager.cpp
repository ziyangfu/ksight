#include "SoaManager.h"
#include <fstream>
#include <iostream>
#include <arpa/inet.h>

namespace ksight {
namespace net {

SoaManager::SoaManager() {}

SoaManager::~SoaManager() {}

bool SoaManager::load_config(const std::string& config_path) {
    std::ifstream f(config_path);
    if (!f.is_open()) {
        return false;
    }

    try {
        nlohmann::json data = nlohmann::json::parse(f);
        if (!data.is_array()) return false;

        for (const auto& item : data) {
            SoaService service;
            service.service_id = item["service_id"].get<uint32_t>();
            service.instance_id = item["instance_id"].get<uint32_t>();
            service.name = item["service_name"].get<std::string>();
            service.ip = item["ip"].get<std::string>();
            service.port = item["port"].get<uint16_t>();
            service.protocol = item["protocol"].get<std::string>();
            service.description = item.value("description", "");

            services_.push_back(service);
            endpoint_index_[{service.ip, service.port}] = services_.size() - 1;
        }
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "Failed to parse SOA config: " << e.what() << std::endl;
        return false;
    }

    return true;
}

const SoaService* SoaManager::find_service_by_endpoint(const std::string& ip, uint16_t port) {
    auto it = endpoint_index_.find({ip, port});
    if (it != endpoint_index_.end()) {
        return &services_[it->second];
    }
    return nullptr;
}

const SoaService* SoaManager::find_service_by_id(uint32_t service_id, uint32_t instance_id) {
    for (const auto& service : services_) {
        if (service.service_id == service_id && service.instance_id == instance_id) {
            return &service;
        }
    }
    return nullptr;
}

} // namespace net
} // namespace ksight
