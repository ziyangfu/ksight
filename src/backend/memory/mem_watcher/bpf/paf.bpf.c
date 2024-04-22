/**
 * \brief 监控内核中的`get_page_from_freelist`函数。这个函数在内核中
 * 		  用于从内存空闲页列表中获取一个页面
 * \details
 * 			| 参数    | 含义                                 |
			| ------- | ------------------------------------ |
			| min     | 内存管理区处于最低警戒水位的页面数量 |
			| low     | 内存管理区处于低水位的页面数量       |
			| high    | 内存管理区处于高水位的页面数量       |
			| present | 内存管理区实际管理的页面数量         |
			| flag    | 申请页面时的权限（标志）             |
	\todo 文件名称不直观，考虑改名
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
	__type(key, int);
	__type(value, int);
} last_val SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_RINGBUF);
	__uint(max_entries, 256 * 1024);
} rb SEC(".maps");

pid_t user_pid = 0;


SEC("kprobe/get_page_from_freelist")
int BPF_KPROBE(get_page_from_freelist, gfp_t gfp_mask, unsigned int order, 
			   int alloc_flags, const struct alloc_context *ac)
{
	struct paf_event *e; 
	unsigned long *t, y;
	int a;
	pid_t pid = bpf_get_current_pid_tgid() >> 32;
	if (pid == user_pid)
		return 0;

	int val = 1;
	int flag = (int)gfp_mask;
	int *last_flag;
	last_flag = bpf_map_lookup_elem(&last_val, &flag);
	if (!last_flag) {
		bpf_map_update_elem(&last_val, &flag, &val, BPF_ANY);
	}
	else if(*last_flag == val) {
		return 0;
	}

	e = bpf_ringbuf_reserve(&rb, sizeof(*e), 0);
	if (!e)
		return 0;
	y = BPF_CORE_READ(ac, preferred_zoneref, zone, watermark_boost);
	t = BPF_CORE_READ(ac, preferred_zoneref, zone, _watermark);

	e->present = BPF_CORE_READ(ac, preferred_zoneref, zone, present_pages);
	e->min = t[0] + y;
	e->low = t[1] + y;
	e->high = t[2] + y;
	e->flag = (int)gfp_mask;

	bpf_ringbuf_submit(e, 0);
	return 0;
}