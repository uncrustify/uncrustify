#!/bin/python
from __future__ import print_function  # python >= 2.6, chained 'with' >= 2.7

from os.path import dirname, abspath
from os import fdopen as os_fdopen, remove as os_remove, name as os_name
from shutil import copy2
from subprocess import Popen, PIPE
from sys import exit as sys_exit, stderr
from tempfile import mkstemp
from contextlib import contextmanager
from threading import Timer
import re


ROOT_DIR = dirname(dirname(abspath(__file__)))

# ==============================================================================

FILE_BINDINGS = "%s/src/uncrustify_emscripten.cpp" % ROOT_DIR
FILE_TS = "%s/emscripten/libUncrustify.d.ts" % ROOT_DIR

REGION_START = "region enum bindings"
REGION_END = "endregion enum bindings"

''' Enums which values need to be updated in the binding code '''
ENUMS_INFO = [
    {
        'name': 'uncrustify_options',
        'substitute_name': 'Option',
        'filepath': '%s/src/options.h' % ROOT_DIR,
        'extra_arg': []
    },
    {
        'name': 'uncrustify_groups',
        'substitute_name': 'Group',
        'filepath': '%s/src/options.h' % ROOT_DIR,
        'extra_arg': []
    },
    {
        'name': 'argtype_e',
        'substitute_name': 'Argtype',
        'filepath': '%s/src/options.h' % ROOT_DIR,
        'extra_arg': []
    },
    {
        'name': 'log_sev_t',
        'substitute_name': 'LogSev',
        'filepath': '%s/src/log_levels.h' % ROOT_DIR,
        'extra_arg': []
    },
    {
        'name': 'c_token_t',
        'substitute_name': 'Token',
        'filepath': '%s/src/token_enum.h' % ROOT_DIR,
        'extra_arg': []
    },
    {
        'name': 'lang_flag_e',
        'substitute_name': 'LangFlag',
        'filepath': '%s/src/uncrustify_types.h' % ROOT_DIR,
        'extra_arg': ["-extra-arg=-std=c++1z", "-extra-arg=-DEMSCRIPTEN"]
    },
]

# ==============================================================================

NULL_DEV = "/dev/null" if os_name != "nt" else "nul"


@contextmanager
def make_raw_temp_file(*args, **kwargs):
    fd, tmp_file_name = mkstemp(*args, **kwargs)
    try:
        yield (fd, tmp_file_name)
    finally:
        os_remove(tmp_file_name)


@contextmanager
def open_fd(*args, **kwargs):
    fp = os_fdopen(*args, **kwargs)
    try:
        yield fp
    finally:
        fp.close()


def term_proc(proc, timeout):
    """
    helper function terminate a process if a timer times out

    :param proc: the process object that is going to be terminated
    :param timeout: value that will be set to indicate termination
    """
    timeout["value"] = True
    proc.terminate()


def proc_output(args, timeout_sec=10):
    """
    grabs output from called program
    :param args: string array containing program name and program arguments
    :param timeout_sec: max sec the program can run without being terminated
    :return: utf8 decoded program output in a string
    """
    proc = Popen(args, stdout=PIPE)

    timeout = {"value": False}
    if timeout_sec is not None:
        timeout = {"value": False}
        timer = Timer(timeout_sec, term_proc, [proc, timeout])
        timer.start()

    output_b, error_txt_b = proc.communicate()

    if timeout_sec is not None:
        timer.cancel()

    output = output_b.decode("UTF-8")

    if timeout["value"]:
        print("proc timeout: %s" % ' '.join(args), file=stderr)

    return output if not timeout["value"] else None


