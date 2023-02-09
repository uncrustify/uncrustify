[![AppVeyor](https://img.shields.io/appveyor/ci/uncrustify/uncrustify/master.svg?style=flat-square&label=Windows)](https://ci.appveyor.com/project/uncrustify/uncrustify)
[![Coverage Status](https://coveralls.io/repos/github/uncrustify/uncrustify/badge.svg?branch=master)](https://coveralls.io/github/uncrustify/uncrustify?branch=master)
<a href="#"><img src="https://img.shields.io/badge/C++-11-blue.svg?style=flat-square"></a>
[![Conan Center](https://shields.io/conan/v/uncrustify)](https://conan.io/center/uncrustify)

---------------------------

# Uncrustify
A source code beautifier for C, C++, C#, Objective-C, D, Java, Pawn and Vala.

## Features
* Highly configurable - 828 configurable options as of version 0.76.0
- <details><summary>add/remove spaces</summary>

  - `sp_before_sparen`: _Add or remove space before '(' of 'if', 'for', 'switch', 'while', etc._
  - `sp_compare`: _Add or remove space around compare operator '<', '>', '==', etc_
</details>

- <details><summary>add/remove newlines</summary>

  - `nl_if_brace`: _Add or remove newline between 'if' and '{'_
  - `nl_brace_while`: _Add or remove newline between '}' and 'while' of 'do' statement_
</details>

- <details><summary>add/remove blanklines</summary>

  - `eat_blanks_before_close_brace`: _Whether to remove blank lines before '}'_
  - `nl_max`: _The maximum consecutive newlines (3 = 2 blank lines)_
</details>

- <details><summary>indent code</summary>

  - `indent_switch_case`: _indent_switch_case: Spaces to indent 'case' from 'switch'_
  - `indent_class_colon`: _Whether to indent the stuff after a leading base class colon_
</details>

- <details><summary>align code</summary>

  - `align_func_params`: _Align variable definitions in prototypes and functions_
  - `align_struct_init_span`: _The span for aligning struct initializer values (0=don't align)_
</details>

- <details><summary>modify code</summary>

  - `mod_full_brace_for`: _Add or remove braces on single-line 'for' statement_
  - `mod_paren_on_return`: _Add or remove unnecessary paren on 'return' statement_
</details>

Here is an example [configuration file](https://raw.githubusercontent.com/uncrustify/uncrustify/master/documentation/htdocs/ben.cfg.txt),
and here is a [before](https://raw.githubusercontent.com/uncrustify/uncrustify/master/documentation/htdocs/examples/c-1.in.c)
and [after](https://raw.githubusercontent.com/uncrustify/uncrustify/master/documentation/htdocs/examples/c-1.out.c)
C source example.
That should give you a pretty good idea of what Uncrustify can do.





---------------------------------------------------------------------------

## Binaries
Pre compiled binaries for Windows can be downloaded [here](https://sourceforge.net/projects/uncrustify/files/).

## Build
[Python](https://www.python.org/) is an "interpreted high-level programming language for general-purpose programming", for this project it is needed to extend the capabilities of CMake.

[CMake](https://cmake.org/) is a tool that generates build systems
(Makefiles, Visual Studio project files, Xcode project files and others).

To generate a build system for Uncrustify using CMake on UNIX-like systems, create a
build folder and run CMake from it, making sure to specify Release mode:

```bash
$ mkdir build
$ cd build
$ cmake -DCMAKE_BUILD_TYPE=Release ..
```
Other systems may require other flags (e.g. `cmake -G Xcode ..` for Xcode).

Then use the build tools of your build system (in many cases this will
simply be `make`, but on Windows it could be MSBuild or Visual Studio).
Or use CMake to invoke it:

```bash
$ cmake --build . --config Release
```

If testing is enabled, CMake generates a `test` target, which you can
_build_ using your build system tools (usually `make test`). This can also
be invoked using CTest:

```bash
$ ctest -V -C Debug
```

There is also an `install` target, which can be used to install the
Uncrustify executable (typically `make install`).

### A note on CMake configurations
Some build systems are single-configuration, which means you specify the
build type when running CMake (by setting the `CMAKE_BUILD_TYPE`
variable), and the generated files then build that configuration.

An example of a single-configuration build system are Makefiles. You can
build the Release or Debug configurations of Uncrustify (from the build folder) with:

```bash
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
```
or
```bash
$ cmake -DCMAKE_BUILD_TYPE=Debug ..
$ make
```

Other build systems are multi-configuration, which means you specify the
build type when building.

An example of a multi-configuration build system are Visual Studios project
files. When you open the project in Visual Studio, you can select which
configuration to build. You can also do this while building from the
command line with `cmake --build . --config Debug`.


## Bugs
Post any bugs to the issue tracker found on the projects GitHub page:
  https://github.com/uncrustify/uncrustify/issues

Please include the following with your issue:
 - a description of what is not working right
 - input code sufficient to demonstrate the issue
 - expected output code
 - configuration options used to generate the output

More about this is in the [ISSUE_TEMPLATE](https://github.com/uncrustify/uncrustify/blob/master/.github/ISSUE_TEMPLATE)


### Known problems
[Look at the Wiki](https://github.com/uncrustify/uncrustify/wiki/Known-Problems)


## Which repositories have uncrustify?
[Look here](https://repology.org/metapackage/uncrustify/versions)


## Contribute
If you want to add a feature, fix a bug, or implement missing
functionality, feel free to do so! Patches are welcome!
Here are some areas that need attention:

- __Patches for Objective-C support__. We really need someone who knows
  this language as it has more than plenty open issues. A good starting
  point would be to integrate changes made in the
  [Unity fork](https://github.com/Unity-Technologies/uncrustify/tree/fixes/c-oc-java)
- Test Java support and provide feedback (or patches!)
- Test Embedded SQL to see what works
- A logo of some sort
- Anything else that you want to do to make it better?

### A note about pull requests
Firstly take a look at the [CONTRIBUTING.md](https://github.com/uncrustify/uncrustify/blob/master/CONTRIBUTING.md)

Currently we have two continuous integration systems that test your PRs,
TravisCI and Appveyor.
Tested are the test cases, the formatting of the code base and
the output of the command line options.

Test cases can be found in the `tests/` directory. Every file ending with
`.test` is a test set. Inside each line with these components is a
single test: `testNr[!] testConfigFileName testInputFileName [lang]`

The configuration file `testConfigFileName` has to be located inside `tests/config`,
the input file `testInputFileName` inside `tests/input/<testSetName>/`,
and expected results file inside the `tests/expected/<testSetName>/`
directory.
Expected results have the following naming convention: `testNr-testInputFileName`.

Optionally a `!` can follow the `testNr` to enable a custom rerun
configuration.
Rerun configurations need to be named like this:
`testConfigFileName`(without extension)+`.rerun`+`.extension`

Also, optionally a language for the input can be provided with `lang`.

The codebase has to be formatted by the options set up in
`forUncrustifySources.cfg`. Failing to format the sources correctly will
cause TravisCI build failures.

The Command line interface (CLI) output is tested by the
`test_cli_options.sh` script. It is located inside of `tests/cli/` and operates
on the subdirectories of that folder.

If a PR is altering the CLI output, files inside those directories might
need to be manually updated. This often happens when options are
added, removed or altered. Keep in mind that the version string line
(example: `# Uncrustify-0.69.0_f`) of outputs from commands like
`--show-config` should be replaced with a blank line.

### Debugging

The first method is to use uncrustify itself to get debug informations.
Using:
```.txt
   uncrustify -c myExample.cfg -f myExample.cpp -p myExample.p -L A 2>myExample.A
```
you get two files for the first informations.
The p-file gives you details of the parsing process and indentation.
```.txt
# Line                Tag              Parent          Columns Br/Lvl/pp     Flag   Nl  Text
#   1>              CLASS[               NONE][  1/  1/  6/  0][0/0/0][  10070000][0-0] class
#   1>               TYPE[              CLASS][  7/  7/ 14/  1][0/0/0][  10000000][0-0]       Capteur
#   1>         BRACE_OPEN[              CLASS][ 15/ 15/ 16/  1][0/0/0][ 100000400][0-0]               {
```

The A-file gives you many details about the run itself, where the process is running thru,
which values have the most important variables.
```.txt
tokenize(2351): orig line is 1, orig col is 1, Text() 'class', type is CLASS, orig col_end is 6
tokenize(2351): orig line is 1, orig col is 7, Text() 'Capteur', type is WORD, orig col_end is 14
tokenize(2351): orig line is 1, orig col is 15, Text() '{', type is BRACE_OPEN, orig col_end is 16
```

You can also dump the parsing information of each formatting step using the 'dump steps' option.
```.txt
   uncrustify -c myExample.cfg -f myExample.cpp -ds dump
```
This will create a series of 'dump_nnn.log' files, each containing the parsing information at
specific points of the formatting process ('dump_000.log' will list the formatting options in use).

You can combine this option with -p and -L to get a lot of detailed debugging information.
```.txt
   uncrustify -c myExample.cfg -f myExample.cpp -p myExample.p -L A 2>myExample.A -ds dump
```

It might be useful to add some code lines to see where something is happening.
Use the package `unc_tools`.
Remove the comment at line:
```.cpp
#define DEVELOP_ONLY
```
Import the package:
```.cpp
#include "unc_tools.h"
```
Add at some places the line:
```.cpp
prot_the_line(__LINE__, 6, 0);
```
Compile again with DEBUG option.



### How to add an option

If you need a new option, there are a few steps to follow.
Take as example the option `sp_trailing_ret_t`

First define the option:
- Insert the code below to the file src/options.h
_NOTE:
This file is processed by make_options.py, and must conform to a particular
format. Option groups are marked by '//begin ' (in upper case; this example
is lower case to prevent being considered a region marker for code folding)
followed by the group description. Options consist of two lines of
declaration preceded by one or more lines of C++ comments. The comments form
the option description and are taken verbatim, aside from stripping the
leading '// '. Only comments immediately preceding an option declaration,
with no blank lines, are taken as part of the description, so a blank line
may be used to separate notations from a description.
An option declaration is 'extern TYPE\nNAME;', optionally followed by
' // = VALUE' if the option has a default value that is different from the
default-constructed value type of the option. The 'VALUE' must be valid C++
code, and is taken verbatim as an argument when creating the option's
instantiation. Note also that the line break, as shown, is required.
_
```.cpp
// Add or remove space around trailing return operator '->'.
extern Option<iarf_e>
sp_trailing_ret_t;
```
- Insert the code below to the file src/space.cpp
```.cpp
   if (first->Is(CT_TRAILING_RET_T))
   {
      // Add or remove space around trailing return operator '->'.
      log_rule("sp_trailing_ret_t");
      return(options::sp_trailing_ret_t());
   }
```


### Portability

We are pretty sure that nothing OS-specific is used in the code base.
The software has been previously tested on the following operating systems:
- Linux
- QNX
- OS X
- FreeBSD, NetBSD, OpenBSD
- Sun Solaris 9
- Windows (binary available)


---------------------------------------------------------------------------

## Running the program

__NOTE__ This application works reasonably well but it has bugs. Do __not__
apply it on your whole codebase without checking the results!

Here are ways to run it:
```
$ uncrustify -c mystyle.cfg -f somefile.c -o somefile.c.unc
$ uncrustify -c mystyle.cfg -f somefile.c > somefile.c.unc
$ uncrustify -c mystyle.cfg somefile.c
$ uncrustify -c mystyle.cfg --no-backup somefile.c
$ uncrustify -c mystyle.cfg *.c
$ uncrustify -c mystyle.cfg --no-backup *.c
```
The `-c` flag selects the configuration file.
The `-f` flag specifies the input file.
The `-o` flag specifies the output file.
If flag `-f` is used without flag `-o` the output will be send to `stdout`.

Alternatively multiple or single files that should be processed can be
specified at the command end without flags.
If the flag `--no-backup` is missing, every file is saved with the initial
name and an additional suffix (can be changed with --suffix).

For more options descriptions call:
```bash
$ uncrustify -h
```

## Configuring the program
Uncrustify usually reads configuration files that are passed via the `-c`
flag. If the flag is not provided Uncrustify will try to find a
configuration file via the `UNCRUSTIFY_CONFIG` environment variable or a
file with the name `.uncrustify.cfg` or `uncrustify.cfg` in your home folder.

To get a list of:
- all available options use:
  ```bash
  uncrustify --show-config
  ```

- all available options in a usable configuration file format use:
  ```bash
  uncrustify --update-config
  ```

  or

  ```bash
  uncrustify --update-config-with-doc
  ```

  As the names suggest both options can produce output that adds newly
  introduced options to your old configuration file. For this your old
  configuration file has to be passed via the `-c` flag:
  ```bash
  uncrustify --update-config-with-doc -c path/to/your.cfg
  ```

Example configuration files that can be used as a starting point can be
found in the `etc/` directory (such as [ben.cfg](./etc/ben.cfg)).

Modify to your liking. Use a quality side-by-side diff tool to determine
if the program did what you wanted. Repeat until your style is refined.

To ease the process a bit, some 3rd party tools are available:
- [Universal Indent GUI](http://universalindent.sourceforge.net/) - A
  cross-platform graphical configuration file editor for many code
  beautifiers, including Uncrustify.
- [uncrustify_config](https://github.com/CDanU/uncrustify_config) - A web
  configuration tool based on Uncrustify's emscripten interface.
- [UncrustifyX](https://github.com/ryanmaxwell/UncrustifyX) - Uncrustify
  utility and documentation browser for Mac OS X

Under Windows:
Uncrustify is a command-line tool, if you run it by double-clicking the
executable, it will open a command prompt run the executable
(which prints the help message), and then immediately close the window
as uncrustify exits.

You can open the command prompt (which is an interactive terminal
window that allows you to run commands without it closing as soon as
they exit) and run uncrustify.exe there.

## Using uncrustify with vim
Have a look [here](https://github.com/cofyc/vim-uncrustify)
  
## Using uncrustify with IntelliJ
Have a look at https://plugins.jetbrains.com/plugin/17528-uncrustify
