/*!
    \brief  wirefisher kernel code
    \details
    TC流量控制
*/
#include "tc_cgroup.bpf.h"
#include "tc_eth.bpf.h"
#include "tc_port.bpf.h"
#include "tc_process,bpf.h"

// tc_cgroup
SEC("cgroup_skb/egress")
int cgroup_skb_egress(struct __sk_buff *ctx) {
  return cgroup_handle(ctx, EGRESS);
}

SEC("cgroup_skb/ingress")
int cgroup_skb_ingress(struct __sk_buff *ctx) {
  return cgroup_handle(ctx, INGRESS);
}

// tc_eth
SEC("tc")
int tc_egress(struct __sk_buff *ctx) { return tc_handle(ctx, EGRESS); }

SEC("tc")
int tc_ingress(struct __sk_buff *ctx) { return tc_handle(ctx, INGRESS); }

// tc_port
SEC("netfilter")
int netfilter_hook(struct bpf_nf_ctx *ctx) { return netfilter_handle(ctx); }

// tc_process
SEC("kprobe/security_socket_recvmsg")
int BPF_KPROBE(security_socket_recvmsg, struct socket *sock,
               struct msghdr *msg) {
  save_sock(sock);

  struct sock *sk = BPF_CORE_READ(sock, sk);
  if (sk) {
    __u16 skproto = BPF_CORE_READ(sk, sk_protocol);
    if (skproto != IPPROTO_UDP) {
      return 0;
    }

    __u32 daddr = BPF_CORE_READ(sk, __sk_common.skc_rcv_saddr);
    __u16 dport = BPF_CORE_READ(sk, __sk_common.skc_num);

    if (daddr == 0) {
      __u32 key = 0;
      __u32 *local_ip = bpf_map_lookup_elem(&local_ip_map, &key);
      if (local_ip) {
        daddr = bpf_ntohl(*local_ip);
      }
    }

    struct net_group key = {};
    key.ip = bpf_ntohl(daddr);
    key.port = bpf_ntohs(dport);
    key.protocol = IPPROTO_UDP;

    struct ProcInfo proc = {};
    proc.pid = bpf_get_current_pid_tgid() >> 32;
    bpf_get_current_comm(proc.comm, sizeof(proc.comm));
    bpf_map_update_elem(&tuple_map, &key, &proc, BPF_ANY);
  }

  return 0;
}

SEC("kprobe/security_socket_sendmsg")
int BPF_KPROBE(security_socket_sendmsg, struct socket *sock) {
  if (!sock) {
    return 0;
  }

  save_sock(sock);
  return 0;
}

SEC("netfilter")
int netfilter_hook(struct bpf_nf_ctx *ctx) {
  struct process_rule *rule;
  struct message_get mes = {0};
  __u32 rule_key = 0;

  rule = bpf_map_lookup_elem(&process_rules, &rule_key);

  if (!rule) {
    return NF_ACCEPT;
  }

  if (!ctx || !ctx->skb) {
    return NF_ACCEPT;
  }

  __u32 hook_state = BPF_CORE_READ(ctx->state, hook);

  if (rule->gress == EGRESS && hook_state != NF_INET_LOCAL_OUT) {
    return NF_ACCEPT;
  }

  if (rule->gress == INGRESS && hook_state != NF_INET_LOCAL_IN) {
    return NF_ACCEPT;
  }

  volatile struct sock *pre_sk = BPF_CORE_READ(ctx->skb, sk);

  struct ProcInfo *proc;
  struct net_group key = {};
  int i = parse_sk_buff(ctx->skb, INGRESS, &key);
  if (i == false) {
    return NF_ACCEPT;
  }

  if (hook_state == NF_INET_LOCAL_IN && key.protocol == IPPROTO_UDP) {
    proc = bpf_map_lookup_elem(&tuple_map, &key);
  } else {
    if (!pre_sk) {
      return NF_ACCEPT;
    }
    struct sock *sk_ptr = (struct sock *)pre_sk;
    proc = bpf_map_lookup_elem(&sock_map, &sk_ptr);
  }

  if (!proc) {
    return NF_ACCEPT;
  }

  __u32 pid = proc->pid;
  if (pid == 0) {
    return NF_ACCEPT;
  }

  if (rule->target_pid != proc->pid) {
    return NF_ACCEPT;
  }

  __u64 now = bpf_ktime_get_ns();
  __u32 flow_key = 1;
  struct flow_rate_info *info =
      bpf_map_lookup_elem(&flow_rate_stats, &flow_key);
  if (!info) {
    struct flow_rate_info new_flow = {.window_start_ns = now,
                                      .total_bytes = ctx->skb->len,
                                      .packet_bytes = ctx->skb->len,
                                      .last_ns = now,
                                      .instance_rate_bps = 0,
                                      .rate_bps = 0,
                                      .peak_rate_bps = 0,
                                      .smooth_rate_bps = 0};
    bpf_map_update_elem(&flow_rate_stats, &flow_key, &new_flow, BPF_ANY);
  }
  info = bpf_map_lookup_elem(&flow_rate_stats, &flow_key);
  if (info) {
    update_flow_rate(info, ctx->skb->len);
    mes.rate_bps = info->rate_bps;
    mes.instance_rate_bps = info->instance_rate_bps;
    mes.peak_rate_bps = info->peak_rate_bps;
    mes.smoothed_rate_bps = info->smooth_rate_bps;
  }

  send_message(&mes);

  __u64 bucket_key = proc->pid;
  struct rate_limit rate = {.bucket_key = &bucket_key,
                            .buckets = &buckets,
                            .packet_len = ctx->skb->len,
                            .rate_bps = rule->rate_bps,
                            .time_scale = rule->time_scale};

  if (rate_limit_check(&rate) == ACCEPT) {
    return NF_ACCEPT;
  }
  return NF_DROP;
}
