README for Uncrustify
=====================

[![Travis CI](https://img.shields.io/travis/uncrustify/uncrustify/master.svg?style=flat-square&label=Linux)](https://travis-ci.org/uncrustify/uncrustify)
[![AppVeyor](https://img.shields.io/appveyor/ci/uncrustify/uncrustify/master.svg?style=flat-square&label=Windows)](https://ci.appveyor.com/project/uncrustify/uncrustify)

Post any bugs to the issue tracker found on the project's GitHub page:
  https://github.com/uncrustify/uncrustify/issues

Please include the following with your issue:
 - a description of what is not working right
 - input code sufficient to demonstrate the issue
 - expected output code
 - configuration options used to generate the output

If the issue cannot be easily reproduced, then it isn't likely to be fixed.


Building using CMake
--------------------

[CMake](https://cmake.org/) is a tool that generates build systems (Makefiles,
Visual Studio project files, and others).

To generate a build system for Uncrustify using CMake, create a build folder
and run CMake from it:

```.bash
$ mkdir build
$ cd build
$ cmake ..
```

Then use the build tools of your build system (in many cases this will simply
be `make`, but on Windows it could be MSBuild or Visual Studio). Or use CMake
to invoke it:

```.bash
$ cmake --build .
```

If testing is enabled, CMake generates a `test` target, which you can _build_
using your build system tools (usually `make test`). This can also be invoked
using CTest:

```.bash
$ ctest -V -C Debug
```

There is also an `install` target, which can be used to install the Uncrustify
executable (typically `make install`).


A note on CMake configurations
------------------------------

Some build systems are single-configuration, which means you specify the build
type when running CMake (by setting the `CMAKE_BUILD_TYPE` variable), and the
generated files then build that configuration.

An example of a single-configuration build system is Makefiles. You can build
the Release configuration of Uncrustify (from the build folder) with:

```.bash
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
```

Other build systems are multi-configuration, which means you specify the build
type when building.

An example of a multi-configuration build system is Visual Studio project
files. When you open the project in Visual Studio, you can select which
configuration to build. You can also do this while building from the command
line with `cmake --build . --config Release`.


Building using Autotools
------------------------

Quick start:
```
$ ./autogen.sh
$ ./configure
$ make
```

The executable is src/uncrustify.
Copy that to your ~/bin/ folder or wherever you want.


Building the program using Xcode on Mac OS X
---------------------------------------------

You can of course just open the Xcode project and build uncrustify using the
default 'Debug' configuration but if you want to install it the 'Install'
configuration will not work from within Xcode.

For that you will have to use the Xcode command line tool 'xcodebuild'.

To do that, cd into the uncrustify project folder where uncrustify.xcodeproj
resides and enter the following command:

```
$ sudo xcodebuild -configuration 'Install'
```

You will be prompted for the root level password. By doing this you will install
uncrustify into /usr/local/bin. The install location can be changed by editing
the Build Settings for the uncrustify target. The setting you need to change is
called, surprisingly enough, 'Installation Directory'.


Configuring the program
-----------------------

Examine the example config files in `etc` (such as [ben.cfg](./etc/ben.cfg))
and/or read [configuration.txt](./documentation/htdocs/configuration.txt).
Copy the existing config file that closely matches your style and put in
`~/.uncrustify/`. Find complete configuration file options
[in this file](./documentation/htdocs/config.txt). Modify to your
liking.


Running the program (and refining your style)
---------------------------------------------

As of the current release, I don't particularly trust this program to not make
mistakes and screw up my whitespace formatting.

Here's how to run it:
```
$ uncrustify -c ~/.uncrustify/mystyle.cfg -f somefile.c > somefile.c.unc
```

The -c option selects the configuration file.
The -f option specifies the input file.
The output is sent to stdout.  Error messages are sent to stderr.

Use a quality side-by-side diff tool to determine if the program did what you
wanted.
Repeat until your style is refined.


Running the program (once you've found your style)
--------------------------------------------------

Write a script to automate the above.
Check out etc/dofiles.sh for an example.
That script is used as follows:

1. navigate one level above your project
2. make a list of file to process
  ```
  $ find myproj -name "*.[ch]" > files.txt
  ```
3. ```$ sh etc/dofiles.sh files.txt```
4. Use your favorite diff/merge program to merge in the changes
  ```
  $ xxdiff out/myproj myproj
  ```
