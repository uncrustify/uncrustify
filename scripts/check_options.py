#! /usr/bin/env python
#
# Check the option usage.
# Make sure the union member matches the option type.
#
import sys, os, fnmatch

# just use the first letter of the member name - should be unique
opt_suffix = {
    'b' : 'AT_BOOL',
    'a' : 'AT_IARF',
    'n' : 'AT_NUM',
    'l' : 'AT_LINE',
    't' : 'AT_POS'
    }
opts = { }

def check_file (fn):
    problems = 0
    fd = open(fn, 'r')
    line_no = 0
    for line in fd:
        line_no = line_no + 1
        cpd = line.find('cpd.settings[UO_')
        if cpd > 0:
            sb = line[cpd:].find(']')
            opt = line[cpd + 13 : cpd + sb]
            mem = line[cpd + sb + 2]
            if opt in opts and mem in opt_suffix:
                if opts[opt] != opt_suffix[mem]:
                    print fn + '[%d]' % (line_no) , opt, 'should use', opts[opt], 'not', opt_suffix[mem]
                    problems += 1
    return problems

def main (argv):
    # Read in all the options
    of = open(os.path.join('src', 'options.cpp'), 'r');
    for line in of:
        if line.find('unc_add_option') > 0 and line.find('UO_') > 0:
            ps = line.split(',')
            if len(ps) >= 3:
                opts[ps[1].strip()] = ps[2].strip()
    of.close()

    # Get a list of all the source files
    ld = os.listdir('src')
    src_files = fnmatch.filter(ld, '*.cpp')
    src_files.extend(fnmatch.filter(ld, '*.h'))


    # Check each source file
    problems = 0
    for fn in src_files:
        problems += check_file(os.path.join('src', fn))
    if problems == 0:
        print 'No problems found'

if __name__ == '__main__':
    main(sys.argv)

