#!/usr/bin/python

"""
test_cli_options.py

Tests output generated by Uncrustifys commandline options
(excluding actual source code formatting)

:author:  DanielChumak
:license: GPL v2+
"""

from __future__ import print_function

import sys
from sys import stderr, argv, exit as sys_exit
from os import makedirs, remove, name as os_name
from os.path import dirname, relpath, abspath, isdir, isfile, join as path_join, split as path_split
from shutil import rmtree, copyfile
from subprocess import Popen, PIPE
from io import open
from datetime import date
import re
import difflib
import argparse
import pprint
import traceback

if os_name == 'nt':
    EX_OK = 0
    EX_USAGE = 64
    EX_SOFTWARE = 70
    NULL_DEVICE = 'nul'
else:
    from os import EX_OK, EX_USAGE, EX_SOFTWARE
    NULL_DEVICE = '/dev/null'

RE_CALLSTACK = r'\[CallStack:( \w+:\w+(, \w+:\w+)*|-DEBUG NOT SET-)?\]'
RE_DO_SPACE = (r'\n\ndo_space : WARNING: unrecognized do_space:'
               r'\n[^\n]+\n[^\n]+\n')


def eprint(*args, **kwargs):
    """
        print() wrapper that sets file=stderr
    """
    print(*args, file=stderr, **kwargs)


def decode_out(text):
    text = text.decode('utf-8')
    text = text.replace(u'\r\n', u'\n')
    text = text.replace(u'\r', u'\n')
    return text


def proc(bin_path, args_arr=()):
    """
    simple Popen wrapper to return std out/err utf8 strings


    Parameters
    ----------------------------------------------------------------------------
    :param bin_path: string
        path to the binary that is going to be called

    :param args_arr : list/tuple
        all needed arguments


    :return: string, string
    ----------------------------------------------------------------------------
        generated output of both stdout and stderr

    >>> proc("echo", "test")
    'test'
    """
    if not isfile(bin_path):
        eprint("bin 1 is not a file: %s" % bin_path)
        return False

    # call uncrustify, hold output in memory
    call_arr = [bin_path]
    call_arr.extend(args_arr)
    proc = Popen(call_arr, stdout=PIPE, stderr=PIPE)

    out_txt, err_txt = proc.communicate()

    return decode_out(out_txt), decode_out(err_txt)


def write_to_output_path(output_path, result_str):
    """
    writes the contents of result_str to the output path
    """
    print("Auto appending differences to: " + output_path)

    '''
    newline = None: this outputs  \r\n
    newline = "\r": this outputs  \r
    newline = "\n": this outputs  \n
    newline = ""  : this outputs  \n
    For the sake of consistency, all newlines are now being written out as \n
    However, if the result_str itself contains \r\n, then \r\n will be output
    as this code doesn't post process the data being written out
    '''
    with open(output_path, 'w', encoding="utf-8", newline="\n") as f:
        f.write(result_str)


def get_file_content(fp):
    """
    returns file content as an utf8 string or None if fp is not a file


    Parameters
    ----------------------------------------------------------------------------
    :param fp: string
        path of the file that will be read


    :return: string or None
    ----------------------------------------------------------------------------
    the file content

    """
    out = None

    print("1 +++ "+sys.version)
    #eprint(abspath(fp))
    #for line in traceback.format_stack():
    #    print(line.strip())
    if isfile(fp):
        with open(fp, encoding="utf-8", newline="\n") as f:
            out = f.read()
    else:
        #eprint(abspath(fp))
        eprint("is 2 not a file: %s" % fp)

    return out


