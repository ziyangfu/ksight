/**
* @brief 读取pagemap与kpagecount，获取物理页面的引用计数
* @compile
	g++ -o mem_count mem_count.cpp

*/

#include <unistd.h>
#include <fcntl.h>
#include <stdint.h>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <string>

// 获取页面大小
size_t getPageSize() {
    return sysconf(_SC_PAGESIZE);
}

// 打开文件，返回文件描述符
int openFile(const char* path) {
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open");
    }
    return fd;
}

// 读取文件偏移偏移处的uint64_t数据
uint64_t preadData(int fd, off_t offset) {
    uint64_t data = 0;
    ssize_t n = pread(fd, &data, sizeof(data), offset);
    if (n != sizeof(data)) {
        perror("pread");
        return 0;
    }
    return data;
}

// 获取虚拟地址对应的PFN（页框号）
uint64_t getPFN(int pagemap_fd, uint64_t vaddr) {
    size_t pageSize = getPageSize();
    uint64_t vpageIndex = vaddr / pageSize;
    off_t offset = vpageIndex * sizeof(uint64_t);
    uint64_t entry = preadData(pagemap_fd, offset);
    if (!(entry & (1ULL << 63))) {
        std::cerr << "Page not present in RAM." << std::endl;
        return 0;
    }
    uint64_t pfn = entry & ((1ULL << 55) - 1);
    return pfn;
}

// 获取物理页面的引用计数（需要kpagecount的文件描述符）
uint64_t getPageRefCount(int kpagecount_fd, uint64_t pfn) {
    off_t offset = pfn * sizeof(uint64_t);
    uint64_t refCount = preadData(kpagecount_fd, offset);
    return refCount;
}

// 关闭文件描述符
void closeFile(int fd) {
    if (fd >= 0) {
        close(fd);
    }
}

int main(int argc, char* argv[]) {
    if (argc == 3)
    {
        std::cout << "pid + vaddr mode" << std::endl;
        pid_t pid = std::stoi(argv[1]);
        uint64_t vaddr = 0;
        std::stringstream ss;
        ss << std::hex << argv[2];
        ss >> vaddr;

        std::string p = "/proc/" + std::to_string(pid) + "/pagemap";
        const char* pagemap_path = p.c_str();

        // 打开 pagemap 文件
        int pagemap_fd = openFile(pagemap_path);
        if (pagemap_fd < 0) {
            return 1;
        }

        // 打开 kpagecount 文件
        const char* kpagecount_path = "/proc/kpagecount";
        int kpagecount_fd = openFile(kpagecount_path);
        if (kpagecount_fd < 0) {
            closeFile(pagemap_fd);
            return 1;
        }

        uint64_t pfn = getPFN(pagemap_fd, vaddr);
        if (pfn == 0) {
            closeFile(pagemap_fd);
            closeFile(kpagecount_fd);
            return 1;
        }

        uint64_t refCount = getPageRefCount(kpagecount_fd, pfn);

        std::cout << "PFN: 0x" << std::hex << pfn << std::endl;
        std::cout << "Mapping reference count: " << std::dec << refCount << std::endl;

        closeFile(pagemap_fd);
        closeFile(kpagecount_fd);
    }

    if (argc == 2) {
        std::cout << "pfn mode" << std::endl;
        // 打开 kpagecount 文件
        const char* kpagecount_path = "/proc/kpagecount";
        int kpagecount_fd = openFile(kpagecount_path);
        if (kpagecount_fd < 0) {
            // closeFile(pagemap_fd);
            return 1;
        }
        uint64_t pfn = 0;
        std::stringstream ss;
        ss << std::hex << argv[1];
        ss >> pfn;
        uint64_t refCount = getPageRefCount(kpagecount_fd, pfn);

        std::cout << "PFN: 0x" << std::hex << pfn << std::endl;
        std::cout << "Mapping reference count: " << std::dec << refCount << std::endl;
    }
    


    // if (argc != 3) {
    //     std::cerr << "Usage: " << argv[0] << " <pid> <virtual_address_in_hex>\n";
    //     return 1;
    // }

    

    return 0;
}
