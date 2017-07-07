#! /usr/bin/env python
#
# Check the option usage.
# Make sure the union member matches the option type.
#
from os.path import dirname, join, abspath
from os import listdir, EX_OK, EX_DATAERR
from fnmatch import filter

# just use the first letter of the member name - should be unique
map_access_type = {
    'b': 'AT_BOOL',
    'a': 'AT_IARF',
    'n': 'AT_NUM',
    'u': 'AT_UNUM',
    'l': 'AT_LINE',
    't': 'AT_POS',
}
map_option_type = {}


# checks if while accessing the cpd.settings the right union accessor is used in the file
def check_file(file_path):
    problems = 0
    line_no = 0

    fd = open(file_path, 'r')
    for line in fd:
        line_no += 1

        pos_cpd_s = line.find('cpd.settings[UO_')
        pos_cpd_e = line[pos_cpd_s:].find(']')
        if pos_cpd_s > 0 and pos_cpd_e > 0:
            pos_option_s = pos_cpd_s + 13
            pos_option_e = pos_cpd_s + pos_cpd_e

            option = line[pos_option_s : pos_option_e]
            union_access = line[pos_option_e + 2]

            if option in map_option_type and union_access in map_access_type:
                if map_option_type[option] != map_access_type[union_access]:
                    print("%s [%d] %s should use %s not %s" % (file_path, line_no, option,
                                                               map_option_type[option], map_access_type[union_access]))
                    problems += 1
    return problems


def fill_map_option_type(file_path):
    # Read in all the options
    fd = open(file_path, 'r')
    for line in fd:
        if line.find('unc_add_option') > 0 and line.find('UO_') > 0:
            splits = line.split(',')
            if len(splits) >= 3:
                map_option_type[splits[1].strip()] = splits[2].strip()
    fd.close()


def main():
    src_dir = join(dirname(dirname(abspath(__file__))), 'src')
    fill_map_option_type(join(src_dir, 'options.cpp'))

    # Get a list of all the source files
    ld = listdir(src_dir)
    src_files = filter(ld, '*.cpp')
    src_files.extend(filter(ld, '*.h'))

    # Check each source file
    problems = 0
    for fn in src_files:
        problems += check_file(join(src_dir, fn))
    if problems == 0:
        print("No problems found")
        return EX_OK
    else:
        return EX_DATAERR

if __name__ == '__main__':
    exit(main())

