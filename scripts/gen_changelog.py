#!/usr/bin/env python

'''
This script attempts to extract what options have been added since the
specified revision (usually a tag, but any revision that git recognizes may be
provided). It accepts an optional second revision to use as the cut-off. The
default is your LOCAL "master". Thus, you should ensure that this is up to date
before running this script.

.. caution::

   This script functions by making a bunch of assumptions that added options
   will cause specific patterns to appear in patches. This logic is likely
   susceptible to edge cases and may well miss additions or generate false
   positives. (In particular, options that are not new but have been relocated
   will likely show up.) It is ***strongly recommended*** that the output of
   this script be independently verified by inspecting the mentioned commits
   and comparing the contents of ``options.h`` at the relevant revisions.
'''

import argparse
import git
import os
import re
import sys
import time

re_option = re.compile(r'[+]extern (Bounded)?Option<[^>]+>')


# =============================================================================
class Changeset(object):
    # -------------------------------------------------------------------------
    def __init__(self, repo, sha):
        self.sha = sha
        self.options = []

        commit = repo.commit(sha)
        ad = time.gmtime(commit.authored_date)
        self.date = time.strftime('%Y-%m-%d', ad)

        info = repo.git.log('-1', '--raw', '--abbrev=40', '--pretty=',
                            sha, '--', ':src/options.h').split(' ')
        if len(info) < 5:
            return

        diff = repo.git.diff('-U0', info[2], info[3]).split('\n')
        next_is_option = False
        for line in diff:
            if re_option.match(line):
                next_is_option = True
            elif next_is_option:
                if line[0] == '+':
                    self.options.append(line[1:].split(';')[0])
                next_is_option = False


# -----------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description='Generate changelog for new options')

    root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    parser.add_argument('--repo', type=str, default=root,
                        help='Path to uncrustify git repository')
    parser.add_argument('since', type=str,
                        help='Revision (tag) of previous uncrustify version')
    parser.add_argument('until', type=str, default='master', nargs='?',
                        help='Revision (tag) of next uncrustify version')

    args = parser.parse_args()
    repo = git.Repo(args.repo)
    revs = repo.git.log('--pretty=%H', '--reverse',
                        '{}..{}'.format(args.since, args.until),
                        '--', ':src/options.h').split('\n')

    if revs == ['']:
        print('No changes were found')
        return 1

    new_options = set()
    changes = []
    for r in revs:
        relevant = False
        c = Changeset(repo, r)
        for o in c.options:
            if o not in new_options:
                new_options.add(o)
                relevant = True

        if relevant:
            changes.append(c)

    for c in changes:
        print('{} ({})'.format(c.date, c.sha))
        for o in c.options:
            print('  {}'.format(o))

    return 0


# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if __name__ == '__main__':
    sys.exit(main())
