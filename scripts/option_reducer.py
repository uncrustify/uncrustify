#!/usr/bin/python
"""
option_reducer.py

reduces options in a given config file to the minimum while still maintaining
desired formatting

:author:  Daniel Chumak
:license: GPL v2+
"""

# Possible improvements:
# - parallelize add_back()
# - (maybe) reduce amount of written config file, see Uncrustify --set

from __future__ import print_function  # python >= 2.6
import argparse

from os import name as os_name, sep as os_path_sep, fdopen as os_fdopen, \
    remove as os_remove
from os.path import exists, join as path_join
from subprocess import Popen, PIPE
from sys import exit as sys_exit, stderr, stdout
from shutil import rmtree
from multiprocessing import cpu_count
from tempfile import mkdtemp, mkstemp
from contextlib import contextmanager
from collections import OrderedDict
from threading import Timer
from multiprocessing.pool import Pool
from itertools import combinations

FLAGS = None
NULL_DEV = "/dev/null" if os_name != "nt" else "nul"


def enum(**enums):
    return type('Enum', (), enums)


RESTULTSFLAG = enum(NONE=0, REMOVE=1, KEEP=2)
ERROR_CODE = enum(NONE=0, FLAGS=200, SANITY0=201, SANITY1=202)
MODES = ("reduce", "no-default")


@contextmanager
def make_temp_directory():
    """
    Wraps tempfile.mkdtemp to use it inside a with statement that auto deletes
    the temporary directory with its content after the with block closes


    :return: str
    ----------------------------------------------------------------------------
        path to the generated directory
    """
    temp_dir = mkdtemp()
    try:
        yield temp_dir
    finally:
        rmtree(temp_dir)


@contextmanager
def make_raw_temp_file(*args, **kwargs):
    """
    Wraps tempfile.mkstemp to use it inside a with statement that auto deletes
    the file after the with block closes


    Parameters
    ----------------------------------------------------------------------------
    :param args, kwargs:
        arguments passed to mkstemp


    :return: int, str
    ----------------------------------------------------------------------------
        the file descriptor and the file path of the created temporary file
    """
    fd, tmp_file_name = mkstemp(*args, **kwargs)
    try:
        yield (fd, tmp_file_name)
    finally:
        os_remove(tmp_file_name)


@contextmanager
def open_fd(*args, **kwargs):
    """
    Wraps os.fdopen to use it inside a with statement that auto closes the
    generated file descriptor after the with block closes


    Parameters
    ----------------------------------------------------------------------------
    :param args, kwargs:
        arguments passed to os.fdopen


    :return: TextIOWrapper
    ----------------------------------------------------------------------------
        open file object connected to the file descriptor
    """
    fp = os_fdopen(*args, **kwargs)
    try:
        yield fp
    finally:
        fp.close()


def term_proc(proc, timeout):
    """
    helper function to terminate a process


    Parameters
    ----------------------------------------------------------------------------
    :param proc: process object
        the process object that is going to be terminated

    :param timeout: dictionary
        a dictionary (used as object reference) to set a flag that indicates
        that the process is going to be terminated
    """
    timeout["value"] = True
    proc.terminate()


