使用范例

```bash
sudo ./ipcwatcher -p <pid>



```


```bash
fzy@fzy-Lenovo:~/Downloads/04_bcc_ebpf/ksight/build/src/backend/ipc/ipc_watcher$ sudo ./ipcwatcher -u |grep 2278245
286574682620   2282502    /usr/bin/python3               2278245    /usr/bin/python3               14         SOCK_STREAM  <none>                        
286575683126   2282502    /usr/bin/python3               2278245    /usr/bin/python3               14         SOCK_STREAM  <none>                        
286576684392   2282502    /usr/bin/python3               2278245    /usr/bin/python3               14         SOCK_STREAM  <none>                        
286577685683   2282502    /usr/bin/python3               2278245    /usr/bin/python3               14         SOCK_STREAM  <none>                        
286578687164   2282502    /usr/bin/python3               2278245    /usr/bin/python3               14         SOCK_STREAM  <none>                        
286579688227   2282502    /usr/bin/python3               2278245    /usr/bin/python3               14         SOCK_STREAM  <none>                        
286580689499   2282502    /usr/bin/python3               2278245    /usr/bin/python3               14         SOCK_STREAM  <none>      
```


带payload，原始信息
```bash
fzy@fzy-Lenovo:~/Downloads/04_bcc_ebpf/ksight/build/src/backend/ipc/ipc_watcher$ sudo ./ipcwatcher -u | grep 2294614
288555552381 2294625    /usr/bin/python3                    2294614    /usr/bin/python3                    14         SOCK_STREAM  <none>                         Hello, Server!                                              
288556553661 2294625    /usr/bin/python3                    2294614    /usr/bin/python3                    14         SOCK_STREAM  <none>                         Hello, Server!                                              
288557555000 2294625    /usr/bin/python3                    2294614    /usr/bin/python3                    14         SOCK_STREAM  <none>                         Hello, Server!      
```

带payload hex输出
```bash
fzy@fzy-Lenovo:~/Downloads/04_bcc_ebpf/ksight/build/src/backend/ipc/ipc_watcher$ sudo ./ipcwatcher -u | grep 2295660
289064585472 2295661    /usr/bin/python3                    2295660    /usr/bin/python3                    14         SOCK_STREAM  <none>                         48 65 6c 6c 6f 2c 20 53 65 72 76 65 72 21                   
289065586492 2295661    /usr/bin/python3                    2295660    /usr/bin/python3                    14         SOCK_STREAM  <none>                         48 65 6c 6c 6f 2c 20 53 65 72 76 65 72 21                   
289066587943 2295661    /usr/bin/python3                    2295660    /usr/bin/python3                    14         SOCK_STREAM  <none>                         48 65 6c 6c 6f 2c 20 53 65 72 76 65 72 21                   

```

