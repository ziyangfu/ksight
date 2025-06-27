# ipcwatcher - 进程间通信观测工具
## 1. 工具介绍
- **简介**

  unix domian socket（uds）作为一种IPC方式，与TCP/IP采用相同的socket接口，在本机通信中广泛应用。本机环境下由于不用经过网络协议栈，所以在性能上比TCP更有优势。比如Linux的X11，就是通过uds通信的。
  同时由于uds具有传递文件描述符的能力，且共享内存作为最快的IPC方式，没有自己的同步机制。因此，一种常规做法是采用uds作为共享内存的同步机制，并通过uds在进程间传递memfd。这种方式在通信中间件的设计实现上很常见，例如字节跳动的采用go语言实现的shmipc。
  **目前的观测手段**
  目前查看下来，观测手段较少。

  - 在系统调用跟踪方面，可以用strace跟踪到。
  - 在信息展示方面，可以用ss观测。
  - 而在数据包跟踪方面，没看到原生的跟踪工具，除了使用bpftrace可以跟踪一下之外，一种常规做法是，借助socat将uds的数据包转发到一个TCP连接上，然后使用wireshark监控TCP数据，间接观测uds的数据。

  不知道是否还有其他的观测工具，大家知道可以说下。
## 2. 依赖库
spdlog
fmt
argparse
libpacap


需求描述：
UDS

关键点：在不修改代码，重新编译程序，或者是一个第三方可执行文件时，可以
监控UDS IPC消息

1. 查看当前系统中所有的UDS连接信息，类似于ss命令 ss -x
2. 连接状态跟踪，显示UDS连接状态，构建类似于TCP的状态机跟踪机制
3. 统计信息展示，收集并展示每个UDS的流量统计，如发送或接收的数据量，错误数等
4. 进程相关，通过进程号，将UDS的通信关联到具体的用户空间进程


低功耗运行模式，即使用pr_debug打印到pipe上，而不是poll到用户空间并打印
2. 抓包与数据监控，追踪特定的UDS数据包信息，数据量统计、数据内容
3. 将追踪的信息保存为pcap文件，可以在wireshark中回放
4. uds消息头自定义，根据给定的消息头配置文件，解析UDS消息




shm








TODO:

共享内存的数据发送与接收观测

1. 匿名共享内存
2. 文件共享内存
3. 支持共享内存头的自定义，以一种格式，例如yaml或json
4. 零拷贝调试与追踪



要在 Linux 内核中像 Wireshark 追踪 TCP/IP 协议栈那样查看 Unix 域套接字（UDS）的方方面面，需要实现以下关键功能和需求：

---

