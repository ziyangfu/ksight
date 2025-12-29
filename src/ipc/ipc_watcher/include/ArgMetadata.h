/*!
 * \brief Argument metadata definition for ipc_watcher
 * \file  ArgMetadata.h
 * */

#ifndef IPC_IPC_WATCHER_ARG_METADATA_H
#define IPC_IPC_WATCHER_ARG_METADATA_H

#include <string>
#include <vector>
#include <any>

namespace ipc::ipcWatcher {

/*!
 * \brief Metadata for a single command-line argument
 */
struct ArgMetadata {
    std::vector<std::string> flags;      // e.g., {"-u", "--uds"}
    std::string help;                    // Help text
    std::string type;                    // "bool", "int", "string"
    std::any default_value;              // Default value (type-erased)
    bool required;                       // Whether the argument is required
    
    ArgMetadata(std::vector<std::string> f, std::string h, std::string t, 
                std::any dv, bool req = false)
        : flags(std::move(f)), help(std::move(h)), type(std::move(t)), 
          default_value(std::move(dv)), required(req) {}
};

/*!
 * \brief Get all command-line argument metadata
 * \return Vector of argument metadata
 */
inline std::vector<ArgMetadata> getArgsMetadata() {
    return {
        {{"-u", "--uds"}, 
         "Trace unix domain socket", 
         "bool", 
         false, 
         false},
        
        {{"-m", "--mmap"}, 
         "Trace mmap", 
         "bool", 
         false, 
         false},
        
        {{"-p", "--pid"}, 
         "filter via send pid", 
         "int", 
         0, 
         false},
        
        {{"--filterPath"}, 
         "Filter path", 
         "string", 
         std::string(""), 
         false},
        
        {{"--traceNoAnonUds"}, 
         "only trace no anon uds like /tmp/sample.uds", 
         "bool", 
         false, 
         false},
        
        {{"--payload"}, 
         "Print payload", 
         "bool", 
         false, 
         false},
        
        {{"--force"}, 
         "Force enable payload printing", 
         "bool", 
         false, 
         false},
        
        {{"--pcapFile"}, 
         "Save output to pcap file", 
         "string", 
         std::string(""), 
         false},
        
        {{"--fromJson"}, 
         "read config args from json file", 
         "bool", 
         false, 
         false},
        
        {{"--vvv", "--verbose"}, 
         "Output more information", 
         "bool", 
         false, 
         false},
        
        {{"--generateConfigJson"}, 
         "Generate json config file", 
         "bool", 
         false, 
         false}
    };
}

}  // namespace ipc::ipcWatcher

#endif //IPC_IPC_WATCHER_ARG_METADATA_H
