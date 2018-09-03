#!/usr/bin/env python
#
# Reads tests from the .test files on the command line (or the built-in set)
# and runs them, or writes a CTest script to run them.
#
# * @author  Ben Gardner        October 2009
# * @author  Guy Maurel         October 2015
# * @author  Matthew Woehlke    June 2018
#

import argparse
import os
import sys

import test_uncrustify as tu


# -----------------------------------------------------------------------------
def main(argv):
    parser = argparse.ArgumentParser(description='Run uncrustify format tests')
    tu.add_format_tests_arguments(parser)
    args = tu.parse_args(parser)

    # Read tests
    tests = []
    print('Tests: {!s}'.format(args.tests))
    for group in args.tests:
        tests_file = os.path.join(tu.test_dir, '{}.test'.format(group))
        tests += tu.read_format_tests(tests_file, group)

    if args.write_ctest:
        tu.config.python_exe = args.python
        tu.config.uncrustify_exe = tu.fixup_ctest_path(
            tu.config.uncrustify_exe, args.cmake_config)

        with open(args.write_ctest, 'wt') as f:
            for test in tests:
                test.print_as_ctest(f)

    else:
        if args.select:
            s = tu.Selector(args.select)
        else:
            s = None

        counts = tu.run_tests(tests, args, s)
        tu.report(counts)

        if counts['failing'] > 0:
            sys.exit(2)
        if counts['mismatch'] > 0 or counts['unstable'] > 0:
            sys.exit(1)


# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if __name__ == '__main__':
    sys.exit(main(sys.argv))
