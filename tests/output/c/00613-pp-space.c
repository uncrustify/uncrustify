/**
 * Some file header comment thingy.
 */
#ifndef SOME_H_INCLUDED
#define SOME_H_INCLUDED

#include "Somefile.h"

#define SOMEMACRO    (1+4)

#ifdef WIN32

 #include "windows_compat.h"

#else /* not WIN32 */

 #if defined HAVE_STDINT_H
  #include <stdint.h>
 #elif defined HAVE_INTTYPES_H
  #include <inttypes.h>
  #define YOUR_OS_SUCKS
 #else
  #error "Don't know where int8_t is defined"
 #endif

typedef uint32_t UINT32;

#endif   /* ifdef WIN32 */

#endif   /* SOME_H_INCLUDED */
