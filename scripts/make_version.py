#!/usr/bin/env python
#
# Rebuilds the version using git describe
#
from sys import exit
from subprocess import Popen, check_call, PIPE
from os.path import join, dirname, abspath, exists
from os import name as os_name
from sys import argv, exit
import re

if os_name == 'nt':
    EX_OK = 0
    EX_USAGE = 64
    EX_IOERR = 74
else:
    from os import EX_IOERR, EX_OK, EX_USAGE

def main(args):
    root = dirname(dirname(abspath(__file__)))
    git_path = join(root, '.git')
    hg_path = join(root, '.hg')

    txt = ""
    error_txt= ""

    if exists(git_path):
        try:
            proc = Popen(['git', 'describe', '--always', '--dirty'], stdout=PIPE, stderr=PIPE, cwd=root)
            txt_b, error_txt_b = proc.communicate()
            txt = txt_b.decode("UTF-8").strip().lower()
            error_txt = "%d: %s" % (proc.returncode, error_txt_b.decode("UTF-8").strip().lower())
        except:
            print("Failed to retrieve version from git")
            exit(EX_IOERR)
    elif exists(hg_path):
        try:
            check_call(['hg', 'gexport'])
            proc0 = Popen(['hg', '--config', 'defaults.log=', 'log', '-r', '.', '--template', '{gitnode}'], stdout=PIPE, stderr=PIPE, cwd=root)
            node_b, error_txt_b = proc0.communicate()
            node = node_b.decode("UTF-8")
            error_txt = "%d: %s" % (proc0.returncode, error_txt_b.decode("UTF-8").strip().lower())

            proc1 = Popen(['git', '--git-dir=.hg/git', 'describe', '--long', '--tags', '--always', node], stdout=PIPE, stderr=PIPE, cwd=root)
            txt_b, error_txt_b = proc1.communicate()
            txt = txt_b.decode("UTF-8").lower()
            error_txt += ", %d: %s" % (proc1.returncode, error_txt_b.decode("UTF-8").strip().lower())
        except:
            print("Failed to retrieve version from hg")
            exit(EX_IOERR)
    else:
        print("Unknown version control system in '%s'." % root)
        exit(EX_USAGE)

    version_pattern = re.compile(r"""
        ^
        (                      #1: full match
            uncrustify-
            (\d+\.\d+(\.\d+)?) #2: version 0.64.2 (,#3 optional 3rd nr)
            (                  #4: additional version info (long string format)
                -(\d+)         #5: tag commit distance
                -g(\w{7,})     #g-prefix + #6: commithash
            )?
        |
            (\w{7,})           #7: commithash only format (last N commits pulled and no tag available)
        )
        (-(dirty))?            #9: optional dirty specifier (#8,)
        $
    """, re.X)
    r_match = version_pattern.match(txt)

    if r_match is None:
        print("Regex version match failed on: '%s' (%s)" % (txt, error_txt))
        exit(EX_IOERR)

    if r_match.group(2) is not None:
        string_groups = [r_match.group(2)]
        if r_match.group(5) is not None and r_match.group(6) is not None:
            string_groups.append(r_match.group(5))
            string_groups.append(r_match.group(6))
    else:
        string_groups = [r_match.group(7)]

    if r_match.group(9) is not None:
        string_groups.append(r_match.group(9))


    for g in string_groups:
        if g is None:
            print("Unexpected empty regex group")
            exit(EX_IOERR)

    print("%s" % "-".join(string_groups))
    return EX_OK


if __name__ == "__main__":
    main(argv[1:])
