#!/usr/bin/env python
#
# Checks the formatting of uncrustify's own sources.
#
# * @author  Matthew Woehlke    June 2018
#

import argparse
import os
import sys

import test_uncrustify as tu


# -----------------------------------------------------------------------------
def main(argv):
    parser = argparse.ArgumentParser(description='Run uncrustify source tests')
    tu.add_source_tests_arguments(parser)
    args = tu.parse_args(parser)

    # Get required filesystem information
    root = os.path.dirname(tu.test_dir)
    src_dir = os.path.join(root, 'src')
    config = os.path.join(root, 'forUncrustifySources.cfg')

    # Create tests
    tests = []
    for s in os.listdir(src_dir):
        if os.path.splitext(s)[1] in ('.cpp', '.h'):
            t = tu.SourceTest()
            filepath = os.path.join(src_dir, s)
            t.build(test_input=filepath, test_lang='CPP', test_config=config,
                    test_expected=filepath)
            tests.append(t)

    counts = tu.run_tests(tests, args)
    tu.report(counts)

    if counts['failing'] > 0:
        sys.exit(2)
    if counts['mismatch'] > 0:
        sys.exit(1)


# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if __name__ == '__main__':
    sys.exit(main(sys.argv))
