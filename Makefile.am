## Process this file with automake to produce Makefile.in

AUTOMAKE_OPTIONS=foreign no-dependencies

SUBDIRS = src man

pkgdata_DATA = etc/ben.cfg etc/d.cfg etc/defaults.cfg etc/gnu-indent.cfg etc/kr-indent.cfg etc/linux-indent.cfg etc/linux.cfg etc/mono.cfg etc/xsupplicant.cfg

# INCLUDES = @INCLUDES@

testdir = tests
TESTS = run_tests.sh

EXTRA_DIST = scripts run_tests.sh make_token_names.sh $(pkgdata_DATA) \
	tests/*.test tests/*.sh tests/*.py tests/config tests/input \
	tests/output win32 uncrustify.xcodeproj osx

check-local:

DISTCLEANFILES =
CLEANFILES = *~ *.bak
MAINTAINERCLEANFILES = aclocal.m4 Makefile.in
