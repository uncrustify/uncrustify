============================
 Uncrustify Release Process
============================

.. Update the date in the next line when editing this document!

*This document was last updated on 2024-11-17, for Uncrustify 0.80.1.*

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

Preliminary checks
==================

Prior to making a release, verify that the repository is in a stable state
and that all CI (continuous integration - AppVeyor) has passed.
This should ensure all tests pass and building
(including cross-compiling) for Windows is working.

Before starting the release process, first check that:

  - your local clone of ``uncrustify/uncrustify`` is up to date
    (don't use your uncrustify fork clone for preparing a release)
  - you are on the ``master`` branch
  - ``CMAKE_BUILD_TYPE`` is set to ``Release`` (or ``RelWithDebInfo``)
  - you have a valid PAT for your admin account. See below on how
    to get a new PAT if needed.
  - ``.git/config`` contains your PAT information. Again see below. 

Getting a new GitHub PAT and set up git correctly
=================================================

Info about PAT can be found here:
https://github.blog/2020-12-15-token-authentication-requirements-for-git-operations/
https://docs.github.com/en/authentication/keeping-your-account-and-data-secure/creating-a-personal-access-token

To get a new PAT for your admin account follow these steps:
  - login with an admin account at https://github.com/uncrustify/uncrustify
  - on the right, click on the photo
  - scroll down to "Settings"
  - on the left, scroll down to "Developer settings", and click
  - on the left, click on "Personal access tokens"
  - choose "Tokens (classic), click
  - if necessary "Delete" expired token(s)
  - click on "Generate new token"
  - choose "Generate new token (classic)", click
  - choose a "what's this token for"
  - click on "repo"
  - scroll down to bottom and click on "Generate token"
  - make sure to copy your personal access token now. You won’t be able to see it again!
  - copy the token "ghp_******"
  - update the file ``.git/config`` using the new token
      [remote "origin"]
          url = https://<admin account>:<PAT>@github.com/uncrustify/uncrustify.git
  - check that the PAT is correct with:
    .. code::
       $ git config --local --get remote.origin.url


Preparing a Candidate
=====================

First update the version number and date at the beginning of this document
and then update the list of authors with:

  .. code::

     $ ./scripts/prepare_list_of_authors.sh

Build uncrustify (including builds for Windows) and verify that they are successful
and all tests are passed. See section "Create Binaries" below for the required steps.

Run the following commands to start the release preparation:

  .. code::

     $ ./scripts/release_tool.py init
     $ ./scripts/release_tool.py update path/to/uncrustify

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

   $ ./scripts/gen_changelog.py uncrustify-0.0.0

Replace ``0.0.0`` with the version of the *previous* release.
This will generate a bunch of output like::

   0123456789abcdef0123456789abcdef01234567
     Added   : better_name                          Jan 13 1970
     Removed : poor_name                            Jan 13 1970
   fedcba9876543210fedcba9876543210fedcba98
     Added   : new_option_1                         Jan 18 1970
     Added   : new_option_2                         Jan 18 1970

Copy the output from the script at the top of ``:/ChangeLog``
and add a new release header (don't forget to add the date!)

Finalize the Code Changes
=========================

Inspect your working tree.
Use ``git add -p`` to stage the changes made to the documentation
and other artifacts that contain version-dependent information.
Verify that only desired changes are staged and that your working
tree is otherwise clean.

When you are ready, commit the changes using:

.. code::

   $ ./scripts/release_tool.py commit

(If you prefer, you can also commit the changes manually;
the script just fills in the commit message for you.)

Submit and Tag the Release
==========================

Push the release candidate branch to GitHub and create a pull request.
Make sure to use your NON-admin account for creating the pull request,
so that later you can use your admin account to approve it.

.. code::

   $ git push -u origin HEAD

This will push the branch and commit to the remote upstream and will print
out a link that you can use to create a pull request in a web browser.

Once the pull request has completed the various CI checks, merge it.

Switch to ``master`` branch and check out the latest commit that includes
the previously merged pull request and then tag the release using:

.. code::

   $ ./scripts/release_tool.py tag

Note that this will only work if the merge of the release candidate
is the most recent commit upstream.
Otherwise, the merge commit must be specified by using the ``-c`` option.
The command will automatically push the tag upstream as well.

You can check the new tag with the following commands, which will list
all existing tags locally and remotely, respectively

.. code::

   git tag
   git ls-remote --tags origin

(Tagging the release does not need to be done on any particular branch.
The command will not affect or look at your work tree at all.)

Finally, delete the release branch upstream and locally

.. code::

   $ git push -d origin <release branch name>
   $ git branch -D <release branch name>

Create Binaries
===============

Create a tarball:

.. code::

   $ cd /path/to/uncrustify
   $ git archive -o ../uncrustify-0.1.2.tar.gz --prefix=uncrustify-uncrustify-0.1.2/ uncrustify-0.1.2

Grab a copy of the sources from GitHub:

.. code::

   $ cd /path/to/uncrustify/..
   $ wget https://github.com/uncrustify/uncrustify/archive/uncrustify-0.1.2.zip
   $ unzip -e uncrustify-0.1.2.zip

Build the Linux binaries:

.. code::

   $ cd /path/to/uncrustify-uncrustify-0.1.2
   $ mkdir build
   $ cd build
   $ cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
   $ ninja
   $ ctest
   $ ./uncrustify --version

Next, build the 32 and 64 bit Windows binaries:

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

Create a release on github
==========================

- Login with an admin account at https://github.com/uncrustify/uncrustify
- Navigate to https://github.com/uncrustify/uncrustify/releases and click on
  the "Draft a new release" button at the top of the page
- Select the corresponding release tag under the "Choose a tag" combobox
- Add the release version under "Release title" as "Uncrustify 0.xx.y"
- Upload the Windows binaries and the source code zip/tarball files in the section
  "Attach binaries by dropping them here or selecting them": these will show up as
  "Assets" under the release text. Executables for Windows 32 and 64 and source
  code in tar.gz and zip format need to be uploaded.
- Add release text in describing section. It is recommended to copy the text
  from previous releases and update the related files. Make sure to use bold text
  to highlight the various sections (use '### ' at the beginning of the line)
- Publish the release by clicking on the "Publish release" button.

Upload to SourceForge
=====================

- Login as admin under https://sourceforge.net/projects/uncrustify/
- Change to https://sourceforge.net/projects/uncrustify/files/
- "Add Folder"; the name should be e.g. "uncrustify-0.1.2"
- Navigate to the new folder
  (e.g. https://sourceforge.net/projects/uncrustify/files/uncrustify-0.1.2/)
- Click "Add File" and upload the following files
  (adjusting for the actual version number):
  Click "Done" when all files have been uploaded.

  * README.md
  * uncrustify-0.1.2.tar.gz
  * uncrustify-0.1.2.zip
  * buildwin-32/uncrustify-0.1.2_f-win32.zip
  * buildwin-64/uncrustify-0.1.2_f-win64.zip

- Upload the documentation using the following commands:

  .. code::

     $ cd /path/to/uncrustify
     $ scp -r documentation/htdocs/* ChangeLog \
       USERNAME,uncrustify@web.sourceforge.net:htdocs/

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
#. Create release on Github.
#. Upload the release and documentation to SourceForge.
#. Announce the release!

.. _MinGW: http://www.mingw.org/
.. _GitPython: https://github.com/gitpython-developers/GitPython
.. _Ninja: https://ninja-build.org/
