#include <iostream>
#include <string>
#include "E2ETracer.h"
#include "SoaManager.h"
#include "argparse/argparse.hpp"

using namespace ksight::net;

namespace ksight {
namespace net {
SoaManager g_soa_manager;
} // namespace net
} // namespace ksight

int main(int argc, char *argv[]) {
    argparse::ArgumentParser program("e2etracer");
    
    program.add_argument("--config")
           .help("Path to SOA config JSON file")
           .required();

    program.add_argument("--serviceID")
           .help("Filter by Service ID")
           .scan<'i', int>();

    program.add_argument("--instanceID")
           .help("Filter by Instance ID")
           .scan<'i', int>();

    program.add_argument("--someip")
           .help("Enable SOME/IP Header capture and parsing")
           .default_value(false)
           .implicit_value(true);

    try {
        program.parse_args(argc, argv);
    } catch (const std::runtime_error& err) {
        std::cerr << err.what() << std::endl;
        std::cerr << program;
        return 1;
    }

    // 1. 加载配置
    std::string config_path = program.get<std::string>("--config");
    if (!g_soa_manager.load_config(config_path)) {
        std::cerr << "Failed to load config: " << config_path << std::endl;
        return 1;
    }

    // 2. 准备追踪器
    E2ETracer tracer;

    // 3. 设置过滤器 (必须在 init() 之前调用，以配置 BPF rodata)
    if (program.present("--serviceID")) {
        uint32_t sid = program.get<int>("--serviceID");
        uint32_t iid = program.present("--instanceID") ? program.get<int>("--instanceID") : 0;
        tracer.set_service_filter(sid, iid);
    }

    // 4. 初始化追踪器并加载 BPF
    if (tracer.init() != 0) {
        return 1;
    }

    std::cout << "e2etracer started. Press Ctrl+C to stop..." << std::endl;

    // 4. 开始追踪
    return tracer.start();
}
