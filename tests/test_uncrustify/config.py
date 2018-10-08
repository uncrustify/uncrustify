# Global state.
#
# * @author  Ben Gardner        October 2009
# * @author  Guy Maurel         October 2015
# * @author  Matthew Woehlke    June 2018
#

import os

test_dir = os.path.dirname(os.path.dirname(os.path.realpath(__file__)))

# sorted by lexicographic order
all_tests = [
    'c-sharp',
    'c',
    'cpp',
    'd',
    'ecma',
    'imported',
    'java',
    'objective-c',
    'pawn',
    'staging',
    'vala',
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