### 1. **抓包与数据监控**
- **需求**：捕获 UDS 的通信数据流，包括发送和接收的数据内容。
- **实现方式**：
  - 利用 [skb](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/drivers/atm/he.h#L132-L132)（socket buffer）在 UDS 的收发路径上进行钩子插入（如通过 kprobe 或 tracepoint）。
  - 使用 eBPF 程序拦截 `unix_stream_sendmsg`、`unix_dgram_recvmsg` 等函数。

---

### 2. **连接状态追踪**
- **需求**：实时查看 UDS 的连接状态（如 [CONNECTED](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/drivers/scsi/53c700.h#L372-L373), [LISTEN](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/drivers/infiniband/hw/cxgb4/iw_cxgb4.h#L777-L777), [CLOSED](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/net/atm/mpoa_caches.h#L90-L91)）。
- **实现方式**：
  - 监控 `unix_accept`, `unix_connect`, `unix_release` 等函数。
  - 构建类似 TCP 的状态机跟踪机制。

---

### 3. **统计信息展示**
- **需求**：收集并展示每个 UDS 的流量统计（如发送/接收的数据量、错误数等）。
- **实现方式**：
  - 在 `struct unix_sock` 中维护计数器。
  - 提供 `/proc/<pid>/unix_stats` 接口或 sysfs 展示。

---

### 4. **进程关联**
- **需求**：将 UDS 的通信关联到具体的用户空间进程。
- **实现方式**：
  - 记录 socket 对应的 [task_struct](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/include/linux/sched.h#L662-L1225) 和 PID。
  - 结合 [proc](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/scripts/gdb/linux/proc.py#L0-L0) 文件系统显示 PID、进程名等信息。

---

### 5. **文件路径解析**
- **需求**：显示 UDS 的绑定路径（如 `/tmp/mysocket`）。
- **实现方式**：
  - 从 `struct unix_address` 获取路径信息。
  - 支持抽象命名空间（以 `\0` 开头的路径）的识别与展示。

---

### 6. **支持监听与事件通知**
- **需求**：提供类似于 `tcpdump` 的实时监听工具，支持过滤和事件触发。
- **实现方式**：
  - 使用 netlink 或字符设备接口向用户态推送事件。
  - 支持按路径、PID、协议类型（SOCK_STREAM/SOCK_DGRAM）过滤。

---

### 7. **安全与隔离**
- **需求**：确保 UDS 抓包行为不会影响系统稳定性或泄露敏感信息。
- **实现方式**：
  - 限制只有 root 权限才能启用监控。
  - 提供 SELinux/AppArmor 集成控制访问权限。

---

### 8. **调试与日志记录**
- **需求**：为内核模块添加调试日志输出，便于排查问题。
- **实现方式**：
  - 使用 [pr_debug()](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/tools/perf/util/debug.h#L25-L27) / [printk()](file:///home/fzy/Documents/13_软件开发资料/2.Linux系统/3.内核/01.内核源码/04_Linux_kernel_5.10_GIT/include/linux/printk.h#L174-L175) 输出详细信息。
  - 支持动态开关调试选项（如通过 module_param 控制）。

---

### 9. **eBPF 集成**
- **需求**：利用现代 eBPF 技术实现高效、低开销的 UDS 监控。
- **实现方式**：
  - 编写 BCC/BTF 工具挂钩到 `sys_enter_unix_*` 系列函数。
  - 收集上下文信息并导出到用户空间。

---

### 10. **用户空间工具开发**
- **需求**：构建一个类 `Wireshark` 的 GUI 或命令行工具来展示 UDS 数据。
- **实现方式**：
  - 使用 Python + PySide/PyQt 实现图形界面。
  - 命令行工具可基于 `libnl` 或自定义 socket 接口与内核交互。

---

### 总结
| 功能点 | 描述 |
|--------|------|
| 抓包与监控 | 捕获 UDS 发送/接收的数据 |
| 状态追踪 | 显示连接状态变化 |
| 统计信息 | 流量、错误计数等 |
| 进程关联 | 查看 PID、进程名 |
| 路径解析 | 显示绑定路径 |
| 实时监听 | 类似 tcpdump 的监听 |
| 安全机制 | 权限控制、防止滥用 |
| 日志与调试 | 方便内核模块调试 |
| eBPF 支持 | 高效低开销的监控方案 |
| 用户工具 | 提供可视化分析界面 |

这些功能的实现将帮助开发者深入理解 UDS 的运行机制，并用于调试复杂的应用场景。




在 共享内存通信 中，用户态程序通过 mmap 系统调用将一段物理内存映射到自己的虚拟地址空间中。随后，用户程序可以通过直接访问这段虚拟内存来读取或写入数据，这些操作最终会反映到物理内存上。

下面详细说明这一过程的机制和步骤：

🧠 一、核心流程概述
用户程序调用 mmap 创建共享内存映射
内核分配物理页并建立页表映射
用户程序访问虚拟地址 → 缺页异常触发页表填充
物理内存内容被加载到 CPU 缓存中
用户程序读取/写入数据 → 数据同步到物理内存
🔁 二、详细步骤解析
✅ Step 1：用户程序调用 mmap
c
void *addr = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
fd: 通常是打开的文件描述符（如 /dev/shm 或普通文件）或匿名共享内存。
MAP_SHARED: 表示多个进程共享该内存区域。
返回值 addr 是用户程序可以访问的虚拟内存地址。
在内核中，这个调用最终会调用 do_mmap，创建一个新的虚拟内存区域（VMA），并设置标志位为 VM_SHARED。

✅ Step 2：内核初始化 VMA 和物理内存映射
内核为这段虚拟内存区域分配一个 vm_area_struct（VMA）结构体。
如果是文件映射：
调用 shmem_zero_setup() 或 do_mmap() 初始化共享内存对象（如 shmem）。
文件内容会被缓存在 page cache 中。
如果是匿名共享内存：
使用 anon_vma 来管理多个进程共享的页面。
✅ Step 3：用户程序首次访问虚拟内存，触发缺页中断
当用户程序第一次访问 addr 指向的虚拟地址时，由于此时页表项（PTE）为空，CPU 触发缺页异常（Page Fault）。

缺页处理路径如下：
进入缺页处理函数 handle_mm_fault()
根据 VMA 类型判断是否为文件映射或匿名映射
如果是文件映射且为共享映射，调用 do_shared_fault()
如果是匿名映射，调用 do_anonymous_page()
相关函数：

do_shared_fault()：用于共享文件映射
do_anonymous_page()：用于匿名共享内存
__do_fault()：负责从磁盘或 page cache 中读取文件页
finish_fault()：将物理页映射到 PTE 中
✅ Step 4：物理页被映射到页表中
内核从伙伴系统或 page cache 中获取物理页。
构造 PTE（页表项）并插入到当前进程的页表中。
更新 TLB（Translation Lookaside Buffer）缓存。
例如：

c
entry = mk_pte(page, vma->vm_page_prot); // 构造 PTE
set_pte_at(vma->vm_mm, addr, pte, entry); // 插入到页表
✅ Step 5：用户程序读取数据
一旦页表项建立，用户程序就可以像访问普通内存一样访问 addr：

c
char *data = (char *)addr;
printf("Data: %s\n", data);
此时，CPU 会自动将虚拟地址转换为物理地址。
如果开启了缓存一致性（Cache Coherency），CPU 会确保读取的是最新的物理内存内容。
🧩 三、关键结构与函数说明
结构体/函数	作用
struct vm_area_struct *vma	表示一段虚拟内存区域，包含权限、偏移等信息
pte_t *pte	页表项，表示虚拟地址到物理地址的映射
mk_pte(page, prot)	构造一个 PTE，将物理页与虚拟地址关联
do_shared_fault()	处理共享文件映射的缺页
do_anonymous_page()	处理匿名共享内存的缺页
__do_fault()	从磁盘或 page cache 中读取文件页
finish_fault()	完成缺页处理，将物理页映射到页表
🔄 四、多进程共享内存的数据一致性
多个进程通过 mmap(..., MAP_SHARED) 映射同一段物理内存后：

所有进程看到的是同一块物理内存。
写入操作对其他进程可见（需注意同步问题）。
缓存一致性由硬件（如 MESI 协议）保障。
⚙️ 五、实际场景举例
示例：两个进程共享内存并通信
进程 A（写入者）
c
int shm_fd = shm_open("/my_shm", O_CREAT | O_RDWR, 0666);
ftruncate(shm_fd, 4096);

char *ptr = mmap(0, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
strcpy(ptr, "Hello from process A");
进程 B（读取者）
c
int shm_fd = shm_open("/my_shm", O_RDONLY, 0);
char *ptr = mmap(0, 4096, PROT_READ, MAP_SHARED, shm_fd, 0);
printf("Read: %s\n", ptr);  // 输出 "Hello from process A"
📌 六、总结
阶段	用户程序行为	内核行为
1. mmap 调用	分配虚拟地址空间	创建 VMA，设置 VM_SHARED
2. 首次访问	触发缺页中断	缺页处理，分配物理页
3. 页表建立	虚拟地址 → 物理地址	构造 PTE 并插入页表
4. 数据读取	直接访问虚拟内存	CPU 自动转换物理地址
5. 多进程共享	多个进程访问同一地址	所有进程共享物理页
   📚 七、参考资料推荐
   Linux 内核源码：
   mm/memory.c
   mm/mmap.c
   书籍推荐：
   《Understanding the Linux Virtual Memory Manager》
   《Linux Kernel Development》by Robert Love
   如果你希望进一步了解如何通过 eBPF 或 perf 工具监控共享内存的访问行为，或者需要构建一个完整的共享内存通信框架（包括同步、缓冲区管理、错误处理等），我可以继续为你提供详细方案和代码模板