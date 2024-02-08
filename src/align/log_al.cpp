/**
 * @file log_al.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/log_al.h"

#include "uncrustify.h"


void align_log_al(log_sev_t sev, size_t line)
{
   if (log_sev_on(sev))
   {
      log_fmt(sev, "%s(%d): line %zu, cpd.al_cnt is %zu\n",
              __func__, __LINE__, line, cpd.al_cnt);

      for (size_t idx = 0; idx < cpd.al_cnt; idx++)
      {
         log_fmt(sev, "   cpd.al[%2.1zu].col is %2.1zu, cpd.al[%2.1zu].len is %zu, type is %s\n",
                 idx, cpd.al[idx].col, idx, cpd.al[idx].len,
                 get_token_name(cpd.al[idx].type));
      }

      log_fmt(sev, "\n");
   }
} // align_log_al
