// SPDX-License-Identifier: GPL-2.0
/*
 * An RMS (Rate-Monotonic Scheduling) implementation.
 *
 * This scheduler operates as a fixed-priority preemptive scheduler where tasks with
 * shorter periods (higher rates) are assigned higher priority. It demonstrates the
 * following characteristics:
 * - Strict priority-based execution according to task periods
 * - Preemptive behavior to ensure timely execution of high-rate tasks
 * - Schedulability analysis to guarantee task deadlines under utilization bounds
 *
 * While simple in principle, this scheduler provides predictable real-time performance
 * for periodic task systems. The fixed-priority approach makes it particularly suitable
 * for embedded systems and applications with hard real-time requirements where temporal
 * correctness is crucial. The schedulability test allows system designers to verify
 * whether a task set can meet all deadlines before deployment.
 *
 * Note that while RMS is optimal among fixed-priority schedulers, it does have limitations:
 * - Maximum achievable utilization is bounded (approximately 69.3% for large task sets)
 * - Handling aperiodic or sporadic tasks requires additional mechanisms
 *
 * Feature Flags:
 *
 * FEATURE_MIGRATION:
 *   When enabled, this feature handles CPU migration of RMS tasks by:
 *   - Detecting when a task is moved to a different CPU via set_cpumask()
 *   - Preserving the remaining time of the task's period timer
 *   - Restarting the timer on the new CPU with the preserved remaining time
 *   This ensures temporal correctness when tasks are migrated between CPUs,
 *   maintaining the rate-monotonic scheduling guarantees.
 *
 * FEATURE_BUDGET_OVERRUN:
 *   When enabled, this feature provides graceful handling of tasks that exceed
 *   their allocated budget by:
 *   - Allowing the current task to continue running if it exceeds its budget
 *   - Preventing immediate preemption when budget expires
 *   - Still maintaining overall scheduling guarantees through the periodic
 *     scheduling mechanism
 *
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 */
#include <scx/common.bpf.h>

char _license[] SEC("license") = "GPL";

#ifdef DEBUG
#define bpf_debug bpf_printk
#else
#define bpf_debug(fmt, ...) {}
#endif

#define TASK_RUNNING			0x00000000   // include/linux/sched.h
#define TASK_INTERRUPTIBLE		0x00000001
#define TASK_UNINTERRUPTIBLE		0x00000002
#define TASK_NEW			0x00000800
#define TASK_DEAD			0x00000080

#define SCHED_EXT		7

#define TIF_NEED_RESCHED 1  // arch/arm64/include/asm/thread_info.h, X86是3

#define BUDGET_CALLBACK	0
#define PERIOD_CALLBACK	1

#define MAX_TASK_NR	2048
#define MAX_CPU_NR	32

#define SHARED_DSQ	MAX_CPU_NR

UEI_DEFINE(uei);

const volatile s32 usersched_pid;  // 当前rms服务的pid

enum rms_timer_set_remain_mode {
	SET_REMAIN_MODE_REMAIN = 0,
	SET_REMAIN_MODE_ABS = 1,
};

struct rms_timer {
	struct bpf_timer t;
	u64 periodic_time;
	u64 remain_time;
	u64 expire_time;
	u64 pending;
	u64 callback_index;
	pid_t arg;
};

struct task_attr {
	int cpu;
	u64 budget;
	u64 period;
	u64 is_yield;
#ifdef FEATURE_MIGRATION
	u64 is_migrate;
#endif
};

struct utilization_entry {
	double cpu_utilization;
	int task_num;
};

struct task_queue {   // RMS的CPU就绪队列
	struct bpf_spin_lock lock;
	pid_t pid[MAX_TASK_NR];
	uint head, tail;
};

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__type(key, u32);
	__type(value, struct task_queue);
	__uint(max_entries, MAX_CPU_NR);
} cpu_queues SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, MAX_TASK_NR);
	__type(key, pid_t);
	__type(value, struct rms_timer);
} sched_timer_hmap SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, MAX_TASK_NR);
	__type(key, pid_t);
	__type(value, struct rms_timer);
} budget_timer_hmap SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_HASH);
	__uint(max_entries, MAX_TASK_NR);
	__type(key, pid_t);
	__type(value, struct task_attr);
} task_attr_hmap SEC(".maps");

