README for Uncrustify
=====================

Post any bugs to the issue tracker found on the project's Sourceforge page:
  http://sourceforge.net/projects/uncrustify

Please include the following with your issue:
 - a description of what is not working right
 - input code sufficient to demonstrate the issue
 - expected output code
 - configuration options used to generate the output

If the issue cannot be easily reproduced, then it isn't likely to be fixed.


Building the program
--------------------

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