def uncrustify(unc_bin_path, cfg_file_path, unformatted_file_path,
               lang=None, debug_file=None, check=False):
    """
    executes Uncrustify and captures its stdout


    Parameters
    ----------------------------------------------------------------------------
    :param unc_bin_path: str
        path to the Uncrustify binary

    :param cfg_file_path: str
        path to a config file for Uncrustify

    :param unformatted_file_path: str
        path to a file that is going to be formatted

    :param lang: str / None
        Uncrustifys -l argument

    :param debug_file: str / None
        Uncrustifys -p argument

    :param check: bool
        Used to control whether Uncrustifys --check is going to be used


    :return: str / None
    ----------------------------------------------------------------------------
        returns the stdout from Uncrustify or None if the process takes to much
        time (set to 5 sec)
    """

    args = [unc_bin_path, "-q", "-c", cfg_file_path, '-f',
            unformatted_file_path]
    if lang:
        args.extend(("-l", lang))
    if debug_file:
        args.extend(('-p', debug_file))
    if check:
        args.append('--check')

    proc = Popen(args, stdout=PIPE, stderr=PIPE)

    timeout = {"value": False}
    timer = Timer(5, term_proc, [proc, timeout])
    timer.start()

    output_b, error_txt_b = proc.communicate()

    timer.cancel()

    if timeout["value"]:
        print("uncrustify proc timeout: %s" % ' '.join(args), file=stderr)
        return None

    error = error_txt_b.decode("UTF-8")
    if error:
        print("Uncrustify %s stderr:\n %s" % (unformatted_file_path, error),
              file=stderr)

    return output_b


def same_expected_generated(formatted_path, unc_bin_path, cfg_file_path,
                            input_path, lang=None):
    """
    Calls uncrustify and compares its generated output with the content of a
    file


    Parameters
    ----------------------------------------------------------------------------
    :param formatted_path: str
        path to a file containing the expected content

    :params unc_bin_path, cfg_file_path, input_path, lang: str, str, str,
                                                           str / None
        see uncrustify()


    :return: bool
    ----------------------------------------------------------------------------
        True if the strings match, False otherwise
    """

    expected_string = ''
    with open(formatted_path, 'rb') as f:
        expected_string = f.read()

    formatted_string = uncrustify(unc_bin_path, cfg_file_path, input_path, lang)

    return True if formatted_string == expected_string else False


def process_uncrustify(args):
    """
    special wrapper for same_expected_generated()

    accesses global var(s): RESTULTSFLAG


    Parameters
    ----------------------------------------------------------------------------
    :param args: list / tuple< int, ... >
        this function is intended to be called by multiprocessing.pool.map()
        therefore all arguments are inside a list / tuple:
            id: int
                an index number needed by the caller to differentiate runs

            other parameters:
                see same_expected_generated()


    :return: tuple< int, RESTULTSFLAG >
    ----------------------------------------------------------------------------
        returns a tuple containing the id and a RESTULTSFLAG, REMOVE if both
        strings are equal, KEEP if not
    """

    id = args[0]
    res = same_expected_generated(*args[1:])

    return id, RESTULTSFLAG.REMOVE if res else RESTULTSFLAG.KEEP


def write_config_file(args):
    """
    Writes all but one excluded option into a config file


    Parameters
    ----------------------------------------------------------------------------
    :param args: list / tuple< list< tuple< str, str > >, str, int >
        this function is intended to be called by multiprocessing.pool.map()
        therefore all arguments are inside a list / tuple:

            config_list: list< tuple< str, str > >
                a list of tuples containing option names and values

            tmp_dir: str
                path to a directory in which the config file is going to be
                written

            exclude_idx: int
                index for an option that is not going to be written into the
                config file
    """

    config_list, tmp_dir, exclude_idx = args

    with open("%s%suncr-%d.cfg" % (tmp_dir, os_path_sep, exclude_idx),
              'w') as f:
        print_config(config_list, target_file_obj=f, exclude_idx=exclude_idx)


def write_config_file2(args):
    """
    Writes two option lists into a config file


    Parameters
    ----------------------------------------------------------------------------
    :param args: list< tuple< str, str > >,
                 list< tuple< str, str > >, str, int
        this function is intended to be called by multiprocessing.pool.map()
        therefore all arguments are inside a list / tuple:

            config_list: list< tuple< str, str > >
                the first list of tuples containing option names and values

            test_list: list< tuple< str, str > >
                the second list of tuples containing option names and values

            tmp_dir: str
                path to a directory in which the config file is going to be
                written

            idx: int
                index that is going to be used for the filename
    """

    config_list0, config_list1, tmp_dir, idx = args

    with open("%s%suncr-r-%d.cfg" % (tmp_dir, os_path_sep, idx), 'w') as f:
        print_config(config_list0, target_file_obj=f)
        print("", end='\n', file=f)
        print_config(config_list1, target_file_obj=f)


