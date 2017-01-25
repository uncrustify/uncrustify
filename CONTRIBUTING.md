# Contributing to Uncrustify

## How to contribute

There are lots of ways to contribute to Uncrustify:
- Reporting Bugs
- Suggesting Enhancements
- Submit Pull Requests

## Making changes

Best practices:
- Branches should be named after _what_ the change is about
- Branches should not be named after the issue number, developer name, etc.
- Branch name should contain keywords like `fix`, `feature`, etc.
- It is recommended to do only one thing at a time and get that to master, splitting your work in multiple PRs if necessary
- The PR title should represent _what_ is being changed (could be a rephrasing of the branch name if set correctly)
- The PR description should document the _why_ the change needed to be done and not _what_ which should be obvious by doing the code review
- Try to avoid merges, always rebase your work on top of master before creating a PR. This will reduce the likelihood of conflicts and test failures
- No PR should be approved to be merged in master if the work is incomplete (no fix-up commits). That's why core reviews are for, not for automatic testing by a CI server. As a reviewer be very critical but not try to over-engineer things
- The changes should always be accompanied by regression tests (if not possible, state why not ?)
