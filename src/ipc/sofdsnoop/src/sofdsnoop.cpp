#include <iostream>
#include <csignal>

#include "argparse/argparse.hpp"
#include "spdlog/spdlog.h"
#include "fmt/format.h"

#include "ConfigArgs.h"
#include "ArgParser.h"
#include "SofdsnoopBpf.h"

int main(int argc, char *argv[]) {
    spdlog::set_level(spdlog::level::info);

    ipc::sofdsnoop::ConfigArgs config;
    argparse::ArgumentParser parser("sofdsnoop");
    ipc::sofdsnoop::cmdParser(parser, config);

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
        ipc::sofdsnoop::SofdsnoopBpf sofdsnoop(config);

        sofdsnoop.init();
        sofdsnoop.load();
        sofdsnoop.attach();
        sofdsnoop.poll();
    } catch (const std::exception& e) {
        SPDLOG_ERROR("Exception: {}", e.what());
        return 1;
    }

    return 0;
}