def gen_multi_combinations(elements, N):
    """
    generator function that generates, based on a set of elements, all
    combinations of 1..N elements


    Parameters
    ----------------------------------------------------------------------------
    :param elements: list / tuple
        a list of elements from which the combinations will be generated

    :param N:
        the max number of element in a combination


    :return: list
    ----------------------------------------------------------------------------
        yields a single combination of the elements

    >>> gen_multi_combinations(["a", "b", "c"], 3)
    (a); (b); (c); (a,b); (a,c); (b,c); (a,b,c)
    """

    fields = len(elements)
    if N > fields:
        raise Exception("Error: N > len(options)")
    if N <= 0:
        raise Exception("Error: N <= 0")

    for n in range(1, N + 1):
        yield combinations(elements, n)


def add_back(unc_bin_path, input_files, formatted_files, langs, options_r,
             options_k, tmp_dir):
    """
    lets Uncrustify format files with generated configs files until all
    formatted files match their according expected files.

    Multiple config files are generated based on a (base) list of Uncrustify
    options combined with additional (new) options derived from combinations of
    another list of options.


    accesses global var(s): RESTULTSFLAG


    Parameters
    ----------------------------------------------------------------------------
    :param unc_bin_path: str
        path to the Uncrustify binary

    :param input_files: list / tuple< str >
        a list containing paths to a files that are going to be formatted

    :param formatted_files: list / tuple< str >
        a list containing paths to files containing the expected contents

    :param langs: list / tuple< str > / None
        a list of languages the files, used as Uncrustifys -l argument
        can be None or shorter than the amount of provided files

    :param options_r: list< tuple< str, str > >
        the list of options from which combinations will be derived

    :param options_k: list< tuple< str, str > >
        the (base) list of Uncrustify options

    :param tmp_dir: str
        the directory in which the config files will be written to


    :return: list< tuple< str, str > > / None
    ----------------------------------------------------------------------------
        list of additional option that were needed to generate matching file
        contents
    """

    lang_max_idx = -1 if langs is None else len(langs) - 1
    file_len = len(input_files)

    if len(formatted_files) != file_len:
        raise Exception("len(input_files) != len(formatted_files)")

    for m_combination in gen_multi_combinations(options_r, len(options_r)):
        for idx, (r_combination) in enumerate(m_combination):
            write_config_file2((options_k, r_combination, tmp_dir, idx))

            cfg_file_path = "%s%suncr-r-%d.cfg" % (tmp_dir, os_path_sep, idx)
            res = []

            for file_idx in range(file_len):
                lang = None if idx > lang_max_idx else langs[file_idx]

                r = process_uncrustify(
                    (0, formatted_files[file_idx], unc_bin_path, cfg_file_path,
                     input_files[file_idx], lang))
                res.append(r[1])

            # all files, flag = remove -> option can be removed -> equal output
            if res.count(RESTULTSFLAG.REMOVE) == len(res):
                return r_combination
    return None


def sanity_raw_run(args):
    """
    wrapper for same_expected_generated(), prints error message if the config
    file does not generate the expected result

    Parameters
    ----------------------------------------------------------------------------
    :param args:
        see same_expected_generated


    :return:
    ----------------------------------------------------------------------------
        see same_expected_generated
    """
    res = same_expected_generated(*args)

    if not res:
        formatted_file_path = args[0]
        config_file_path = args[2]
        input_file_path = args[3]

        print("\nprovided config does not create formatted source file:\n"
              "    %s\n    %s\n->| %s"
              % (input_file_path, config_file_path, formatted_file_path),
              file=stderr)
    return res


