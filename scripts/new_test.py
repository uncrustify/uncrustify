#!/usr/bin/env python
from sys import exit as sys_exit, stdin
from os import path
import re

def parse_test(data):
    lines = [ line.strip() for line in data.splitlines() ]
    
    tests = [ 
        {
            # 'prefix': m.group(1),
            'id': m.group(2),
            'config': m.group(3),
            'input': m.group(4),
            'output': m.group(5)
        } for m in [ re.match(line, r'\s*(#|!)?(\d)\s+(\S+)\s+(\S+)$') for line in lines ] if m and not m.group(1) ]

    print tests


def main():
    root_dir = path.dirname(path.dirname(path.abspath(__file__)))
    test_dir = path.join(root_dir, "tests")
    config_dir = path.join(test_dir, "config")
    input_dir = path.join(test_dir, "input")
    output_dir = path.join(test_dir, "output")

    staging_test_file = path.join(test_dir, "staging.test")
    staging_config_dir = path.join(config_dir, "staging")
    staging_input_dir = path.join(input_dir, "staging")
    staging_output_dir = path.join(output_dir, "staging")


    # source = [ x.strip() for x in stdin.readlines() ]

    with open(staging_test_file, 'r') as f:
        test_data = f.read()
        parse_test(test_data)


if __name__ == "__main__":
    sys_exit(main())