def check_generated_output(gen_expected_path, gen_result_path,
                           result_manip=None, program_args=None):
    """
    compares the content of two files,

    is intended to compare a file that was generated during a call of Uncrustify
    with a file that has the expected content


    Parameters
    ----------------------------------------------------------------------------
    :param gen_expected_path: string
        path to a file that will be compared with the generated file

    :param gen_result_path: string
        path to the file that will be generated by Uncrustify

    :param result_manip: lambda OR list or tuple of lambdas
        optional lambda function(s) that will be applied (before the comparison)
        on the content of the generated file,
        the lambda function(s) should accept one string parameter

    :param program_args: tuple of options
        a collection of multiple options used to add extra functionality to the
        script (i.e. auto apply changes or show diffs on command line)

    :return: bool
    ----------------------------------------------------------------------------
    True or False depending on whether both files have the same content

    >>> check_generated_output("/dev/null", "/dev/null")
    True
    """

    gen_exp_txt = get_file_content(gen_expected_path)
    if gen_exp_txt is None:
        return False

    gen_res_txt = get_file_content(gen_result_path)
    if gen_res_txt is None:
        return False

    if result_manip is not None:
        if type(result_manip) is list or type(result_manip) is tuple:
            for m in result_manip:
                gen_res_txt = m(gen_res_txt)
        else:
            gen_res_txt = result_manip(gen_res_txt)

    if gen_res_txt != gen_exp_txt:
        with open(gen_result_path, 'w', encoding="utf-8", newline="") as f:
            f.write(gen_res_txt)

        if program_args.apply and program_args.auto_output_path:
            write_to_output_path(program_args.auto_output_path, gen_res_txt)
            return True
        elif program_args.diff:
            print("\n************************************")
            print("Problem (1) with %s" % gen_result_path)
            print("************************************")

            file_diff = difflib.ndiff(gen_res_txt.splitlines(False),
                                      gen_exp_txt.splitlines(False))

            for line in file_diff:
                pprint.PrettyPrinter(indent=4, width=280).pprint(line)

            return False
        else:
            print("\nProblem (2) with %s" % gen_result_path)
            print("use(gen): '--diff' to find out why %s %s are different"
                  % (gen_result_path, gen_expected_path))
            return False

    remove(gen_result_path)
    #print(abspath(gen_result_path))

    return True


def check_std_output(expected_path, result_path, result_str, result_manip=None,
                     program_args=None):
    """
    compares output generated by Uncrustify (std out/err) with a the content of
    a file

    Parameters
    ----------------------------------------------------------------------------
    :param expected_path: string
        path of the file that will be compared with the output of Uncrustify

    :param result_path: string
        path to which the Uncrustifys output will be saved in case of a mismatch

    :param result_str: string (utf8)
        the output string generated by Uncrustify

    :param result_manip: lambda OR list or tuple of lambdas
        see result_manip for check_generated_output

    :param program_args: tuple of options
        a collection of multiple options used to add extra functionality to the
        script (i.e. auto apply changes or show diffs on command line)

    :return: bool
    ----------------------------------------------------------------------------
    True or False depending on whether both files have the same content

    """
    exp_txt = get_file_content(expected_path)
    if exp_txt is None:
        return False

    if result_manip is not None:
        if type(result_manip) is list or type(result_manip) is tuple:
            for m in result_manip:
                result_str = m(result_str)
        else:
            result_str = result_manip(result_str)

    if result_str != exp_txt:
        with open(result_path, 'w', encoding="utf-8", newline="\n") as f:
            f.write(result_str)

        if program_args.apply and program_args.auto_output_path:
            write_to_output_path(program_args.auto_output_path, result_str)
            return True

        if program_args.diff:
            print("\n************************************")
            print("Problem (3) with result_path   is %s" % result_path)
            print("                 expected_path is %s" % expected_path)
            print("************************************")

            file_diff = difflib.ndiff(result_str.splitlines(False),
                                      exp_txt.splitlines(False))

            """
            change the value of width
            look at: If compact is false (the default)...
            """
            for line in file_diff:
                pprint.PrettyPrinter(indent=4, width=280).pprint(line)
        else:
            print("\nProblem (4) with %s" % result_path)
            print("use: '--diff' to find out why %s %s are different"
                  % (result_path, expected_path))
        return False
    return True


