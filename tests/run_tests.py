#! /usr/bin/env python
#
# Scans the .test files on the command line and parses each, running
# the tests listed.  Results are printed out.
#
# This could all be done with bash, but I wanted to use python. =)
# Anyway, this was all done while waiting in the Denver airport.
#

import sys
import os
import string
import filecmp

# OK, so I just had way too much fun with the colors..

if os.name == "nt":	# windoze doesn't support ansi sequences
	NORMAL      = ""
	BOLD        = ""
	UNDERSCORE  = ""
	REVERSE     = ""
else:	
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

if os.name == "nt":	# windoze doesn't support ansi sequences
	FAIL_COLOR     = ""
	PASS_COLOR     = ""
	MISMATCH_COLOR = ""
	UNSTABLE_COLOR = ""
else:
	FAIL_COLOR     = UNDERSCORE
	PASS_COLOR     = FG_GREEN
	MISMATCH_COLOR = FG_RED #REVERSE
	UNSTABLE_COLOR = FGB_CYAN

# log_levels:
# bit 0: show a diff on unstable or failures
# bit 1: show passes
# bit 2: show commands
log_level = 0

def usage_exit():
	print "Usage: \n" + sys.argv[0] + " testfile"
	sys.exit()

def run_tests(test_name, config_name, input_name, lang):
	expected_name = os.path.join(os.path.dirname(input_name), test_name + '-' + os.path.basename(input_name))
	# print "Test:  ", test_name
	# print "Config:", config_name
	# print "Input: ", input_name
	# print 'Output:', expected_name

	if not config_name.startswith(os.sep):
		config_name = os.path.join('config', config_name)

	resultname = os.path.join('results', expected_name)
	outputname = os.path.join('output', expected_name)
	try:
		os.makedirs(os.path.dirname(resultname))
	except:
		pass

	cmd = "%s/uncrustify -q -c %s -f input/%s %s > %s" % (os.path.abspath('../src'), config_name, input_name, lang, resultname)
	if log_level & 2:
		print "RUN: " + cmd
	a = os.system(cmd)
	if a != 0:
		print FAIL_COLOR + "FAILED: " + NORMAL + test_name
		return -1

	try:
		if not filecmp.cmp(resultname, outputname):
			print MISMATCH_COLOR + "MISMATCH: " + NORMAL + test_name
			if log_level & 1:
				cmd = "diff -u %s %s" % (outputname, resultname)
				os.system(cmd)
			return -1
	except:
		print MISMATCH_COLOR + "MISSING: " + NORMAL + test_name
		return -1

	# The file in results matches the file in output.
	# Re-run with the output file as the input to check stability.
	cmd = "%s/uncrustify -q -c %s -f %s %s > %s" % (os.path.abspath('../src'), config_name, outputname, lang, resultname)
	if log_level & 2:
		print "RUN: " + cmd
	a = os.system(cmd)
	if a != 0:
		print FAIL_COLOR + "FAILED2: " + NORMAL + test_name
		return -1

	try:
		if not filecmp.cmp(resultname, outputname):
			print UNSTABLE_COLOR + "UNSTABLE: " + NORMAL + test_name
			if log_level & 1:
				cmd = "diff -u %s %s" % (outputname, resultname)
				os.system(cmd)
			return -2
	except:
		# impossible
		print UNSTABLE_COLOR + "MISSING: " + NORMAL + test_name
		return -1

	if log_level & 4:
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
	unst_count = 0
	for line in fd:
		line = string.rstrip(string.lstrip(line))
		parts = string.split(line)
		if (len(parts) < 3) or (parts[0][0] == '#'):
			continue
		lang = ""
		if len(parts) > 3:
			lang = "-l " + parts[3]
		rt = run_tests(parts[0], parts[1], parts[2], lang)
		if rt < 0:
			if rt == -1:
				fail_count += 1
			else:
				unst_count += 1
		else:
			pass_count += 1
	return [pass_count, fail_count, unst_count]

#
# entry point
#

if __name__ == '__main__':
	args = []
	the_tests = []
	for arg in sys.argv[1:]:
		if arg.startswith('-'):
			for cc in arg[1:]:
				if cc == 'd':       # show diff on failure
					log_level |= 1
				elif cc == 'c':     # show commands
					log_level |= 2
				elif cc == 'p':     # show passes
					log_level |= 4
				else:
					sys.exit('Unknown option "%s"' % (cc))
		else:
			args.append(arg)

	if len(args) == 0:
		the_tests += "c-sharp c cpp d java pawn objective-c vala ecma".split()
	else:
		the_tests += args

	#print args
	print "Tests: " + str(the_tests)
	pass_count = 0
	fail_count = 0
	unst_count = 0

	for item in the_tests:
		passfail = process_test_file(item + '.test')
		if passfail != None:
			pass_count += passfail[0]
			fail_count += passfail[1]
			unst_count += passfail[2]

	print "Passed %d / %d tests" % (pass_count, pass_count + fail_count)
	if fail_count > 0:
		print BOLD + "Failed %d test(s)" % (fail_count) + NORMAL
		sys.exit(1)
	else:
		txt = BOLD + "All tests passed" + NORMAL
		if unst_count > 0:
			txt += ", but some files were unstable"
		print txt
		sys.exit(0)

