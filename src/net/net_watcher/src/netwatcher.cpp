#include <iostream>
#include <csignal>
#include <atomic>

#include "argparse/argparse.hpp"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

#include "ConfigArgs.h"
#include "ArgParser.h"
#include "NetWatcherBpf.h"
#include "JsonConfigGenerator.h"

std::atomic<bool> gStoped(false);

void signalHandler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        SPDLOG_INFO("Received signal {}, preparing to exit...", signum);
        gStoped = true;
    }
}

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::info);
    
    net::netWatcher::ConfigArgs config;
    argparse::ArgumentParser parser("net_watcher");
    net::netWatcher::cmdParser(parser, config);

    try {
        parser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        SPDLOG_ERROR("{}", err.what());
        std::cerr << parser;
        return 1;
    }

    // Handle JSON config generation
    if (config.generateConfigJson) {
        if (net::netWatcher::generateJsonConfig("netwatcher")) {
            fmt::print("JSON configuration file generated successfully: netwatcher_args.json\n");
            return 0;
        } else {
            SPDLOG_ERROR("Failed to generate JSON configuration file");
            return 1;
        }
    }

    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);

    net::netWatcher::NetWatcherBpf netWatcher(config);
    
    netWatcher.open();
    netWatcher.setBpfProgsLoadOpt();
    netWatcher.setRodataFlags();
    netWatcher.load();
    netWatcher.attach();

    SPDLOG_INFO("NetWatcher started. Press Ctrl+C to stop.");
    
    netWatcher.poll();

    return 0;
}
