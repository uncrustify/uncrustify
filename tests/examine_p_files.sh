# 28. 5.2022
#
# Guy Maurel
#
#sh generate_p_files.sh
#
P_FILES="../build/P-files"
#
cd ${P_FILES}
grep "BRACED_INIT_LIST" * | grep -v CT_BRACED_INIT_LIST | grep -v _INCLUDED > BRACED_INIT_LIST_found_list.txt
# split with : to get the filenames only
gawk -f ../../tests/BRACED_INIT_LIST_found_list.awk < BRACED_INIT_LIST_found_list.txt > BRACED_INIT_LIST_only_FN.txt
set -x
sort BRACED_INIT_LIST_only_FN.txt > BRACED_INIT_LIST_sorted.txt
uniq BRACED_INIT_LIST_sorted.txt > BRACED_INIT_LIST_uniq.txt
