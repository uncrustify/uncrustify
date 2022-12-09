============================
 Uncrustify Release Process
============================

.. Update the date in the next line when editing this document!

*This document was last updated on 2022-11-22, for Uncrustify 0.76.0.*

This document uses "0.1.2" throughout as an example version number.
Whenever you see this, you should substitute the version number
of the new release being prepared.

Paths are specified in git syntax, i.e. ``:/`` is the repository root.

Requirements
============

This document assumes you are using a Linux-based OS.
While it should be possible to cut a release on Windows,
using e.g. the `Git for Windows SDK <https://gitforwindows.org/>`_
or a MinGW_ environment, the names and/or arguments to some commands
may be different.


In addition to the build and test requirements for Uncrustify itself
(CMake, a C++ compiler, Python, git), you will also need:

- tar
- python3-git
- Binutils-mingw-w64
- Gcc-mingw-w64
- G++-mingw-w64
- zip
- wget (optional)
- scp (to update documentation on the SourceForge page)

Using packages provided by your OS distribution is *strongly* recommended.
(Exact package names may vary depending on your distribution.)
Examples use ``wget`` to download files via command line,
but any mechanism of obtaining files over HTTPS may be employed.

Preparing a Candidate
=====================

The first step, obviously, is deciding to make a release.
Prior to making a release, verify that the repository is in a stable state
and that all CI (continuous integration - AppVeyor) has passed.
This should ensure all tests pass and building
(including cross-compiling) for Windows is working.

Once the release process is started,
only pull requests needed to fix critical bugs,
or related to the release process, should be accepted.
(This will minimize the need to redo or repeat work
such as updating the documentation, especially the change log.)

To start the release process, first check that:

- You are on the ``master`` branch
- Your local clone is up to date
- ``CMAKE_BUILD_TYPE`` is set to ``Release`` (or ``RelWithDebInfo``)
- Your build is up to date
- check the list of authors with scripts/prepare_list_of_authors.sh

You might need a new PAT for your account, for your admin-account.
See:
https://github.blog/2020-12-15-token-authentication-requirements-for-git-operations/
https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/creating-a-personal-access-token

Then, run::

   $ scripts/release_tool.py init
   $ scripts/release_tool.py update path/to/uncrustify

(Replace ``path/to/uncrustify`` with the path to the Uncrustify executable
you just built, e.g. ``build/uncrustify``.)

This will create a branch for the release candidate
and perform some automated updates to various files.
With no arguments, ``init`` will prompt you for the new version number,
defaulting to ``x.(y+1).0``, where ``x.y.z`` is the previous release.
The ``--version`` argument may also be used to specify the version
(e.g. if the script will not be able to prompt for input).

After, you should check that the following files
show the correct version number and option count:

