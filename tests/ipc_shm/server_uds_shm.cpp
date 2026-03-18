#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/uio.h>

int main() {
    pid_t pid = getpid();
    std::cout << "Server PID is: " << pid << std::endl;
    // 创建Unix域套接字
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    // 绑定套接字
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, "/tmp/memfd_socket", sizeof(addr.sun_path) - 1);

    unlink("/tmp/memfd_socket"); // 确保路径可用

    if (bind(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        perror("bind");
        return 1;
    }

    // 监听连接
    if (listen(sockfd, 1) == -1) {
        perror("listen");
        return 1;
    }

    std::cout << "Server waiting for connection..." << std::endl;

    // 接受连接
    int connfd = accept(sockfd, nullptr, nullptr);
    if (connfd == -1) {
        perror("accept");
        return 1;
    }

    // 接收memfd
    char buf[CMSG_SPACE(sizeof(int))];
    msghdr msg{};
    iovec iov{};
    cmsghdr* cmsg;

    char dummy;
    iov.iov_base = &dummy;
    iov.iov_len = 1;

    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    if (recvmsg(connfd, &msg, 0) == -1) {
        perror("recvmsg");
        return 1;
    }

    cmsg = CMSG_FIRSTHDR(&msg);
    if (cmsg == nullptr || cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS) {
        std::cerr << "Invalid control message" << std::endl;
        return 1;
    }

    int memfd = *reinterpret_cast<int*>(CMSG_DATA(cmsg));

    // 映射共享内存
    const size_t size = 4096;
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // 读取客户端写入的数据
    std::cout << "Server read from shared memory: " << static_cast<char*>(ptr) << std::endl;

    // 向共享内存写入数据
    const char* response = "server!";
    std::memcpy(ptr, response, std::strlen(response) + 1);
    std::cout << "dummy = " << dummy << std::endl;

    // 清理
    munmap(ptr, size);
    close(memfd);
    close(connfd);
    close(sockfd);

    return 0;
}
