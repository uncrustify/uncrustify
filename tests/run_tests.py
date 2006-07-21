#! /usr/bin/env python
#
# Scans the .test files on the command line and parses each, running
# the tests listed.  Results are printed out.
#
# This could all be done with bash, but I wanted to use python. =)
# Anyway, this was all done while waiting in the Denver airport.
#
# $Id: $
#

import sys
import os
import string

NORMAL = "\033[0m"
GREEN  = "\033[92m"
YELLOW = "\033[93m"

log_level = 1

def usage_exit():
    print "Usage: \n" + sys.argv[0] + " testfile"
    sys.exit()

def run_tests(test_name, config_name, input_name, expected_name):
    # print "Test:   " + test_name
    # print "Config: " + config_name
    # print "Input:  " + input_name
    # print "Output: " + expected_name

    resultname = os.path.join('results', expected_name)
    try:
        os.makedirs(os.path.dirname(resultname))
    except:
        pass

    cmd = "../src/uncrustify -q -c config/%s -f input/%s > %s" % (config_name, input_name, resultname)
    if log_level >= 2:
        print "RUN: " + cmd
    a = os.system(cmd)
    if a != 0:
        print RED + "FAILED: " + NORMAL + test_name
        return

    cmd = "diff -u %s output/%s > /dev/null" % (resultname, expected_name)
    if log_level >= 2:
        print "RUN: " + cmd
    b = os.system(cmd)
    if b != 0:
        print YELLOW + "MISMATCH: " + NORMAL + test_name
        return

    if log_level >= 1:
        print GREEN + "PASSED: " + NORMAL + test_name

def process_test_file(filename):
    fd = open(filename, "r")
    if fd == None:
        print "Unable to open " + filename
        return
    print "Processing " + filename
    for line in fd:
        line = string.rstrip(string.lstrip(line))
        parts = string.split(line)
        if (len(parts) < 4) or (parts[0][0] == '#'):
            continue
        run_tests(parts[0], parts[1], parts[2], parts[3])


#
# entry point
#

if __name__ == '__main__':

    args = []
    the_tests = []
    for arg in sys.argv[1:]:
        if arg == '-q':
            log_level = 0
        elif arg == '-v':
            log_level = 2
        else:
            args.append(arg)

    if len(args) == 0:
        the_tests += "c-sharp c cpp d java".split()
    else:
        the_tests += args

    #print args
    print "Tests: " + str(the_tests)

    for item in the_tests:
        process_test_file(item + '.test')

    sys.exit()

