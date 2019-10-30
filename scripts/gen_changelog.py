#!/usr/bin/env python

'''
This script attempts to extract what options have been added since the
specified revision (usually a tag, but any revision that git recognizes may be
provided). It accepts an optional second revision to use as the cut-off. The
default is your LOCAL "master". Thus, you should ensure that this is up to date
before running this script.

This script works by extracting the set of options before and after every
commit that affected ':src/options.h' and computing the differences. It should,
therefore, be fairly robust (for example, options that moved around won't show
up). However, if an option is removed and subsequently re-added, or if an
option was added and subsequently removed, the resulting records will need to
be reconciled manually.
'''

import argparse
import git
import os
import re
import sys
import time

re_option = re.compile(r'extern (Bounded)?Option<[^>]+>')


# -----------------------------------------------------------------------------
def extract_options(repo, blob_id):
    from git.util import hex_to_bin

    blob = git.Blob(repo, hex_to_bin(blob_id))
    content = blob.data_stream.stream
    options = set()

    for line in iter(content.readline, b''):
        line = line.decode('utf-8').strip()

        if re_option.match(line):
            line = content.readline().decode('utf-8').strip()
            options.add(line.split(';')[0])

    return options


# =============================================================================
class Changeset(object):
    # -------------------------------------------------------------------------
    def __init__(self, repo, sha):
        self.sha = sha
        self.added_options = set()
        self.removed_options = set()

        commit = repo.commit(sha)
        ad = time.gmtime(commit.authored_date)
        self.date = time.strftime('%b %d %Y', ad).replace(' 0', '  ')

        info = repo.git.log('-1', '--raw', '--abbrev=40', '--pretty=',
                            sha, '--', ':src/options.h').split(' ')
        if len(info) < 5:
            return

        old_options = extract_options(repo, info[2])
        new_options = extract_options(repo, info[3])
        self.added_options = new_options.difference(old_options)
        self.removed_options = old_options.difference(new_options)


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

    changes = []
    for r in revs:
        c = Changeset(repo, r)
        if len(c.added_options) or len(c.removed_options):
            changes.append(c)

    for c in changes:
        print(c.sha)
        for o in c.added_options:
            print('  Added   : {:36} {}'.format(o, c.date))
        for o in c.removed_options:
            print('  Removed : {:36} {}'.format(o, c.date))

    return 0


# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if __name__ == '__main__':
    sys.exit(main())
