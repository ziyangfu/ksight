#include "ArgParser.h"
#include "TcpNagleBpf.h"
#include "spdlog/spdlog.h"
#include <iostream>

using namespace net::tcpNagle;

int main(int argc, const char **argv) {
  argparse::ArgumentParser parser("tcpnagle");
  ConfigArgs config;

  // 1. 解析参数
  cmdParser(parser, config);
  try {
    parser.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cerr << err.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  // 2. 初始化并运行 BPF 程序
  TcpNagleBpf bpf(config);
  
  SPDLOG_INFO("正在加载 eBPF 程序以观测 Nagle 算法状态...");
  bpf.openAndLoad();
  
  SPDLOG_INFO("正在扫描活跃的 TCP 连接...");
  bpf.run();

  return 0;
}
