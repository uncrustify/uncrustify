#!/bin/python
from __future__ import print_function  # python >= 2.6
import argparse
import errno
import threading
import re

from os import makedirs, path, name as os_name, linesep as os_linesep, \
               sep as os_sep, fdopen
from subprocess import Popen, PIPE
from sys import exit as sys_exit, stderr, stdout
from shutil import rmtree
from multiprocessing import cpu_count
from math import floor
from tempfile import mkdtemp, mkstemp
from contextlib import contextmanager
from collections import OrderedDict
from threading import Timer

FLAGS = None
NULL_DEV = "/dev/null" if os_name != "nt" else "nul"


def enum(**enums):
    return type('Enum', (), enums)

RestultsFlag = enum(NONE=0, REMOVE=1, KEEP=2)


def thread_wrapper(func, args, res):
    res.append(func(*args))

def parse_config_file(file_data):
    """"parse config file data into config list"""
    # dict used to only save the last option setting if the same option occurs
    # multiple times, without this:
    #   optionA0 can be removed because optionA1 = s0, and
    #   optionA1 can be removed because optionA0 = s0
    # -> optionA0, optionA1 are both removed
    config_map = OrderedDict()

    # special keys may not have this limitation, as for example
    # 'set x y' and 'set x z' do not overwrite each other
    special_keys = {'macro-open' 'macro-else', 'macro-close', 'set', 'type',
                    'file_ext', 'define'}
    special_list = []

    # filter out comments
    lines = [line for line in file_data.splitlines() if not re.match(r'\s*#', line)]

    for line in lines:
        pound_pos = line.find('#')
        if pound_pos != -1:
            line = line[:pound_pos]

        split_pos = line.find('=')
        if split_pos == -1:
            split_pos = line.find(' ')
        if split_pos == -1:
            continue

        key = line[:split_pos].strip()
        value = line[split_pos+1:].strip()

        if key in special_keys:
            special_list.append((key, value))
        else:
            config_map[key] = value

    config_list = list(config_map.items())
    config_list += special_list

    return config_list


def read_config_file(file_path):
    with open(file_path, 'r') as f:
        return parse_config_file(f.read())


def term_proc(proc, timeout):
    timeout["value"] = True
    proc.terminate()


def uncrustify(unc_bin_path, cfg_path, unformatted_file_path, lang=None, debug_file=None, check=False):
    args = [unc_bin_path, "-c", cfg_path, '-f', unformatted_file_path, "-q"]
    if lang is not None:
        args.append("-l")
        args.append(lang)
    if debug_file is not None:
        args.append('-p')
        args.append(debug_file)
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
    if error is not None and len(error) > 0:
        print("Uncrustify error in %s: %s" % (unformatted_file_path, error),
              file=stderr)

    return output_b


def get_cfg_dump(unc_bin_path, cfg_path, tmp_dir):
    """Uses Uncrustify's debug dump to get the non-default option overrides with
    the include statements resolved.
    """
    fd, unc_fn = mkstemp(dir=tmp_dir, suffix='.unc')

    uncrustify(unc_bin_path, cfg_path, NULL_DEV, debug_file=unc_fn, check=True)

    with fdopen(fd, 'r') as fp:
        options_str = fp.read()

    # We are responsible to delete the file, but it will be done when the temp
    # directory will be removed

    return options_str


def process_uncrustify(unc_bin_path, input_path, formatted_path,
                       tmp_dir, cfg_idx, lang=None):
    cfg_file_path = "%s%suncr-%d.cfg" % (tmp_dir, os_sep, cfg_idx)

    with open(formatted_path, 'rb') as f:
        _expected_string = f.read()

    formatted_string = uncrustify(unc_bin_path, cfg_file_path, input_path, lang)

    # remove_flag: strings match without cfg_idx -> option_idx, return true
    return RestultsFlag.REMOVE if formatted_string == _expected_string \
        else RestultsFlag.KEEP


def split_process_uncrustify(start, end, unc_bin_path, input_files,
                             formatted_files, tmp_dir, options_len, langs=()):
    lang_max_idx = len(langs) - 1

    # boolean values don't work here as the span of start -> end can be smaller
    # than options_len, so ints are used: 0 (default) untouched
    #                                     1 can be removed
    #                                     2 cannot be removed
    remove_falgs = [RestultsFlag.NONE] * options_len

    for idx in range(start, end):
        file_idx = idx // options_len
        option_idx = idx % options_len

        lang = None if file_idx > lang_max_idx else langs[file_idx]

        # print("input: %s, option_id: %d" % (input_files[file_idx], option_idx))

        remove_flag = process_uncrustify(unc_bin_path, input_files[file_idx],
                                         formatted_files[file_idx], tmp_dir,
                                         option_idx, lang)

        if remove_falgs[option_idx] != RestultsFlag.KEEP:
            remove_falgs[option_idx] = remove_flag

    return remove_falgs


