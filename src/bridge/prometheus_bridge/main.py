#!/usr/bin/python3
"""
采集后端数据，并将数据打包成prometheus格式数据，并通过http发送给prometheus server
"""
__author__ = 'fzy'
__version__ = "0.0.1"

import http
import os.path
import time
import http
import prometheus_client

if not os.path.exists("kernel_data.db"):
    import db_table


def version():
    return "prometheus client version is : " + __version__


def get_system_time():
    current_sys_time = time.strftime("%Y-%m-%d %H:%M:%S", time.localtime())
    return current_sys_time

def sender():
    ip_address = '127.0.0.1'
    url = f'http://{ip_address}/metrics'

def main():
    sender()



if __name__ == '__main__':
    main()