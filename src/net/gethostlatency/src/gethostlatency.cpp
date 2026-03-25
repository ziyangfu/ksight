#include <iostream>
#include <csignal>
#include <atomic>

#include "argparse/argparse.hpp"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

#include "ConfigArgs.h"
#include "ArgParser.h"
#include "GethostlatencyBpf.h"

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::info);
    
    net::getHostLatency::ConfigArgs config;
    argparse::ArgumentParser parser("gethostlatency");
    net::getHostLatency::cmdParser(parser, config);

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
        net::getHostLatency::GethostlatencyBpf gethostlatency(config);
        
        gethostlatency.init();
        gethostlatency.load();
        gethostlatency.attach();

        SPDLOG_INFO("Gethostlatency started. Press Ctrl+C to stop.");
        
        gethostlatency.poll();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Exception: {}", e.what());
        return 1;
    }

    return 0;
}
