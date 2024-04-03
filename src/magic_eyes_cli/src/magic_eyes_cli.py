#!/usr/bin/env python3
"""
copyright(c) 2024 by MagicEyes, all rights reserved.
\brief
    Entry point for MagicEyes commandline tools
"""
__author__  = "MagicEyes coder"
__version__ = " 0.0.1 "

import os
import sys
if sys.version_info < (3,8):
    sys.exit('This program requires Python version 3.8 or newer!')


from core.cli_arg_parser import CliArgParser

sys.path.append(os.path.dirname(os.path.abspath('.')))

if __name__ == '__main__':
    # parse command line arguments
    if sys.argv[1] == "-h" or sys.argv[1] == "--help":
        add_help = True
    else:
        add_help = False
    cli_arg_parser = CliArgParser(is_add_help=add_help)
    # sys.argv[1:]
    args = cli_arg_parser.parse_args(sys.argv[1:])

    if hasattr(args, 'func'):
        args.func()
    else:
        print("TODO:")