struct {
	__uint(type, BPF_MAP_TYPE_ARRAY);
	__uint(max_entries, MAX_CPU_NR);
	__type(key, u32);
	__type(value, struct utilization_entry);
} rms_entry_map SEC(".maps");

static u64 rms_get_timestamp(void)
{
	return bpf_ktime_get_ns();
}

static inline struct bpf_spin_lock *get_cpu_queue_lock(int cpu)
{
	return &((struct task_queue *)bpf_map_lookup_elem(&cpu_queues, &cpu))->lock;
}

static inline struct task_queue *get_cpu_queue(int cpu)
{
	return bpf_map_lookup_elem(&cpu_queues, &cpu);
}

static inline bool empty_queue(struct task_queue *queue)
{
	return queue->head == queue->tail;
}

static inline bool full_queue(struct task_queue *queue)
{
	return (queue->tail + 1) % MAX_TASK_NR == queue->head;
}

static bool cpu_queue_push(int cpu, pid_t pid)
{
	struct task_queue *queue = get_cpu_queue(cpu);
	struct bpf_spin_lock *lock = get_cpu_queue_lock(cpu);

	if (!queue || !lock)
		return false;

	bpf_spin_lock(lock);

	if (full_queue(queue) || queue->tail >= MAX_TASK_NR) {
		bpf_spin_unlock(lock);
		return false;
	}

	queue->pid[queue->tail] = pid;    // 插在队列尾部
	queue->tail = (queue->tail + 1) % MAX_TASK_NR;
	bpf_spin_unlock(lock);
	return true;
}

static bool cpu_queue_pop(int cpu, pid_t *pid)
{
	struct task_queue *queue = get_cpu_queue(cpu);
	struct bpf_spin_lock *lock = get_cpu_queue_lock(cpu);

	if (!queue || !lock)
		return false;

	bpf_spin_lock(lock);

	if (empty_queue(queue) || queue->head >= MAX_TASK_NR) {
		bpf_spin_unlock(lock);
		return false;
	}
	*pid = queue->pid[queue->head];
	queue->head = (queue->head + 1) % MAX_TASK_NR;
	bpf_spin_unlock(lock);
	return true;
}

static void rms_timer_init(struct rms_timer *timer, void *map)
{
	bpf_timer_init(&timer->t, map, CLOCK_MONOTONIC);
	timer->periodic_time = 0;
	timer->remain_time = 0;
	timer->pending = 0;
	timer->callback_index = 0;
	timer->arg = 0;
}

static int rms_timer_cancel(struct rms_timer *timer)
{
	u64 expire, now;

	timer->periodic_time = 0;
	if (bpf_timer_cancel(&timer->t) < 0)
		return -1;

	timer->pending = 0;
	expire = timer->expire_time;
	now = rms_get_timestamp();

	if (expire >= now)
		timer->remain_time = expire - now;
	else
		timer->remain_time = 0;

	return 0;
}

static int rms_timer_set_remain(struct rms_timer *timer, u64 tim, enum rms_timer_set_remain_mode mode)
{
	if (mode == SET_REMAIN_MODE_REMAIN) {
		timer->remain_time = tim;
	} else if (mode == SET_REMAIN_MODE_ABS) {
		u64 expire = timer->expire_time;

		if (expire >= tim)
			timer->remain_time = expire - tim;
		else
			timer->remain_time = 0;
	} else {
		return -1;
	}
	return 0;
}

static u64 rms_timer_get_remain(struct rms_timer *timer)
{
	u64 expire, now, remain_time;

	if (!timer->pending) {
		remain_time = timer->remain_time;
	} else {
		expire = timer->expire_time;
		now = rms_get_timestamp();

		if (expire > now)
			remain_time = expire - now;
		else
			remain_time = 0;
	}
	return remain_time;
}

