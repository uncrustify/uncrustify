#! /usr/bin/env python
# $Id$
#
#  Creates a possibly faster lookup table for tokens, etc.
#

def scan_file (filename):
	fh = open(filename, 'r')
	lines = fh.readlines()
	cur_token = ''
	token_idx = 0
	args = []
	for line in lines:
		line = line.strip()
		if line.startswith('static const chunk_tag_t'):
			idx = line.find('[')
			if idx > 0:
				cur_token = line[25:idx].strip()
				token_idx = 0
		else:
			if len(cur_token) > 0:
				idx1 = line.find('{')
				idx2 = line.find('CT_')
				if idx1 >= 0 and idx2 > idx1:
					tok = line[idx1 + 1:idx2].strip()
					tok = tok[1:-2]  # strip off open quotes and commas
					args.append([tok, "%s[%d]" % (cur_token, token_idx)])
					token_idx += 1
	return args

def build_table (db, prev, arr):
	#print 'prev "%s"' % prev, 'len(arr)', len(arr)

	start_idx = len(arr)

	# do the current level first
	did_one = 0
	for i in db:
		did_one = 1
		en = db[i]
		# [ char, string, next_index, something ]
		#print 'doin', prev + en[0]
		arr.append([en[0], prev + en[0], 0, en[2]])

	if did_one > 0:
		arr.append(['', '', 0, None])
		# update the one-up level index
		if len(prev) > 0:
			for idx in range(0, len(arr)):
				if arr[idx][1] == prev:
					#print 'updated', prev, 'to', start_idx
					arr[idx][2] = start_idx
					break

	# Now do each sub level
	for i in db:
		en = db[i]
		build_table(en[3], prev + en[0], arr)

def add_to_db(entry, db_top):
	"""
	find or create the entry for the first char
	"""
	str = entry[0]
	db_cur = db_top
	for idx in range(0, len(str)):
		if not str[idx] in db_cur:
			db_cur[str[idx]] = [ str[idx], 0, None, {} ]
		dbe = db_cur[str[idx]]
		if idx == len(str) - 1:
			dbe[2] = entry
		else:
			db_cur = dbe[3]

if __name__ == '__main__':
	pl = scan_file('src/punctuators.cpp')
	pl.sort()

	db = {}
	for a in pl:
		add_to_db(a, db)

	arr = []
	build_table(db, '', arr)
	idx = 0
	print "/**"
	print " * @file punctuators.h"
	print " * Automatically generated"
	print " * $Id$"
	print " */"
	print "static const lookup_entry_t punc_table[] ="
	print "{"
	max_len = 0
	for i in arr:
		rec = i[3]
		if rec != None and len(rec[1]) > max_len:
			max_len = len(rec[1])
		
	for i in arr:
		rec = i[3]
		if len(i[0]) == 0:
			print "   {   0,   0, NULL %s },   // %3d:" % ((max_len - 4) * ' ', idx)
		elif rec == None:
			print "   { '%s', %3d, NULL %s },   // %3d: '%s'" % (i[0], i[2], (max_len - 4) * ' ', idx, i[1])
		else:
			print "   { '%s', %3d, &%s%s },   // %3d: '%s'" % (i[0], i[2], rec[1], (max_len - len(rec[1])) * ' ', idx, i[1])
		idx += 1
	print '};'

