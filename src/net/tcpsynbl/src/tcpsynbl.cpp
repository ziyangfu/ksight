#include <iostream>
#include <csignal>

#include "argparse/argparse.hpp"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

#include "ConfigArgs.h"
#include "ArgParser.h"
#include "TcpsynblBpf.h"

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::info);

    net::tcpsynbl::ConfigArgs config;
    argparse::ArgumentParser parser("tcpsynbl");
    net::tcpsynbl::cmdParser(parser, config);

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
        net::tcpsynbl::TcpsynblBpf tcpsynbl(config);

        tcpsynbl.init();
        tcpsynbl.load();
        tcpsynbl.attach();
        tcpsynbl.run();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Exception: {}", e.what());
        return 1;
    }

    return 0;
}
