# Contributing to Uncrustify

## How to contribute

There are lots of ways to contribute to Uncrustify:
- Report Issues
- Propose Features or Improvements
- Submit Pull Requests

## Making changes

* Pull latest _master_ and create a new branch:
    - Branch name _should_ use lowercase, using `-` to separate words and not `_`. Other special characters _should_ be avoided.
    - A hierarchical structure _can_ be designated using `/` (e.g. `area/topic`). The last part of the name can be keywords like `bugfix`, `feature`, `optim`, `docs`, `refactor`, `test`, etc.
    - Branches _should_ be named after _what_ the change is about
    - Branches _should not_ be named after the issue number, developer name, etc.
* Organize your work:
    - Specialize your branch to target only one thing. Split your work in multiple branches if necessary
    - Make commits of logical units
    - Try to write a quality commit message:
        + Separate subject line from body with a blank line
        + Limit subject line to 50 characters
        + Capitalize the subject line
        + Do not end the subject line with a period
        + Use imperative present tense in the subject line. A proper subject can complete the sentence _If applied, this commit will, [subject]_.
        + Wrap the body at 72 characters
        + Include motivation for the change and contrast its implementation with previous behavior. Explain the _what_ and _why_ instead of _how_.
* Definition of done:
    - The code is clean and documented where needed
    - The change has to be complete (no upcoming fix-up commits)
    - The change should always be accompanied by regression tests (explain why if not possible)
* Preparing a Pull Request (PR):
    - To reduce the likelihood of conflicts and test failures, try to avoid merges, rebasing your work on top of latest master before creating a PR 
    - Verify that your code is properly formatted by running `scripts/Run_uncrustify_for_sources`
    - The PR title should represent _what_ is being changed (a rephrasing of the branch name if set correctly)
    - The PR description should document the _why_ the change needed to be done and _not how_ which should be obvious by doing the code review