- ``:/CMakeLists.txt`` (version number only; look for ``UNCRUSTIFY_VERSION``)
- ``:/package.json`` (version number only; you'll see it, the file is tiny)
- ``:/README.md`` (look for "options as of version")
- ``:/documentation/htdocs/index.html`` (look for "options as of version")

(Note that ``uncrustify`` itself will not show the new version number
until the final release has been tagged.)

Update Documentation
====================

Update ``:/ChangeLog``.
There is a helper script, ``:/scripts/gen_changelog.py``,
that can help extract new options since the previous release:

.. code::

   $ scripts/gen_changelog.py uncrustify-0.0.0

Replace ``0.0.0`` with the version of the *previous* release.
This will generate a bunch of output like::

   0123456789abcdef0123456789abcdef01234567
     Added   : better_name                          Jan 13 1970
     Removed : poor_name                            Jan 13 1970
   fedcba9876543210fedcba9876543210fedcba98
     Added   : new_option_1                         Jan 18 1970
     Added   : new_option_2                         Jan 18 1970

Your goal is to turn the "raw" output into something like this::

   Deprecated options:
     - poor_name                             Jan 13 1970
       Renamed to better_name

   New options:
     - new_option_1                         Jan 18 1970
     - new_option_1                         Jan 18 1970

To accomplish this, you will need to inspect any removed options,
possibly consulting the commits in which they were removed,
to determine the reason for deprecation and what replacement is recommended.
(Note that it may not be as simple as "use X instead".)
Also watch for options that were added and subsequently renamed
since the last release. (This has happened a few times.
In such cases, the new name should show up as an ordinary "new" option,
and the old name should be entirely omitted from the change log.)

It helps to copy the output to a scratch file for editing.
Move deprecated options to the top and add a "Deprecated options:" header,
then add a "New options:" header in front of what's left,
and remove the commit SHAs (``sed -r '/^[[:xdigit:]]{40}/d``
if you don't want to do it by hand).
Then, check that the options are in order by date;
date of authorship vs. date of merge may cause discrepancies.
Finally, replace occurrences of ``\w+ +:`` with ``-``
(if your editor supports regular expressions;
otherwise you can individually replace ``Added   :`` and ``Removed :``).

Add a new release header (don't forget to add the date!) to the change log
and insert the list of option changes as created above.
Also fill in the list of resolved issues, new keywords (if any),
as well as any other changes that need to be mentioned.

If any command line arguments have been added or changed,
including descriptions for the same, check to see if
``:/man/uncrustify.1.in`` needs to be updated.
(Hopefully this happened when the source was changed!)

Finalize the Code Changes
=========================

Inspect your working tree.
Use ``git add -p`` to stage the changes made to the documentation
and other artifacts that contain version-dependent information.
Verify that only desired changes are staged,
and that your working tree is otherwise clean.

Now is a good time to recheck
that everything builds, and that all the tests pass.
This is also a good time to manually test 32- and 64-bit builds.

When you are ready, commit the changes using:

.. code::

   $ scripts/release_tool.py commit

(If you prefer, you can also commit the changes manually;
the script just fills in the commit message for you.)

Submit and Tag the Release
==========================

Push the release candidate branch to GitHub, and create a pull request.
Once the pull request is merged, tag the release using:
Make sure, the file .git/config has the right 'admin' value:
[remote "origin"]
        url = https://<admin account>:<PAT>@github.com/uncrustify/uncrustify.git
Check it with:
$ git config --local --get remote.origin.url

.. code::

   $ scripts/release_tool.py tag

Note that this will only work if the merge of the release candidate
is the most recent commit upstream.
Otherwise, the merge commit must be specified by using the ``-c`` option.

(Tagging the release does not need to be done on any particular branch.
The command will not affect or look at your work tree at all.)

Create Binaries
===============

Now that the release is published, grab a copy of the sources from GitHub:

.. code::

   $ wget https://github.com/uncrustify/uncrustify/archive/uncrustify-0.1.2.zip
   $ unzip -e uncrustify-0.1.2.zip

Next, build the 32- and 64-bit Windows binaries:

.. code::

   $ cd /path/to/uncrustify-uncrustify-0.1.2
   $ mkdir buildwin-32
   $ cd buildwin-32
   $ cmake -G Ninja \
           -DCMAKE_BUILD_TYPE=Release \
           -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-mingw32.cmake \
           -DCMAKE_EXE_LINKER_FLAGS="-static -s" \
           ..
   $ ninja
   $ cpack

.. code::

   $ cd /path/to/uncrustify-uncrustify-0.1.2
   $ mkdir buildwin-64
   $ cd buildwin-64
   $ cmake -G Ninja \
           -DCMAKE_BUILD_TYPE=Release \
           -DCMAKE_TOOLCHAIN_FILE=../cmake/Toolchain-mingw64.cmake \
           -DCMAKE_EXE_LINKER_FLAGS="-static -s" \
           ..
   $ ninja
   $ cpack

Create a tarball:

.. code::

   $ cd /path/to/uncrustify
   $ git archive -o uncrustify-0.1.2.tar.gz --prefix=uncrustify-0.1.2/ uncrustify-0.1.2
TODO: find the best strategie...

(If you don't have Ninja_, or just don't want to use it for whatever reason,
omit ``-G Ninja`` and run ``make`` instead of ``ninja``.)

This is also a good time to test the tagged build on Linux:

.. code::

   $ wget https://github.com/uncrustify/uncrustify/archive/uncrustify-0.1.2.tar.gz
   $ tar xzf uncrustify-0.1.2.tar.gz
   $ cd uncrustify-uncrustify-0.1.2
   $ mkdir build
   $ cd build
   $ cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
   $ ninja
   $ ctest
   $ ./uncrustify --version

Upload to SourceForge
=====================

- Login as admin under https://sourceforge.net/projects/uncrustify/
- Change to https://sourceforge.net/projects/uncrustify/files/
- "Add Folder"; the name should be e.g. "uncrustify-0.1.2"
- Navigate to the new folder
  (e.g. https://sourceforge.net/projects/uncrustify/files/uncrustify-0.1.2/)
- "Add File"; upload the following files
  (adjusting for the actual version number):

  - README.md
  - uncrustify-0.1.2.tar.gz
  - buildwin-32/uncrustify-0.1.2_f-win32.zip
  - buildwin-64/uncrustify-0.1.2_f-win64.zip

- "Done"
- Upload the documentation:

  .. code::

     $ scp -r documentation/htdocs/* ChangeLog \
       USER,uncrustify@web.sourceforge.net:htdocs/

- Use the web interface (file manager) to create the release folder
  and upload the files to SourceForge.

Announce the Release (Optional)
===============================

The new release is live! Spread the word! Consider these ideas:

- Create a news item.
- Update freshmeat.net project.

Release Checklist
=================

The following list serves as a quick reference for making a release.
These items are explained in greater detail above.

#. Verify that CI passes

#. Use ``release_tool.py`` to initialize the release
   and perform automated updates. Check:

   #. ``:/CMakeLists.txt``
   #. ``:/package.json``
   #. ``:/README.md``
   #. ``:/documentation/htdocs/index.html``

#. Update documentation as needed:

   #. ``:/ChangeLog``
   #. ``:/man/uncrustify.1.in``

#. Stage changes.
#. Test everything again.
#. Finalize the code changes.
#. Push to GitHub and create a merge request.
#. Tag the merged release branch.
#. Create Windows (32- and 64-bit) binaries.
#. Run a test build on Linux.
#. Upload the release and documentation to SourceForge.
#. Announce the release!

.. _MinGW: http://www.mingw.org/
.. _GitPython: https://github.com/gitpython-developers/GitPython
.. _Ninja: https://ninja-build.org/
