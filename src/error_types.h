/**
 * @file error_types.h
 *
 * Defines the error codes that are used throughout uncrustify
 *
 * @license GPL v2+
 */
#ifndef ERROR_TYPES_H_INCLUDED
#define ERROR_TYPES_H_INCLUDED

#if 1
#include <stdlib.h>      // provides EXIT_SUCCESS and EXIT FAILURE

// TODO: if we decided to only use EX_OK and EX_xxx we can avoid including stdlib.h here

#else
// TODO: I left this to show my modifications remove it after the PR was reviewed

// the good old SUCCESS/FAILURE
#define SUCCESS    0      //! same as EX_OK */
#define FAILURE    -1     //! incompatible to EXIT_FAILURE
#endif


#if defined (WIN32) || defined (__QNXNTO__)
// Windows does not know sysexists.h. Thus define the error codes

#define EX_OK             0    //! successful termination
#define EX__BASE          64   //! base value for error messages
#define EX_USAGE          64   //! command line usage error
#define EX_DATAERR        65   //! data format error
#define EX_NOINPUT        66   //! cannot open input
#define EX_NOUSER         67   //! addressee unknown
#define EX_NOHOST         68   //! host name unknown
#define EX_UNAVAILABLE    69   //! service unavailable
#define EX_SOFTWARE       70   //! internal software error
#define EX_OSERR          71   //! system error (e.g., can't fork)
#define EX_OSFILE         72   //! critical OS file missing
#define EX_CANTCREAT      73   //! can't create (user) output file
#define EX_IOERR          74   //! input/output error
#define EX_TEMPFAIL       75   //! temp failure; user is invited to retry
#define EX_PROTOCOL       76   //! remote error in protocol
#define EX_NOPERM         77   //! permission denied
#define EX_CONFIG         78   //! configuration error
#define EX__MAX           78   //! maximum listed value

#else // not WIN32 or not __QNXNTO__
// TODO: do all non windows systems know sysexits.h?
//       Linux knows: /usr/include/sysexits.h
#include "sysexits.h"      // comes from BSD
#endif

#endif /* ERROR_TYPES_H_INCLUDED */