def sanity_run(args):
    """
    wrapper for same_expected_generated(), prints error message if the config
    file does not generate the expected result


    Parameters
    ----------------------------------------------------------------------------
    :param args:
        see same_expected_generated


    :return:
    ----------------------------------------------------------------------------
        see same_expected_generated
    """
    res = same_expected_generated(*args)

    if not res:
        formatted_file_path = args[0]
        input_file_path = args[3]

        print("\ngenerated config does not create formatted source file:\n"
              "    %s\n    %s"
              % (input_file_path, formatted_file_path), file=stderr)
    return res


def sanity_run_splitter(uncr_bin, config_list, input_files, formatted_files,
                        langs, tmp_dir, jobs):
    """
    writes config option into a file and tests if every input file is formatted
    so that is matches the content of the according expected file


    Parameters
    ----------------------------------------------------------------------------
    :param uncr_bin: str
        path to the Uncrustify binary

    :param config_list: list< tuple< str, str > >
        a list of tuples containing option names and values

    :param input_files: list / tuple< str >
        a list containing paths to a files that are going to be formatted

    :param formatted_files: list / tuple< str >
        a list containing paths to files containing the expected contents

    :param langs: list / tuple< str > / None
        a list of languages the files, used as Uncrustifys -l argument
        can be None or shorter than the amount of provided files

    :param tmp_dir: str
        the directory in which the config files will be written to

    :param jobs: int
        number of processes to use


    :return: bool
    ----------------------------------------------------------------------------
        True if all files generate correct results, False oterhwise
    """

    file_len = len(input_files)
    if len(formatted_files) != file_len:
        raise Exception("len(input_files) != len(formatted_files)")

    gen_cfg_path = path_join(tmp_dir, "gen.cfg")
    with open(gen_cfg_path, 'w') as f:
        print_config(config_list, target_file_obj=f)

    lang_max_idx = -1 if langs is None else len(langs) - 1
    args = []

    for idx in range(file_len):
        lang = None if idx > lang_max_idx else langs[idx]

        args.append((formatted_files[idx], uncr_bin, gen_cfg_path,
                     input_files[idx], lang))

    pool = Pool(processes=jobs)
    sr = pool.map(sanity_run, args)

    return False not in sr


def print_config(config_list, target_file_obj=stdout, exclude_idx=()):
    """
    prints config options into a config file


    Parameters
    ----------------------------------------------------------------------------
    :param config_list: list< tuple< str, str > >
        a list containing pairs of option names and option values

    :param target_file_obj: file object
        see file param of print()

    :param exclude_idx: int / list< int >
        index of option(s) that are not going to be printed
    """

    if not config_list:
        return
    config_list_len = len(config_list)

    # check if exclude_idx list is empty -> assign len
    if type(exclude_idx) in (list, tuple) and not exclude_idx:
        exclude_idx = [config_list_len]
    else:
        # sort it, unless it is an int -> transform into a list
        try:
            exclude_idx = sorted(exclude_idx)
        except TypeError:
            exclude_idx = [exclude_idx]

    # extracted first loop round:
    # do not print '\n' for the ( here non-existing) previous line
    if exclude_idx[0] != 0:
        print("%s = %s" % (config_list[0][0].ljust(31, ' '), config_list[0][1]),
              end='', file=target_file_obj)
    # also print space if a single option was provided and it is going to be
    # excluded. This is done in order to be able to differentiate between
    # --empty-nochange and the case where all options can be removed
    elif config_list_len == 1:
        print(' ', end='', file=target_file_obj)
        return

    start_idx = 1
    for end in exclude_idx:
        end = min(end, config_list_len)

        for idx in range(start_idx, end):
            print("\n%s = %s"
                  % (config_list[idx][0].ljust(31, ' '), config_list[idx][1]),
                  end='', file=target_file_obj)

        start_idx = min(end + 1, config_list_len)

    # after
    for idx in range(start_idx, config_list_len):
        print("\n%s = %s"
              % (config_list[idx][0].ljust(31, ' '), config_list[idx][1]),
              end='', file=target_file_obj)


