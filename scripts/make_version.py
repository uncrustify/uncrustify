#!/usr/bin/env python
#
# Rebuilds the version using git describe
#
from sys import exit
from subprocess import check_output, check_call
from os.path import join, dirname, abspath, exists
from os import EX_OK, EX_USAGE, EX_IOERR


def main():
    root = dirname(dirname(abspath(__file__)))
    git_path = join(root, '.git')
    hg_path = join(root, '.hg')

    if exists(git_path):
        try:
            txt = check_output(['git', 'describe', '--long', '--always', '--dirty']).strip()
        except:
            print("Failed to retrieve version from git")
            return EX_IOERR
    elif exists(hg_path):
        try:
            check_call(['hg', 'gexport'])
            node = check_output(['hg', '--config', 'defaults.log=', 'log', '-r', '.', '--template', '{gitnode}']).strip()
            txt = check_output(['git', '--git-dir=.hg/git', 'describe', '--long', '--tags', '--always', node]).strip()
        except:
            print("Failed to retrieve version from hg")
            return EX_IOERR
    else:
        print("Unknown version control system.")
        return EX_USAGE

    # convert the git describe text to a version
    pts = txt.decode("ascii").split('-', 2)
    print("full version: %s.%s" % (pts[1], pts[2]))
    print(" tag version: %s" % pts[1])

    return EX_OK

if __name__ == '__main__':
    exit(main())
