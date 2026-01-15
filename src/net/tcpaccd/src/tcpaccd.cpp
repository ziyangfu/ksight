#include <atomic>
#include <csignal>
#include <iostream>

#include "ArgParser.h"
#include "ConfigArgs.h"
#include "TcpAccdBpf.h"
#include "argparse/argparse.hpp"
#include "spdlog/spdlog.h"

std::atomic<bool> gStoped(false);

void signalHandler(int signum) {
  if (signum == SIGINT) {
    SPDLOG_INFO("Received SIGINT, preparing to exit...");
    gStoped = true;
  }
}

int main(int argc, char **argv) {
  spdlog::set_level(spdlog::level::info);
  signal(SIGINT, signalHandler);

  net::tcpAccd::ConfigArgs config;
  argparse::ArgumentParser parser("tcpaccd");
  net::tcpAccd::cmdParser(parser, config);

  try {
    parser.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    SPDLOG_ERROR("{}", err.what());
    return 1;
  }

  net::tcpAccd::TcpAccdBpf tcpAccdBpf(config);

  tcpAccdBpf.open();
  tcpAccdBpf.load();
  tcpAccdBpf.attach();

  SPDLOG_INFO("tcpaccd is running. Press Ctrl+C to stop.");

  while (!gStoped) {
    tcpAccdBpf.poll();
  }

  tcpAccdBpf.destroy();
  SPDLOG_INFO("tcpaccd exited.");

  return 0;
}