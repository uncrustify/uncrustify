# Class encapsulating a unit test.
#
# * @author  Ben Gardner        October 2009
# * @author  Guy Maurel         October 2015
# * @author  Matthew Woehlke    June 2018
#

import filecmp
import os
import re
import subprocess
import sys
import errno

from .ansicolor import printc
from .config import (config, test_dir, FAIL_ATTRS, PASS_ATTRS,
                     MISMATCH_ATTRS, UNSTABLE_ATTRS)
from .failure import (ExecutionFailure, MismatchFailure, MissingFailure,
                      TestDeclarationParseError, UnexpectedlyPassingFailure,
                      UnstableFailure)


# =============================================================================
class SourceTest(object):
    # -------------------------------------------------------------------------
    def __init__(self):
        self.test_result_dir = 'results'

        self.diff_text = 'MISMATCH'
        self.diff_attrs = MISMATCH_ATTRS
        self.diff_exception = MismatchFailure

    # -------------------------------------------------------------------------
    def _check_attr(self, name):
        if not hasattr(self, name) or getattr(self, name) is None:
            raise AttributeError(
                'Test is missing required attribute {!r}'.format(name))

    # -------------------------------------------------------------------------
    def _make_abs(self, name, base):
        path = getattr(self, name)
        if not os.path.isabs(path):
            setattr(self, name, os.path.join(test_dir, base, path))

    # -------------------------------------------------------------------------
    def _diff(self, expected, actual):
        sys.stdout.flush()
        cmd = [config.git_exe, 'diff', '--no-index', expected, actual]
        subprocess.call(cmd)

    # -------------------------------------------------------------------------
    def build(self, test_input, test_lang, test_config, test_expected):
        self.test_name = os.path.basename(test_input)
        self.test_lang = test_lang
        self.test_input = test_input
        self.test_config = test_config
        self.test_expected = test_expected
        self.test_xfail = False

    # -------------------------------------------------------------------------
    def _check(self):
        self._check_attr('test_name')
        self._check_attr('test_lang')
        self._check_attr('test_input')
        self._check_attr('test_config')
        self._check_attr('test_expected')
        self._check_attr('test_xfail')

    # -------------------------------------------------------------------------
    def run(self, args):
        self._check()

        _expected = self.test_expected
        _result = os.path.join(args.result_dir, self.test_result_dir,
                               os.path.basename(os.path.dirname(_expected)),
                               os.path.basename(_expected))

        if args.verbose:
            print(self.test_name)
            print('  Language : {}'.format(self.test_lang))
            print('     Input : {}'.format(self.test_input))
            print('    Config : {}'.format(self.test_config))
            print('  Expected : {}'.format(_expected))
            print('    Result : {}'.format(_result))
            print('     XFail : {}'.format(self.test_xfail))

        if not os.path.exists(os.path.dirname(_result)):
            try:
                os.makedirs(os.path.dirname(_result))
            except OSError as e:
                if e.errno != errno.EEXIST:
                    raise

        cmd = [
            config.uncrustify_exe,
            '-q',
            '-l', self.test_lang,
            '-c', self.test_config,
            '-f', self.test_input,
            '-o', _result
        ]
        if args.debug:
            cmd += [
                '-LA',
                '-p', _result + '.unc'
            ]

        else:
            cmd += ['-LA']

        if args.show_commands:
            printc('RUN: ', repr(cmd))

        try:
            output = subprocess.check_output(cmd, stderr=subprocess.STDOUT)
        except subprocess.CalledProcessError as exc:
            output = exc.output
            if not self.test_xfail:
                print(output.rstrip())
                msg = '{} (Uncrustify error code {})'
                msg = msg.format(self.test_name, exc.returncode)
                printc('FAILED: ', msg, **FAIL_ATTRS)
                raise ExecutionFailure(exc)
            elif args.xdiff:
                print(output.rstrip())
        finally:
            if args.debug:
                with open(_result + '.log', 'wt') as f:
                    f.write(output)

        try:
            has_diff = not filecmp.cmp(_expected, _result)
            if has_diff and not self.test_xfail:
                if args.diff:
                    self._diff(_expected, _result)
                printc('{}: '.format(self.diff_text),
                       self.test_name, **self.diff_attrs)
                raise self.diff_exception(_expected, _result)
            if not has_diff and self.test_xfail:
                raise UnexpectedlyPassingFailure(_expected, _result)
            if has_diff and self.test_xfail:
                if args.xdiff:
                    self._diff(_expected, _result)
                    if not args.show_all:
                        printc('XFAILED: ', self.test_name, **PASS_ATTRS)
        except OSError as exc:
            printc('MISSING: ', self.test_name, **self.diff_attrs)
            raise MissingFailure(exc, _expected)


