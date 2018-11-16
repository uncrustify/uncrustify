#! /bin/sh
#
# Prepare the list of authors
# guy maurel
# 2017-11-02
#
LIST_1="TheListOfAuthors.txt"
echo "Author:"                    > ${LIST_1}
echo "2005 - 2016 : Ben Gardner" >> ${LIST_1}
echo ""                          >> ${LIST_1}
echo "Maintenance:"              >> ${LIST_1}
echo "Guy Maurel"                >> ${LIST_1}
echo ""                          >> ${LIST_1}
echo "Other collaborators:"      >> ${LIST_1}
git log --format='%aN' \
   | sort -u \
   | grep -v "^ben$" \
   | grep -v "^bengardner$" \
   | grep -v "^Ben Gardner$" \
   | grep -v "^CDanU$" \
   | grep -v "^DVE2000$" \
   | grep -v "^Gilles$" \
   | grep -v "^Guy Maurel$" \
   | grep -v "^brmqk3$" \
   | grep -v "^csobeski$" \
   | grep -v "^dbeard$" \
   | grep -v "^gmaurel$" \
   | grep -v "^hdwobben$" \
   | grep -v "^ipaterson$" \
   | grep -v "^jlee975$" \
   | grep -v "^logan.gauthier@metova.com$" \
   | grep -v "^nivekkagicom$" \
   | grep -v "^popipo$" \
   | grep -v "^raefaldhia$" \
   | grep -v "^rdan$" \
   | grep -v "^tpltnt$" \
   | grep -v "^versusvoid$" \
   | grep -v "^void$" \
      >> ${LIST_1}
#
mv ${LIST_1} AUTHORS
