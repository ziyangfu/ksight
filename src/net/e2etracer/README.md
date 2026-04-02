# e2etracer - 车载 SOA 全链路追踪工具

## 1. 工具定位
`e2etracer` 旨在为车载 SOA 服务（SOME/IP, DDS）提供从应用层到内核物理层的“垂向全链路”追踪。它能够精确测量每一层的处理耗时，帮助开发者快速定位丢包、抖动或高延迟的根因。

## 2. 追踪路径
- **User (App)**: 通过 USDT/Uprobe 捕获发送起点。
- **Middleware**: 捕获中间件序列化和排队耗时。
- **Syscall**: 进入内核网络栈的时刻。
- **NetStack (L4, L3, L2)**: 传输层、网络层和链路层的层间耗时。
- **Driver/NIC**: 硬件实际发出或接收的时刻。

## 3. 使用方法

### 编译
```bash
mkdir build && cd build
cmake .. && make e2etracer
```

### 运行
```bash
# 基本运行（追踪 5 元组）
sudo ./e2etracer --config soa_config.json

# 追踪特定服务（开启 SOME/IP 解析）
sudo ./e2etracer --config soa_config.json --someip --serviceID 0x1234
```

## 4. 配置文件格式 (soa_config.json)
```json
[
    {
        "service_id": 4660,
        "instance_id": 1,
        "service_name": "Front_Camera_Service",
        "ip": "192.168.1.10",
        "port": 30491,
        "protocol": "UDP"
    }
]
```

## 5. 注意事项
- 本工具依赖 eBPF **fentry/fexit** 特性，推荐内核版本 **5.10+**。
- 为降低性能损耗，在处理高频数据流时，请务必使用 `--serviceID` 进行过滤。
- 对于 TCP 字节流，工具抓取负载的前 16 字节可能因为分片而未包含完整 Header，建议优先用于 UDP 通信或分析固定长报文。
