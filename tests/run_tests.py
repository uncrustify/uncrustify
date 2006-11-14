#! /usr/bin/env python
#
# Scans the .test files on the command line and parses each, running
# the tests listed.  Results are printed out.
#
# This could all be done with bash, but I wanted to use python. =)
# Anyway, this was all done while waiting in the Denver airport.
#
# $Id$
#

import sys
import os
import string

# OK, so I just had way too much fun with the colors..
NORMAL      = "\033[0m"
BOLD        = "\033[1m"
UNDERSCORE  = "\033[1m"
REVERSE     = "\033[7m"

FG_BLACK    = "\033[30m"
FG_RED      = "\033[31m"
FG_GREEN    = "\033[32m"
FG_YELLOW   = "\033[33m"
FG_BLUE     = "\033[34m"
FG_MAGNETA  = "\033[35m"
FG_CYAN     = "\033[36m"
FG_WHITE    = "\033[37m"

FGB_BLACK   = "\033[90m"
FGB_RED     = "\033[91m"
FGB_GREEN   = "\033[92m"
FGB_YELLOW  = "\033[93m"
FGB_BLUE    = "\033[94m"
FGB_MAGNETA = "\033[95m"
FGB_CYAN    = "\033[96m"
FGB_WHITE   = "\033[97m"


BG_BLACK    = "\033[40m"
BG_RED      = "\033[41m"
BG_GREEN    = "\033[42m"
BG_YELLOW   = "\033[43m"
BG_BLUE     = "\033[44m"
BG_MAGNETA  = "\033[45m"
BG_CYAN     = "\033[46m"
BG_WHITE    = "\033[47m"

BGB_BLACK   = "\033[100m"
BGB_RED     = "\033[101m"
BGB_GREEN   = "\033[102m"
BGB_YELLOW  = "\033[103m"
BGB_BLUE    = "\033[104m"
BGB_MAGNETA = "\033[105m"
BGB_CYAN    = "\033[106m"
BGB_WHITE   = "\033[107m"

# after all that, I chose c
FAIL_COLOR     = UNDERSCORE
PASS_COLOR     = FG_GREEN
MISMATCH_COLOR = FG_RED #REVERSE

log_level = 1

def usage_exit():
	print "Usage: \n" + sys.argv[0] + " testfile"
	sys.exit()

def run_tests(test_name, config_name, input_name):
	expected_name = os.path.join(os.path.dirname(input_name), test_name + '-' + os.path.basename(input_name))
	# print "Test:  ", test_name
	# print "Config:", config_name
	# print "Input: ", input_name
	# print 'Output:', expected_name

	resultname = os.path.join('results', expected_name)
	try:
		os.makedirs(os.path.dirname(resultname))
	except:
		pass

	cmd = "%s/uncrustify -q -c config/%s -f input/%s > %s" % (os.path.abspath('../src'), config_name, input_name, resultname)
	if log_level >= 2:
		print "RUN: " + cmd
	a = os.system(cmd)
	if a != 0:
		print FAIL_COLOR + "FAILED: " + NORMAL + test_name
		return -1

	cmd = "diff -u %s output/%s > /dev/null" % (resultname, expected_name)
	if log_level >= 2:
		print "RUN: " + cmd
	b = os.system(cmd)
	if b != 0:
		print MISMATCH_COLOR + "MISMATCH: " + NORMAL + test_name
		return -1

	if log_level >= 1:
		print PASS_COLOR + "PASSED: " + NORMAL + test_name
	return 0

def process_test_file(filename):
	fd = open(filename, "r")
	if fd == None:
		print "Unable to open " + filename
		return None
	print "Processing " + filename
	pass_count = 0
	fail_count = 0
	for line in fd:
		line = string.rstrip(string.lstrip(line))
		parts = string.split(line)
		if (len(parts) < 3) or (parts[0][0] == '#'):
			continue
		if run_tests(parts[0], parts[1], parts[2]) < 0:
			fail_count += 1
		else:
			pass_count += 1
	return [pass_count, fail_count]

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
		the_tests += "c-sharp c cpp d java pawn".split()
	else:
		the_tests += args

	#print args
	print "Tests: " + str(the_tests)
	pass_count = 0
	fail_count = 0

	for item in the_tests:
		passfail = process_test_file(item + '.test')
		if passfail != None:
			pass_count += passfail[0]
			fail_count += passfail[1]

	print "Passed %d / %d tests" % (pass_count, pass_count + fail_count)
	if fail_count > 0:
		print BOLD + "Failed %d test(s)" % (fail_count) + NORMAL
	else:
		print BOLD + "All tests passed" + NORMAL
	sys.exit()