def get_non_default_options(unc_bin_path, cfg_file_path):
    """
    calls Uncrustify to generate a debug file from which a config only with
    non default valued options are extracted

    accesses global var(s): NULL_DEV


    Parameters
    ----------------------------------------------------------------------------
    :param unc_bin_path: str
        path to the Uncrustify binary

    :param cfg_file_path: str
        path to a config file for Uncrustify


    :return: list< str >
    ----------------------------------------------------------------------------
        amount of lines in the provided and shortened config
    """
    lines = []

    with make_raw_temp_file(suffix='.unc') as (fd, file_path):
        # make debug file
        uncrustify(unc_bin_path, cfg_file_path, NULL_DEV, debug_file=file_path,
                   check=True)

        # extract non comment lines -> non default config lines
        with open_fd(fd, 'r') as fp:
            lines = fp.read().splitlines()
            lines = [line for line in lines if not line[:1] == '#']

    return lines


def parse_config_file(file_obj):
    """
    Reads in a Uncrustify config file


    Parameters
    ----------------------------------------------------------------------------
    :param file_obj:
        the file object of an opened config file


    :return: list< tuple< str, str > >
    ----------------------------------------------------------------------------
        a list containing pairs of option names and option values
    """
    # dict used to only save the last option setting if the same option occurs
    # multiple times, without this:
    #   optionA0 can be removed because optionA1 = s0, and
    #   optionA1 can be removed because optionA0 = s0
    # -> optionA0, optionA1 are both removed
    config_map = OrderedDict()

    # special keys may not have this limitation, as for example
    # 'set x y' and 'set x z' do not overwrite each other
    special_keys = {'macro-open', 'macro-else', 'macro-close', 'set', 'type',
                    'file_ext', 'define'}
    special_list = []

    for line in file_obj:
        # cut comments
        pound_pos = line.find('#')
        if pound_pos != -1:
            line = line[:pound_pos]

        split_pos = line.find('=')
        if split_pos == -1:
            split_pos = line.find(' ')
        if split_pos == -1:
            continue

        key = line[:split_pos].strip()
        value = line[split_pos + 1:].strip()

        if key in special_keys:
            special_list.append((key, value))
        else:
            config_map[key] = value

    config_list = list(config_map.items())
    config_list += special_list

    return config_list


def count_lines(file_path):
    """
    returns the count of lines in a file by counting '\n' chars

    Parameters
    ----------------------------------------------------------------------------
    :param file_path: str
        file in which the lines will be counted


    :return: int
    ----------------------------------------------------------------------------
        number a lines
    """
    in_count = 0
    with open(file_path, 'r') as f:
        in_count = f.read().count('\n') + 1
    return in_count


