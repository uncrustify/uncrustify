#!/usr/bin/env python
from sys import exit as sys_exit, stdin
from os import path
import re
import argparse

def parse_test(data):
    """parses a test file data into a simple python list of dictionaries"""
    lines = [line.strip() for line in data.splitlines()]
    tests = [{'id'    : int(m.group(2)),
              'config': m.group(3),
              'input' : m.group(4)}
             for m in [re.match(r'\s*(#|!)?(\d+)\s+(\S+)\s+(\S+)$', line) for line in lines]
             if m and not m.group(1)]

    return tests


def main():
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument('--lang', type=str, help="Language, (CPP, CS)", default='CPP')
    arg_parser.add_argument('--id', type=int, help="bug number", default=99999)

    args = arg_parser.parse_args()

    ext = args.lang.lower()
    if ext == 'cpp':
        cfg = "Cpp"
    elif ext == 'cs':
        cfg = "CSharp"

    root_dir = path.dirname(path.dirname(path.abspath(__file__)))
    test_dir = path.join(root_dir, "tests")
    config_dir = path.join(test_dir, "config")
    input_dir = path.join(test_dir, "input")
    output_dir = path.join(test_dir, "output")

    staging_test_file = path.join(test_dir, "staging.test")
    staging_config_dir = path.join(config_dir, "staging")
    staging_input_dir = path.join(input_dir, "staging")
    staging_output_dir = path.join(output_dir, "staging")

    tests = None
    with open(staging_test_file, 'r') as f:
        test_data = f.read()
        tests = parse_test(test_data)

    max_test = max(tests, key=lambda x: x['id'])
    new_id = max_test['id'] + 1

    file_name = "UNI-%d.%s" % (args.id, ext)
    config = "staging/Uncrustify.%s.cfg" % (cfg)
    input = "staging/" + file_name
    test = "%d %s %s" % (new_id, config, input)

    input_file_path = path.join(staging_input_dir, file_name)

    input_data = stdin.read()
    with open(input_file_path, 'w') as f:
        f.write(input_data)

    output_file_path = path.join(staging_output_dir, "%s-%s" % (new_id, file_name))
    with open(output_file_path, 'w') as f:
        f.write(input_data)

    with open(staging_test_file, 'a') as f:
        f.write("%s\n" % test)

    print("Test %d was added to staging test\n" % (new_id))

if __name__ == "__main__":
    sys_exit(main())