实例：
```bash
fzy@fzy-Lenovo:<xx>/src/backend/ipc/ipc_watcher$ sudo ./ipcwatcher -u
config_.pid val = 0Tracing UDS send/recv events... Ctrl+C to exit
Timestamp      sendPID    sendComm                       recvPID    recvComm                       Size       Type         Path                          
284596670884   2212981    /snap/clion/353/bin/clion      822        /usr/sbin/rsyslogd             133        SOCK_STREAM  <none>                        
284598877992   1082459    /opt/wechat/RadiumWMPF/runtime 1          /sbin/init                     104        SOCK_STREAM  <none>                        
284609802564   2337       /opt/sogoupinyin/files/bin/sog 822        /usr/sbin/rsyslogd             76         SOCK_STREAM  <none>                        
284611106344   2148       /usr/lib/xorg/Xorg             822        /usr/sbin/rsyslogd             8          SOCK_STREAM                                
284621541717   2148       /usr/lib/xorg/Xorg             822        /usr/sbin/rsyslogd             8          SOCK_STREAM                                
284621652874   2148       /usr/lib/xorg/Xorg             1          /sbin/init                     40         SOCK_STREAM                                
284623333648   2148       /usr/lib/xorg/Xorg             822        /usr/sbin/rsyslogd             8          SOCK_STREAM                                
284623438030   2148       /usr/lib/xorg/Xorg             822        /usr/sbin/rsyslogd             40         SOCK_STREAM                                
284623450421   170957     /usr/share/code/code           822        /usr/sbin/rsyslogd             129        SOCK_STREAM  <none>                        
284628485607   1082459    /opt/wechat/RadiumWMPF/runtime 822        /usr/sbin/rsyslogd             104        SOCK_STREAM  <none>                        
284634717671   1082205    /opt/wechat/RadiumWMPF/runtime 308        /lib/systemd/systemd-journald  104        SOCK_STREAM  <none>                        
284634718567   1082459    /opt/wechat/RadiumWMPF/runtime 822        /usr/sbin/rsyslogd             104        SOCK_STREAM  <none>                        
284634737352   807        /usr/bin/dbus-daemon           308        /lib/systemd/systemd-journald  255        SOCK_STREAM  /run/dbus/system_bus_socket   
284634744743   2212981    /snap/clion/353/bin/clion      308        /lib/systemd/systemd-journald  16376      SOCK_STREAM  <none>                        
284634745463   2289       /usr/bin/gnome-shell           308        /lib/systemd/systemd-journald  20         SOCK_STREAM  <none>                        
284634745768   2212981    /snap/clion/353/bin/clion      822        /usr/sbin/rsyslogd             16392      SOCK_STREAM  <none>                        
284634746359   2212981    /snap/clion/353/bin/clion      308        /lib/systemd/systemd-journald  16392      SOCK_STREAM  <none>                        
284634747035   2212981    /snap/clion/353/bin/clion      308        /lib/systemd/systemd-journald  16384      SOCK_STREAM  <none>                        
284634747731   2212981    /snap/clion/353/bin/clion      308        /lib/systemd/systemd-journald  16364      SOCK_STREAM  <none>                        
284634748307   2212981    /snap/clion/353/bin/clion      308        /lib/systemd/systemd-journald  16380      SOCK_STREAM  <none>                        
284634748732   807        /usr/bin/dbus-daemon           308        /lib/systemd/systemd-journald  72         SOCK_STREAM  /run/dbus/system_bus_socket   
284634749021   1          /sbin/init                     822        /usr/sbin/rsyslogd             178        SOCK_STREAM  <none>                        
284634749503   2212981    /snap/clion/353/bin/clion      308        /lib/systemd/systemd-journald  16372      SOCK_STREAM  <none>                        
284634750052   2148       /usr/lib/xorg/Xorg             308        /lib/systemd/systemd-journald  32         SOCK_STREAM                                
284634752097   1082459    /opt/wechat/RadiumWMPF/runtime 308        /lib/systemd/systemd-journald  104        SOCK_STREAM  <none>                        
284634770809   1          /sbin/init                     308        /lib/systemd/systemd-journald  1092       SOCK_STREAM  <none>                        
284634771000   1          /sbin/init                     822        /usr/sbin/rsyslogd             1092       SOCK_STREAM  <none>                        
284634771164   1          /sbin/init                     1          /sbin/init                     224        SOCK_STREAM  <none>                        
284634780870   2269694    /usr/lib/NetworkManager/nm-dis 308        /lib/systemd/systemd-journald  184        SOCK_STREAM  <none>                        
284634781231   807        /usr/bin/dbus-daemon           308        /lib/systemd/systemd-journald  215        SOCK_STREAM  /run/dbus/system_bus_socket   
284645007260   807        /usr/bin/dbus-daemon           308        /lib/systemd/systemd-journald  189        SOCK_STREAM  /run/dbus/system_bus_socket   
284645007323   807        /usr/bin/dbus-daemon           822        /usr/sbin/rsyslogd             189        SOCK_STREAM  /run/dbus/system_bus_socket   
284621653139   842        /lib/systemd/systemd-logind    940682     /usr/share/code/code           10         SOCK_DGRAM   <none>                        
284621653134   350        /lib/systemd/systemd-udevd     1554       /usr/bin/dockerd               10         SOCK_DGRAM   <none>                        
284665225205   2289       /usr/bin/gnome-shell           308        /lib/systemd/systemd-journald  24         SOCK_STREAM  <none>                        
284665225458   2148       /usr/lib/xorg/Xorg             822        /usr/sbin/rsyslogd             32         SOCK_STREAM                               
```


共享内存

传统工具或市场上以后的工具
```bash
fzy@fzy-Lenovo:~$ ipcs -m

------ Shared Memory Segments --------
key        shmid      owner      perms      bytes      nattch     status      
0x7ef6b94f 0          daemon     666        256        1                       
0xccf909d3 1          daemon     666        4096       1                       
0x06377f5f 2          daemon     666        128868     1                       
0x00000000 2195459    fzy        600        524288     2          dest         
0x00000000 2195462    fzy        600        524288     2          dest         
0x00000000 2195463    fzy        600        524288     2          dest         
0x00000000 16         fzy        600        524288     2          dest         
0x00000000 2195476    fzy        600        524288     2          dest    
```



