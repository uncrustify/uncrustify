# Class encapsulating a unit test.
#
# * @author  Ben Gardner        October 2009
# * @author  Guy Maurel         October 2015
# * @author  Matthew Woehlke    June 2018
#

import filecmp
import os
import subprocess
import sys

from .config import (config, test_dir, FAIL_ATTRS,
                     MISMATCH_ATTRS, UNSTABLE_ATTRS)
from .failure import (ExecutionFailure, MissingFailure,
                      MismatchFailure, UnstableFailure)

try:
    from .ansicolor import printc
except Exception:
    def printc(ctext, ntext, *args, **kwargs):
        print(ctext + ntext)


# =============================================================================
class Test(object):
    pass_config = ['test_config', 'test_rerun_config']
    pass_input = ['test_input', 'test_rerun_expected']
    pass_expected = ['test_expected', 'test_rerun_expected']
    pass_result_dir = ['results', 'results_2']

    pass_diff_text = ['MISMATCH', 'UNSTABLE']
    pass_diff_attrs = [MISMATCH_ATTRS, UNSTABLE_ATTRS]

    pass_note = ['', ' (re-run)']
    pass_exception = [MismatchFailure, UnstableFailure]

    # -------------------------------------------------------------------------
    def build_from_declaration(self, decl, group):
        parts = decl.split()
        test_dir = os.path.dirname(parts[2])

        if len(parts) == 4:
            self.test_lang = parts[3]
        else:
            self.test_lang = test_dir

        if parts[0].endswith('!'):
            num = parts[0][:-1]
        else:
            num = parts[0]

        self.test_config = parts[1]
        self.test_input = parts[2]
        self.test_expected = os.path.join(
            test_dir, '{}-{}'.format(num, os.path.basename(parts[2])))

        def rerun_file(name):
            parts = name.split('.')
            return '.'.join(parts[:-1] + ['rerun'] + parts[-1:])

        if parts[0].endswith('!'):
            self.test_rerun_config = rerun_file(self.test_config)
            self.test_rerun_expected = rerun_file(self.test_expected)
        else:
            self.test_rerun_config = self.test_config
            self.test_rerun_expected = self.test_expected

        self.test_name = '{}:{}'.format(group, num)

    # -------------------------------------------------------------------------
    def build_from_args(self, args):
        self.test_name = args.name
        self.test_lang = args.lang
        self.test_input = args.input
        self.test_config = args.config
        self.test_expected = args.expected
        self.test_rerun_config = args.rerun_config or args.config
        self.test_rerun_expected = args.rerun_expected or args.expected

    # -------------------------------------------------------------------------
    def check(self):
        def check_attr(name):
            if not hasattr(self, name) or getattr(self, name) is None:
                raise AttributeError(
                    'Test is missing required attribute {!r}'.format(name))

        check_attr('test_name')
        check_attr('test_lang')
        check_attr('test_input')
        check_attr('test_config')
        check_attr('test_expected')
        check_attr('test_rerun_config')
        check_attr('test_rerun_expected')

        def make_abs(name, base):
            path = getattr(self, name)
            if not os.path.isabs(path):
                setattr(self, name, os.path.join(test_dir, base, path))

        make_abs('test_input', 'input')
        make_abs('test_config', 'config')
        make_abs('test_expected', 'expected')
        make_abs('test_rerun_config', 'config')
        make_abs('test_rerun_expected', 'expected')

    # -------------------------------------------------------------------------
    def print_as_ctest(self, out_file=sys.stdout):
        self.check()

        out_file.write(
            ('add_test({test_name}\n' +
             '  "{python_exe}" "{test_runner}" "{test_name}"\n' +
             '    --executable     "{uncrustify_exe}"\n' +
             '    --lang           "{test_lang}"\n' +
             '    --input          "{test_input}"\n' +
             '    --config         "{test_config}"\n' +
             '    --expected       "{test_expected}"\n' +
             '    --rerun-config   "{test_rerun_config}"\n' +
             '    --rerun-expected "{test_rerun_expected}"\n' +
             '    -d --git         "{git_exe}"\n' +
             ')\n').format(
                 test_runner=os.path.join(test_dir, 'run_test.py'),
                 python_exe=config.python_exe,
                 uncrustify_exe=config.uncrustify_exe,
                 git_exe=config.git_exe,
                 **self.__dict__))
        out_file.write(
            ('set_tests_properties({}\n' +
             '  PROPERTIES LABELS "{}"\n)\n').format(
                 self.test_name, self.test_name.split(':')[0]))

    # -------------------------------------------------------------------------
    def _diff(self, expected, actual):
        sys.stdout.flush()
        cmd = [config.git_exe, 'diff', '--no-index', expected, actual]
        subprocess.call(cmd)

    # -------------------------------------------------------------------------
    def _run_pass(self, i, args):
        _config = getattr(self, self.pass_config[i])
        _input = getattr(self, self.pass_input[i])
        _expected = getattr(self, self.pass_expected[i])
        _result = os.path.join(args.result_dir, self.pass_result_dir[i],
                               os.path.basename(os.path.dirname(_expected)),
                               os.path.basename(_expected))

        if args.verbose:
            print(self.test_name + self.pass_note[i])
            print('  Language : {}'.format(self.test_lang))
            print('     Input : {}'.format(_input))
            print('    Config : {}'.format(_config))
            print('  Expected : {}'.format(_expected))
            print('    Result : {}'.format(_result))

        if not os.path.exists(self.pass_result_dir[i]):
            os.makedirs(self.pass_result_dir[i])

        cmd = [
            config.uncrustify_exe,
            '-q',
            '-l', self.test_lang,
            '-c', _config,
            '-f', _input,
            '-o', _result
        ]
        if args.debug:
            cmd += [
                '-LA',
                '-p', _result + '.unc'
            ]
            stderr = open(_result + '.log')

        else:
            cmd += ['-L1,2']
            stderr = None

        if args.show_commands:
            printc('RUN: ', repr(cmd))

        try:
            subprocess.check_call(cmd, stderr=stderr)
        except subprocess.CalledProcessError as exc:
            msg = '{}{}: Uncrustify error code {}'
            msg = msg.format(self.test_name, self.pass_note[i], exc.returncode)
            printc('FAILED: ', msg, **FAIL_ATTRS)
            raise ExecutionFailure(exc)
        finally:
            del stderr

        try:
            if not filecmp.cmp(_expected, _result):
                printc('{}: '.format(self.pass_diff_text[i]),
                       self.test_name + self.pass_note[i],
                       **self.pass_diff_attrs[i])
                if args.diff:
                    self._diff(_expected, _result)
                raise self.pass_exception[i](_expected, _result)
        except OSError as exc:
            printc('MISSING: ', self.test_name + self.pass_note[i],
                   **self.pass_diff_attrs[i])
            raise MissingFailure(exc, _expected)

    # -------------------------------------------------------------------------
    def run(self, args):
        self.check()

        self._run_pass(0, args)
        self._run_pass(1, args)