def check_uncrustify_output(
        uncr_bin,
        program_args,
        args_arr=(),
        out_expected_path=None, out_result_manip=None, out_result_path=None,
        err_expected_path=None, err_result_manip=None, err_result_path=None,
        gen_expected_path=None, gen_result_manip=None, gen_result_path=None):
    """
    compares outputs generated by Uncrustify with files

    Parameters
    ----------------------------------------------------------------------------
    :param uncr_bin: string
        path to the Uncrustify binary

    :param program_args: tuple of options
        a collection of multiple options used to add extra functionality to the
        script (i.e. auto apply changes or show diffs on command line)

    :param args_arr: list/tuple
        Uncrustify commandline arguments

    :param out_expected_path: string
        file that will be compared with Uncrustifys stdout output

    :param out_result_manip: string
        lambda function that will be applied to Uncrustifys stdout output
        (before the comparison with out_expected_path),
        the lambda function should accept one string parameter

    :param out_result_path: string
        path where Uncrustifys stdout output will be saved to in case of a
        mismatch

    :param err_expected_path: string
        path to a file that will be compared with Uncrustifys stderr output

    :param err_result_manip: string
        see out_result_manip (is applied to Uncrustifys stderr instead)

    :param err_result_path: string
        see out_result_path (is applied to Uncrustifys stderr instead)

    :param gen_expected_path: string
        path to a file that will be compared with a file generated by Uncrustify

    :param gen_result_path: string
        path to a file that will be generated by Uncrustify

    :param gen_result_manip:
        see out_result_path (is applied, in memory, to the file content of the
        file generated by Uncrustify instead)


    :return: bool
    ----------------------------------------------------------------------------
    True if all specified files match up, False otherwise
    """
    # check param sanity
    if not out_expected_path and not err_expected_path and not gen_expected_path:
        eprint("No expected comparison file provided")
        return False

    if bool(gen_expected_path) != bool(gen_result_path):
        eprint("'gen_expected_path' and 'gen_result_path' must be used in "
               "combination")
        return False

    if gen_result_manip and not gen_result_path:
        eprint("Set up 'gen_result_path' if 'gen_result_manip' is used")

    out_res_txt, err_res_txt = proc(uncr_bin, args_arr)

    ret_flag = True

    if program_args.apply:
        valid_path = [out_expected_path, err_expected_path, gen_expected_path]
        program_args.auto_output_path = next(item for item in valid_path if item is not None)

    if out_expected_path and not check_std_output(
            out_expected_path, out_result_path, out_res_txt,
            result_manip=out_result_manip,
            program_args=program_args):
        ret_flag = False

    if program_args.apply:
        valid_path = [err_expected_path, out_expected_path, gen_expected_path]
        program_args.auto_output_path = next(item for item in valid_path if item is not None)

    if err_expected_path and not check_std_output(
            err_expected_path, err_result_path, err_res_txt,
            result_manip=err_result_manip,
            program_args=program_args):
        ret_flag = False

    if gen_expected_path and not check_generated_output(
            gen_expected_path, gen_result_path,
            result_manip=gen_result_manip,
            program_args=program_args):
        ret_flag = False

    return ret_flag


def clear_dir(path):
    """
    clears a directory by deleting and creating it again


    Parameters
    ----------------------------------------------------------------------------
    :param path:
        path of the directory


    :return: void
    """
    if isdir(path):
        rmtree(path)
    makedirs(path)


def reg_replace(pattern, replacement):
    """
    returns a generated lambda function that applies a regex string replacement


    Parameters:
    ----------------------------------------------------------------------------

    :param pattern: regex pattern
        the pattern that will be used to find targets to replace

    :param replacement: string
        the replacement that will be applied


    :return: lambda function
    ----------------------------------------------------------------------------
        the generated lambda function, takes in a string on which the
        replacement will be applied and returned

    >>>  l = reg_replace(r"a", "b")
    >>>  a = l("a")
    'b'
    """
    return lambda text: re.sub(pattern, replacement, text)


def string_replace(string_target, replacement):
    """
    returns a generated lambda function that applies a string replacement

    like reg_replace, uses string.replace() instead
    """
    return lambda text: text.replace(string_target, replacement)


