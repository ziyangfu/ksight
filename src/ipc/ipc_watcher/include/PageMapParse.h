/*!
 * \brief 解析 /proc/pid/pagemap 文件的接口
 * \details 这部分需要超级用户权限root，注意 X86_64架构与ARM64架构的差异
 * */

#ifndef IPC_IPC_WATCHER_PAGEMAP_PARSE_H
#define IPC_IPC_WATCHER_PAGEMAP_PARSE_H

namespace ipcwatcher {
    void vaddrToPfn(unsigned long vaddr, unsigned long *pfn);
}

#endif //IPC_IPC_WATCHER_PAGEMAP_PARSE_H
