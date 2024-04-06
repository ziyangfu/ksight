"""
    for argc parse
"""

import argparse
# import argcomplete  # TODO: 自动补全工具
import os
import multiprocessing
from utils.filesystem import print_all_tools
from utils.filesystem import get_all_tools


def process():
    print("process")
    # list all process tools
    pass

def memory():
    print("memory")
    pass

def fs():
    print("fs")
    pass

def net():
    print("net")
    pass

def system_diagnosis():
    print("system diagnosis")
    pass



class CliArgParser:
    """
    class of commandline argc praser
    """
    def __init__(self, is_add_help) -> None:
        """Initialize the parser """
        self._arg_parser = argparse.ArgumentParser(
            description=''' magic_eyes_cli: command tools for Linux kernel diagnosis and optimization ''',
            add_help=is_add_help,
            epilog='''eg: magic_eyes_cli list''')
        self._parsed_args = None
        self._setup_args()
        # argcomplete.autocomplete(self._arg_parser) #TODO

    def parse_args(self, args):
        """ Parse the given arguments and return them """
        self._parsed_args = self._arg_parser.parse_args(args)
        return self._parsed_args

    def _setup_args(self):
        # 通用参数部分，组内选项是互斥的  
        common_opts_group = self._arg_parser.add_argument_group("all of common options")
        comm_opts = common_opts_group.add_mutually_exclusive_group()
        comm_opts.add_argument(
            "-l", "--list", action='store_true',
            help=" list all avaliable tools ")
        comm_opts.add_argument(
            "-c", "--check", action='store_true',
            help="check all tools' dependency, and whether it can be run in current platform"
        )


        subparser = self._arg_parser.add_subparsers(dest='command')
        
        list_subParser = subparser.add_parser(
            'list',
            help=" list all avaliable tools "
        )
        list_subParser.set_defaults(func=print_all_tools)

        # 获取subsystem以及下属的工具清单
        tools_lists = get_all_tools()




        # subsystem部分
        process_parser = subparser.add_parser(
            'process', 
            help="Linux process subsystem"
        )
        memory_subparser = subparser.add_parser(
            'memory',
            help="Linux memory subsystem"
        )
        memory_subparser.set_defaults(func=memory)

        fs_subparser = subparser.add_parser(
            'fs',
            help="Linux fs subsystem"
        )
        fs_subparser.set_defaults(func=fs)

        net_subparser = subparser.add_parser(
            'net',
            help="Linux memory subsystem"
        )
        net_subparser.set_defaults(func=net)

        # subsystem下tool部分
        
        for subsystem, tools in tools_lists:
            for tool in tools:


                if subsystem == "process":
                    # process
                    process_subparser = process_parser.add_subparsers(dest='command')
                    proc_image = process_subparser.add_parser(
                        '{tool}',
                        help="process subsystem tool"
                    )




       



    def exit(self, status=os.EX_OK, message=None):
        self._arg_parser.exit(status=status, message=message)


def cpu_num():
    try:
        num = multiprocessing.cpu_count()
    except BaseException:
        num = 1
    return int(num)