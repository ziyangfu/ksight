/*!
 * \brief 使用uds传递memfd，使用共享内存进行数据传递
 * \usage
 *      compile ：
 *          g++ ./client_uds_shm.cpp -o client
 *          g++ ./server_uds_shm.cpp -o server
 * */

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
    std::cout << "Client PID is: " << pid << std::endl;
    // 创建memfd
    int memfd = memfd_create("shared_mem", MFD_CLOEXEC);
    if (memfd == -1) {
        perror("memfd_create");
        return 1;
    }

    // 设置共享内存大小
    const size_t size = 4096;
    if (ftruncate(memfd, size) == -1) {
        perror("ftruncate");
        return 1;
    }

    // 映射共享内存
    void* ptr = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
    if (ptr == MAP_FAILED) {
        perror("mmap");
        return 1;
    }

    // 初始化共享内存
    const char* message = "Hello from client!";
    std::memcpy(ptr, message, std::strlen(message) + 1);

    // 创建Unix域套接字
    int sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return 1;
    }

    // 连接到服务端
    sockaddr_un addr{};
    addr.sun_family = AF_UNIX;
    std::strncpy(addr.sun_path, "/tmp/memfd_socket", sizeof(addr.sun_path) - 1);

    if (connect(sockfd, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) == -1) {
        perror("connect");
        return 1;
    }

    // 发送memfd
    char buf[CMSG_SPACE(sizeof(int))];
    msghdr msg{};
    iovec iov{};
    cmsghdr* cmsg;

    char dummy = 's';
    iov.iov_base = &dummy;
    iov.iov_len = 1;

    msg.msg_name = nullptr;
    msg.msg_namelen = 0;
    msg.msg_iov = &iov;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(int));

    *reinterpret_cast<int*>(CMSG_DATA(cmsg)) = memfd;

    msg.msg_controllen = cmsg->cmsg_len;

    if (sendmsg(sockfd, &msg, 0) == -1) {
        perror("sendmsg");
        return 1;
    }

    // 等待服务端响应
    std::cout << "Client waiting for server response..." << std::endl;
    sleep(2);

    // 读取服务端写入的数据
    std::cout << "Client read from shared memory: " << static_cast<char*>(ptr) << std::endl;

    // 清理
    munmap(ptr, size);
    close(memfd);
    close(sockfd);

    return 0;
}
