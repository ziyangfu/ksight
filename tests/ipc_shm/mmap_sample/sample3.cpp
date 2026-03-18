#include <iostream>
#include <fstream>
#include <vector>
#include <sys/mman.h> // For mmap
#include <sys/stat.h> // For fstat
#include <fcntl.h>    // For open
#include <unistd.h>   // For close, lseek
#include <string.h>   // For strerror
#include <errno.h>    // For errno

int main() {
    const char* filename = "test_file_explicit_addr.txt";
    int fd = -1;
    void *addr1 = MAP_FAILED;
    void *addr2 = MAP_FAILED;

    // 使用一个固定且较小的地址作为目标，例如 0x10000000
    // 注意：并不是所有地址都可以随意映射，可能需要 root 权限或特定的系统配置。
    // 在大多数现代系统中，直接请求低地址可能会被拒绝，或者映射到更高的可用地址。
    // 这里的 0x10000000 是一个演示用途的示例。
    // 更现实的做法是让内核选择地址 (传 NULL)。
    const void* preferred_addr1 = (void*)0x10000000;
    const void* preferred_addr2 = (void*)0x20000000;

    // 1. 创建一个测试文件 (如果不存在)
    std::ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << "Data for first explicit mapping.\n";
        outfile.close();
    } else {
        std::cerr << "Error creating test file: " << filename << std::endl;
        return 1;
    }

    // 2. 打开文件以获取文件描述符
    fd = open(filename, O_RDWR); // O_RDWR for read and write access
    if (fd == -1) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return 1;
    }
    // 设置共享内存大小
    if (ftruncate(fd, 4096*2) == -1) {
        perror("ftruncate");
        close(fd);
        return 1;
    }

    // 3. 第一次显式地址映射
    // 尝试将 4096 字节映射到 preferred_addr1，使用 MAP_SHARED。
    // 注意：MAP_FIXED 标志可以强制使用指定地址，但可能导致现有映射被覆盖。
    // 这里不使用 MAP_FIXED，允许内核在 preferred_addr1 无法使用时选择一个替代地址。
    addr1 = mmap(const_cast<void*>(preferred_addr1), 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr1 == MAP_FAILED) {
        std::cerr << "Error on first explicit mmap (requested at " << preferred_addr1 << "): " << strerror(errno) << std::endl;
        // 如果指定地址失败，可能需要尝试其他地址或不指定地址
        // 让我们也尝试一次不指定地址的映射作为对比
        std::cout << "Attempting fallback mmap (no specific address requested)..." << std::endl;
        addr1 = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr1 == MAP_FAILED) {
            std::cerr << "Fallback mmap failed as well." << std::endl;
            close(fd);
            return 1;
        }
        std::cout << "Fallback mapping successful at address: " << addr1 << std::endl;
    } else {
        std::cout << "First explicit mapping at address: " << addr1 << std::endl;
    }

    // 4. 第二次显式地址映射
    // 尝试将 4096 字节映射到 preferred_addr2
    addr2 = mmap(const_cast<void*>(preferred_addr2), 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr2 == MAP_FAILED) {
        std::cerr << "Error on second explicit mmap (requested at " << preferred_addr2 << "): " << strerror(errno) << std::endl;
        // 同样，尝试回退映射
        std::cout << "Attempting fallback mmap (no specific address requested)..." << std::endl;
        addr2 = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (addr2 == MAP_FAILED) {
            std::cerr << "Fallback mmap failed as well." << std::endl;
            munmap(addr1, 4096); // Clean up first mapping
            close(fd);
            return 1;
        }
        std::cout << "Fallback mapping successful at address: " << addr2 << std::endl;
    } else {
        std::cout << "Second explicit mapping at address: " << addr2 << std::endl;
    }

    // 5. 验证映射是否在预期地址 (如果成功)
    if (addr1 != preferred_addr1) {
        std::cout << "Note: First mapping was not at the requested " << preferred_addr1 << ", but at " << addr1 << std::endl;
    }
    if (addr2 != preferred_addr2) {
        std::cout << "Note: Second mapping was not at the requested " << preferred_addr2 << ", but at " << addr2 << std::endl;
    }

    // 6. 写入和读取数据 (如果成功映射)
    if (addr1 != MAP_FAILED && addr2 != MAP_FAILED) {
        char* char_addr1 = static_cast<char*>(addr1);
        char_addr1[0] = 'A';
        char_addr1[1] = 'B';
        std::cout << "\nWritten 'AB' to first mapping." << std::endl;

        char* char_addr2 = static_cast<char*>(addr2);
        char_addr2[0] = 'C';
        char_addr2[1] = 'D';
        std::cout << "Written 'CD' to second mapping." << std::endl;

        std::cout << "Content from first mapping: " << char_addr1[0] << char_addr1[1] << std::endl;
        std::cout << "Content from second mapping: " << char_addr2[0] << char_addr2[1] << std::endl;
    }

    // 7. 解除映射
    if (addr1 != MAP_FAILED && munmap(addr1, 4096) == -1) {
        std::cerr << "Error unmapping first mapping: " << strerror(errno) << std::endl;
    } else if (addr1 != MAP_FAILED) {
        std::cout << "\nFirst mapping unmapped successfully." << std::endl;
    }

    if (addr2 != MAP_FAILED && munmap(addr2, 4096) == -1) {
        std::cerr << "Error unmapping second mapping: " << strerror(errno) << std::endl;
    } else if (addr2 != MAP_FAILED) {
        std::cout << "Second mapping unmapped successfully." << std::endl;
    }

    // 8. 关闭文件描述符
    close(fd);
    std::cout << "File descriptor closed." << std::endl;

    // 9. 清理测试文件 (可选)
    remove(filename); // uncomment to remove the file after execution

    return 0;
}

