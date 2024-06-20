/**
* 追踪Linux内核内存规整情况，内存规整可以减少内存碎片化，从而提高内存利用率
*/
// SPDX-License-Identifier: GPL-2.0
// Copyright (c) 2024 fzy
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_core_read.h>
#include <bpf/bpf_tracing.h>
#include "mem_watcher.h"

char LICENSE[] SEC("license") = "Dual BSD/GPL";

struct {
    __uint(type, BPF_MAP_TYPE_RINGBUF);
    __uint(max_entries, 256 * 1024);
} rb SEC(".maps");

// 统计单位时间（1s）触发内存规整的次数
SEC("kprobe/__alloc_pages_direct_compact")
int BPF_KPROBE(memory_compact, gfp_t gfp_mask, unsigned int order,
    unsigned int alloc_flags, const struct alloc_context *ac)
{
    struct compaction_data_t *data;
    data = bpf_ringbuf_reserve(&rb, sizeof(*data), 0);
    if (!data)
        return 0;

    data->pid = bpf_get_current_pid_tgid() >> 32;
    data->pages = BPF_CORE_READ(oc, totalpages);
    bpf_get_current_comm(&data->fcomm, sizeof(data->fcomm));
    bpf_probe_read_kernel(&data->tcomm, sizeof(data->tcomm), BPF_CORE_READ(oc, chosen, comm));

    bpf_ringbuf_submit(data, 0);

    return 0;
}