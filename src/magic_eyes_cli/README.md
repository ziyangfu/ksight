后端命令行工具

- 统一后端工具使用，唯一命令行前端
- 采用类似ip子命令模式，具有命令行补全功能

  - 采用argcomplete？readline？
- 安装在 <install_dir>/sbin下


1. argparse中的-h参数拦截
2. 固定的参数部分
3. 动态的参数部分
   1. 扫描后端工具文件夹，读取文件夹名与文件名
      1. 每次运行均扫描一次，或者将扫描结果保存在一个cache文件中
4. 若是动态参数，即使用工具，则使用subprocess创建一个子进程，将命令参数传递出去
5. 命令自动补全

```bash
# 1. 列出所有可用的工具
MagicEyes list
MagicEyes check
MagicEyes help
#2. 自动补全
MagicEyes cpu cpu_watcher -h
# <--------------自动补全 | 非自动补全
```


```bash
# 生成requirements.txt
pip3 freeze > requirements.txt
#  安装
pip3 install -r requiredments.txt
```
