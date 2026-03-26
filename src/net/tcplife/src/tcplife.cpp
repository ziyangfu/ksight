#include <iostream>
#include <csignal>
#include <atomic>

#include "argparse/argparse.hpp"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

#include "ConfigArgs.h"
#include "ArgParser.h"
#include "TcplifeBpf.h"

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::info);
    
    net::tcplife::ConfigArgs config;
    argparse::ArgumentParser parser("tcplife");
    net::tcplife::cmdParser(parser, config);

    try {
        parser.parse_args(argc, argv);
    } catch (const std::exception& err) {
        SPDLOG_ERROR("{}", err.what());
        std::cerr << parser;
        return 1;
    }

    if (config.verbose) {
        spdlog::set_level(spdlog::level::debug);
    }

    try {
        net::tcplife::TcplifeBpf tcplife(config);
        
        tcplife.init();
        tcplife.load();
        tcplife.attach();

        tcplife.poll();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Exception: {}", e.what());
        return 1;
    }

    return 0;
}
