[![Travis CI](https://img.shields.io/travis/uncrustify/uncrustify/master.svg?style=flat-square&label=Linux)](https://travis-ci.org/uncrustify/uncrustify)
[![AppVeyor](https://img.shields.io/appveyor/ci/uncrustify/uncrustify/master.svg?style=flat-square&label=Windows)](https://ci.appveyor.com/project/uncrustify/uncrustify)
[![Coverity](https://scan.coverity.com/projects/8264/badge.svg)](https://scan.coverity.com/projects/uncrustify)
<a href="#"><img src="https://img.shields.io/badge/C++-11-blue.svg?style=flat-square"></a>

---------------------------

# Uncrustify
A source code beautifier for C, C++, C#, ObjectiveC, D, Java, Pawn and VALA

## Features
* highly configurable - 609 configurable options as of version 0.66.1
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
Pre compiled binaries for Windows can be downloaded [here](https://sourceforge.net/projects/uncrustify/files/uncrustify/).

## Build
[CMake](https://cmake.org/) is a tool that generates build systems
(Makefiles, Visual Studio project files, Xcode project files and others).

To generate a build system for Uncrustify using CMake, create a build
folder and run CMake from it:

```bash
$ mkdir build
$ cd build
$ cmake ..
```
(Use `cmake -G Xcode ..` for Xcode)

Then use the build tools of your build system (in many cases this will
simply be `make`, but on Windows it could be MSBuild or Visual Studio).
Or use CMake to invoke it:

```bash
$ cmake --build .
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
build the Release configuration of Uncrustify (from the build folder) with:

```bash
$ cmake -DCMAKE_BUILD_TYPE=Release ..
$ make
```

Other build systems are multi-configuration, which means you specify the
build type when building.

An example of a multi-configuration build system are Visual Studios project
files. When you open the project in Visual Studio, you can select which
configuration to build. You can also do this while building from the
command line with `cmake --build . --config Release`.


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
and expected results file inside the `tests/output/<testSetName>/`
directory.
Expected results have the following naming convention: `testNr-testConfigFileName`.

Optionally a `!` can follow the `testNr` to enable a custom rerun
configuration.
Rerun configurations need to be named like this:
`testConfigFileName`(without extension)+`.rerun`+`.exension`

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
(example: `# Uncrustify-0.65_f`) of outputs from commands like
`--show-config` should be replaced with a blank line.

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
If the flag `--no-backup` is missing, every file saved with the initial
name and an additional suffix (can be changed with --suffix).

For more options descriptions call:
```bash
$ uncrustify -h
```

## Configuring the program
Uncrustify usually reads configuration files that are passed via the `-c`
flag. If the flag is not provided Uncrustify will try to find a
configuration file via the `UNCRUSTIFY_CONFIG` environment variable or a
file with the name `uncrustify` or `.uncrustify` in your home folder.

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
  configuration tool based on Uncrustifys emscripten interface.
- [UncrustifyX](https://github.com/ryanmaxwell/UncrustifyX) - Uncrustify
  utility and documentation browser for Mac OS X

