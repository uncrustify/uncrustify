# Global state.
#
# * @author  Ben Gardner        October 2009
# * @author  Guy Maurel         October 2015
# * @author  Matthew Woehlke    June 2018
#

import os

test_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

all_tests = [
    'c-sharp',
    'c',
    'cpp',
    'd',
    'java',
    'pawn',
    'objective-c',
    'vala',
    'ecma',
    'imported',
    'staging'
]

FAIL_ATTRS     = {'bold': True}
PASS_ATTRS     = {'fore': 2}  # Green
MISMATCH_ATTRS = {'fore': 1}  # Red
UNSTABLE_ATTRS = {'fore': 6}  # Cyan
SKIP_ATTRS     = {'fore': 3}  # Yellow


# =============================================================================
class config(object):
    uncrustify_exe = None
    python_exe = None
    git_exe = 'git'
