#!/usr/bin/env python
"""This is a wrapper around the split function parser.
Note: must define SPLIT_FUNCTION_PARSER_PATH
Note: must define SPLIT_FUNCTION_PARSER_OUTPUT_FOLDER_PATH
"""

import sys
import os


def run_split_function_parser(args, file_path):
    args_string = ''
    for i in args:
        args_string = args_string + ' ' + i 
    split_function_parser = os.getenv('SPLIT_FUNCTION_PARSER_PATH')    
    os.system( split_function_parser + ' ' + args_string + ' ' + file_path )