def s_path_join(path, *paths):
    """
    Wrapper for the os.path.join function, splits every path component to
    replace it with a system specific path separator. This is for consistent
    path separators (and also systems that don't use either '\' or '/')


    Parameter
    ----------------------------------------------------------------------------
    :params path, paths: string
        see os.path.join

    :return: string
    ----------------------------------------------------------------------------
        a joined path, see os.path.join

    >>> s_path_join('./z/d/', '../a/b/c/f')
    r'.\z\a\b\c\f'
    """
    p_splits = list(path_split(path))
    for r in map(path_split, paths):
        p_splits.extend(r)
    return path_join(*p_splits)


def main(args):
    print("die Version ist :"+sys.version)
    print("A +++ "+sys.version)
    #sys_exit()
    # set working dir to script dir
    script_dir = dirname(relpath(__file__))
    print("B +++ "+sys.version)
    #sys_exit()

    parser = argparse.ArgumentParser(description='Test CLI Options')
    print("C +++ "+sys.version)
    #sys_exit()
    parser.add_argument('--diff', action='store_true',
                        help='show diffs when there is a test mismatch')
    print("D +++ "+sys.version)
    #sys_exit()
    parser.add_argument('--apply', action='store_true',
                        help='auto apply the changes from the results folder to the output folder')
    print("E +++ "+sys.version)
    #sys_exit()
    parser.add_argument('--build',
                        default=s_path_join(script_dir, '../../build'),
                        help='specify location of the build directory')
    print("F +++ "+sys.version)
    sys_exit()
    parser.add_argument('--config',
                        default='Release',
                        help='the build_type value (Release or Debug)')
    print("G +++ "+sys.version)
    sys_exit()
    parser.add_argument('--test',
                        default=s_path_join(script_dir, '../../build/tests/cli'),
                        help='specify the location of the test cli build folder')
    print("H +++ "+sys.version)
    sys_exit()

    parsed_args = parser.parse_args()

    print("2 +++ "+sys.version)
    # find the uncrustify binary
    bin_found = False
    uncr_bin = ''
    bd_dir = parsed_args.build
    test_dir = parsed_args.test

    bin_paths = [s_path_join(bd_dir, 'uncrustify'),
                 s_path_join(bd_dir, 'uncrustify.exe'),
                 s_path_join(bd_dir, 'Debug/uncrustify'),
                 s_path_join(bd_dir, 'Debug/uncrustify.exe'),
                 s_path_join(bd_dir, 'Release/uncrustify'),
                 s_path_join(bd_dir, 'Release/uncrustify.exe'),
                 s_path_join(bd_dir, 'RelWithDebInfo/uncrustify'),
                 s_path_join(bd_dir, 'RelWithDebInfo/uncrustify.exe'),
                 s_path_join(bd_dir, 'MinSizeRel/uncrustify'),
                 s_path_join(bd_dir, 'MinSizeRel/uncrustify.exe')]
    for uncr_bin in bin_paths:
        if not isfile(uncr_bin):
            eprint("is 3 not a file: %s" % uncr_bin)
        else:
            print("Uncrustify binary found: %s" % uncr_bin)
            bin_found = True
            break
    if not bin_found:
        eprint("No Uncrustify binary found")
        sys_exit(EX_USAGE)
    # guy
    print("3 +++ "+sys.version)
    print("Summary")
    print("OS is %s" % os_name)

    clear_dir(s_path_join(test_dir, 'results'))

    return_flag = True

    #print("Test help ...")
    ##
    ## Test help
    ##   -h -? --help --usage
    #if not check_uncrustify_output(
    #        uncr_bin,
    #        parsed_args,
    #        out_expected_path=s_path_join(script_dir, 'output/help.txt'),
    #        out_result_path=s_path_join(test_dir, 'results/help.txt'),
    #        out_result_manip=[
    #            string_replace(' --mtime      : Preserve mtime on replaced files.\n', ''),
    #            string_replace('.exe', ''),
    #            reg_replace(r'currently \d+ options', 'currently x options')
    #        ]):
    #    return_flag = False
    #print("Test help is OK")

    #print("Test false parameter ...")
    ##
    ## Test false parameter
    ##   --xyz
    #if not check_uncrustify_output(
    #        uncr_bin,
    #        parsed_args,
    #        args_arr=['--xyz'],
    #        err_expected_path=s_path_join(script_dir, 'output/xyz-err.txt'),
    #        err_result_path=s_path_join(test_dir, 'results/xyz-err.txt')
    #        ):
    #    return_flag = False
    #print("Test false parameter is OK")

    #print("Test Version ...")
    ##
    ## Test Version
    ##   -v
    #if not check_uncrustify_output(
    #        uncr_bin,
    #        parsed_args,
    #        args_arr=['-v'],
    #        out_expected_path=s_path_join(script_dir, 'output/v-out.txt'),
    #        out_result_path=s_path_join(test_dir, 'results/v-out.txt'),
    #        out_result_manip=reg_replace(r'Uncrustify.+', 'Uncrustify')
    #        ):
    #    return_flag = False
    #print("Test Version is OK")

    ## temporary removed
    ##print("Test --show-config ...")
    ###
    ### Test --show-config
    ###
    ##if not check_uncrustify_output(
    ##        uncr_bin,
    ##        parsed_args,
    ##        args_arr=['--show-config'],
    ##        out_expected_path=s_path_join(script_dir, 'output/show_config.txt'),
    ##        out_result_path=s_path_join(test_dir, 'results/show_config.txt'),
    ##        out_result_manip=reg_replace(r'\# Uncrustify.+', '')
    ##        ):
    ##    return_flag = False
    ##print("Test --show-config is OK")

    #print("Test the truncate option ...")
    ##
    ## Test the truncate option
    ##
    #if not check_uncrustify_output(
    #        uncr_bin,
    #        parsed_args,
    #        args_arr=['-c', s_path_join(script_dir, 'config/truncate.cfg'),
    #                  '-f', s_path_join(script_dir, 'input/truncate.cpp'),
    #                  '-o', NULL_DEVICE,
    #                  '-L', '83'],
    #        err_expected_path=s_path_join(script_dir, 'output/truncate.txt'),
    #        err_result_path=s_path_join(test_dir, 'results/truncate.txt'),
    #        err_result_manip=[reg_replace(r'\([0-9]+\)', ' '),
    #                          reg_replace(RE_DO_SPACE, '')]
    #        ):
    #    return_flag = False
    #print("Test the truncate option is OK")

    ##print("Test --update-config ...")
    ## temporary removed
    ###
    ### Test --update-config
    ###
    ##if not check_uncrustify_output(
    ##        uncr_bin,
    ##        parsed_args,
    ##        args_arr=['-c', s_path_join(script_dir, 'config/mini_d.cfg'),
    ##                  '--update-config'],
    ##        out_expected_path=s_path_join(script_dir, 'output/mini_d_uc.txt'),
    ##        out_result_path=s_path_join(test_dir, 'results/mini_d_uc.txt'),
    ##        out_result_manip=reg_replace(r'\# Uncrustify.+', ''),
    ##        err_expected_path=s_path_join(script_dir, 'output/mini_d_error.txt'),
    ##        err_result_path=s_path_join(test_dir, 'results/mini_d_error0.txt'),
    ##        err_result_manip=string_replace('\\', '/')
    ##        ):
    ##    return_flag = False

    ##if not check_uncrustify_output(
    ##        uncr_bin,
    ##        parsed_args,
    ##        args_arr=['-c', s_path_join(script_dir, 'config/mini_nd.cfg'),
    ##                  '--update-config'],
    ##        out_expected_path=s_path_join(script_dir, 'output/mini_nd_uc.txt'),
    ##        out_result_path=s_path_join(test_dir, 'results/mini_nd_uc.txt'),
    ##        out_result_manip=reg_replace(r'\# Uncrustify.+', ''),
    ##        err_expected_path=s_path_join(script_dir, 'output/mini_d_error.txt'),
    ##        err_result_path=s_path_join(test_dir, 'results/mini_d_error1.txt'),
    ##        err_result_manip=string_replace('\\', '/')
    ##        ):
    ##    return_flag = False
    ##print("Test --update-config is OK")

    ##print("Test --update-config-with-doc ...")
    ###
    ### Test --update-config-with-doc
    ###
    ##if not check_uncrustify_output(
    ##        uncr_bin,
    ##        parsed_args,
    ##        args_arr=['-c', s_path_join(script_dir, 'config/mini_d.cfg'),
    ##                  '--update-config-with-doc'],
    ##        out_expected_path=s_path_join(script_dir, 'output/mini_d_ucwd.txt'),
    ##        out_result_path=s_path_join(test_dir, 'results/mini_d_ucwd.txt'),
    ##        out_result_manip=reg_replace(r'\# Uncrustify.+', ''),
    ##        err_expected_path=s_path_join(script_dir, 'output/mini_d_error.txt'),
    ##        err_result_path=s_path_join(test_dir, 'results/mini_d_error2.txt'),
    ##        err_result_manip=string_replace('\\', '/')
    ##        ):
    ##    return_flag = False
    ##print("Test --update-config-with-doc is OK")

    ##if not check_uncrustify_output(
    ##        uncr_bin,
    ##        parsed_args,
    ##        args_arr=['-c', s_path_join(script_dir, 'config/mini_nd.cfg'),
    ##                  '--update-config-with-doc'],
    ##        out_expected_path=s_path_join(script_dir, 'output/mini_nd_ucwd.txt'),
    ##        out_result_path=s_path_join(test_dir, 'results/mini_nd_ucwd.txt'),
    ##        out_result_manip=reg_replace(r'\# Uncrustify.+', ''),
    ##        err_expected_path=s_path_join(script_dir, 'output/mini_d_error.txt'),
    ##        err_result_path=s_path_join(test_dir, 'results/mini_d_error3.txt'),
    ##        err_result_manip=string_replace('\\', '/')
    ##        ):
    ##    return_flag = False

    #print("Test -p ...")
    ##
    ## Test -p
    ##
    #if os_name != 'nt':
    #    if not check_uncrustify_output(
    #            uncr_bin,
    #            parsed_args,
    #            args_arr=['-c', s_path_join(script_dir, 'config/mini_nd.cfg'),
    #                      '-f', s_path_join(script_dir, 'input/testSrcP.cpp'),
    #                      '-p', s_path_join(test_dir, 'results/p.txt')],
    #            gen_expected_path=s_path_join(script_dir, 'output/p.txt'),
    #            gen_result_path=s_path_join(test_dir, 'results/p.txt'),
    #            gen_result_manip=reg_replace(r'\# Uncrustify.+[^\n\r]', '')
    #            ):
    #        return_flag = False

    #    if not check_uncrustify_output(
    #            uncr_bin,
    #            parsed_args,
    #            args_arr=['-f', s_path_join(script_dir, 'input/class_enum_struct_union.cpp'),
    #                      '-p', s_path_join(test_dir, 'results/class_enum_struct_union.txt')],
    #            gen_expected_path=s_path_join(script_dir, 'output/class_enum_struct_union.txt'),
    #            gen_result_path=s_path_join(test_dir, 'results/class_enum_struct_union.txt'),
    #            gen_result_manip=reg_replace(r'\# Uncrustify.+[^\n\r]', '')
    #            ):
    #        return_flag = False

    #    if not check_uncrustify_output(
    #            uncr_bin,
    #            parsed_args,
    #            args_arr=['-f', s_path_join(script_dir, 'input/in_fcn_def.cpp'),
    #                      '-p', s_path_join(test_dir, 'results/in_fcn_def.txt')],
    #            gen_expected_path=s_path_join(script_dir, 'output/in_fcn_def.txt'),
    #            gen_result_path=s_path_join(test_dir, 'results/in_fcn_def.txt'),
    #            gen_result_manip=reg_replace(r'\# Uncrustify.+[^\n\r]', '')
    #            ):
    #        return_flag = False
    #print("Test -p is OK")

    ##print("Test -p and -c with '-' input ...")
    #if os_name == 'nt' or check_uncrustify_output(
    #        uncr_bin,
    #        parsed_args,
    #        args_arr=['-c', '-',
    #                  '-f', NULL_DEVICE,
    #                  '-p', '-'],
    #        out_expected_path=s_path_join(script_dir, 'output/pc-.txt'),
    #        out_result_manip=reg_replace(r'\# Uncrustify.+[^\n\r]', ''),
    #        out_result_path=s_path_join(test_dir, 'results/pc-.txt')
    #):
    #    pass

    ##print("Test -p and -c with '-' input ...")
    ##
    ## Test -p and -c with '-' input
    ##
    #else:
    #    return_flag = False
    ##print("Test -p and -c with '-' input is OK")

    #print("Test -p and --debug-csv-format option ...")
    ##
    ## Test -p and --debug-csv-format option
    ##
    #if os_name != 'nt' and not check_uncrustify_output(
    #        uncr_bin,
    #        parsed_args,
    #        args_arr=['-c', '-',
    #                  '-f', s_path_join(script_dir, 'input/class_enum_struct_union.cpp'),
    #                  '-p', s_path_join(test_dir, 'results/class_enum_struct_union.csv'),
    #                  '--debug-csv-format'],
    #        gen_expected_path=s_path_join(script_dir, 'output/class_enum_struct_union.csv'),
    #        gen_result_path=s_path_join(test_dir, 'results/class_enum_struct_union.csv'),
    #        ):
    #    return_flag = False
    #print("Test -p and --debug-csv-format option is OK")

    if parsed_args.config == 'Debug':
        print("Test --tracking space:FILE ...")
        print("  config is Debug")
        #
        # Test --tracking space:FILE
        #
        if os_name != 'nt':
            # doesn't work under windows
            temp_result_path = s_path_join(script_dir, 'results/Debug_tracking_space.html')
            abc = "space:" + temp_result_path                   # Issue #4066
            if not check_uncrustify_output(
                    uncr_bin,
                    parsed_args,
                    args_arr=['-c', s_path_join(script_dir, 'config/tracking_space.cfg'),
                              '-f', s_path_join(script_dir, 'input/tracking_space.cpp'),
                              '--tracking',
                              abc,
                              s_path_join(script_dir, 'results/Debug_tracking_space.html')
                              #, '-L'
                              #, 'A'
                              #, '2'
                              #, '>'
                              #, '/home/guy/A-T.txt'
                              ],
                    gen_expected_path=s_path_join(script_dir, 'output/Debug_tracking_space.html'),
                    gen_result_path=s_path_join(script_dir, 'results/Debug_tracking_space.html')
                    ):
                return_flag = False
        print("Test --tracking space:FILE is OK")

    sys_exit()

    print("Test --replace ...")
    #
    # Test --replace
    #
    copyfile("input/backup.h-save", "input/backup.h")
    if not check_uncrustify_output(
            uncr_bin,
            parsed_args,
            args_arr=['-c', s_path_join(script_dir, 'config/replace.cfg'),
                      '-F', s_path_join(script_dir, 'input/replace.list'),
                      '--replace', '--no-backup'],
            gen_expected_path=s_path_join(script_dir, 'output/backup.h'),
            gen_result_path=s_path_join(script_dir, 'input/backup.h'),
            err_expected_path=s_path_join(script_dir, 'output/replace.txt'),
            err_result_path=s_path_join(test_dir, 'results/replace.txt'),
            ):
        return_flag = False
    print("Test --replace is OK")

    print("Test --universalindent ...")
    # The flag CMAKE_BUILD_TYPE must be set to "Release", or all lines with
    # 'Description="<html>(<number>)text abc.</html>" must be changed to
    # 'Description="<html>text abc.</html>"
    #
    # OR it is possible to introduce a new parameter: gen_expected_manip
    #
    # The last "reg_replace(r'\r', '')" is necessary under Windows, because
    # fprintf puts a \r\n at the end of a line. To make the check, we use
    # output/universalindent.cfg, generated under Linux, with only \n at the
    # end of a line.
    if not check_uncrustify_output(
            uncr_bin,
            parsed_args,
            args_arr=['-o', s_path_join(test_dir, 'results/universalindent.cfg'),
                      '--universalindent'],
            gen_expected_path=s_path_join(script_dir, 'output/universalindent.cfg'),
            gen_result_path=s_path_join(test_dir, 'results/universalindent.cfg'),
            gen_result_manip=[reg_replace(r'version=U.+', ''),
                              reg_replace(r'\(\d+\)', ''),
                              reg_replace(r'\r', '')]
            ):
        return_flag = False
    print("Test --universalindent is OK")

    print("Test -L ...")
    # Debug Options:
    #   -L
    # look at src/log_levels.h
    Ls_A = ['9', '21', '25', '28', '31', '36', '66', '92']
    for L in Ls_A:
        if not check_uncrustify_output(
                uncr_bin,
                parsed_args,
                args_arr=['-c', NULL_DEVICE, '-L', L, '-o', NULL_DEVICE,
                          '-f', s_path_join(script_dir, 'input/testSrc.cpp')],
                err_expected_path=s_path_join(script_dir, 'output/%s.txt' % L),
                err_result_path=s_path_join(test_dir, 'results/%s.txt' % L),
                err_result_manip=[reg_replace(r'\([0-9]+\)', ' '),
                                  reg_replace(r'\:[0-9]+\)', ' '),
                                  reg_replace(r'\[line [0-9]+', '[ '),
                                  reg_replace(r'   \[[_|,|1|A-Z]*\]', '   []'),
                                  reg_replace(r', \[[_|,|1|A-Z]*\]', ', []'),
                                  reg_replace(r', \[0[xX][0-9a-fA-F]+:[_|,|1|A-Z]*\]', ', []'),
                                  reg_replace(r'   \[0[xX][0-9a-fA-F]+:[_|,|1|A-Z]*\]', '   []'),
                                  reg_replace(r'^[ \t]*[_A-Za-z][_A-Za-z0-9]*::', ''),
                                  reg_replace(RE_CALLSTACK, '[CallStack]'),
                                  reg_replace(RE_DO_SPACE, ''),
                                  reg_replace(r'Chunk::', '')]
            ):
            return_flag = False
    print("Test -L is OK")

    # Test logger buffer overflow
    if not check_uncrustify_output(
            uncr_bin,
            parsed_args,
            args_arr=['-c', NULL_DEVICE, '-L', '99', '-o', NULL_DEVICE,
                      '-f', s_path_join(script_dir, 'input/logger.cs')],
            err_expected_path=s_path_join(script_dir, 'output/logger_cs_L_99.txt'),
            err_result_path=s_path_join(test_dir, 'results/logger_cs_L_99.txt'),
            err_result_manip=reg_replace(r'[0-9]', '')
            ):
        return_flag = False

    # misc error_tests
    error_tests = ["I-842", "unmatched_close_pp"]
    for test in error_tests:
        if not check_uncrustify_output(
                uncr_bin,
                parsed_args,
                args_arr=['-c', s_path_join(script_dir, 'config/%s.cfg' % test),
                          '-f', s_path_join(script_dir, 'input/%s.cpp' % test),
                          '-o', NULL_DEVICE, '-q'],
                err_expected_path=s_path_join(script_dir, 'output/%s.txt' % test),
                err_result_path=s_path_join(test_dir, 'results/%s.txt' % test)
                ):
            return_flag = False

    print("Test $(year) keyword ...")
    # Test $(year) keyword (issue #3251)
    if not check_uncrustify_output(
            uncr_bin,
            parsed_args,
            args_arr=['-c', s_path_join(script_dir, 'config/copyright-header.cfg'),
                      '-f', s_path_join(script_dir, 'input/testSrc.cpp')],
            out_expected_path=s_path_join(script_dir, 'output/copyright-header.cpp'),
            out_result_path=s_path_join(test_dir, 'results/copyright-header.cpp'),
            out_result_manip=string_replace(str(date.today().year), 'this year'),
            ):
        return_flag = False
    print("Test $(year) keyword is OK")

    if return_flag:
        print("all tests are OK")
        sys_exit(EX_OK)
    else:
        print("some problem(s) are still present")
        sys_exit(EX_SOFTWARE)


if __name__ == "__main__":
    main(argv[1:])
