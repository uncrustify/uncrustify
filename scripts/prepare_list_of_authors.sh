#! /bin/sh
#
# Prepare the list of authors
# guy maurel
# 2022-11-22
#
LIST_1="TheListOfAuthors.txt"
{
  echo "Author:"
  echo "2005 - 2016 : Ben Gardner"
  echo ""
  echo "Maintenance:"
  echo "Guy Maurel"
  echo "Michele Calgaro"
  echo "Matthew Woehlke"
  echo ""
  echo "until 2022-11-22:"
  echo "Other collaborators:"
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
    | grep -v "^dundargoc$" \
    | grep -v "^fwojcik$" \
    | grep -v "^gmaurel$" \
    | grep -v "^hdwobben$" \
    | grep -v "^ipaterson$" \
    | grep -v "^jlee975$" \
    | grep -v "^logan.gauthier@metova.com$" \
    | grep -v "^micheleCTDE$" \
    | grep -v "^micheleCTDEAdmin$" \
    | grep -v "^nivekkagicom$" \
    | grep -v "^Pawel Benetkiewicz$" \
    | grep -v "^PoeticPeteAdmin$" \
    | grep -v "^popipo$" \
    | grep -v "^raefaldhia$" \
    | grep -v "^rdan$" \
    | grep -v "^tpltnt$" \
    | grep -v "^versusvoid$" \
    | grep -v "^void$" \

  } > ${LIST_1}
#
mv ${LIST_1} AUTHORS
