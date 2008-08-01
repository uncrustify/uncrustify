/**
 * @file detect.cpp
 * Scans the parsed file and tries to determine options.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 *
 * $Id$
 */
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "ChunkStack.h"
#include "align_stack.h"
#include "prototypes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "unc_ctype.h"

/**
 * Scan the parsed file and detect some options
 */
void detect_options()
{
   chunk_t *pc;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {

   }
}

