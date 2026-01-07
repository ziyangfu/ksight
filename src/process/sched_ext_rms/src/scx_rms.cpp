#include <iostream>
#include <signal.h>
#include <unistd.h>
#include "ScxRmsBpf.h"
#include "spdlog/spdlog.h"

using namespace process::scheExtRms;

static volatile int exit_req = 0;

static void sigint_handler(int sig) {
    exit_req = 1;
}

int main(int argc, char **argv) {
    int opt;
    while ((opt = getopt(argc, argv, "v")) != -1) {
        switch (opt) {
        case 'v':
            std::cout << "SCX RMS Scheduler v1.0.0" << std::endl;
            return 0;
        }
    }

    signal(SIGINT, sigint_handler);
    signal(SIGTERM, sigint_handler);

    try {
        ScxRmsBpf bpf;
        bpf.open();
        bpf.load();
        bpf.attach();

        std::cout << "bpf loader pid " << getpid() << std::endl;

        while (!exit_req && !bpf.shouldExit()) {
            sleep(1);
        }

        bpf.reportExit();
        std::cout << "exited bpf scheduler!" << std::endl;
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Exception: {}", e.what());
        return 1;
    }

    return 0;
}