static void refresh_timer(struct rms_timer *timer)
{
	if (timer->periodic_time > 0) {
		/* To keep period accurate, directly add periodic_time to expire_time */
		timer->expire_time += timer->periodic_time;
		timer->remain_time = timer->periodic_time;
		timer->pending = 1;
		bpf_timer_start(&timer->t, timer->expire_time, BPF_F_TIMER_ABS | BPF_F_TIMER_CPU_PIN);
	}
}

static void exit_rms_task(pid_t pid)
{
	struct rms_timer *p_budget = bpf_map_lookup_elem(&budget_timer_hmap, &pid);
	struct rms_timer *p_sched = bpf_map_lookup_elem(&sched_timer_hmap, &pid);
	struct task_attr *p_attr = bpf_map_lookup_elem(&task_attr_hmap, &pid);

	if (p_sched) {
		if (rms_timer_cancel(p_sched)) {
			bpf_debug("bpf timer cancel failed");
			return;
		}
		bpf_map_delete_elem(&sched_timer_hmap, &pid);
	}
	if (p_budget) {
		if (rms_timer_cancel(p_budget)) {
			bpf_debug("bpf timer cancel failed");
			return;
		}
		bpf_map_delete_elem(&budget_timer_hmap, &pid);
	}
	if (p_attr)
		bpf_map_delete_elem(&task_attr_hmap, &pid);
}

static int rms_callback_budget(void *map, int *key, struct rms_timer *timer)
{
	pid_t pid = timer->arg;
	struct task_attr *p_attr = bpf_map_lookup_elem(&task_attr_hmap, &pid);

	if (!p_attr) {
		scx_bpf_error("!p_attr\n");
		return 0;
	}

	timer->remain_time = p_attr->budget;

	if (!p_attr->is_yield)
		p_attr->is_yield = 1;
#ifdef FEATURE_BUDGET_OVERRUN
	else {
		/* rms task has done, keep current task running */
		struct task_struct *current = (struct task_struct *)bpf_get_current_task_btf();
		struct rms_timer *current_budget;

		if (!current)
			return 0;

		pid = current->pid;
		current_budget = bpf_map_lookup_elem(&budget_timer_hmap, &pid);

		if (current->policy == SCHED_EXT && current_budget) {
			if (rms_timer_cancel(current_budget))
				scx_bpf_error("bpf timer cancel failed");
			rms_timer_set_remain(current_budget, rms_get_timestamp(), SET_REMAIN_MODE_ABS);
		}
	}
#endif
	// 请求一次内核抢占，强制当前任务停止执行
	scx_bpf_kick_cpu(p_attr->cpu, SCX_KICK_PREEMPT);
	timer->pending = 0;
	refresh_timer(timer);
	return 0;
}
// 周期时间到后的回调函数
static int rms_callback_period(void *map, int *key, struct rms_timer *timer)
{
	pid_t pid = timer->arg;
	struct task_struct *t = bpf_task_from_pid(pid);
	struct task_attr *t_attr;
	struct rms_timer *t_budget;

	if (!t)
		goto rms_exited;

	if (t->policy != SCHED_EXT) {
		scx_bpf_error("bpf %d %s policy != SCHED_EXT\n", pid, t->comm);
		goto rms_exited;
	}

	t_attr = bpf_map_lookup_elem(&task_attr_hmap, &pid);
	t_budget = bpf_map_lookup_elem(&budget_timer_hmap, &pid);

	if (!t_attr || !t_budget)
		goto rms_exited;

	t_attr->is_yield = 0;

	bpf_debug("bpf push %d %s on cpu%d", pid, t->comm, t_attr->cpu);

	cpu_queue_push(t_attr->cpu, pid);
	scx_bpf_kick_cpu(t_attr->cpu, SCX_KICK_PREEMPT); // 以中断的形式唤醒？
	t_budget->remain_time = t_attr->budget;  // 剩下的时间为什么是预算？
	timer->pending = 0;
	bpf_task_release(t);
	refresh_timer(timer);
	return 0;

rms_exited:
	if (t)
		bpf_task_release(t);
	exit_rms_task(timer->arg);
	return 0;
}
// 定时器，到期时间，周期，callback号，flag
static void rms_timer_start(struct rms_timer *timer, u64 tim, u64 period_tim, u64 callback_index, pid_t arg)
{
	if (timer->pending) {
		if (rms_timer_cancel(timer))
			bpf_debug("bpf timer cancel failed");
	}

	timer->periodic_time = period_tim;
	timer->expire_time = tim;
	timer->remain_time = tim - rms_get_timestamp();
	timer->callback_index = callback_index;
	timer->arg = arg;
	// 定时器到期后的回调函数
	if (timer->callback_index == BUDGET_CALLBACK)
		bpf_timer_set_callback(&timer->t, (void *)rms_callback_budget);
	if (timer->callback_index == PERIOD_CALLBACK)
		bpf_timer_set_callback(&timer->t, (void *)rms_callback_period);

	timer->pending = 1;
	bpf_timer_start(&timer->t, tim, BPF_F_TIMER_ABS | BPF_F_TIMER_CPU_PIN);
}

