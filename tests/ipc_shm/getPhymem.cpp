/**
 * @brief 根据pid与虚拟地址， 读取pagemap，获取物理地址
 * @compile 
 *      g++ -o get_phy_mem getPhymem.cpp
 * @usage 
 *      sudo ./get_phy_mem <PID> <Virtual Address>
*/

#include <iostream>
#include <cstdint>
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstring>
#include <sstream>

using namespace std;

// 获取页大小（通常为4096字节）
static size_t PAGE_SIZE = getpagesize();

// 检查是否具有超级用户权限
void check_root_privileges() {
    if (geteuid() != 0) {
        cerr << "Error: This program requires root privileges to access /proc/<pid>/pagemap" << endl;
        cerr << "Please run with sudo or as root user." << endl;
        exit(EXIT_FAILURE);
    }
}


// 检查虚拟地址是否有效并转换为uintptr_t
bool parse_vaddr(const string& input, uintptr_t& result) {
    istringstream iss(input);
    iss >> hex >> result;
    return !iss.fail();
}

int test(pid_t pid, std::string vaddr_str) {
    // 输入PID和虚拟地址
    //cout << "Enter target PID: ";
    //pid_t pid;
    //cin >> pid;
    //pid = 1067171;
    
    //cout << "Enter virtual address (hex format, e.g., 0x7ffe12345678): ";
    //string vaddr_str;
    // cin >> vaddr_str;
    //vaddr_str = "00007fce0563a000";
    
    uintptr_t vaddr;
    if (!parse_vaddr(vaddr_str, vaddr)) {
        cerr << "Invalid virtual address format\n";
        return EXIT_FAILURE;
    }

    // 构造pagemap路径
    char pagemap_path[256];
    snprintf(pagemap_path, sizeof(pagemap_path), "/proc/%d/pagemap", pid);

    // 打开pagemap文件
    const int fd = open(pagemap_path, O_RDONLY);
    if (fd == -1) {
        cerr << "Failed to open pagemap: " << strerror(errno) << "\n";
        return EXIT_FAILURE;
    }
    std::cout << "open pagemap OK!" << std::endl;

    // 计算页索引和偏移
    const uintptr_t vpage = vaddr / PAGE_SIZE;
    const off_t file_offset = vpage * sizeof(uint64_t);
    const uintptr_t page_offset = vaddr % PAGE_SIZE;

    // 读取pagemap条目
    uint64_t entry;
    if (pread(fd, &entry, sizeof(entry), file_offset) != sizeof(entry)) {
        cerr << "Failed to read pagemap entry: " << strerror(errno) << "\n";
        close(fd);
        return EXIT_FAILURE;
    }
    close(fd);

    // 检查页面是否存在于物理内存
    if (!(entry & (1ULL << 63))) {
        cerr << "Page not present in physical memory\n";
        return EXIT_FAILURE;
    }
    
    // 提取物理页帧号（PFN）
    const uint64_t pfn = entry & 0x7FFFFFFFFFFFFF;
    cout << "pfn = " << entry << endl;
    // 计算物理地址
    const uintptr_t phys_addr = (pfn << 12) | page_offset;

    // 输出结果
    cout << "Physical address: 0x" << hex << phys_addr << "\n";
    return EXIT_SUCCESS;

}

int main(int argc, char* argv[]) {
    // 检查超级用户权限
    check_root_privileges();
    if (argc < 3) {
        cerr << "Usage: " << argv[0] << " <PID> <Virtual Address>" << endl;
        return EXIT_FAILURE;
    }

    pid_t pid = static_cast<pid_t>(stol(argv[1]));
    string vaddr_str = argv[2];

    std::cout << "PID: " << pid << " Virtual Address: " << vaddr_str << std::endl;

    return test(pid, vaddr_str);
}