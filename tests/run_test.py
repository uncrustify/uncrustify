#!/usr/bin/env python
#
# Runs a single test. Results are printed out.
#
# * @author  Matthew Woehlke    June 2018
#

import test_uncrustify as tu

import argparse
import sys


# -----------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(description='Run uncrustify test')
    tu.add_test_arguments(parser)
    args = tu.parse_args(parser)

    test = tu.FormatTest()
    test.build_from_args(args)

    try:
        test.run(args)
    except tu.Failure as f:
        sys.stderr.write('{}\n'.format(f))
        sys.exit(-1)


# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if __name__ == '__main__':
    main()
