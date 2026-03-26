VLAN 与 MAC 绑定检查
合理性： 高。在多网段车载或工业以太网中，VLAN 隔离是基础安全要求。

实现逻辑： * 从 ifconfig 或 ip -d link show 提取子接口信息（如 eth.7 对应 VLAN ID 7）。

比对 JSON 中定义的子接口与对应的 ether (MAC地址) 是否一致