def get_enum_lines(enum_info):
    """
    extracts enum values from a file via clang-check

    :param enum_info: dict with:
                        'name' (name of the enum),
                        'filepath' (file containing the enum definition),
                        'extra_arg' (extra arguments passed to clang-check)
    :return: list containing enum values
    """
    cut_len = len(enum_info['name']) + 2  # "$enumName::"

    proc_args = ["clang-check", "-p=%s" % NULL_DEV, enum_info['filepath'],
                 "-ast-dump", '-ast-dump-filter=%s::' % enum_info['name']]
    proc_args += enum_info['extra_arg']

    output = proc_output(proc_args)
    if output is None or len(output) == 0:
        print("%s: empty clang-check return" % get_enum_lines.__name__,
              file=stderr)
        return ()

    reg_obj = re.compile("(%s::\w+)" % enum_info['name'])

    lines = [m.group(1)[cut_len:] for l in output.splitlines()
             for m in [re.search(reg_obj, l)] if m]
    if len(lines) == 0:
        print("%s: no enum_info names found" % get_enum_lines.__name__,
              file=stderr)
        return ()
    return lines


def write_ts(opened_file_obj, enum_info):
    """
    writes enum values in a specific typescript d.ts file format

    :param opened_file_obj: opened file file object (with write permissions)
    :param enum_info: dict with:
                        'name' (name of the enum),
                        'substitute_name' (substitute name for the enum),
                        'filepath' (file containing the enum definition),
                        'extra_arg' (extra arguments passed to clang-check)
    :return: False on failure else True
    """
    lines = get_enum_lines(enum_info)
    if len(lines) == 0:
        return False

    opened_file_obj.write('    export interface %s extends EmscriptenEnumType\n'
                          '    {\n'
                          % enum_info['substitute_name'])
    for line in lines:
        opened_file_obj.write('        %s : EmscriptenEnumTypeObject;\n' % line)
    opened_file_obj.write('    }\n\n')
    return True


def write_bindings(opened_file_obj, enum_info):
    """
    writes enum values in a specific emscripten embind enum bindings format

    :param opened_file_obj: opened file file object (with write permissions)
    :param enum_info: dict with:
                        'name' (name of the enum),
                        'filepath' (file containing the enum definition),
                        'extra_arg' (extra arguments passed to clang-check)
    :return: False on failure else True
    """
    lines = get_enum_lines(enum_info)
    if len(lines) == 0:
        return False

    opened_file_obj.write('   enum_<%s>(STRINGIFY(%s))'
                          % (enum_info['name'], enum_info['name']))
    for line in lines:
        opened_file_obj.write('\n      .value(STRINGIFY(%s), %s)'
                              % (line, line))
    opened_file_obj.write(';\n\n')
    return True


def update_file(file_path, writer_func, enums_info):
    """
    reads in a file and replaces old enum value in a region, which is defined by
    region start and end string, with updated ones

    :param file_path: file in which the replacement will be made
    :param writer_func: name of the function that will be called to write new
                        content
    :param enums_info:list of dicts each containing:
                    'name' (name of the enum),
                    'substitute_name' (substitute name for the enum),
                    'filepath' (file containing the enum definition),
                    'extra_arg' (extra arguments passed to clang-check)
    :return: False on failure else True
    """
    in_target_region = False

    reg_obj_start = re.compile(".*%s$" % REGION_START)
    reg_obj_end = re.compile(".*%s$" % REGION_END)
    reg_obj = reg_obj_start

    with make_raw_temp_file(suffix='.unc') as (fd, tmp_file_path):
        with open(file_path, 'r') as fr, open_fd(fd, 'w') as fw:
            for line in fr:
                match = None if reg_obj is None else re.search(reg_obj, line)

                if match is None and not in_target_region:
                    fw.write(line)                # write out of region code

                elif match is not None and not in_target_region:
                    fw.write(line)                # hit the start region

                    in_target_region = True
                    reg_obj = reg_obj_end

                    for enum in enums_info:
                        succes_flag = writer_func(fw, enum)
                        if not succes_flag:       # abort, keep input file clean
                            return False

                elif match is None and in_target_region:
                    pass                          # ignore old binding code

                elif match and in_target_region:  # hit the endregion
                    fw.write(line)

                    in_target_region = False
                    reg_obj = None

        copy2(tmp_file_path, file_path)           # overwrite input file
        return True


def main():
    flag = update_file(FILE_BINDINGS, write_bindings, ENUMS_INFO)
    if not flag:
        return 1

    flag = update_file(FILE_TS, write_ts, ENUMS_INFO)
    if not flag:
        return 1

    return 0

if __name__ == "__main__":
    sys_exit(main())