def reduce(options_list):
    """
    Reduces the given options to a minimum

    accesses global var(s): FLAGS, RESTULTSFLAG, ERROR_CODE

    Parameters
    ----------------------------------------------------------------------------
    :param options_list: list< tuple< str, str > >
        the list of options that are going to be reduced

    :return: int, list< tuple< str, str > >
        status return code, reduced options
    """
    config_list_len = len(options_list)
    ret_flag = ERROR_CODE.NONE

    file_count = len(FLAGS.input_file_path)
    lang_max_idx = -1 if FLAGS.lang is None else len(FLAGS.lang) - 1

    pool = Pool(processes=FLAGS.jobs)
    with make_temp_directory() as tmp_dir:
        # region sanity run ----------------------------------------------------
        args = []
        for idx in range(file_count):
            lang = None if idx > lang_max_idx else FLAGS.lang[idx]

            args.append((FLAGS.formatted_file_path[idx],
                         FLAGS.uncrustify_binary_path, FLAGS.config_file_path,
                         FLAGS.input_file_path[idx], lang))
        sr = pool.map(sanity_raw_run, args)
        del args[:]

        if False in sr:
            return ERROR_CODE.SANITY0, []
        del sr[:]

        # endregion
        # region config generator loop -----------------------------------------
        args = []

        for e_idx in range(config_list_len):
            args.append((options_list, tmp_dir, e_idx))
        pool.map(write_config_file, args)

        del args[:]

        # endregion
        # region main loop -----------------------------------------------------
        args = []
        jobs = config_list_len * file_count

        for idx in range(jobs):
            file_idx = idx // config_list_len
            option_idx = idx % config_list_len

            cfg_file_path = "%s%suncr-%d.cfg" \
                            % (tmp_dir, os_path_sep, option_idx)
            lang = None if idx > lang_max_idx else FLAGS.lang[file_idx]

            args.append((idx, FLAGS.formatted_file_path[file_idx],
                         FLAGS.uncrustify_binary_path, cfg_file_path,
                         FLAGS.input_file_path[file_idx], lang))

        results = pool.map(process_uncrustify, args)
        del args[:]
        # endregion
        # region clean results -------------------------------------------------
        option_flags = [RESTULTSFLAG.NONE] * config_list_len

        for r in results:
            idx = r[0]
            flag = r[1]

            option_idx = idx % config_list_len

            if option_flags[option_idx] == RESTULTSFLAG.KEEP:
                continue

            option_flags[option_idx] = flag
        del results[:]
        # endregion

        options_r = [options_list[idx] for idx, x in enumerate(option_flags)
                     if x == RESTULTSFLAG.REMOVE]
        options_list = [options_list[idx] for idx, x in enumerate(option_flags)
                        if x == RESTULTSFLAG.KEEP]

        del option_flags[:]

        # region sanity run ----------------------------------------------------
        # options can be removed one at a time generating appropriate results,
        # oddly enough sometimes a config generated this way can fail when a
        # combination of multiple options is missing
        s_flag = True
        if options_r:
            s_flag = sanity_run_splitter(
                FLAGS.uncrustify_binary_path, options_list,
                FLAGS.input_file_path, FLAGS.formatted_file_path, FLAGS.lang,
                tmp_dir, FLAGS.jobs)

        if not s_flag:
            ret_flag = ERROR_CODE.SANITY1
            print("\n\nstumbled upon complex option dependencies in \n"
                  "    %s\n"
                  "trying to add back minimal amount of removed options\n"
                  % FLAGS.config_file_path, file=stderr)

            ret_options = add_back(
                FLAGS.uncrustify_binary_path, FLAGS.input_file_path,
                FLAGS.formatted_file_path, FLAGS.lang, options_r,
                options_list, tmp_dir)

            if ret_options:
                options_list.extend(ret_options)

                s_flag = sanity_run_splitter(
                    FLAGS.uncrustify_binary_path, options_list,
                    FLAGS.input_file_path, FLAGS.formatted_file_path,
                    FLAGS.lang, tmp_dir, FLAGS.jobs)

                if s_flag:
                    print("Success!", file=stderr)
                    ret_flag = ERROR_CODE.NONE
                    # endregion
    return ret_flag, options_list if ret_flag == ERROR_CODE.NONE else []


