#!/usr/bin/env python

import argparse
import git  # /usr/lib/python3/dist-packages/git/__init__.py
import os
import re  # Support for regular expressions (RE).
import sys

# use the variable DEBUG to get more output
DEBUG = 1

if sys.version_info[0] < 3:
    input = raw_input

re_desc = re.compile(r'^uncrustify-([0-9]+[.][0-9]+[.][0-9]+)')
re_branch = re.compile(r'^uncrustify-RC-([0-9]+[.][0-9]+[.][0-9]+)')
re_merge = re.compile(r'^Merge pull request #[0-9]+ from [^/]+/(.*)')
re_version = re.compile(r'^[0-9]+[.][0-9]+[.][0-9]+$')
re_option_count = re.compile(r'There are currently ([0-9]+) options')


# -----------------------------------------------------------------------------
def fatal(msg):
    raise Exception(msg)


# -----------------------------------------------------------------------------
def get_version_str(repo, candidate=True, required=True):
    if candidate:
        if DEBUG:
            print('git.symbolic-ref("-q", "--short", "HEAD")')
        b = repo.git.symbolic_ref('-q', '--short', 'HEAD')
        m = re_branch.match(b)
        if m:
            return m.group(1)

    if DEBUG:
        print('git.describe("HEAD")')

    d = repo.git.describe('HEAD')
    m = re_desc.match(d)
    if m:
        return m.group(1)

    if required:
        fatal('Unable to determine current version')

    return None


# -----------------------------------------------------------------------------
def get_version_info(repo, candidate=True, required=True):
    s = get_version_str(repo, candidate, required)
    return tuple(map(int, s.split('.')))


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
    print('Updated: {}'.format(path))


# -----------------------------------------------------------------------------
def generate(repo, version, path, *args):
    import subprocess

    p = os.path.join(repo.working_tree_dir, path)
    with open(p, 'w') as f:
        c = subprocess.check_call(args, stdout=f)
    print('Created: {}'.format(path))

    alter(repo, path,
          r'Uncrustify-[0-9.]+(-[0-9]+-[0-9a-f]+(-dirty)?)?',
          r'Uncrustify-{}'.format(version))


# -----------------------------------------------------------------------------
def cmd_init(repo, args):
    v = args.version
    if v is None:
        c = get_version_info(repo, candidate=False, required=False)
        if c:
            n = '.'.join(map(str, (c[0], c[1] + 1, 0)))
            v = input('Version to be created? [{}] '.format(n))
            if len(v) == 0:
                v = n

        else:
            v = input('Version to be created? ')

    if DEBUG:
        print('The value for version is', v)

    if not re_version.match(v):
        fatal('Bad version number, \'{}\''.format(v))
    if DEBUG:
        s = 'git.checkout(-b, "uncrustify-RC-{}")'.format(v)
        print(s)

    repo.git.checkout('-b', 'uncrustify-RC-{}'.format(v))


# -----------------------------------------------------------------------------
def cmd_update(repo, args):
    v = get_version_str(repo)
    c = get_option_count(args.executable)

    alter(repo, 'CMakeLists.txt',
          r'(set *[(] *UNCRUSTIFY_VERSION +")[0-9.]+',
          r'\g<1>{}'.format(v))
    alter(repo, 'package.json',
          r'("version" *): *"[0-9.]+"',
          r'\g<1>: "{}"'.format(v))
    alter(repo, 'README.md',
          r'[0-9]+ configurable options as of version [0-9.]+',
          r'{} configurable options as of version {}'.format(c, v))
    alter(repo, 'documentation/htdocs/index.html',
          r'[0-9]+ configurable options as of version [0-9.]+',
          r'{} configurable options as of version {}'.format(c, v))

    generate(repo, v, 'etc/defaults.cfg',
             args.executable, '--show-config')
    generate(repo, v, 'documentation/htdocs/default.cfg',
             args.executable, '--show-config')
    generate(repo, v, 'documentation/htdocs/config.txt',
             args.executable, '--show-config')
    generate(repo, v, 'etc/uigui_uncrustify.ini',
             args.executable, '--universalindent')


# -----------------------------------------------------------------------------
def cmd_commit(repo, args):
    v = get_version_str(repo)
    message = 'Prepare Uncrustify v{} release'.format(v)

    extra_args = []
    if args.amend:
        extra_args += ['--amend', '--date=now']

    repo.git.commit('-m', message, *extra_args)


# -----------------------------------------------------------------------------
def cmd_tag(repo, args):
    import uuid

    # user_name = repo.config_reader().get_value('user', 'name')
    url = repo.config_reader().get_value('remote "origin"', 'url')
    print('The user account is', url)
    print('you need an "admin" account')
    v = input('Do you really want to use this account? (yes/no) ')
    if len(v) == 0:
        fatal("0 End")
    else:
        print(v)
        if v != "yes":
            fatal("2 End")

    # Determine location of remote repository
    if args.ssh:
        s = 'git@{}:'.format(args.server)
    else:
        s = 'https://{}/'.format(args.server)
    r = '{}{}/{}.git'.format(s, args.organization, args.project)

    # Fetch upstream
    u = repo.create_remote(str(uuid.uuid4()), r)
    try:
        u.fetch(refspec='master')

        # Get log
        if hasattr(args, 'commit'):
            c = repo.commit(args.commit)
        else:
            c = repo.commit('{}/master'.format(u.name))
        m = re_merge.match(c.message.split('\n')[0])
        if m is None:
            fatal('Last commit is not a merge of a release candidate?')

        m = re_branch.match(m.group(1))
        if m is None:
            fatal('Failed to extract version from release candidate merge')
        v = m.group(1)

        # Create and push tag
        extra_args = {}
        if args.force:
            extra_args['force_with_lease'] = True

        tag = 'uncrustify-{}'.format(v)
        message = 'Create Uncrustify v{} release'.format(v)
        repo.git.tag('-a', tag, c, '-m', message, '--force')
        u.push(refspec=tag, **extra_args)

    finally:
        repo.delete_remote(u)


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
    parser_update.set_defaults(func=cmd_update, keepversion=False)
    parser_update.add_argument('executable',
                               help='path to uncrustify executable')

    parser_commit = subparsers.add_parser(
        'commit', help='commit changes for new version')
    parser_commit.set_defaults(func=cmd_commit)
    parser_commit.add_argument('-a', '--amend', action='store_true',
                               help='amend a previous release commit')

    parser_tag = subparsers.add_parser(
        'tag', help='tag release and push tag to github')
    parser_tag.set_defaults(func=cmd_tag)
    parser_tag.add_argument('--ssh', action='store_true',
                            help='use ssh (instead of HTTPS) to push')
    parser_tag.add_argument('-s', '--server', default='github.com',
                            help='push to specified server')
    parser_tag.add_argument('-o', '--organization', default='uncrustify',
                            help='push to specified user or organization')
    parser_tag.add_argument('-p', '--project', default='uncrustify',
                            help='push to specified project')
    parser_tag.add_argument('-c', '--commit',
                            help='tag specified commit '
                                 '(instead of latest \'master\')')
    parser_tag.add_argument('-f', '--force', action='store_true',
                            help='force push the tag')

    args = parser.parse_args()
    repo = git.Repo(args.repo)
    args.func(repo, args)

    return 0


# %%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%

if __name__ == '__main__':
    sys.exit(main())
