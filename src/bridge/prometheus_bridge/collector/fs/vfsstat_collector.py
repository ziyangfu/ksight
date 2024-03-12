#!/usr/bin/python3
"""
\brief 将vfsstat工具采集到的信息进行规范化处理，处理后数据为prometheus格式数据
"""
import os.path
import subprocess

from prometheus_client import start_http_server

# https://prometheus.github.io/client_python/getting-started/three-step-demo/

"""
    读取命令行参数，以sudo权限启动
    捕获后端工具的数据，规范化存储
"""

"""
TIME         READ/s  WRITE/s  FSYNC/s   OPEN/s CREATE/s
16:38:39:        86       41        0        3        0
16:38:40:        50       31        0        3        0
16:38:41:        28       23        0        0        0
16:38:42:       181       93        0       28        0
16:38:43:       197      166        0       10        0
16:38:44:       392      351        0       11        0
16:38:45:       234      209        0        0        0

"""
proc_path = os.path.curdir + "../backend/fs"
proc_name = "vfsstat"

output = subprocess.check_output('sudo /home/fzy/Downloads/04_bcc_ebpf/MagicEyes/src/bridge/prometheus_client/examples/vfsstat', shell=True, text=True)
# output = subprocess.run(, shell=True, text=True)

# output = subprocess.Popen("sudo", "/home/fzy/Downloads/04_bcc_ebpf/MagicEyes/src/bridge/prometheus_client/examples/vfsstat", shell=True, text=True)

output_list = output.split('\n')

print(output)



