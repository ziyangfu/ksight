
/*!
	\brief 提取各种类型内存的活动和非活动页面数量，以及其他内存回收相关的统计数据，
			除了常规的事件信息外，程序还输出了与内存管理相关的详细信息，包括了不同
			类型内存的活动（active）和非活动（inactive）页面，未被驱逐（unevictable）
			页面，脏（dirty）页面，写回（writeback）页面，映射（mapped）页面，
			以及各种类型的内存回收相关统计数据。
	\details
			| file_active    | 活跃文件映射内存大小              |
			| file_inactive  | 不活跃文件映射内存大小            |
			| unevictable    | 不可回收内存大小                 |
			| dirty          | 脏页大小                        |
			| writeback      | 正在回写的内存大小                |
			| anonpages      | RMAP页面                        |
			| mapped         | 所有映射到用户地址空间的内存大小 	|
			| shmem          | 共享内存                         |
			| kreclaimable   | 内核可回收内存                    |
			| slab           | 用于slab的内存大小                |
			| sreclaimable   | 可回收slab内存                   |
			| sunreclaim     | 不可回收slab内存                 |
			| NFS_unstable   | NFS中还没写到磁盘中的内存          |
			| writebacktmp   | 回写所使用的临时缓存大小           |
			| anonhugepages  | 透明巨页大小                     |
			| shmemhugepages | shmem或tmpfs使用的透明巨页       |
*/
#include "vmlinux.h"
#include <bpf/bpf_helpers.h>
#include <bpf/bpf_tracing.h>
#include <bpf/bpf_core_read.h>
#include "mem_watcher.h"

char LICENSE[] SEC("license") = "Dual BSD/GPL";

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 256 * 1024);
	__type(key, unsigned long);
	__type(value, int);
} last_val1 SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 256 * 1024);
	__type(key, unsigned long);
	__type(value, int);
} last_val2 SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, 256 * 1024);
	__type(key, unsigned long);
	__type(value, int);
} last_val3 SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_RINGBUF);
	__uint(max_entries, 256 * 1024);
} rb SEC(".maps");

pid_t user_pid = 0;


SEC("kprobe/get_page_from_freelist")
int BPF_KPROBE(get_page_from_freelist_second, gfp_t gfp_mask, unsigned int order, int alloc_flags, const struct alloc_context *ac) {
	struct sysstat_event *e;
	unsigned long *t;
	pid_t pid = bpf_get_current_pid_tgid() >> 32;
	if (pid == user_pid)
		return 0;
	t = (unsigned long *)BPF_CORE_READ(ac, preferred_zoneref, zone, zone_pgdat, vm_stat);
	unsigned long anon_ina = t[0] * 4;
	unsigned long anon_mapped = t[17] * 4;
	unsigned long file_mapped = t[18] * 4;
	int val = 1;
	int *last_1, *last_2, *last_3;
	last_1 = bpf_map_lookup_elem(&last_val1, &anon_ina);
	last_2 = bpf_map_lookup_elem(&last_val2, &anon_mapped);
	last_3 = bpf_map_lookup_elem(&last_val3, &file_mapped);
	if (!last_1 || !last_2 || !last_3) {
		bpf_map_update_elem(&last_val1, &anon_ina, &val, BPF_ANY);
		bpf_map_update_elem(&last_val2, &anon_mapped, &val, BPF_ANY);
		bpf_map_update_elem(&last_val3, &file_mapped, &val, BPF_ANY);
	}
	else if ((*last_1 && *last_2 && *last_3) == val) {
		return 0;
	}
	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;

	//	e->present = BPF_CORE_READ(ac, preferred_zoneref, zone, zone_pgdat, node_spanned_pages);
	//t = (unsigned long *)BPF_CORE_READ(ac, preferred_zoneref, zone, zone_pgdat, vm_stat);
	//	t = (unsigned long *)BPF_CORE_READ(ac, preferred_zoneref, zone, vm_stat);
	e->anon_inactive = t[0] * 4;
	e->anon_active = t[1] * 4;
	e->file_inactive = t[2] * 4;
	e->file_active = t[3] * 4;
	e->unevictable = t[4] * 4;


	e->file_dirty = t[20] * 4;
	e->writeback = t[21] * 4;
	e->anon_mapped = t[17] * 4;
	e->file_mapped = t[18] * 4;
	e->shmem = t[23] * 4;

	e->slab_reclaimable = t[5] * 4;
	e->kernel_misc_reclaimable = t[29] * 4;
	e->slab_unreclaimable = t[6] * 4;

	e->unstable_nfs = t[27] * 4;
	e->writeback_temp = t[22] * 4;

	e->anon_thps = t[26] * 4;
	e->shmem_thps = t[24] * 4;
	e->pmdmapped = t[25] * 4;
	bpf_ringbuf_submit(e, 0);
	return 0;
}
