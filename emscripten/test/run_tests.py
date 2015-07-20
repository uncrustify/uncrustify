#!/usr/bin/env python
#
# Rebuilds the version using git describe
#
import sys
from subprocess import Popen
from os import listdir, EX_OK, EX_USAGE, EX_SOFTWARE
from os.path import isfile, isdir, abspath, basename
from tempfile import NamedTemporaryFile
from glob import glob


def main(args):
    if len(args) < 2 or not isfile(args[0]) or not isdir(args[1]):
        print("Usage:")
        print("   arg 1: libUncrustify.js file path")
        print("   arg 2: test directory path")
        return EX_USAGE

    c_red = '\033[31m'
    c_green = '\033[32m'
    c_end = '\033[0m'

    js_file_path = args[0]
    passed = 0
    total = 0

    test_files_dir = abspath(args[1])
    test_files = glob(test_files_dir+"/test_*.js")
    temp_file = NamedTemporaryFile(delete=True)

    for test_file_path in test_files:
        total += 1
        pt_strg = "Testing %s: " % basename(test_file_path)
        pt_strg_len = len(pt_strg)

        sys.stdout.write(pt_strg)

        with open(temp_file.name, 'r+') as t:
            process = Popen(["node", test_file_path, js_file_path], stderr=t, stdout=t)
            process.wait()

            if process.returncode == 0:
                print(("%spassed.%s" % (c_green, c_end)).rjust(86-pt_strg_len))
                passed += 1
            else:
                print(("%sfailed!%s" % (c_red, c_end)).rjust(78-pt_strg_len))

                t.seek(0)
                text = t.read()
                print(text)

    if total == 0:
        print("%sError%s: no test files found in %s" % (c_red, c_end, test_files_dir))
        return EX_USAGE

    print('-' * 80)

    if passed == total:
        print("%sAll %s tests passed%s" % (c_green, total, c_end))
        return EX_OK
    else:
        print("%sWarning%s: %s/%s tests passed" % (c_red, c_end, passed, total))
        return EX_SOFTWARE

if __name__ == '__main__':
    exit(main(sys.argv[1:]))
