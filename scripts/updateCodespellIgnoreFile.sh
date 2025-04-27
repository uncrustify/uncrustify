#!/bin/bash
# This script updates the codespell ignore file.
# Run it from within the root directory
codespell_ignore_file=$(dirname "$0")/codespell-lines-ignore.txt
echo > "${codespell_ignore_file}"
# When '--git-only' will be available in codespell, use it instead of 'git ls-files'
# shellcheck disable=2046
codespell --builtin "clear,informal,names" $(git ls-files) \
  | sed -n -E 's@^([^:]+):[[:digit:]]+:[[:space:]](\S+)[[:space:]].*@git grep -hP '\''\\b\2\\b'\'' -- "\1" >> '"${codespell_ignore_file}"'@p' \
  | while read -r line ; do eval "$line" ; done
sort -u "${codespell_ignore_file}" > _tmp.txt ; mv _tmp.txt "${codespell_ignore_file}"
