/*!
 * \brief Generate JSON configuration file from argument metadata for net_watcher
 * \file  JsonConfigGenerator.h
 * */

#ifndef NET_NET_WATCHER_JSON_CONFIG_GENERATOR_H
#define NET_NET_WATCHER_JSON_CONFIG_GENERATOR_H

#include "ArgMetadata.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <string>
#include <any>

namespace net::netWatcher {

/*!
 * \brief Convert std::any to JSON value based on type string
 */
inline nlohmann::json anyToJson(const std::any& value, const std::string& type) {
    if (type == "bool") {
        return std::any_cast<bool>(value);
    } else if (type == "int") {
        return std::any_cast<int>(value);
    } else if (type == "string") {
        return std::any_cast<std::string>(value);
    }
    return nullptr;
}

/*!
 * \brief Generate JSON configuration file from argument metadata
 * \param toolName The name of the tool (used for filename: <toolName>_args.json)
 * \return true if successful, false otherwise
 */
inline bool generateJsonConfig(const std::string& toolName) {
    nlohmann::json j;
    
    // Set tool name
    j["tool_name"] = toolName;
    
    // Get all argument metadata
    auto argsMetadata = getArgsMetadata();
    
    // Convert to JSON array
    nlohmann::json options = nlohmann::json::array();
    
    for (const auto& meta : argsMetadata) {
        nlohmann::json opt;
        opt["flags"] = meta.flags;
        opt["help"] = meta.help;
        opt["type"] = meta.type;
        opt["default"] = anyToJson(meta.default_value, meta.type);
        opt["required"] = meta.required;
        
        options.push_back(opt);
    }
    
    j["options"] = options;
    
    // Create filename: <toolName>_args.json
    std::string filename = toolName + "_args.json";
    
    try {
        std::ofstream outFile(filename);
        if (!outFile.is_open()) {
            return false;
        }
        
        // Write JSON with pretty formatting (4 spaces indentation)
        outFile << j.dump(4) << std::endl;
        outFile.close();
        
        return true;
    } catch (const std::exception& e) {
        return false;
    }
}

}  // namespace net::netWatcher

#endif // NET_NET_WATCHER_JSON_CONFIG_GENERATOR_H
