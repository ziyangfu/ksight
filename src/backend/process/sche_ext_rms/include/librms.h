// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2025 Li Auto Inc. and its affiliates
 */
#ifndef __LIBRMS_H__
#define __LIBRMS_H__

#ifdef __cplusplus
extern "C" {
#endif
#include <stdint.h>
int sched_set_rms_params(int cpu, uint64_t budget, uint64_t period);
int sched_rms(int cpu, uint64_t budget, uint64_t period);
int drain_rms_exit_queue(int cpu, uint64_t budget, uint64_t period);
#ifdef __cplusplus
}
#endif

#endif