/**
 * @file nl_create_list_liner.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "nl_create_list_liner.h"

#include "newline_del_between.h"


void nl_create_list_liner(Chunk *brace_open)
{
   LOG_FUNC_ENTRY();

   // See if we get a newline between the next text and the vbrace_close
   if (brace_open == nullptr)
   {
      return;
   }
   Chunk *closing = brace_open->GetNextType(CT_BRACE_CLOSE, brace_open->GetLevel());
   Chunk *tmp     = brace_open;

   do
   {
      if (tmp->Is(CT_COMMA))
      {
         return;
      }
      tmp = tmp->GetNext();
   } while (tmp != closing);

   newline_del_between(brace_open, closing);
} // nl_create_list_liner
