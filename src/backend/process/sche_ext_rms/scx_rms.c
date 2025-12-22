// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <sched.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <libgen.h>
#include <bpf/bpf.h>
#include <pthread.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/types.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/syscall.h>

#include <scx/common.h>
#include "scx_rms.bpf.skel.h"

#define SCHED_EXT 7
#define SCHED_FLAG_KEEP_PARAMS		0x10

#define VERSION "1.0.0"

static volatile int exit_req;

struct rms_params {
	int cpu;
	uint64_t budget;
	uint64_t period;
};

static void sigint_handler(int sig)
{
	exit_req = 1;
}

int main(int argc, char **argv)
{
	struct scx_rms *skel;
	struct bpf_link *link;
	int opt;
	int rms_params_fd, rms_entry_fd;

	while ((opt = getopt(argc, argv, "v")) != -1) {
		switch (opt) {
		case 'v':
			printf("SCX RMS Scheduler v%s\n", VERSION);
			return 0;
		}
	}

	signal(SIGINT, sigint_handler);
	signal(SIGTERM, sigint_handler);

	libbpf_set_strict_mode(LIBBPF_STRICT_ALL);

	skel = scx_rms__open();
	SCX_BUG_ON(!skel, "Failed to open skel");

	skel->rodata->usersched_pid = getpid();

	SCX_BUG_ON(scx_rms__load(skel), "Failed to load skel");

	link = bpf_map__attach_struct_ops(skel->maps.rms_ops);
	SCX_BUG_ON(!link, "Failed to attach struct_ops");

	rms_params_fd = bpf_map__fd(skel->maps.task_attr_hmap);
	SCX_BUG_ON(rms_params_fd < 0, "Failed to map task_attr_hmap");
	if (bpf_map__pin(skel->maps.task_attr_hmap, "/sys/fs/bpf/task_attr_hmap")) {
		fprintf(stderr, "Failed to pin task_attr_hmap\n");
		return -1;
	}

	rms_entry_fd = bpf_map__fd(skel->maps.rms_entry_map);
	SCX_BUG_ON(rms_entry_fd < 0, "Failed to map rms_entry_map");
	if (bpf_map__pin(skel->maps.rms_entry_map, "/sys/fs/bpf/rms_entry_map")) {
		fprintf(stderr, "Failed to pin rms_entry_map\n");
		return -1;
	}

	printf("bpf loader pid %d\n", getpid());

	while (!exit_req && !UEI_EXITED(skel, uei))
		sleep(10);

	bpf_map__unpin(skel->maps.task_attr_hmap, "/sys/fs/bpf/task_attr_hmap");
	bpf_map__unpin(skel->maps.rms_entry_map, "/sys/fs/bpf/rms_entry_map");

	bpf_link__destroy(link);
	UEI_REPORT(skel, uei);
	scx_rms__destroy(skel);
	printf("exited bpf scheduler!\n");

	return 0;
}
