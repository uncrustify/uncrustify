#!/usr/bin/env python

import argparse
import math
import os
import subprocess
import sys

from multiprocessing import cpu_count

default_jobs = min(cpu_count() + 2, cpu_count() * 2)

# -----------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description='Run CTest')
    parser.add_argument('-q', '--quiet', action='store_true',
                        help='suppress output of failing tests')
    parser.add_argument('-j', '--parallel', type=int, default=default_jobs,
                        help='number of jobs to use for parallel execution')
    parser.add_argument('args', metavar='ARGS', nargs='*', default=[],
                        help='additional arguments to pass to CTest')
    args = parser.parse_args()

    if not os.path.exists('CTestTestfile.cmake'):
        print('No test configuration file found!')
        print('(Note: This script must be run from your build directory.)')
        sys.exit(-1)

    cmd = ['ctest', '-j{}'.format(args.parallel)]
    if not args.quiet:
        cmd.append('--output-on-failure')
    cmd += args.args

    try:
        subprocess.check_call(cmd)
    except subprocess.CalledProcessError as exc:
        sys.exit(exc.returncode)


# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if __name__ == '__main__':
    main()
