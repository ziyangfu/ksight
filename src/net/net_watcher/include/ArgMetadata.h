/*!
 * \brief Argument metadata definition for net_watcher
 * \file  ArgMetadata.h
 * */

#ifndef NET_NET_WATCHER_ARG_METADATA_H
#define NET_NET_WATCHER_ARG_METADATA_H

#include <string>
#include <vector>
#include <any>

namespace net::netWatcher {

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
        {{"-a", "--all"}, 
         "set to trace CLOSED connection", 
         "bool", 
         false, 
         false},
        
        {{"-e", "--err"}, 
         "set to trace TCP error packets", 
         "bool", 
         false, 
         false},
        
        {{"-x", "--extra"}, 
         "set to trace extra conn info", 
         "bool", 
         false, 
         false},
        
        {{"-r", "--retrans"}, 
         "set to trace extra retrans info", 
         "bool", 
         false, 
         false},
        
        {{"-t", "--time"}, 
         "set to trace layer time of each packet", 
         "bool", 
         false, 
         false},
        
        {{"-i", "--http"}, 
         "set to trace http info", 
         "bool", 
         false, 
         false},
        
        {{"-s", "--sport"}, 
         "trace this source port only", 
         "int", 
         0, 
         false},
        
        {{"-d", "--dport"}, 
         "trace this destination port only", 
         "int", 
         0, 
         false},
        
        {{"-u", "--udp"}, 
         "trace the udp message", 
         "bool", 
         false, 
         false},
        
        {{"-n", "--net_filter"}, 
         "trace ipv4 packget filter", 
         "bool", 
         false, 
         false},
        
        {{"-k", "--drop_reason"}, 
         "trace kfree", 
         "bool", 
         false, 
         false},
        
        {{"-F", "--addr_to_func"}, 
         "translation addr to func and offset", 
         "bool", 
         false, 
         false},
        
        {{"-I", "--icmptime"}, 
         "set to trace layer time of icmp", 
         "bool", 
         false, 
         false},
        
        {{"-S", "--tcpstate"}, 
         "set to trace tcpstate", 
         "bool", 
         false, 
         false},
        
        {{"-L", "--timeload"}, 
         "analysis time load", 
         "bool", 
         false, 
         false},
        
        {{"-D", "--dns"}, 
         "set to trace dns information", 
         "bool", 
         false, 
         false},
        
        {{"-A", "--stack"}, 
         "set to trace of stack", 
         "bool", 
         false, 
         false},
        
        {{"-M", "--mysql"}, 
         "set to trace mysql information", 
         "bool", 
         false, 
         false},
        
        {{"-R", "--redis"}, 
         "set to trace redis information", 
         "bool", 
         false, 
         false},
        
        {{"-C", "--count"}, 
         "specify the time to count the number of requests", 
         "int", 
         0, 
         false},
        
        {{"-T", "--rtt"}, 
         "set to trace rtt", 
         "bool", 
         false, 
         false},
        
        {{"-U", "--rst_counters"}, 
         "set to trace rst", 
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

}  // namespace net::netWatcher

#endif // NET_NET_WATCHER_ARG_METADATA_H