def reduce_mode():
    """
    the mode that minimizes a config file as much as possible

    accesses global var(s): FLAGS, ERROR_CODE
    """
    ret_flag = ERROR_CODE.NONE
    option_list = {}

    # gen & parse non default config
    lines = get_non_default_options(FLAGS.uncrustify_binary_path,
                                    FLAGS.config_file_path)
    option_list = parse_config_file(lines)
    config_list_len = len(option_list)

    config_lines_init = count_lines(FLAGS.config_file_path)
    config_lines_ndef = len(lines)
    del lines[:]

    # early return if all options are already removed at this point
    if config_list_len == 0:
        if not FLAGS.empty_nochange \
                or (config_lines_init - config_lines_ndef) > 0:
            if not FLAGS.quiet:
                print("\n%s" % '# '.ljust(78, '-'))

            print(" ")

            if not FLAGS.quiet:
                print("%s" % '# '.ljust(78, '-'))
                print("# initial config lines: %d,\n"
                      "# default options and unneeded lines: %d,\n"
                      "# unneeded options: 0,\n"
                      "# kept options: 0"
                      % (config_lines_init, config_lines_init))
        print("ret_flag: 0", file=stderr)
        return ERROR_CODE.NONE

    # gen reduced options
    config_lines_redu = -1
    for i in range(FLAGS.passes):
        old_config_lines_redu = config_lines_redu

        ret_flag, option_list = reduce(option_list)
        config_lines_redu = len(option_list)

        if ret_flag != ERROR_CODE.NONE \
                or config_lines_redu == old_config_lines_redu:
            break

    if ret_flag == ERROR_CODE.NONE:
        # use the debug file trick again to get correctly sorted options
        with make_raw_temp_file(suffix='.unc') as (fd, file_path):
            with open_fd(fd, 'w') as f:
                print_config(option_list, target_file_obj=f)

            lines = get_non_default_options(FLAGS.uncrustify_binary_path,
                                            file_path)
            option_list = parse_config_file(lines)

        # print output + stats
        if not FLAGS.empty_nochange or config_lines_ndef != config_lines_redu:
            if not FLAGS.quiet:
                print("\n%s" % '# '.ljust(78, '-'))

            print_config(option_list)

            if not FLAGS.quiet:
                print("\n%s" % '# '.ljust(78, '-'))
                print("# initial config lines: %d,\n"
                      "# default options and unneeded lines: %d,\n"
                      "# unneeded options: %d,\n"
                      "# kept options: %d"
                      % (config_lines_init,
                         config_lines_init - config_lines_ndef,
                         config_lines_ndef - config_lines_redu,
                         config_lines_redu))

    print("ret_flag: %d" % ret_flag, file=stderr)
    return ret_flag


def no_default_mode():
    """
    the mode removes all unnecessary lines and options with default values

    accesses global var(s): FLAGS, ERROR_CODE
    """

    lines = get_non_default_options(FLAGS.uncrustify_binary_path,
                                    FLAGS.config_file_path, )
    config_lines_ndef = len(lines)
    config_lines_init = count_lines(FLAGS.config_file_path)

    if not FLAGS.empty_nochange or (config_lines_ndef != config_lines_init):
        if not FLAGS.quiet:
            print("%s" % '# '.ljust(78, '-'))

        options_str = '\n'.join(lines)
        if not options_str:
            print(" ")
        else:
            print(options_str, file=stdout)

        if not FLAGS.quiet:
            print("%s" % '# '.ljust(78, '-'))
            print("# initial config lines: %d,\n"
                  "# default options and unneeded lines: %d,\n"
                  % (config_lines_init, config_lines_init - config_lines_ndef))

    return ERROR_CODE.NONE


def main():
    """
    calls the mode that was specified by the -m script argument,
    defaults to reduce_mode if not provided or unknown mode

    accesses global var(s): MODES, FLAGS


    :return: int
    ----------------------------------------------------------------------------
        return code
    """
    if FLAGS.mode == MODES[1]:
        return no_default_mode()

    return reduce_mode()


def valid_file(arg_parser, *args):
    """
    checks if on of the provided paths is a file


    Parameters
    ----------------------------------------------------------------------------
    :param arg_parser:
        argument parser object that is called if no file is found

    :param args: list< str >
        a list of file path that is going to be checked


    :return: str
    ----------------------------------------------------------------------------
        path to an existing file
    """
    arg = None
    found_flag = False
    for arg in args:
        if exists(arg):
            found_flag = True
            break
    if not found_flag:
        arg_parser.error("file(s) do not exist: %s" % args)

    return arg


