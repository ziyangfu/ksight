#ifndef NET_TCP_NAGLE_AGENT_COM_H
#define NET_TCP_NAGLE_AGENT_COM_H

#include <string>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <nlohmann/json.hpp>
#include "spdlog/spdlog.h"

namespace net::tcpNagle {

class AgentCom {
public:
    static bool send(const nlohmann::json& j) {
        const char* sock_path = getenv("KSIGHT_AGENT_SOCK");
        if (!sock_path) {
            SPDLOG_ERROR("KSIGHT_AGENT_SOCK environment variable not set");
            return false;
        }

        int fd = socket(AF_UNIX, SOCK_STREAM, 0);
        if (fd < 0) {
            SPDLOG_ERROR("Failed to create UDS socket: {}", strerror(errno));
            return false;
        }

        struct sockaddr_un addr;
        memset(&addr, 0, sizeof(addr));
        addr.sun_family = AF_UNIX;
        strncpy(addr.sun_path, sock_path, sizeof(addr.sun_path) - 1);

        if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            SPDLOG_ERROR("Failed to connect to Agent UDS at {}: {}", sock_path, strerror(errno));
            close(fd);
            return false;
        }

        std::string s = j.dump();
        ssize_t sent = 0;
        ssize_t total = s.length();
        const char* p = s.c_str();

        while (sent < total) {
            ssize_t n = write(fd, p + sent, total - sent);
            if (n < 0) {
                SPDLOG_ERROR("Failed to write to Agent UDS: {}", strerror(errno));
                close(fd);
                return false;
            }
            sent += n;
        }

        close(fd);
        return true;
    }
};

} // namespace net::tcpNagle

#endif // NET_TCP_NAGLE_AGENT_COM_H
