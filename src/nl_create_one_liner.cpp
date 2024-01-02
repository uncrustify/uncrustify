/**
 * @file nl_create_one_liner.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "nl_create_one_liner.h"

#include "keywords.h"
#include "newline_del_between.h"


void nl_create_one_liner(Chunk *vbrace_open)
{
   LOG_FUNC_ENTRY();

   // See if we get a newline between the next text and the vbrace_close
   Chunk *tmp   = vbrace_open->GetNextNcNnl();
   Chunk *first = tmp;

   if (  first->IsNullChunk()
      || get_token_pattern_class(first->GetType()) != pattern_class_e::NONE)
   {
      return;
   }
   size_t nl_total = 0;

   while (tmp->IsNot(CT_VBRACE_CLOSE))
   {
      if (tmp->IsNewline())
      {
         nl_total += tmp->GetNlCount();

         if (nl_total > 1)
         {
            return;
         }
      }
      tmp = tmp->GetNext();
   }

   if (  tmp->IsNotNullChunk()
      && first->IsNotNullChunk())
   {
      newline_del_between(vbrace_open, first);
   }
} // nl_create_one_liner