```bash
fzy@fzy-Lenovo:~/Downloads/04_bcc_ebpf/ksight/build/src/backend/ipc/ipc_watcher$ sudo ./ipcwatcher -m
Tracing POSIX shared memory com ... Ctrl+C to exit
timestamp       PID            command              fd                   shm_size             shm_flag             shm_prot            
109876405724    1282728        ipcwatcher           3                    4096                 MAP_SHARED           PROT_READ | PROT_WRITE
109876405739    1282728        ipcwatcher           3                    33558528             MAP_SHARED           PROT_READ           
109877004770    2705           gnome-shell          13                   65536                MAP_SHARED           PROT_READ | PROT_WRITE
109877066875    2705           gnome-shell          13                   12288                MAP_SHARED           PROT_READ | PROT_WRITE
109878060208    2564           Xorg                 15                   40960                MAP_SHARED           PROT_READ | PROT_WRITE
109878060571    2564           Xorg                 15                   40960                MAP_SHARED           PROT_READ | PROT_WRITE
109878060841    2564           Xorg                 15                   40960                MAP_SHARED           PROT_READ | PROT_WRITE
109878061128    2564           Xorg                 15                   40960                MAP_SHARED           PROT_READ | PROT_WRITE
109878061500    2564           Xorg                 15                   40960                MAP_SHARED           PROT_READ | PROT_WRITE
109878063040    2564           Xorg                 15                   4096                 MAP_SHARED           PROT_READ | PROT_WRITE
109878063340    2564           Xorg                 15                   4096                 MAP_SHARED           PROT_READ | PROT_WRITE
109879089547    2705           gnome-shell          13                   12288                MAP_SHARED           PROT_READ | PROT_WRITE


```



```bash
fzy@fzy-Lenovo:~/Downloads/04_bcc_ebpf/ksight/build/src/backend/ipc/ipc_watcher$ sudo ./ipcwatcher -m
Tracing POSIX shared memory com ... Ctrl+C to exit
timestamp       PID        command      fd    shm_size shm_flag     shm_prot                  shm_path                  shm_vm_addr                        
111924416333    1305311    ipcwatcher   3     4096     MAP_SHARED   PROT_READ | PROT_WRITE    anon_inode:bpf-map        7f9f9550e000-7f9f9750f000          
111924416346    1305311    ipcwatcher   3     33558528 MAP_SHARED   PROT_READ                 anon_inode:bpf-map        7f9f9550e000-7f9f9750f000          
111924940064    2564       Xorg:gdrv0   15    65536    MAP_SHARED   PROT_READ | PROT_WRITE    /dev/dri/card0            None                               
111924941818    2564       Xorg:gdrv0   15    65536    MAP_SHARED   PROT_READ | PROT_WRITE    /dev/dri/card0            None                               
111924942994    2564       Xorg:gdrv0   15    65536    MAP_SHARED   PROT_READ | PROT_WRITE    /dev/dri/card0            None                               
111924943080    2564       Xorg:gdrv0   15    65536    MAP_SHARED   PROT_READ | PROT_WRITE    /dev/dri/card0            None                               
111924943614    2564       Xorg         15    65536    MAP_SHARED   PROT_READ | PROT_WRITE    /dev/dri/card0            None                               
111924963545    2564       Xorg         15    65536    MAP_SHARED   PROT_READ | PROT_WRITE    /dev/dri/card0            None 
```





共享内存的**内存泄漏（Memory Leak）**是指在使用共享内存的过程中，由于程序逻辑错误或资源管理不当，导致分配的共享内存没有被正确释放，从而造成内存资源的浪费。这种泄漏不会立即引发系统崩溃，但随着时间推移，未释放的共享内存会不断积累，最终可能导致系统性能下降甚至内存耗尽。

---

## 🧠 一、什么是共享内存泄漏？

共享内存泄漏通常表现为以下几种形式：

### 1. **未调用 `munmap()`**
- 某个进程将共享内存映射到用户空间后（通过 `mmap()`），但没有调用 `munmap()` 解除映射。
- 导致该进程退出后仍保留映射区域，内核无法回收这部分内存。

### 2. **未调用 `shm_unlink()`**
- 使用 `shm_open()` 创建共享内存对象后，若没有调用 `shm_unlink()` 删除该对象，即使所有进程都已关闭文件描述符，该对象仍存在于系统中。
- 类似于文件系统的“硬链接”机制，引用计数不为零时无法删除。

