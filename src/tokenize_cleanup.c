/**
 * @file tokenize_cleanup.c
 * Looks at simple sequences to refine the chunk types.
 * Examples:
 *  - change '[' + ']' into '[]'/
 *  - detect "version = 10;" vs "version (xxx) {"
 *
 * $Id: tokenize.c 84 2006-03-19 02:45:53Z bengardner $
 */

#include "cparse_types.h"
#include "prototypes.h"
#include "chunk_list.h"
#include <string.h>



void tokenize_cleanup(void)
{
   chunk_t *pc = chunk_get_head();
   chunk_t *prev = NULL;
   chunk_t *next;

   pc = chunk_get_head();
   next = chunk_get_next_ncnl(pc);
   while ((pc != NULL) && (next != NULL))
   {
      /* Change '[' + ']' into '[]' */
      if ((pc->type == CT_SQUARE_OPEN) && (next->type == CT_SQUARE_CLOSE))
      {
         pc->type = CT_TSQUARE;
         pc->str  = "[]";
         pc->len  = 2;
         chunk_del(next);
         next = chunk_get_next_ncnl(pc);
      }

      /* Determine the version stuff (D only) */
      if (pc->type == CT_VERSION)
      {
         if (next->type == CT_PAREN_OPEN)
         {
            pc->type = CT_IF;
         }
         else
         {
            if (next->type != CT_ASSIGN)
            {
               LOG_FMT(LERR, "%s: [%d] version: Unexpected token %s\n",
                       __func__, pc->orig_line, get_token_name(next->type));
            }
            pc->type = CT_WORD;
         }
      }

      /* TODO: determine other stuff here */

      prev = pc;
      pc   = next;
      next = chunk_get_next_ncnl(pc);
   }
}