static u64 cpu_to_dsq(s32 cpu)
{
	if (cpu < 0 || cpu >= MAX_CPU_NR) {
		scx_bpf_error("Invalid cpu: %d", cpu);
		return SHARED_DSQ;
	}
	return (u64)cpu;
}

static inline bool is_usersched_task(const struct task_struct *p)
{
	return p->pid == usersched_pid;
}

s32 BPF_STRUCT_OPS(rms_select_cpu, struct task_struct *p, s32 prev_cpu, u64 wake_flags)
{
	bpf_debug("%d %s select_cpu prev cpu%d", p->pid, p->comm, prev_cpu);
	if (bpf_cpumask_test_cpu(prev_cpu, p->cpus_ptr))
		return prev_cpu;

	return scx_bpf_pick_any_cpu(p->cpus_ptr, 0);
}

void BPF_STRUCT_OPS(rms_enqueue, struct task_struct *p, u64 enq_flags)
{
	u32 cur_cpu = bpf_get_smp_processor_id();
	u32 cpu = scx_bpf_task_cpu(p);
	pid_t pid = p->pid;
	struct task_attr *p_attr;

	if (is_usersched_task(p)) {
		// scx_bpf_dispatch(p, SCX_DSQ_LOCAL, SCX_SLICE_DFL, enq_flags);
		scx_bpf_dsq_insert(p, SCX_DSQ_LOCAL, SCX_SLICE_DFL, enq_flags);
		return;
	}

	bpf_debug("bpf enqueue %d %s enq_flags 0x%lx on cpu%u target cpu%u", p->pid, p->comm, enq_flags, cur_cpu, cpu);

	p_attr = bpf_map_lookup_elem(&task_attr_hmap, &pid);

	if (!p_attr)
		return;

	if (!p_attr->is_yield) {
		bpf_debug("bpf dispatch %d %s to cpu%d dsq", p->pid, p->comm, cpu);
		// scx_bpf_dispatch_vtime(p, cpu_to_dsq(cpu), SCX_SLICE_INF, p_attr->period, 0);
		scx_bpf_dsq_insert_vtime(p, cpu_to_dsq(cpu), SCX_SLICE_INF, p_attr->period, 0);
		if (cpu != cur_cpu)
			scx_bpf_kick_cpu(cpu, SCX_KICK_PREEMPT);
	} else {
		bpf_debug("bpf refuse to enqueue a yielded task! %d %s", p->pid, p->comm);
	}
}

void BPF_STRUCT_OPS(rms_dequeue, struct task_struct *p, u64 enq_flags)
{
	if (is_usersched_task(p))
		return;
	bpf_debug("bpf dequeue %d %s on cpu%d", p->pid, p->comm, bpf_get_smp_processor_id());
}

