#ifndef CO_GCC_H_
#define CO_GCC_H_

#ifdef _lint /* Make sure no compiler comes this way */

/*
   The headers included below must be generated; For C++, generate
   with:

   g++ [usual build options] -E -dM t.cpp >lint_cppmac.h

   For C, generate with:

   gcc [usual build options] -E -dM t.c >lint_cmac.h

   ...where "t.cpp" and "t.c" are empty source files.

   It's important to use the same compiler options used when compiling
   project code because they can affect the existence and precise
   definitions of certain predefined macros.  See the preamble to
   co-gcc.lnt for details and a tutorial.
 */
#if defined(__cplusplus)
//#       include "lint_cppmac.h"
#else
#       include "lint_cmac.h"
#endif


/* If the macros given by the generated macro files must be adjusted
   in order for Lint to cope, then you can do so here.  */


#endif /* _lint      */
#endif /* CO_GCC_H_ */
