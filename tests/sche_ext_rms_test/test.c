// demo.c
#include <stdio.h>
#include <unistd.h>
#include <sched.h>
#include <signal.h>
#include <stdlib.h>
#include "librms.h"

static volatile int exit_req;

static void sigint_handler(int sig) {
    exit_req = 1;
}

int main() {
    int cpu = 3;
    unsigned long budget =  5UL * 1000UL * 1000UL;
    unsigned long period = 10UL * 1000UL * 1000UL;

    signal(SIGINT, sigint_handler); 

    // 绑定到CPU3，设置预算5ms，周期10ms
    if (sched_rms(cpu, budget, period) != 0) {
        printf("sched_rms failed\n");
        exit(EXIT_FAILURE);
    }

    while(!exit_req) {
        printf("Running task...\n");
        // TASK
        sched_yield();
    }

    // 清理RMS任务队列
    drain_rms_exit_queue(cpu, budget, period);
    return 0;
}
