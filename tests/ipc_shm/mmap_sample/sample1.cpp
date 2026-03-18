/*
演示如何将同一文件的不同部分映射到内存中，并为每个映射创建独立的 VMA（Virtual Memory Area）
*/

#include <iostream>
#include <fstream>
#include <vector>
#include <sys/mman.h> // For mmap
#include <sys/stat.h> // For fstat
#include <fcntl.h>    // For open
#include <unistd.h>   // For close, lseek

int main() {
    const char* filename = "test_file.txt";
    int fd = -1;
    void *addr1 = MAP_FAILED;
    void *addr2 = MAP_FAILED;

    // 1. 创建一个测试文件 (如果不存在)
    std::ofstream outfile(filename);
    if (outfile.is_open()) {
        outfile << "This is the first part of the file.\n";
        outfile << "This is the second part of the file, located at offset 4096.\n";
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

    // 3. 第一次映射：文件的开始部分 (偏移 0)
    // 映射 4096 字节，保护模式为读写，使用 MAP_SHARED，从文件描述符 fd 的偏移 0 开始。
    addr1 = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr1 == MAP_FAILED) {
        std::cerr << "Error on first mmap: " << errno << std::endl;
        close(fd);
        return 1;
    }
    std::cout << "First mapping at address: " << addr1 << std::endl;

    // 4. 第二次映射：文件的另一部分 (偏移 4096)
    // 映射 4096 字节，保护模式为读写，使用 MAP_SHARED，从文件描述符 fd 的偏移 4096 开始。
    addr2 = mmap(NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 4096);
    if (addr2 == MAP_FAILED) {
        std::cerr << "Error on second mmap: " << errno << std::endl;
        munmap(addr1, 4096); // Unmap the first mapping before exiting
        close(fd);
        return 1;
    }
    std::cout << "Second mapping at address: " << addr2 << std::endl;

    // 5. 验证映射的内容 (可选)
    std::cout << "\n--- Content from first mapping ---" << std::endl;
    // 将 addr1 强制转换为 char* 以便逐字节访问
    char* char_addr1 = static_cast<char*>(addr1);
    for (int i = 0; i < 30 && i < 4096; ++i) { // Print first 30 characters
        std::cout << char_addr1[i];
    }
    std::cout << std::endl;

    std::cout << "\n--- Content from second mapping ---" << std::endl;
    char* char_addr2 = static_cast<char*>(addr2);
    for (int i = 0; i < 40 && i < 4096; ++i) { // Print first 40 characters
        std::cout << char_addr2[i];
    }
    std::cout << std::endl;

    // 6. 解除映射
    if (munmap(addr1, 4096) == -1) {
        std::cerr << "Error unmapping first mapping: " << errno << std::endl;
    } else {
        std::cout << "\nFirst mapping unmapped successfully." << std::endl;
    }

    if (munmap(addr2, 4096) == -1) {
        std::cerr << "Error unmapping second mapping: " << errno << std::endl;
    } else {
        std::cout << "Second mapping unmapped successfully." << std::endl;
    }

    // 7. 关闭文件描述符
    close(fd);
    std::cout << "File descriptor closed." << std::endl;

    // 8. 清理测试文件 (可选)
    remove(filename); // uncomment to remove the file after execution

    return 0;
}