static int dispatch_loop(u32 index, void *context)
{
	struct task_struct *p;
	struct task_attr *t_attr;
	pid_t pid;
	s32 cpu = *((s32 *)context);

	if (!cpu_queue_pop(cpu, &pid))
		return 1;

	p = bpf_task_from_pid(pid);
	if (!p)
		return 1;
	bpf_debug("bpf pop %d %s on cpu%d", pid, p->comm, cpu);
	t_attr = bpf_map_lookup_elem(&task_attr_hmap, &pid);
	if (!t_attr) {
		bpf_task_release(p);
		return 1;
	}
#ifdef FEATURE_MIGRATION
	/* handle cpu migration, reset the timer */
	if (t_attr->is_migrate) {
		struct rms_timer *p_sched =  bpf_map_lookup_elem(&sched_timer_hmap, &pid);

		t_attr->is_migrate = 0;

		if (p_sched) {
			u64 remain = rms_timer_get_remain(p_sched);

			if (!remain)
				remain = t_attr->period;
			bpf_debug("bpf dispatch task cpu migration reset sched timer with remain time %ld", remain);
			rms_timer_start(p_sched, rms_get_timestamp() + remain,
							t_attr->period, PERIOD_CALLBACK, p->pid);
		}
	}
#endif
	bpf_debug("bpf dispatch %d %s on cpu%d", p->pid, p->comm, cpu);
	// scx_bpf_dispatch_vtime(p, cpu_to_dsq(cpu), SCX_SLICE_INF, t_attr->period, 0);
	scx_bpf_dsq_insert_vtime(p, cpu_to_dsq(cpu), SCX_SLICE_INF, t_attr->period, 0);
	bpf_task_release(p);
	return 0;
}

void BPF_STRUCT_OPS(rms_dispatch, s32 cpu, struct task_struct *prev)
{
	bpf_loop(MAX_TASK_NR, dispatch_loop, &cpu, 0);
	// scx_bpf_consume(cpu_to_dsq(cpu));
	scx_bpf_dsq_move_to_local(cpu_to_dsq(cpu));
}

void BPF_STRUCT_OPS(rms_running, struct task_struct *p)
{
	pid_t pid = p->pid;
	struct task_attr *p_attr = bpf_map_lookup_elem(&task_attr_hmap, &pid);

	bpf_debug("bpf running %d %s on cpu%d", p->pid, p->comm, bpf_get_smp_processor_id());

	if (p_attr) {
		struct rms_timer *p_budget = bpf_map_lookup_elem(&budget_timer_hmap, &pid);
#ifdef FEATURE_MIGRATION
		if (p_attr->is_migrate)
			return;
#endif

		if (p_budget) {
			u64 remain = rms_timer_get_remain(p_budget);

			if (!remain)
				remain = p_attr->budget;
			rms_timer_start(p_budget, rms_get_timestamp() + remain,
							0, BUDGET_CALLBACK, pid);
		}
	}
}

void BPF_STRUCT_OPS(rms_stopping, struct task_struct *p, bool runnable)
{
	bpf_debug("bpf stopping %d %s on cpu%d runnable %d state DEAD %d",
			   p->pid, p->comm, bpf_get_smp_processor_id(), runnable, p->__state == TASK_DEAD);
	if (!runnable && p->__state == TASK_DEAD)
		exit_rms_task(p->pid);
}

