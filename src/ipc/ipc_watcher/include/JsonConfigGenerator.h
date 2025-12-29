/*!
 * \brief Generate JSON configuration file from command-line arguments
 * \file  JsonConfigGenerator.h
 * */

#ifndef IPC_IPC_WATCHER_JSON_CONFIG_GENERATOR_H
#define IPC_IPC_WATCHER_JSON_CONFIG_GENERATOR_H

#include "ConfigArgs.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <string>

namespace ipc::ipcWatcher {

/*!
 * \brief Generate JSON configuration file from ConfigArgs
 * \param config The configuration arguments to save
 * \param toolName The name of the tool (used for filename: <toolName>_args.json)
 * \return true if successful, false otherwise
 */
inline bool generateJsonConfig(const ConfigArgs& config, const std::string& toolName) {
    nlohmann::json j;
    
    // Serialize all configuration fields to JSON
    j["traceUds"] = config.traceUds;
    j["traceMmap"] = config.traceMmap;
    j["traceNoAnonUds"] = config.traceNoAnonUds;
    j["pid"] = config.pid;
    j["printPayload"] = config.printPayload;
    j["printPayloadHex"] = config.printPayloadHex;
    j["forcePayload"] = config.forcePayload;
    j["readFromJson"] = config.readFromJson;
    j["verbose"] = config.verbose;
    j["filterPath"] = config.filterPath;
    j["pcapFile"] = config.pcapFile;
    
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

}  // namespace ipc::ipcWatcher

#endif //IPC_IPC_WATCHER_JSON_CONFIG_GENERATOR_H
