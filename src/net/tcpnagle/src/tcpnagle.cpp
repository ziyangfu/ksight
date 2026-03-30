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
  
  SPDLOG_INFO("正在加载 eBPF 程序...");
  bpf.openAndLoad();

  // 如果指定了 cgroup，则执行强制禁用 (SOCK_OPS)
  if (!config.cgroupPath.empty()) {
      bpf.attach();
  }
  
  // 执行现有的套接字扫描 (Iterator)
  SPDLOG_INFO("正在扫描活跃的 TCP 连接状态...");
  bpf.run();

  // 如果是在强制禁用模式，我们需要保持运行以处理新连接
  if (!config.cgroupPath.empty()) {
      SPDLOG_INFO("正在后台运行以强制禁用新连接的 Nagle 算法。按 Ctrl+C 退出...");
      while (true) {
          sleep(10);
      }
  }

  return 0;
}
