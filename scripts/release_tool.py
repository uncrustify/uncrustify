#!/usr/bin/env python

import argparse
import git
import os
import re
import sys

if sys.version_info[0] < 3:
    input = raw_input

re_desc = re.compile(r'^uncrustify[-]([0-9]+[.][0-9]+[.][0-9]+)')
re_version = re.compile(r'^[0-9]+[.][0-9]+[.][0-9]+$')
re_option_count = re.compile(r'There are currently ([0-9]+) options')


# -----------------------------------------------------------------------------
def fatal(msg, code=1):
    sys.stderr.write(msg + '\n')
    sys.exit(code)


# -----------------------------------------------------------------------------
def get_version_str(repo, required=True):
    d = repo.git.describe('HEAD')
    m = re_desc.match(d)
    if m:
        return m.group(1)

    if required:
        fatal('Unable to determine current version')

    return None


# -----------------------------------------------------------------------------
def get_version_info(repo, required=True):
    return tuple(map(int, get_version_str(repo, required).split('.')))


# -----------------------------------------------------------------------------
def get_option_count(executable):
    import subprocess

    out = subprocess.check_output([executable, '--count-options'])
    m = re_option_count.match(out.decode('utf-8'))
    if m is None:
        fatal('Failed to get option count from \'{}\''.format(executable))

    return int(m.group(1))


# -----------------------------------------------------------------------------
def alter(repo, path, old, new):
    p = os.path.join(repo.working_tree_dir, path)
    with open(p, 'r') as f:
        content = f.read()
        content = re.sub(old, new, content)
    with open(p, 'w') as f:
        f.write(content)
    print(path)


# -----------------------------------------------------------------------------
def generate(repo, path, *args):
    import subprocess

    p = os.path.join(repo.working_tree_dir, path)
    with open(p, 'w') as f:
        c = subprocess.check_call(args, stdout=f)
    print(path)


# -----------------------------------------------------------------------------
def cmd_init(repo, args):
    v = args.version
    if v is None:
        c = get_version_info(repo, required=False)
        if c:
            n = '.'.join(map(str, (c[0], c[1] + 1, 0)))
            v = input('Version to be created? [{}] '.format(n))
            if len(v) == 0:
                v = n

        else:
            v = input('Version to be created? ')

    if not re_version.match(v):
        fatal('Bad version number, \'{}\''.format(v))

    tag_message = 'Prepare Uncrustify v{} release'.format(v)
    repo.git.tag('-a', 'uncrustify-{}'.format(v), '-m', tag_message)


# -----------------------------------------------------------------------------
def cmd_update(repo, args):
    v = get_version_str(repo)
    c = get_option_count(args.executable)

    alter(repo, 'CMakeLists.txt',
          r'(set *[(] *CURRENT_VERSION +"Uncrustify)[-][0-9.]+',
          r'\g<1>-{}'.format(v))
    alter(repo, 'package.json',
          r'("version" *): *"[0-9.]+"',
          r'\g<1>: "{}"'.format(v))
    alter(repo, 'README.md',
          r'[0-9]+ configurable options as of version [0-9.]+',
          r'{} configurable options as of version {}'.format(c, v))
    alter(repo, 'documentation/htdocs/index.html',
          r'[0-9]+ configurable options as of version [0-9.]+',
          r'{} configurable options as of version {}'.format(c, v))

    generate(repo, 'etc/defaults.cfg',
             args.executable, '--show-config')
    generate(repo, 'documentation/htdocs/default.cfg',
             args.executable, '--show-config')
    generate(repo, 'documentation/htdocs/config.txt',
             args.executable, '--show-config')
    generate(repo, 'etc/uigui_uncrustify.ini',
             args.executable, '--universalindent')


# -----------------------------------------------------------------------------
def cmd_commit(repo, args):
    v = get_version_str(repo)
    message = 'Create Uncrustify v{} release'.format(v)

    extra_args = []
    if args.amend:
        extra_args += ['--amend', '--date=now']

    repo.git.commit('-m', message, *extra_args)
    repo.git.tag('-a', 'uncrustify-{}'.format(v), '-m', message, '--force')


# -----------------------------------------------------------------------------
def cmd_push(repo, args):
    v = get_version_str(repo)
    tag = 'uncrustify-{}'.format(v)

    if args.ssh:
        s = 'git@{}:'.format(args.server)
    else:
        s = 'https://{}/'.format(args.server)
    r = '{}{}/{}.git'.format(s, args.organization, args.project)

    extra_args = []
    if args.force:
        extra_args.append('--force-with-lease')

    repo.git.push(r, 'HEAD:master', tag, *extra_args)


# -----------------------------------------------------------------------------
def main():
    parser = argparse.ArgumentParser(
        description='Perform release-related actions')

    root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
    parser.add_argument('--repo', type=str, default=root,
                        help='path to uncrustify git repository')

    subparsers = parser.add_subparsers(title='subcommands',
                                       help='action to perform')

    parser_init = subparsers.add_parser(
        'init', help='initialize new version')
    parser_init.set_defaults(func=cmd_init)
    parser_init.add_argument('-v', '--version',
                             help='version number for release')

    parser_update = subparsers.add_parser(
        'update', help='update version information')
    parser_update.set_defaults(func=cmd_update)
    parser_update.add_argument('executable',
                               help='path to uncrustify executable')

    parser_commit = subparsers.add_parser(
        'commit', help='commit changes for new version')
    parser_commit.set_defaults(func=cmd_commit)
    parser_commit.add_argument('--amend', action='store_true',
                               help='amend a previous release commit')

    parser_push = subparsers.add_parser(
        'push', help='push release to github')
    parser_push.set_defaults(func=cmd_push)
    parser_push.add_argument('--ssh', action='store_true',
                             help='use ssh (instead of HTTPS) to push')
    parser_push.add_argument('-s', '--server', default='github.com',
                             help='push to specified server')
    parser_push.add_argument('-o', '--organization', default='uncrustify',
                             help='push to specified user or organization')
    parser_push.add_argument('-p', '--project', default='uncrustify',
                             help='push to specified project')
    parser_push.add_argument('-f', '--force', action='store_true',
                             help='force push')

    args = parser.parse_args()
    repo = git.Repo(args.repo)
    args.func(repo, args)

    return 0


# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if __name__ == '__main__':
    sys.exit(main())
