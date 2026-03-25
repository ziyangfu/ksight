#include <iostream>
#include <csignal>
#include <atomic>

#include "argparse/argparse.hpp"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

#include "ConfigArgs.h"
#include "ArgParser.h"
#include "BindsnoopBpf.h"

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::info);
    
    net::bindSnoop::ConfigArgs config;
    argparse::ArgumentParser parser("bindsnoop");
    net::bindSnoop::cmdParser(parser, config);

    try {
        parser.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        SPDLOG_ERROR("{}", err.what());
        std::cerr << parser;
        return 1;
    }

    if (config.verbose) {
        spdlog::set_level(spdlog::level::debug);
    }

    try {
        net::bindSnoop::BindsnoopBpf bindsnoop(config);
        
        bindsnoop.init();
        bindsnoop.load();
        bindsnoop.attach();

        SPDLOG_INFO("Bindsnoop started. Press Ctrl+C to stop.");
        
        bindsnoop.poll();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Exception: {}", e.what());
        return 1;
    }

    return 0;
}
