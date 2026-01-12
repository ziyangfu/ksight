#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <arpa/inet.h>

#include "tcpaccd.skel.h"

static volatile bool exiting = false;

static void sig_handler(int sig)
{
    exiting = true;
}

int main(int argc, char **argv)
{
    struct bpf_redir *skel;
    int cgfd = -1, ret = 1;

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    // 打开并加载 BPF 程序
    skel = tcpaccd__open();
    if (!skel) {
        fprintf(stderr, "Failed to open BPF skeleton\n");
        goto cleanup;
    }

    // 加载 BPF 程序
    ret = tcpaccd__load(skel);
    if (ret) {
        fprintf(stderr, "Failed to load BPF programs\n");
        goto cleanup;
    }

    // 附加 sockops 程序到 cgroup
    cgfd = open("/sys/fs/cgroup/unified", O_RDONLY);
    if (cgfd < 0) {
        fprintf(stderr, "Failed to open cgroup\n");
        goto cleanup;
    }

    ret = tcpaccd__attach(skel);
    if (ret) {
        fprintf(stderr, "Failed to attach BPF programs\n");
        goto cleanup;
    }

    printf("Socket acceleration loaded successfully! Press Ctrl+C to exit.\n");

    while (!exiting) {
        sleep(1);
    }

    ret = 0;

cleanup:
    if (cgfd >= 0)
        close(cgfd);
    tcpaccd__destroy(skel);
    return ret;
}