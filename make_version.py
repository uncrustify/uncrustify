#!/usr/bin/env python
#
# Rebuilds the version using git describe
#
import sys
import os
import subprocess

def main (argv):
    root = os.path.abspath(os.path.dirname(__file__))
    git_path = os.path.join(root, '.git')
    hg_path = os.path.join(root, '.hg')

    if os.path.exists(git_path):
        try:
            txt = subprocess.check_output(['git', 'describe', '--long', '--always', '--dirty']).strip()
        except:
            print "Failed to retrieve version from git"
            return 1
    elif os.path.exists(hg_path):
        try:
            branch = subprocess.check_output(['hg', 'log', '-r', '.', '--template', '{latesttag}']).strip()
            txt = subprocess.check_output(['git', '--git-dir=.hg/git', 'describe', '--long', '--always', branch]).strip()
        except:
            print "Failed to retrieve version from hg"
            return 1
    else:
        print "Unknown version control system."
        return 1

    # convert the git describe text to a version
    pts = txt.split('-', 2)
    full_vers = '%s.%s' % (pts[1], pts[2])
    tag_vers = pts[1]
    print 'full version:', full_vers
    print ' tag version:', tag_vers

    # rebuild uncrustify_version.h
    fn = os.path.join(root, 'src', 'uncrustify_version.h.in')
    with open(fn, 'rb') as fh:
        data = fh.read()
    with open(fn[:-3], 'wb') as fh:
        fh.write(data.replace('@PACKAGE_VERSION@', full_vers))

    # rebuild configure.in
    fn = os.path.join(root, 'configure.ac')
    with open(fn, 'rb') as fh:
        data = fh.read()
    with open(fn, 'wb') as fh:
        for line in data.splitlines():
            if line.startswith('AC_INIT(uncrustify'):
                pts = line.split(',', 2)
                line = '%s,%s,%s' % (pts[0], tag_vers, pts[2])
            fh.write(line)
            fh.write('\n')
    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