bool BPF_STRUCT_OPS(rms_yield, struct task_struct *from, struct task_struct *to)
{
	pid_t pid = from->pid;
	struct rms_timer *p_budget = bpf_map_lookup_elem(&budget_timer_hmap, &pid);
	struct task_attr *p_attr = bpf_map_lookup_elem(&task_attr_hmap, &pid);

	/* enforce kernel to call bpf_enqueue */
	from->scx.slice = 0;  // 强制立即调度，不听tick周期的节拍，可运行时间到0，也就是触发调度
	if (is_usersched_task(from))
		return true;

	bpf_debug("bpf yield %d %s on cpu%d", pid, from->comm, bpf_get_smp_processor_id());

	if (from->policy == SCHED_EXT && p_budget && p_attr) {
		p_attr->is_yield = 1;
		if (rms_timer_cancel(p_budget))
			scx_bpf_error("bpf timer cancel failed");
		rms_timer_set_remain(p_budget, rms_get_timestamp(),
			 SET_REMAIN_MODE_ABS);
	}
	return true;
}
// 用户空间调用 设置 CPU 亲和性（如 sched_setaffinity）的接口时触发
void BPF_STRUCT_OPS(rms_set_cpumask, struct task_struct *p, const struct cpumask *cpumask)
{
	// 程序绑定CPU核心，然后bpf_get_smp_processor_id获取到
	bpf_debug("bpf set_cpumask %d %s on cpu%d", p->pid, p->comm, bpf_get_smp_processor_id());
	if (is_usersched_task(p))  // rms服务不用rms调度
		return;

	if (p->policy == SCHED_EXT) {
		struct rms_timer empty_timer = {};
		pid_t pid = p->pid;
		struct rms_timer *p_budget = bpf_map_lookup_elem(&budget_timer_hmap, &pid);
		struct rms_timer *p_sched = bpf_map_lookup_elem(&sched_timer_hmap, &pid);
		struct task_attr *p_attr = bpf_map_lookup_elem(&task_attr_hmap, &pid);

		if (!p_budget && !p_sched && p_attr) {
			bpf_debug("bpf first init rms task");

			bpf_debug("rms %d %s budget %ld period %ld", p->pid, p->comm, p_attr->budget, p_attr->period);
			// 作用是什么?
			bpf_map_update_elem(&budget_timer_hmap, &pid, &empty_timer, BPF_NOEXIST);
			bpf_map_update_elem(&sched_timer_hmap, &pid, &empty_timer, BPF_NOEXIST);

			p_budget = bpf_map_lookup_elem(&budget_timer_hmap, &pid);
			p_sched = bpf_map_lookup_elem(&sched_timer_hmap, &pid);

			if (p_budget) {
				rms_timer_init(p_budget, (void *)&budget_timer_hmap);
				// 预算时间，应用空间赋的值，最终到这里
				p_budget->remain_time = p_attr->budget;
			}

			if (p_sched) {
				rms_timer_init(p_sched, (void *)&sched_timer_hmap);
				rms_timer_start(p_sched, rms_get_timestamp() + p_attr->period,
								p_attr->period, PERIOD_CALLBACK, p->pid);
			}
			p->scx.slice = SCX_SLICE_INF;  // task运行时间无穷大
		} else if (p_sched && p_budget && p_attr) {
			u32 prev_cpu = scx_bpf_task_cpu(p);

			if (!bpf_cpumask_test_cpu(prev_cpu, cpumask)) {
				if (rms_timer_cancel(p_sched))
					scx_bpf_error("bpf timer cancel failed");
				if (rms_timer_cancel(p_budget))
					scx_bpf_error("bpf timer cancel failed");
#ifdef FEATURE_MIGRATION
				p_attr->is_migrate = 1;
#endif
			}
		}
	}
}

void BPF_STRUCT_OPS(rms_disable, struct task_struct *p)
{
	if (p->policy == SCHED_EXT) {
		bpf_debug("bpf disable %d %s on cpu%d", p->pid, p->comm, bpf_get_smp_processor_id());
		exit_rms_task(p->pid);
	}
}
// 核心：创建local DSQ 与 global DSQ
s32 BPF_STRUCT_OPS_SLEEPABLE(rms_init)
{
	int err;

	bpf_debug("rms_init");
	if (usersched_pid <= 0) {
		scx_bpf_error("User scheduler pid uninitialized (%d)",
				  usersched_pid);
		return -EINVAL;
	}

	/* Create per-CPU DSQs (used to dispatch tasks directly on a CPU) */
	for (s32 cpu = 0; cpu < MAX_CPU_NR; cpu++) {
		err = scx_bpf_create_dsq(cpu_to_dsq(cpu), -1);
		if (err < 0) {
			scx_bpf_error("Failed to create pcpu DSQ %d: %d", cpu, err);
			return err;
		}
	}

	/* Create the global shared DSQ (for regular tasks) */
	err = scx_bpf_create_dsq(SHARED_DSQ, -1);
	if (err < 0) {
		scx_bpf_error("Failed to create shared DSQ: %d", err);
		return err;
	}
	return 0;
}

