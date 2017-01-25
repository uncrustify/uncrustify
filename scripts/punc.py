#! /usr/bin/env python
#
#  Creates a possibly faster lookup table for tokens, etc.
#
# @author  Ben Gardner
# @license GPL v2+
#
from os.path import dirname, join, abspath


def scan_file(file_path):
    cur_token = ''
    token_idx = 0
    args = []

    fd = open(file_path, 'r')
    for line in fd:
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
                    args.append([tok, '%s[%d]' % (cur_token, token_idx)])
                    token_idx += 1
    return args


def build_table(db, prev, arr):
    # do the current level first
    k = sorted(db)
    if len(k) <= 0:
        return
    k.sort()

    start_idx = len(arr)
    num_left = len(k)

    for i in k:
        en = db[i]
        # [ char, full-string, left-in-group, next_index, table-entry ]
        num_left -= 1
        arr.append([en[0], prev + en[0], num_left, 0, en[2]])

    # update the one-up level index
    if len(prev) > 0:
        for idx in range(0, len(arr)):
            if arr[idx][1] == prev:
                arr[idx][3] = start_idx
                break

    # Now do each sub level
    for i in k:
        en = db[i]
        build_table(en[3], prev + en[0], arr)


def add_to_db(entry, db_top):
    """
    find or create the entry for the first char
    """
    strng = entry[0]
    db_cur = db_top
    for idx in range(0, len(strng)):
        if not strng[idx] in db_cur:
            db_cur[strng[idx]] = [strng[idx], 0, None, {}]

        dbe = db_cur[strng[idx]]

        if idx == len(strng) - 1:
            dbe[2] = entry
        else:
            db_cur = dbe[3]


def main():
    root = dirname(dirname(abspath(__file__)))
    pl = scan_file(join(root, 'src', 'punctuators.cpp'))
    pl.sort()

    db = {}
    for a in pl:
        add_to_db(a, db)

    print("/**")
    print(" * @file punc_table.h")
    print(" * Automatically generated")
    print(" */")
    print("\n")
    print("#ifndef PUNC_TABLE_H_INCLUDED")
    print("#define PUNC_TABLE_H_INCLUDED")
    print("\n")
    print("\n")
    print("static const lookup_entry_t punc_table[] =")
    print("{")

    arr = []
    build_table(db, '', arr)

    idx = 0
    max_len = 0
    for i in arr:
        rec = i[4]
        if rec is not None and len(rec[1]) > max_len:
            max_len = len(rec[1])

    for i in arr:
        rec = i[4]
        if len(i[0]) == 0:
            print("   {   0,  0,  0, NULL %s },   // %3d:" % ((max_len - 4) * ' ', idx))
        elif rec is None:
            print("   { '%s', %2d, %2d, NULL %s },   // %3d: '%s'" % (i[0], i[2], i[3], (max_len - 4) * ' ', idx, i[1]))
        else:
            print("   { '%s', %2d, %2d, &%s%s },   // %3d: '%s'" % (i[0], i[2], i[3], rec[1],
                                                                    (max_len - len(rec[1])) * ' ', idx, i[1]))
        idx += 1

    print("};")
    print("\n")
    print("#endif /* PUNC_TABLE_H_INCLUDED */")
    print("\n")

if __name__ == '__main__':
    exit(main())