def write_config_file(config_list, tmp_dir, exclude_idx):
    config_list_len = len(config_list)

    with open("%s%suncr-%d.cfg" % (tmp_dir, os_sep, exclude_idx), 'w') as f:
        for idx in range(0, config_list_len):
            if idx == exclude_idx:
                continue

            f.write("%s = %s%s" % (config_list[idx][0], config_list[idx][1],
                                   os_linesep))


def split_write_config_file(start, end, config_list, tmp_dir):
    for exclude_idx in range(start, end):
        write_config_file(config_list, tmp_dir, exclude_idx)


def sanity_raw_run(uncrustify_binary_path, config_file_path,
                   input_file_path, formatted_file_path, lang=None):
    with open(formatted_file_path, 'rb') as f:
        expected_string = f.read()

    formatted_string = uncrustify(uncrustify_binary_path, config_file_path,
                                  input_file_path, lang)
    if formatted_string != expected_string:
        print("Provided config does not generate formatted source file: "
              "%s - %s" % (FLAGS.config_file_path, formatted_file_path),
              file=stderr)
        sys_exit(errno.EUSERS)


def sanity_run(uncrustify_binary_path, tmp_dir, config_list, remove_flags,
               input_file_path, formatted_file_path, lang=None):
    ret_flag = True
    gen_cfg_path = path.join(tmp_dir, "gen.cfg")

    with open(formatted_file_path, 'rb') as f:
        expected_string = f.read()

    with open(gen_cfg_path, 'w') as f:
        print_config(config_list, remove_flags, target_file=f)

    formatted_string = uncrustify(uncrustify_binary_path, gen_cfg_path,
                                  input_file_path, lang)

    if formatted_string != expected_string:
        print("Generated config does not create formatted source file: %s"
              % formatted_file_path, file=stderr)
        ret_flag = False

    return ret_flag


def print_config(config_list, remove_flags, flag=RestultsFlag.KEEP,
                target_file=stdout):
    config_list_len = len(config_list)
    loop_flag = False
    set_nl = False

    for idx in range(0, config_list_len):
        if remove_flags[idx] != flag:
            continue

        if set_nl:
            print('', end='\n', file=target_file)

        print("%s = %s" % (config_list[idx][0].ljust(31, ' '),
                           config_list[idx][1]), end='', file=target_file)
        set_nl = True
        loop_flag = True

    if config_list_len > 0 and not loop_flag:
        # print space, used in order to not confuse this with --empty-nochange
        # if all options can be removed
        print(' ', end='', file=target_file)


@contextmanager
def make_temp_directory():
    temp_dir = mkdtemp()
    try:
        yield temp_dir
    finally:
        rmtree(temp_dir)


def main():
    ret_flag = 0

    with make_temp_directory() as tmp_dir:
        config_list = parse_config_file(
            get_cfg_dump(FLAGS.uncrustify_binary_path,
                         FLAGS.config_file_path,
                         tmp_dir)) if FLAGS.resolve else \
            read_config_file(FLAGS.config_file_path)
        config_list_len = len(config_list)
        file_len = len(FLAGS.input_file_path)

        num_splits = FLAGS.jobs
        lang_max_idx = -1 if FLAGS.lang is None else len(FLAGS.lang) - 1

        # sanity run -----------------------------------------------------------
        for idx in range(file_len):
            lang = None if idx > lang_max_idx else FLAGS.lang[idx]

            sanity_raw_run(FLAGS.uncrustify_binary_path,
                           FLAGS.config_file_path,
                           FLAGS.input_file_path[idx],
                           FLAGS.formatted_file_path[idx],
                           lang)

        # config generator loop ------------------------------------------------
        threads = []
        steps = config_list_len
        split_size = steps / num_splits
        for i in range(num_splits):
            start = int(floor(i * split_size))
            end = int(floor((i + 1) * split_size)) if i + 1 != num_splits \
                else steps

            threads.append(
                threading.Thread(target=split_write_config_file,
                                 args=(start, end, config_list, tmp_dir)))
            threads[-1].start()

        for t in threads:
            t.join()
        del threads[:]

        # TODO: pooled threading might be better here
        # main loop ------------------------------------------------------------
        results = []
        threads = []
        steps = config_list_len * file_len
        split_size = steps / num_splits
        for i in range(num_splits):
            start = int(floor(i * split_size))
            end = int(floor((i+1) * split_size)) if i+1 != num_splits else steps

            threads.append(
                threading.Thread(target=thread_wrapper,
                                 args=(split_process_uncrustify, (start, end,
                                       FLAGS.uncrustify_binary_path,
                                       FLAGS.input_file_path,
                                       FLAGS.formatted_file_path,
                                       tmp_dir, config_list_len), results)))
            threads[-1].start()

        for t in threads:
            t.join()
        del threads[:]

        # gen results ----------------------------------------------------------
        remove_flags = [RestultsFlag.NONE] * config_list_len

        for o_idx in range(config_list_len):
            for r_idx in range(num_splits):
                # prevent overwrite on no result flag
                if results[r_idx][o_idx] == RestultsFlag.NONE:
                    continue

                remove_flags[o_idx] = results[r_idx][o_idx]

                # prevent future overwrites,
                # skip to the next option if KEEP flag was set
                if remove_flags[o_idx] == RestultsFlag.KEEP:
                    break

        removed_count = remove_flags.count(RestultsFlag.REMOVE)
        unprocessed_count = remove_flags.count(RestultsFlag.NONE)

        if removed_count > 0 or not FLAGS.empty_nochange:
            if not FLAGS.quiet:
                print("\n%s" % '# '.ljust(78, '-'))

            print_config(config_list, remove_flags)

            if not FLAGS.quiet:
                print("\n%s" % '# '.ljust(78, '-'))
                print("removed: %d, kept: %d, unprocessed: %d"
                      % (removed_count,
                         remove_flags.count(RestultsFlag.KEEP),
                         unprocessed_count))

        if unprocessed_count > 0:
            print("Error: unprocessed options != 0", file=stderr)
            ret_flag = errno.ERANGE

        # sanity run -----------------------------------------------------------
        # sometimes options are removed that should be kept, not sure if this
        # is a bug in this script or if that is something uncrustify related
        # 'sp_after_angle' for example can be devious sometimes
        if removed_count > 0:
            for idx in range(file_len):
                lang = None if idx > lang_max_idx else FLAGS.lang[idx]

                ret = sanity_run(FLAGS.uncrustify_binary_path, tmp_dir,
                                 config_list, remove_flags,
                                 FLAGS.input_file_path[idx],
                                 FLAGS.formatted_file_path[idx], lang)
                if not ret:
                    ret_flag = errno.EMFILE

            if ret_flag == errno.EMFILE:
                print("\nRemoved options:", file=stderr)
                print_config(config_list, remove_flags, flag=RestultsFlag.REMOVE,
                            target_file=stderr)
                print("\n\n", file=stderr)

    return ret_flag