void BPF_STRUCT_OPS(rms_exit, struct scx_exit_info *ei)
{
	UEI_RECORD(uei, ei);
}

/*
 * This rms bpf scheduler can only work on SMP, because we
 * need to use bpf_dispatch function to control the tasks.
 */
SCX_OPS_DEFINE(rms_ops,
	// 选择 CPU：为正在唤醒的任务p选择一个目标 CPU。
	// 任务 p 正在被唤醒（从睡眠状态变为可运行状态）时触发
	.select_cpu		= (void *)rms_select_cpu,
	// 任务入队：将任务p放入 BPF 调度器的就绪队列中
	// 任务p准备运行（成为可运行状态），但未被 select_cpu 直接调度时触发。
	// 例如，任务p 的时间片用完后再次入队
	.enqueue		= (void *)rms_enqueue,
	// 任务出队：从 BPF 调度器的队列中移除任务p
	// 更新任务的调度属性（如优先级、权重）时，或者在其他需要暂时隔离任务的操作中触发。
	.dequeue		= (void *)rms_dequeue,
	// 任务分发：从 BPF 调度器中选择一个或多个任务并将其分发到 CPU 的本地调度队列（DSQ）中
	// 当前 CPU 的本地 DSQ 变空时（即 CPU 无法从本地队列找到下一个任务时）触发
	.dispatch		= (void *)rms_dispatch,
	// 开始运行：任务P即将开始在 CPU 上执行。
	// 任务P被调度并开始在 CPU 上运行时触发
	.running		= (void *)rms_running,
	// 停止运行：任务P即将停止执行（可能是因为时间片耗尽、被抢占或进入睡眠）
	// 任务P被切换出 CPU 时触发
	.stopping		= (void *)rms_stopping,
	// 任务让出：当前任务P主动放弃 CPU。
	// 任务P调用类似 sched_yield() 的系统调用或通过其他机制主动让出 CPU 时触发。
	.yield			= (void *)rms_yield,
	// 设置 CPU 亲和性：更新任务P可以运行的 CPU 集合
	// 用户空间调用 设置 CPU 亲和性（如 sched_setaffinity）的接口时触发
	.set_cpumask	= (void *)rms_set_cpumask,
	// 禁用 BPF 调度：为任务P禁用 BPF 调度。
	// 任务 退出 SCX 调度类、退出系统，或 BPF 调度器 被卸载 时触发
	.disable		= (void *)rms_disable,
	// 初始化 BPF 调度器：在调度器加载后执行一次性的初始化操作。
	// BPF 调度器 首次加载 到内核时触发
	.init			= (void *)rms_init,
	// 清理 BPF 调度器：执行清理操作
	// BPF 调度器 被卸载 或 加载失败 时触发
	.exit			= (void *)rms_exit,
	// kernel/sched/ext.c
	// SCX_OPS_ENQ_LAST: 控制当 CPU 上没有其他可运行任务时的行为。默认情况下，调度器会继续运行当前任务，但设置此标志后会将任务传递给 BPF 调度器处理
	// SCX_OPS_SWITCH_PARTIAL 控制调度器的切换模式。设置时只处理 SCHED_EXT 策略的任务，清除时也处理 SCHED_NORMAL 任务。
	.flags			= SCX_OPS_ENQ_LAST | SCX_OPS_SWITCH_PARTIAL,
	.name			= "rms");
