#!/bin/python
from __future__ import print_function  # python >= 2.6
import glob

from os import chdir, listdir, makedirs, path, name as os_name, \
               linesep as os_linesep, sep as os_sep
from subprocess import Popen, PIPE, call
from sys import exit as sys_exit, stderr
from pprint import pprint
from threading import Timer


def term_proc(proc, timeout):
    timeout["value"] = True
    proc.terminate()


def proc_output(args, timeout_sec=None):
    proc = Popen(args, stdout=PIPE, stderr=PIPE)

    timeout = {"value": False}
    if timeout_sec is not None:
        timeout = {"value": False}
        timer = Timer(timeout_sec, term_proc, [proc, timeout])
        timer.start()

    output_b, error_txt_b = proc.communicate()

    if timeout_sec is not None:
        timer.cancel()

    output = output_b.decode("UTF-8")
    error = error_txt_b.decode("UTF-8")

    if error is not None and len(error) > 0:
        print("proc error: %s" % error, file=stderr)

    if timeout["value"]:
        print("proc timeout: %s" % ' '.join(args), file=stderr)

    return output if proc.returncode == 0 and not timeout["value"] else None


def main():
    root_dir = path.dirname(path.dirname(path.abspath(__file__)))
    test_dir = path.join(root_dir, "tests")
    config_dir = path.join(test_dir, "config")
    input_dir = path.join(test_dir, "input")
    output_dir = path.join(test_dir, "output")

    chdir(test_dir)

    files = glob.glob('./*.test')

    config_map = {}

    for test_file in files:
        rg_string = proc_output(["grep", "-P", ".cfg", test_file])
        if rg_string is None:
            print("error: %s" % test_file, file=stderr)
            continue

        lines = rg_string.splitlines()
        for line in lines:
            line = line.strip()
            splits = line.split()
            split_len = len(splits)

            idx = splits[0]
            if idx[-1] == '!':
                idx = idx[:-1]

            cfg = splits[1]
            file_path = splits[2]
            lang = None if split_len < 4 else splits[3]

            if line[0] == '#':
                continue

            lang_dir, file = file_path.split('/', 1)

            if cfg not in config_map:
                config_map[cfg] = [(idx, file, lang_dir, lang)]
            else:
                config_map[cfg].append((idx, file, lang_dir, lang))

    for key, value_list in config_map.items():
        config_file_path = path.join(config_dir, key)
        args = ["python", "../scripts/option_reducer.py", '-q',
                '-b', "../build/Debug/uncrustify",
                "-c", config_file_path]

        for value_pair in value_list:
            input_file_path = path.join(input_dir, "%s/%s"
                                        % (value_pair[2], value_pair[1]))
            args.append("-i")
            args.append(input_file_path)

            output_file_path = path.join(output_dir, "%s/%s-%s"
                                         % (value_pair[2], value_pair[0],
                                            value_pair[1]))
            args.append("-f")
            args.append(output_file_path)

            if len(value_pair) > 4:
                args.append("-l")
                args.append(value_pair[3])

        print(".", end='', flush=True)
        out = proc_output(args)
        # out = None
        if out is None:
            continue

        with open(config_file_path, 'w') as f:
            f.write(out)

    return 0


if __name__ == "__main__":
    sys_exit(main())