/*
演示如何将同一文件的不同部分以不同的保护权限（如只读或读写）映射到内存中，并为每个映射创建独立的 VMA
*/
#include <iostream>
#include <fstream>
#include <vector>
#include <sys/mman.h> // For mmap
#include <sys/stat.h> // For fstat
#include <fcntl.h>    // For open
#include <unistd.h>   // For close, lseek
#include <string.h>   // For strerror

int main() {
    const char* filename = "test_file_permissions.txt";
    int fd = -1;
    void *addr_readonly = MAP_FAILED;
    void *addr_readwrite = MAP_FAILED;

    // 1. 创建一个测试文件 (如果不存在)
    std::ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << "This is the read-only part.\n";
        outfile << "This is the read-write part.\n";
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

    // 3. 第一次映射：只读权限 (偏移 0)
    // 映射 4096 字节，保护模式为只读 (PROT_READ)，使用 MAP_SHARED，从文件描述符 fd 的偏移 0 开始。
    addr_readonly = mmap(NULL, 4096, PROT_READ, MAP_SHARED, fd, 0);
    if (addr_readonly == MAP_FAILED) {
        std::cerr << "Error on read-only mmap: " << strerror(errno) << std::endl;
        close(fd);
        return 1;
    }
    std::cout << "Read-only mapping at address: " << addr_readonly << std::endl;

    // 4. 第二次映射：读写权限 (偏移 4096)
    // 映射 4096 字节，保护模式为读写 (PROT_READ | PROT_WRITE)，使用 MAP_SHARED，从文件描述符 fd 的偏移 4096 开始。
    addr_readwrite = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 4096);
    if (addr_readwrite == MAP_FAILED) {
        std::cerr << "Error on read-write mmap: " << strerror(errno) << std::endl;
        munmap(addr_readonly, 4096); // Unmap the first mapping before exiting
        close(fd);
        return 1;
    }
    std::cout << "Read-write mapping at address: " << addr_readwrite << std::endl;

    // 5. 尝试修改只读映射 (应该会触发 SIGSEGV)
    std::cout << "\nAttempting to write to read-only mapping..." << std::endl;
    char* char_addr_readonly = static_cast<char*>(addr_readonly);
    // char_addr_readonly[0] = 'X'; // This line is commented out because it would crash the program.
                                // Uncommenting it would demonstrate the protection.

    // 6. 修改读写映射
    std::cout << "Writing to read-write mapping..." << std::endl;
    char* char_addr_readwrite = static_cast<char*>(addr_readwrite);
    char_addr_readwrite[0] = 'M'; // Modify the first character
    std::cout << "Successfully wrote 'M' to read-write mapping." << std::endl;

    // 7. 验证修改 (可选)
    std::cout << "\n--- Content from read-write mapping after modification ---" << std::endl;
    for (int i = 0; i < 20; ++i) {
        std::cout << char_addr_readwrite[i];
    }
    std::cout << std::endl;

    // 8. 解除映射
    if (munmap(addr_readonly, 4096) == -1) {
        std::cerr << "Error unmapping read-only mapping: " << strerror(errno) << std::endl;
    } else {
        std::cout << "\nRead-only mapping unmapped successfully." << std::endl;
    }

    if (munmap(addr_readwrite, 4096) == -1) {
        std::cerr << "Error unmapping read-write mapping: " << strerror(errno) << std::endl;
    } else {
        std::cout << "Read-write mapping unmapped successfully." << std::endl;
    }

    // 9. 关闭文件描述符
    close(fd);
    std::cout << "File descriptor closed." << std::endl;

    // 10. 清理测试文件 (可选)
    remove(filename); // uncomment to remove the file after execution

    return 0;
}