if __name__ == "__main__":
    """
    parses all script arguments and calls main()

    accesses global var(s): FLAGS, ERROR_CODE, MODES
    """
    arg_parser = argparse.ArgumentParser()

    group_general = arg_parser.add_argument_group(
        'general options', 'Options used by both modes')

    group_general.add_argument(
        '-q', '--quiet',
        default=False,
        action='store_true',
        help='Whether or not messages, other than the actual config output, '
             'should be printed to stdout.'
    )
    group_general.add_argument(
        '--empty-nochange',
        default=False,
        action='store_true',
        help='Do not print anything to stdout if no options could be removed'
    )
    group_general.add_argument(
        '-m', '--mode',
        type=str,
        choices=MODES,
        default=MODES[0],
        help="The script operation mode. Defaults to '%s'" % MODES[0]
    )
    group_general.add_argument(
        '-b', '--uncrustify_binary_path',
        metavar='<path>',
        type=lambda x: valid_file(
            arg_parser, x,
            "../build/uncrustify.exe",
            "../build/Debug/uncrustify",
            "../build/Debug/uncrustify.exe",
            "../build/Release/uncrustify",
            "../build/Release/uncrustify.exe"),
        default="../build/uncrustify",
        help="The Uncrustify binary file path. Is searched in known locations "
             "in the 'Uncrustify/build/' directory if no <path> is provided."
    )
    group_general.add_argument(
        '-c', '--config_file_path',
        metavar='<path>',
        type=lambda x: valid_file(arg_parser, x),
        required=True,
        help='Path to the config file.'
    )

    group_reduce = arg_parser.add_argument_group(
        'reduce mode', 'Options to reduce configuration file options')

    group_reduce.add_argument(
        '-i', '--input_file_path',
        metavar='<path>',
        type=lambda x: valid_file(arg_parser, x),
        nargs='+',
        action='append',
        help="Path to the unformatted source file. "
             "Required if mode '%s' is used" % MODES[0]
    )
    group_reduce.add_argument(
        '-f', '--formatted_file_path',
        metavar='<path>',
        type=lambda x: valid_file(arg_parser, x),
        nargs='+',
        action='append',
        help="Path to the formatted source file. "
             "Required if mode '%s' is used" % MODES[0]
    )
    group_reduce.add_argument(
        '-l', '--lang',
        metavar='<str>',
        nargs='+',
        required=False,
        action='append',
        help='Uncrustify processing language for each input file'
    )
    group_reduce.add_argument(
        '-j', '--jobs',
        metavar='<nr>',
        type=int,
        default=cpu_count(),
        help='Number of concurrent jobs.'
    )
    group_reduce.add_argument(
        '-p', '--passes',
        metavar='<nr>',
        type=int,
        default=5,
        help='Max. number of cleaning passes.'
    )

    group_no_default = arg_parser.add_argument_group(
        'no-default mode', 'Options to remove configuration file option with '
                           'default values: ~~_Currently only the general'
                           ' options are used for this mode_~~')
    FLAGS, unparsed = arg_parser.parse_known_args()

    if FLAGS.lang is not None:
        FLAGS.lang = [j for i in FLAGS.lang for j in i]

    if FLAGS.mode == MODES[0]:
        if not FLAGS.input_file_path or not FLAGS.formatted_file_path:
            arg_parser.error("Flags -f and -i are required in Mode '%s'!"
                             % MODES[0])
            sys_exit(ERROR_CODE.FLAGS)

        # flatten 2 dimensional args: -f p -f p -f p -f p0 p1 p2 -> [[],[], ...]
        FLAGS.input_file_path = [j for i in FLAGS.input_file_path for j in i]

        FLAGS.formatted_file_path = [j for i in
                                     FLAGS.formatted_file_path for j in i]

        if len(FLAGS.input_file_path) != len(FLAGS.formatted_file_path):
            print("Unequal amount of input and formatted file paths.",
                  file=stderr)
            sys_exit(ERROR_CODE.FLAGS)

    sys_exit(main())
