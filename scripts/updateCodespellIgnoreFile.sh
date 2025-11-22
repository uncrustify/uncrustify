#!/bin/bash
# This script updates the codespell ignore file.
# Run it from within the root directory

# Define the path to the codespell ignore file
codespell_ignore_file="$(dirname "$0")/codespell-lines-ignore.txt"

# Clear the codespell ignore file
echo > "${codespell_ignore_file}" || { echo "Error: Failed to clear codespell ignore file"; exit 1; }

# Generate the codespell ignore file
# When '--git-only' will be available in codespell, use it instead of 'git ls-files'
# shellcheck disable=2046
codespell --builtin "clear,informal,names" $(git ls-files) \
  | sed -n -E 's@^([^:]+):[[:digit:]]+:[[:space:]](\S+)[[:space:]].*@git grep -hP '\''\\b\2\\b'\'' -- "\1" >> '"${codespell_ignore_file}"'@p' \
  | while read -r line ; do eval "$line" || { echo "Error: Failed to evaluate line: $line"; exit 1; }; done

# Sort and remove duplicates from the codespell ignore file
sort -u "${codespell_ignore_file}" > _tmp.txt || { echo "Error: Failed to sort and remove duplicates"; exit 1; }
mv _tmp.txt "${codespell_ignore_file}" || { echo "Error: Failed to move temporary file"; exit 1; }
