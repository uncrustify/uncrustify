#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.59)
AC_INIT(uncrustify,0.61,,,http://uncrustify.sourceforge.net/)
enable_silent_rules=${enable_silent_rules-yes}
AM_INIT_AUTOMAKE([-Wall silent-rules])
AM_MAINTAINER_MODE
AC_CONFIG_SRCDIR([src/uncrustify.cpp])
AC_CONFIG_HEADER([src/config.h])

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
AC_HEADER_STDC
AC_CHECK_HEADERS([inttypes.h stdint.h])

AC_CHECK_HEADERS([utime.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_C_INLINE

# Checks for library functions.
AC_FUNC_MEMCMP
AC_CHECK_FUNCS([memset strcasecmp strchr strdup strerror strtol strtoul])

AC_MSG_CHECKING([whether our compiler supports __func__])
AC_TRY_COMPILE([],
 [const char *cp = __func__],
 AC_MSG_RESULT([yes]),
 AC_MSG_RESULT([no])
 AC_MSG_CHECKING([whether our compiler supports __FUNCTION__])
 AC_TRY_COMPILE([],
   [const char *cp = __FUNCTION__],
   AC_MSG_RESULT([yes])
   AC_DEFINE(__func__, __FUNCTION__,
         [Define to appropriate substitue if compiler doesnt have __func__]),
   AC_MSG_RESULT([no])
   AC_DEFINE(__func__, __FILE__,
         [Define to appropriate substitue if compiler doesnt have __func__])))

AC_CONFIG_FILES([Makefile src/Makefile man/Makefile man/uncrustify.1 src/uncrustify_version.h])
AC_OUTPUT
