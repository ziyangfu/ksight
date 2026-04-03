Edited solution.md

为了响应您的要求，我们将开发一个全新的、独立的工具 **`e2etracer`**。它将作为一个集大成者，融合用户态 USDT 追踪与内核态协议栈分析，专门为车载 SOA 场景定制。

以下是该工具的完整技术方案设计：

---

# e2etracer：车载 SOA 垂向全链路跟踪工具技术方案

## 1. 核心架构（Architecture）

`e2etracer` 采用“两端一中心”的架构：
*   **采集端（eBPF Probes）**：负责在内核空间捕获 `skb` 各层时间戳及 16 字节 Payload 素材。
*   **注入端（USDT/Uprobe）**：负责在用户态中间件捕获业务 ID 与线程上下文。
*   **分析中心（User-space Orchestrator）**：负责解析 SOA 配置表、解包 BPF 透传数据、多源数据归一化关联。

---

## 2. 跨层关联：双轴追踪算法

为了实现无侵入关联，我们设计了 **“线程轴”** 与 **“特征轴”** 的双轴映射：

1.  **线程轴（Thread-Context Binding）**：
    *   在应用发起调用的瞬间，通过 USDT 记录 `(TID, ServiceID, SessionID)`。
    *   在进入 `sendmsg` 系统调用时，BPF 通过 `bpf_get_current_pid_tgid()` 确定当前报文属于该业务上下文。
2.  **特征轴（Payload-Header Binding）**：
    *   在内核各层处理中，如果无法维持线程连续性（如软中断处理收包），则利用透传的 **SOME/IP Header (16 Bytes)** 作为指纹，通过 `SessionID + MessageID` 在用户态进行异步匹配。

---

## 3. eBPF 埋点清单 (Hook Points)

针对 PREEMPT_RT 环境，全部使用 `fentry/fexit` 以减少对实时性的干扰。

### 3.1 下行全链路 (TX Path)
| 阶段 | Hook 点 | 采集数据 |
| :--- | :--- | :--- |
| **App/Mid** | `USDT: someip:request_send` | ServiceID, InstanceID, SessionID, TID |
| **Syscall** | `fentry/__sys_sendto` | Buffer 长度, TID -> ServiceID 关联 |
| **Layer 4** | `fentry/udp_sendmsg` | `sock` 指针, 源/目的端口 |
| **Layer 3** | `fentry/ip_output` | IP 头部, `skb` 地址 |
| **Layer 2** | `fentry/dev_queue_xmit` | `ifindex`, QDisc 状态 |
| **Driver** | `fentry/ndo_start_xmit` | DMA 发起时间 |

### 3.2 上行全链路 (RX Path)
| 阶段 | Hook 点 | 采集数据 |
| :--- | :--- | :--- |
| **Driver** | `tracepoint/napi/napi_poll` | 硬件收包起点 |
| **Layer 2** | `fentry/netif_receive_skb` | `skb` 地址 |
| **Layer 3** | `fentry/ip_rcv` | IP 地址, VLAN ID |
| **Layer 4** | `fexit/udp_rcv` | 端口号, 16 字节 Payload 透传 |
| **Syscall** | `fexit/__sys_recvfrom` | 拷贝数据完成时间 |

---

## 4. 关键数据结构定义

### 4.1 BPF 事件结构
```c
struct e2e_event {
    u64 ts_app;       // 用户态触发时间
    u64 ts_syscall;   // 系统调用时间
    u64 ts_transport; // 传输层时间
    u64 ts_network;   // 网络层时间
    u64 ts_mac;       // 链路层时间
    u64 ts_phys;      // 驱动发送时间
    
    u32 pid;
    u32 tid;
    u32 saddr, daddr;
    u16 sport, dport;
    u64 skb_addr;
    
    u8 payload_raw[16]; // 透传的 SOME/IP 16 字节头
    bool is_rx;         // 收发方向标识
};
```

### 4.2 BPF Maps
*   **`map_soa_config`**: `Hash{ (dip, dport) -> ServiceID }` —— 静态加速。
*   **`map_thread_context`**: `Hash{ TID -> ServiceContext }` —— 关联应用与内核。
*   **`map_skb_trace`**: `Hash{ skb_addr -> e2e_event }` —— 记录单报文各层状态。

---

## 5. 用户态处理逻辑

1.  **初始化**：
    *   加载 SOA 汇总表到内存 Hash 表。
    *   根据用户参数（如 `--serviceID`）计算需要过滤的 IP/Port 列表，下发到 BPF Map 过滤。
2.  **数据拼装**：
    *   从 BPF RingBuffer 接收 `e2e_event`。
    *   **解析 Payload**：取出 `payload_raw`，按照 SOME/IP 协议规范解析出真正的 `ServiceID`, `InstanceID`。
    *   **二次校准**：如果解析出的 ID 与配置表不符（用于检测异常数据），则标记为协议异常。
    *   **翻译映射**：根据配置表查出对应的应用名称、服务名称（如 `APA_Service`）。

---

## 6. 输出示例

```text
# e2etracer --serviceID 0x123 --instanceID 1
[TIMESTAMP] APP:RadarSender | PID:142 | TID:145 | SERVICE:ADAS_FRONT_RADAR (ID:0x123)
  |- Total E2E Latency: 1450us
  |- [0] App Prepare:     40us (USDT)
  |- [1] Middleware:     120us (USDT -> Syscall Enter)
  |- [2] System Call:     10us (Sys_Enter -> Transport)
  |- [3] Net Stack:      280us (Transport -> IP -> Dev_Xmit)
  |- [4] QDisc Queue:     50us (Dev_Xmit -> NDO_Xmit)
  |- [5] Driver/HW:      950us (NDO_Xmit -> HW_Ack)
```

---

## 7. 性能风险预警与优化

1.  **CPU 开销**：在高频（>5000 pkts/s）场景下，USDT 到内核的频繁切换会有损耗。
    *   **对策**：默认仅采集内核态数据（基于五元组），仅在指定 `--serviceID` 时动态激活 USDT 追踪。
2.  **内存占用**：`skb_trace_map` 需设置合理的清除机制（LRU），防止在复杂网络环境下撑爆。
3.  **Real-time 影响**：在 PREEMPT_RT 环境中，确保 BPF 程序不持有 Spinlock 且不进行长循环。

---

**下一步建议：**
我已将上述方案概要整理，接下来我将为您生成 `e2etracer` 的初步代码框架（包括 BPF 部分和 C++ 骨架）。您是否需要先建立对应的文件结构？