def valid_file(arg_parser, *args):
    arg = None
    found_flag = False
    for arg in args:
        if path.exists(arg):
            found_flag = True
            break
    if not found_flag:
        arg_parser.error("file(s) do not exist: %s" % args)

    return arg


if __name__ == "__main__":
    arg_parser = argparse.ArgumentParser()
    arg_parser.add_argument(
        '-j', '--jobs',
        type=int,
        default=cpu_count(),
        help='Number of concurrent jobs.'
    )
    arg_parser.add_argument(
        '--empty-nochange',
        default=False,
        action='store_true',
        help='Do not print anything to stdout if no options could be removed'
    )
    arg_parser.add_argument(
        '-q', '--quiet',
        default=False,
        action='store_true',
        help='Whether or not messages, other than the actual config output, '
             'should be printed to stdout.'
    )
    arg_parser.add_argument(
        '-b', '--uncrustify_binary_path',
        type=lambda x: valid_file(arg_parser, x,
                                  '../build/uncrustify.exe',
                                  '../build/Debug/uncrustify',
                                  '../build/Debug/uncrustify.exe',
                                  '../build/Release/uncrustify',
                                  '../build/Release/uncrustify.exe',
                                  ),
        default='../build/uncrustify',
        help='Path to the Uncrustify binary file.'
    )
    arg_parser.add_argument(
        '-c', '--config_file_path',
        type=lambda x: valid_file(arg_parser, x),
        required=True,
        help='Path to the config file.'
    )
    arg_parser.add_argument(
        '-i', '--input_file_path',
        type=lambda x: valid_file(arg_parser, x),
        nargs='+',
        required=True,
        action='append',
        help='Path to the unformatted source file.'
    )
    arg_parser.add_argument(
        '-l', '--lang',
        nargs='*',
        action='append',
        help='Uncrustify processing language for each input file'
    )
    arg_parser.add_argument(
        '-f', '--formatted_file_path',
        type=lambda x: valid_file(arg_parser, x),
        nargs='+',
        required=True,
        action='append',
        help='Path to the formatted source file.'
    )
    arg_parser.add_argument(
        '-r', '--resolve',
        default=False,
        action="store_true",
        help="Resolve the config options with non-default values with debug dump"
    )
    FLAGS, unparsed = arg_parser.parse_known_args()

    # flatten 2 dimensional args: -f p -f p -f p -f p0 p1 p2 p3 -> [[],[], ...]
    FLAGS.input_file_path = [j for i in FLAGS.input_file_path for j in i]

    FLAGS.formatted_file_path = [j for i in
                                 FLAGS.formatted_file_path for j in i]
    if FLAGS.lang is not None:
        FLAGS.lang = [j for i in FLAGS.lang for j in i]

    if len(FLAGS.input_file_path) != len(FLAGS.formatted_file_path):
        print("Unequal amount of input and formatted file paths.", file=stderr)
        sys_exit(errno.EUSERS)

    sys_exit(main())
