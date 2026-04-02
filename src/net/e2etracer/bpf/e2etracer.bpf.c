#include "vmlinux.h"
#include "e2etracer.h"
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>

char LICENSE[] SEC("license") = "Dual BSD/GPL";

// 存储 SKB 与追踪上下文的映射
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, 8192);
  __type(key, u64); // skb_addr
  __type(value, struct e2e_packet_event);
} map_skb_trace SEC(".maps");

// 存储线程粒度的业务上下文 (TID -> ServiceContext)
struct {
  __uint(type, BPF_MAP_TYPE_HASH);
  __uint(max_entries, 1024);
  __type(key, u32); // tid
  __type(value, struct e2e_service_context);
} map_thread_context SEC(".maps");

// 用于数据上报的 RingBuffer
struct {
  __uint(type, BPF_MAP_TYPE_RINGBUF);
  __uint(max_entries, 256 * 1024);
} rb SEC(".maps");

// 控制变量：是否开启 Payload 捕获 (主要针对 UDP)
const volatile bool capture_payload = false;

// 过滤变量：只追踪特定的 SOA 服务 (0 表示不限)
const volatile u32 target_service_id = 0;
const volatile u32 target_instance_id = 0;

// 辅助函数：根据 skb 提取五元组
static __always_inline void fill_metadata(struct sk_buff *skb,
                                          struct e2e_packet_event *event) {
  struct iphdr *ip = (struct iphdr *)(BPF_CORE_READ(skb, head) +
                                      BPF_CORE_READ(skb, network_header));
  event->saddr = BPF_CORE_READ(ip, saddr);
  event->daddr = BPF_CORE_READ(ip, daddr);
  event->protocol = BPF_CORE_READ(ip, protocol);

  if (event->protocol == IPPROTO_UDP) {
    struct udphdr *udp =
        (struct udphdr *)(BPF_CORE_READ(skb, head) +
                          BPF_CORE_READ(skb, transport_header));
    event->sport = BPF_CORE_READ(udp, source);
    event->dport = BPF_CORE_READ(udp, dest);
  } else if (event->protocol == IPPROTO_TCP) {
    struct tcphdr *tcp =
        (struct tcphdr *)(BPF_CORE_READ(skb, head) +
                          BPF_CORE_READ(skb, transport_header));
    event->sport = BPF_CORE_READ(tcp, source);
    event->dport = BPF_CORE_READ(tcp, dest);
  }
}

// -----------------------------------------------------------------------------
// USER SPACE CONTEXT INJECTION (USDT / Uprobe)
// -----------------------------------------------------------------------------

// 模拟中间件发送函数的 uprobe。用户态程序需要将此挂载到 vsomeip 等库的发送入口。
SEC("uprobe/middleware_send")
int BPF_KPROBE(uprobe_middleware_send) {
  u32 tid = (u32)bpf_get_current_pid_tgid();
  struct e2e_service_context svc_ctx = {};
  
  // 从寄存器获取参数 (假设参数顺序为 service_id, instance_id)
  svc_ctx.service_id = (u32)PT_REGS_PARM1(ctx);
  svc_ctx.instance_id = (u32)PT_REGS_PARM2(ctx);
  svc_ctx.ts_start = bpf_ktime_get_ns();
  
  bpf_map_update_elem(&map_thread_context, &tid, &svc_ctx, BPF_ANY);
  return 0;
}

// -----------------------------------------------------------------------------
// TX PATH
// -----------------------------------------------------------------------------

SEC("fentry/ip_output")
int BPF_PROG(ip_output_enter, struct net *net, struct sock *sk,
             struct sk_buff *skb) {
  u64 skb_addr = (u64)skb;
  u32 tid = (u32)bpf_get_current_pid_tgid();
  struct e2e_packet_event event = {};

  event.skb_addr = skb_addr;
  event.pid = bpf_get_current_pid_tgid() >> 32;
  event.tid = tid;
  event.ts_network = bpf_ktime_get_ns();
  event.ts_syscall = bpf_ktime_get_ns(); // 简化的起点
  event.is_rx = 0;

  // 尝试关联业务上下文 (Coloring)
  struct e2e_service_context *sctx = (struct e2e_service_context *)bpf_map_lookup_elem(&map_thread_context, &tid);
  if (sctx) {
    // 过滤逻辑：如果在内核态指定了目标服务 ID，则只追踪匹配的服务
    if (target_service_id != 0 && sctx->service_id != target_service_id) {
        return 0;
    }
    
    event.service_id = sctx->service_id;
    event.instance_id = sctx->instance_id;
    event.ts_app = sctx->ts_start;
  } else if (target_service_id != 0) {
    // 如果指定了过滤且当前线程没有业务上下文，则不追踪此数据包
    return 0;
  }

  fill_metadata(skb, &event);
  bpf_map_update_elem(&map_skb_trace, &skb_addr, &event, BPF_ANY);
  return 0;
}

SEC("fentry/dev_queue_xmit")
int BPF_PROG(dev_queue_xmit_enter, struct sk_buff *skb) {
  u64 skb_addr = (u64)skb;
  struct e2e_packet_event *event = (struct e2e_packet_event *)bpf_map_lookup_elem(&map_skb_trace, &skb_addr);
  if (event) {
    event->ts_mac = bpf_ktime_get_ns();
  }
  return 0;
}

// -----------------------------------------------------------------------------
// RX PATH
// -----------------------------------------------------------------------------

SEC("fentry/netif_receive_skb")
int BPF_PROG(netif_receive_skb_enter, struct sk_buff *skb) {
  u64 skb_addr = (u64)skb;
  struct e2e_packet_event event = {};

  event.skb_addr = skb_addr;
  event.ts_mac = bpf_ktime_get_ns();
  event.is_rx = 1;

  bpf_map_update_elem(&map_skb_trace, &skb_addr, &event, BPF_ANY);
  return 0;
}

SEC("fentry/ip_rcv")
int BPF_PROG(ip_rcv_enter, struct sk_buff *skb) {
  u64 skb_addr = (u64)skb;
  struct e2e_packet_event *event = (struct e2e_packet_event *)bpf_map_lookup_elem(&map_skb_trace, &skb_addr);
  if (event) {
    event->ts_network = bpf_ktime_get_ns();
    fill_metadata(skb, event);
  }
  return 0;
}

// 在传输层完成后上报数据到用户态
SEC("fexit/udp_rcv")
int BPF_PROG(udp_rcv_exit, struct sk_buff *skb, int ret) {
  u64 skb_addr = (u64)skb;
  struct e2e_packet_event *event = (struct e2e_packet_event *)bpf_map_lookup_elem(&map_skb_trace, &skb_addr);
  if (!event)
    return 0;

  event->ts_transport = bpf_ktime_get_ns();

  // 针对 UDP 的 Payload 捕获
  if (event->protocol == IPPROTO_UDP && capture_payload) {
    u32 data_off = BPF_CORE_READ(skb, transport_header) + sizeof(struct udphdr);
    bpf_skb_load_bytes(skb, data_off, event->payload_raw, MAX_PAYLOAD_LEN);
  }

  // 提交到 RingBuffer
  struct e2e_packet_event *e = (struct e2e_packet_event *)bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
  if (e) {
    *e = *event;
    bpf_ringbuf_submit(e, 0);
  }

  bpf_map_delete_elem(&map_skb_trace, &skb_addr);
  return 0;
}
