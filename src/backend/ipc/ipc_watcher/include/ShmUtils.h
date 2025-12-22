#ifndef IPC_IPC_WATCHER_SHM_UTILS_H
#define IPC_IPC_WATCHER_SHM_UTILS_H

#include <string>
#include <vector>

namespace ipc::ipcWatcher::shmUtils {

std::vector<int> commandToPid(const std::string& processName);
std::string pidToCommand(std::uint32_t pid);
void handleCommand(std::string& command);
std::string getShmPath(int pid, int fd);  /** 仅针对非匿名 文件共享映射 */

std::vector<unsigned long> getShmVmAddr(int pid, std::string& shmPath); /** 起始地址与结束地址 */
std::string getShmVmAddrString(int pid, std::string& shmPath);
/*!
 * \brief 输入pid与虚拟地址，获取共享内存的物理地址
 * */
uintptr_t vaddrToPhysicalAddr(pid_t pid, unsigned long vaddr);
std::string vaddrToPhysicalAddrString(pid_t pid, unsigned long vaddr);

std::string getShmProtString(unsigned long prot);
std::string getShmFlagString(unsigned long flag);

std::string toHex(const char* data, size_t len);
std::string hexToString(const char* data, size_t len);

/*!
 * \brief 轮询 cat /proc/{pid}/fd/{fd}的数据，这是共享内存写入与读取的payload
 *        识别到数据改变时，输出一次。
 * */
std::string readShmPayloadCycle(int pid, int fd);
/*!
 * \brief 输入十进制数，转为16进制数
 * */
std::string decimalToHex(long long decimal, bool withPrefix);
/*!
 * \brief 在用户空间，获取该段物理内存的引用计数（_mapcount），即有多少个进程在引用该段物理内存
 * */
int getShmMapCount(unsigned long pfn);
int getShmMapCountViaPhyAddr(uintptr_t phyAddr);

}

#endif //IPC_IPC_WATCHER_SHM_UTILS_H

