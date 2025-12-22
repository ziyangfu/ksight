// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sched.h>
#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/syscall.h>
#include <sys/types.h>
#include <sys/prctl.h>
#include <bpf/bpf.h>
#include <fcntl.h>
#include <sys/file.h>

#define SCHED_EXT 7
#define MAX_CPU_NR 24

struct task_attr {
	int cpu;
	uint64_t budget;
	uint64_t period;
	uint64_t is_yield;
#ifdef FEATURE_MIGRATION
	uint64_t is_migrate;
#endif
};

struct utilization_entry {
	double cpu_utilization;
	int task_num;
};

struct sched_attr {
	uint32_t size;
	uint32_t sched_policy;
	uint64_t sched_flags;
	int32_t sched_nice;
	uint32_t sched_priority;
	uint64_t sched_runtime;
	uint64_t sched_deadline;
	uint64_t sched_period;
};

static int sched_setattr(pid_t pid, const struct sched_attr *attr, unsigned int flags)
{
	return syscall(SYS_sched_setattr, pid, attr, flags);
}

static void print_error(void)
{
	char error[256];
	char *ret = strerror_r(errno, error, sizeof(error));

	printf("Error code: %d\n", errno);
	printf("Error message: %s\n", ret);
}

bool check_rms_params(struct task_attr *params, double cur_u, int cur_n)
{
	if (params->cpu < 0 || params->cpu >= MAX_CPU_NR || !params->budget || !params->period)
		return 0;

	double new_u = (double)params->budget / (double)params->period;
	double n = (double)cur_n + 1;

	return (cur_u + new_u) <= n * (pow(2, 1 / n) - 1);
}

int sched_set_rms_params(int cpu, uint64_t budget, uint64_t period)
{
	int pid = gettid();
	int task_attr_fd, rms_entry_fd, lock_fd;
	char lock_file_path[256];
	struct utilization_entry entry_map;
	struct task_attr params;

	task_attr_fd = bpf_obj_get("/sys/fs/bpf/task_attr_hmap");
	if (task_attr_fd < 0) {
		perror("Failed to open task_attr_hmap");
		return -1;
	}

	rms_entry_fd = bpf_obj_get("/sys/fs/bpf/rms_entry_map");
	if (rms_entry_fd < 0) {
		close(task_attr_fd);
		perror("Failed to open rms_entry_fd");
		return -1;
	}

	snprintf(lock_file_path, sizeof(lock_file_path), "/tmp/rms_cpu_%d.lock", cpu);

	lock_fd = open(lock_file_path, O_CREAT | O_RDWR, 0666);
	if (lock_fd < 0) {
		perror("Failed to open lock file");
		pid = -1;
		goto OUT_CLOSE_BPF_MAPS;
	}

	if (flock(lock_fd, LOCK_EX) < 0) {
		perror("Failed to acquire lock");
		pid = -1;
		close(lock_fd);
		goto OUT_CLOSE_BPF_MAPS;
	}

	if (bpf_map_lookup_elem(rms_entry_fd, &cpu, &entry_map)) {
		printf("lookup elem failed!\n");
		pid = -1;
		goto OUT;
	}

	printf("cpu%d U: %f N: %d\n", cpu, entry_map.cpu_utilization, entry_map.task_num);
	params.cpu = cpu;
	params.budget = budget;
	params.period = period;
	params.is_yield = 0;
#ifdef FEATURE_MIGRATION
	params.is_migrate = 0;
#endif

	if (!check_rms_params(&params, entry_map.cpu_utilization, entry_map.task_num)) {
		printf("check_rms_params failed!\n");
		pid = -1;
		goto OUT;
	}

	entry_map.cpu_utilization += (double)params.budget / (double)params.period;
	entry_map.task_num++;

	printf("cpu%d U: %f N: %d\n", cpu, entry_map.cpu_utilization, entry_map.task_num);

	if (bpf_map_update_elem(task_attr_fd, &pid, &params, 0)) {
		perror("bpf_map_update_elem task_attr_hmap");
		pid = -1;
		goto OUT;
	}

	if (bpf_map_update_elem(rms_entry_fd, &cpu, &entry_map, 0)) {
		perror("bpf_map_update_elem entry_map");
		pid = -1;
		goto OUT;
	}

	printf("rms init cpu%d pid %d\n", cpu, getpid());
OUT:
	flock(lock_fd, LOCK_UN);
	close(lock_fd);

OUT_CLOSE_BPF_MAPS:
	close(rms_entry_fd);
	close(task_attr_fd);

	return pid;
}

int sched_rms(int cpu, uint64_t budget, uint64_t period)
{
	int err;
	struct sched_attr attr;

	if (cpu >= 0)
		err = sched_set_rms_params(cpu, budget, period);
	else
		err = sched_set_rms_params(sched_getcpu(), budget, period);

	if (err < 0) {
		printf("send params failed!\n");
		return -1;
	}

	if (cpu >= 0) {
		cpu_set_t cpus;

		CPU_ZERO(&cpus);
		CPU_SET(cpu, &cpus);
		if (sched_setaffinity(0, sizeof(cpu_set_t), &cpus)) {
			printf("sched_setaffinity error\n");
			print_error();
			return -1;
		}
		printf("sched_setaffinity %d ok\n", cpu);
	}

	memset(&attr, 0, sizeof(struct sched_attr));
	attr.size = sizeof(struct sched_attr);
	attr.sched_policy = SCHED_EXT;
	if (sched_setattr(0, &attr, 0)) {
		printf("sched_setattr ext error\n");
		print_error();
		return -1;
	}
	return 0;
}

int drain_rms_exit_queue(int cpu, uint64_t budget, uint64_t period)
{
	int ret = 0, sched_cpu;
	int rms_entry_fd, lock_fd;
	char lock_file_path[256];
	struct utilization_entry entry_map;

	rms_entry_fd = bpf_obj_get("/sys/fs/bpf/rms_entry_map");
	if (rms_entry_fd < 0) {
		perror("Failed to open rms_entry_fd");
		return -1;
	}

	if (cpu < 0)
		sched_cpu = sched_getcpu();
	else
		sched_cpu = cpu;

	snprintf(lock_file_path, sizeof(lock_file_path), "/tmp/rms_cpu_exit_%d.lock", sched_cpu);

	lock_fd = open(lock_file_path, O_CREAT | O_RDWR, 0666);

	if (lock_fd < 0) {
		perror("Failed to open lock file");
		ret = -1;
		goto OUT_CLOSE_BPF_MAPS;
	}

	if (flock(lock_fd, LOCK_EX) < 0) {
		perror("Failed to acquire lock");
		close(lock_fd);
		ret = -1;
		goto OUT_CLOSE_BPF_MAPS;
	}

	if (!bpf_map_lookup_elem(rms_entry_fd, &sched_cpu, &entry_map)) {
		printf("cpu%d U: %f N: %d\n", sched_cpu, entry_map.cpu_utilization, entry_map.task_num);
		entry_map.cpu_utilization -= (double)budget / (double)period;
		entry_map.task_num--;
		printf("cpu%d U: %f N: %d\n", sched_cpu, entry_map.cpu_utilization, entry_map.task_num);
		if (bpf_map_update_elem(rms_entry_fd, &sched_cpu, &entry_map, 0)) {
			perror("bpf_map_update_elem entry_map");
			ret = -1;
			goto OUT;
		}
	}

OUT:
	flock(lock_fd, LOCK_UN);
	close(lock_fd);

OUT_CLOSE_BPF_MAPS:
	close(rms_entry_fd);
	return ret;
}