# =============================================================================
class FormatTest(SourceTest):
    pass_config = ['test_config', 'test_rerun_config']
    pass_input = ['test_input', 'test_expected']
    pass_expected = ['test_expected', 'test_rerun_expected']

    re_test_declaration = re.compile(r'^(?P<num>\d+)(?P<mark>[~!]*)\s+'
                          r'(?P<config>\S+)\s+(?P<input>\S+)'
                          r'(?:\s+(?P<lang>\S+))?$')

    # -------------------------------------------------------------------------
    def _build_pass(self, i):
        p = SourceTest()

        p.test_name = self.test_name
        p.test_lang = self.test_lang
        p.test_config = getattr(self, self.pass_config[i])
        p.test_input = getattr(self, self.pass_input[i])
        p.test_expected = getattr(self, self.pass_expected[i])
        p.test_xfail = self.test_xfail
        if i == 1 and not os.path.exists(p.test_expected):
            p.test_expected = getattr(self, self.pass_expected[0])

        return p

    # -------------------------------------------------------------------------
    def _build_passes(self):
        self._check()
        self._check_attr('test_rerun_config')
        self._check_attr('test_rerun_expected')

        self._make_abs('test_input', 'input')
        self._make_abs('test_config', 'config')
        self._make_abs('test_expected', 'expected')
        self._make_abs('test_rerun_config', 'config')
        self._make_abs('test_rerun_expected', 'expected')

        self.test_passes = [
            self._build_pass(0),
            self._build_pass(1)]

        self.test_passes[1].test_name = self.test_name + ' (re-run)'
        self.test_passes[1].test_result_dir = 'results_2'
        self.test_passes[1].diff_text = 'UNSTABLE'
        self.test_passes[1].diff_attrs = UNSTABLE_ATTRS
        self.test_passes[1].diff_exception = UnstableFailure

    # -------------------------------------------------------------------------
    def build_from_declaration(self, decl, group, line_number):
        match = self.re_test_declaration.match(decl)
        if not match:
            raise TestDeclarationParseError(group, line_number)

        num = match.group('num')
        is_rerun = ('!' in match.group('mark'))
        is_xfail = ('~' in match.group('mark'))

        self.test_xfail = is_xfail

        self.test_config = match.group('config')
        self.test_input = match.group('input')

        test_dir = os.path.dirname(self.test_input)
        test_filename = os.path.basename(self.test_input)

        if match.group('lang'):
            self.test_lang = match.group('lang')
        else:
            self.test_lang = test_dir

        self.test_expected = os.path.join(
            test_dir, '{}-{}'.format(num, test_filename))

        def rerun_file(name):
            parts = name.split('.')
            return '.'.join(parts[:-1] + ['rerun'] + parts[-1:])

        if is_rerun:
            self.test_rerun_config = rerun_file(self.test_config)
            self.test_rerun_expected = rerun_file(self.test_expected)
        else:
            self.test_rerun_config = self.test_config
            self.test_rerun_expected = self.test_expected

        self.test_name = '{}:{}'.format(group, num)

        self._build_passes()

    # -------------------------------------------------------------------------
    def build_from_args(self, args):
        self.test_name = args.name
        self.test_lang = args.lang
        self.test_input = args.input
        self.test_config = args.config
        self.test_expected = args.expected
        self.test_rerun_config = args.rerun_config or args.config
        self.test_rerun_expected = args.rerun_expected or args.expected
        self.test_xfail = args.xfail

        self._build_passes()

    # -------------------------------------------------------------------------
    def print_as_ctest(self, out_file=sys.stdout):
        self._check()

        def to_cmake_path(obj):
            if type(obj) is dict:
                return {k: to_cmake_path(v) for k, v in obj.items()}
            if type(obj) is str:
                return obj.replace(os.sep, '/')
            return obj

        runner = os.path.join(test_dir, 'run_test.py')

        out_file.write(
            ('add_test({test_name}\n' +
             '  "{python_exe}" -S "{test_runner}" "{test_name}"\n' +
             '    --executable     "{uncrustify_exe}"\n' +
             '    --lang           "{test_lang}"\n' +
             '    --input          "{test_input}"\n' +
             '    --config         "{test_config}"\n' +
             '    --expected       "{test_expected}"\n' +
             '    --rerun-config   "{test_rerun_config}"\n' +
             '    --rerun-expected "{test_rerun_expected}"\n' +
             '    -d --git         "{git_exe}"\n' +
             '{xfail}' +
             ')\n').format(
                 test_runner=to_cmake_path(runner),
                 python_exe=to_cmake_path(config.python_exe),
                 uncrustify_exe=to_cmake_path(config.uncrustify_exe),
                 git_exe=to_cmake_path(config.git_exe),
                 xfail=('    --xfail\n' if self.test_xfail else ''),
                 **to_cmake_path(self.__dict__)))
        out_file.write(
            ('set_tests_properties({}\n' +
             '  PROPERTIES LABELS "{}"\n)\n').format(
                 self.test_name, self.test_name.split(':')[0]))
        #out_file.write(
        #    ('set_tests_properties({}\n' +
        #     '  PROPERTIES DEPENDS "sources_format"\n)\n').format(
        #         self.test_name))

    # -------------------------------------------------------------------------
    def run(self, args):
        for p in self.test_passes:
            p.run(args)
