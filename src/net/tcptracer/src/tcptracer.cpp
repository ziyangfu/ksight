#include <iostream>
#include <csignal>
#include <atomic>

#include "argparse/argparse.hpp"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

#include "ConfigArgs.h"
#include "ArgParser.h"
#include "TcptracerBpf.h"

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::info);
    
    net::tcptracer::ConfigArgs config;
    argparse::ArgumentParser parser("tcptracer");
    net::tcptracer::cmdParser(parser, config);

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
        net::tcptracer::TcptracerBpf tcptracer(config);
        
        tcptracer.init();
        tcptracer.load();
        tcptracer.attach();

        tcptracer.poll();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Exception: {}", e.what());
        return 1;
    }

    return 0;
}
