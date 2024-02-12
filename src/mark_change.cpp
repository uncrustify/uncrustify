/**
 * @file mark_change.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "mark_change.h"

#include "logger.h"
#include "uncrustify_types.h"


void mark_change(const char *func, size_t line)
{
   LOG_FUNC_ENTRY();

   cpd.changes++;

   if (cpd.pass_count == 0)
   {
      LOG_FMT(LCHANGE, "%s(%d): change %d on %s:%zu\n",
              __func__, __LINE__, cpd.changes, func, line);
   }
} // mark_change