### 3. **未关闭文件描述符**
- 即使调用了 `munmap()`，但如果未调用 [close(fd)](file:///home/fzy/Downloads/04_bcc_ebpf/ksight/third_party/fmt/test/posix-mock.h#L47-L47) 关闭共享内存的文件描述符，也会导致资源未完全释放。

### 4. **异常退出未清理**
- 进程在执行过程中发生异常（如崩溃、信号中断等），未能执行正常的清理代码（如 `munmap()` 和 [close()](file:///home/fzy/Downloads/04_bcc_ebpf/ksight/third_party/fmt/test/posix-mock.h#L47-L47)）。

---

## ⚠️ 二、哪些场景下容易发生共享内存泄漏？

### 场景 1：多进程通信未统一释放
- 多个进程共享同一块内存区域，但只有部分进程调用 `munmap()` 或 [close()](file:///home/fzy/Downloads/04_bcc_ebpf/ksight/third_party/fmt/test/posix-mock.h#L47-L47)。
- 常见于服务端/客户端模型中，客户端意外退出而未通知服务端释放资源。

### 场景 2：守护进程长期运行
- 长期运行的服务（如监控工具、日志收集器）如果每次启动新任务都创建新的共享内存，但没有定期清理旧的，就容易累积大量未释放的共享内存。

### 场景 3：动态分配但未跟踪生命周期
- 在动态创建多个共享内存对象的场景中（如基于事件驱动的 IPC），如果没有良好的资源追踪机制，容易遗漏某些对象的释放操作。

### 场景 4：跨线程访问控制不当
- 多线程环境下，一个线程负责分配共享内存，另一个线程负责释放，若线程间协调不当，可能造成释放失败。

### 场景 5：使用匿名共享内存（如 `memfd_create`）
- 匿名共享内存没有名字，依赖文件描述符传递和管理。一旦某个接收方忘记关闭 fd 或未进行映射解除，就可能造成泄漏。

---

## 🔍 三、如何检测共享内存泄漏？

### 方法 1：查看 `/dev/shm`
```bash
ls -l /dev/shm
```

- 列出当前系统中所有命名共享内存对象。
- 如果发现大量未命名或无关联进程的共享内存对象，可能是泄漏。

### 方法 2：使用 `ipcs` 查看 IPC 资源
```bash
ipcs -m
```

- 显示系统中所有 System V 共享内存段。
- 可以看到 key、shmid、owner、size 等信息。

### 方法 3：检查 `/proc/<pid>/maps`
```cpp
// C++ 示例：读取某进程的 maps 文件
std::ifstream maps(fmt::format("/proc/{}/maps", pid));
std::string line;
while (std::getline(maps, line)) {
    if (line.find("/dev/shm") != std::string::npos) {
        // 找到共享内存映射
    }
}
```


### 方法 4：eBPF 监控系统调用
- 挂载 `sys_enter_shm_open`, `sys_exit_mmap`, `sys_exit_munmap` 等 tracepoint。
- 统计未匹配的 mmap/munmap 对，或未 unlink 的 shm 对象。

---

## ✅ 四、如何避免共享内存泄漏？

### 推荐做法：
1. **RAII 模式封装共享内存资源**
   - 使用智能指针或 RAII 封装类自动管理 `mmap` 和 [close](file:///home/fzy/Downloads/04_bcc_ebpf/ksight/third_party/fmt/test/posix-mock.h#L47-L47)。
2. **注册清理钩子函数**
   - 使用 `atexit()` 或 `pthread_cleanup_push()` 注册清理函数，确保进程退出前释放资源。
3. **统一释放入口**
   - 设计统一的资源释放接口，避免多个地方重复释放或遗漏。
4. **使用 eBPF 实时监控**
   - 如上所述，通过 eBPF 程序实时统计共享内存的分配与释放情况，及时发现异常。
5. **增加超时机制**
   - 对长期未使用的共享内存设置自动清理策略（如定时扫描并释放）。

---

## 📦 五、示例：C++ 中共享内存泄漏模拟

```cpp
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <iostream>

int main() {
    const char* name = "/my_shared_memory";
    int size = 4096;

    int shm_fd = shm_open(name, O_CREAT | O_RDWR, 0666);
    ftruncate(shm_fd, size);

    void* ptr = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);

    // 错误：没有调用 munmap 和 close
    // 正确做法应在退出前调用：
    // munmap(ptr, size);
    // close(shm_fd);
    // shm_unlink(name);

    std::cout << "Shared memory created but not cleaned up!" << std::endl;
    return 0;
}
```


> 上述代码会导致共享内存泄漏，因为进程退出前没有执行任何清理操作。

---

## 🧩 六、结语

共享内存泄漏虽然不像堆内存泄漏那样频繁出现，但在系统级编程、高性能 IPC、eBPF 监控等领域却非常关键。通过合理的设计模式、资源管理机制以及借助 eBPF 的可观测性手段，可以有效预防和定位这类问题。

如果你希望我为你提供一个完整的共享内存泄漏检测模块（包括 eBPF 程序 + 用户态分析工具），我可以根据 MagicEyes 架构为你生成可集成的代码模板。是否需要？
