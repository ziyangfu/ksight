#ifndef E2E_TRACER_H
#define E2E_TRACER_H

// 基础类型定义，适配 BPF 和用户态
#if defined(__BPF_TRACING__) || defined(__VMLINUX_H__)
#ifndef __VMLINUX_H__
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
#endif
#else
#include <stdint.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
#endif

#define MAX_PAYLOAD_LEN 16

// 线程粒度的业务上下文（用于 TCP 染色）
struct e2e_service_context {
  u32 service_id;
  u32 instance_id;
  u64 ts_start; // 应用层发起调用的时间
};

struct e2e_packet_event {
  u64 skb_addr;
  u32 pid;
  u32 tid;

  // 时间戳 (纳秒)
  u64 ts_app;       // 应用层（uprobe）时间
  u64 ts_syscall;   // 系统调用进入
  u64 ts_transport; // 传输层处理完成
  u64 ts_network;   // 网络层处理完成
  u64 ts_mac;       // 链路层处理完成
  u64 ts_phys;      // 驱动发送时间

  // 网络元数据
  u32 saddr;
  u32 daddr;
  u16 sport;
  u16 dport;
  u8 protocol;
  u8 is_rx; // 0: TX, 1: RX

  // 业务元数据
  u32 service_id;
  u32 instance_id;

  // 透传位: 仅用于 UDP 或小包
  u8 payload_raw[MAX_PAYLOAD_LEN];
};

#endif /* E2E_TRACER_H */
