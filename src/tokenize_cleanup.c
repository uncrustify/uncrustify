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

static void check_template(chunk_t *start);


void tokenize_cleanup(void)
{
   chunk_t *pc   = chunk_get_head();
   chunk_t *prev = NULL;
   chunk_t *next;

   pc   = chunk_get_head();
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

      /* CT_WORD followed by CT_PAREN_OPEN is CT_FUNCTION */
      if ((pc->type == CT_WORD) && (next->type == CT_PAREN_OPEN))
      {
         pc->type = CT_FUNCTION;
      }

      /**
       * Change CT_WORD after CT_ENUM, CT_UNION, or CT_STRUCT to CT_TYPE
       * Change CT_WORD before CT_WORD to CT_TYPE
       */
      if (next->type == CT_WORD)
      {
         if ((pc->type == CT_ENUM) ||
             (pc->type == CT_UNION) ||
             (pc->type == CT_STRUCT))
         {
            next->type = CT_TYPE;
         }
         if (pc->type == CT_WORD)
         {
            pc->type = CT_TYPE;
         }
      }

      /**
       * Change CT_STAR to CT_PTR_TYPE if preceeded by CT_TYPE,
       * CT_QUALIFIER, or CT_PTR_TYPE.
       */
      if ((next->type == CT_STAR) &&
          ((pc->type == CT_TYPE) ||
           (pc->type == CT_QUALIFIER) ||
           (pc->type == CT_PTR_TYPE)))
      {
         next->type = CT_PTR_TYPE;
      }

      /**
       * Change angle open followed by
       */
      if (pc->type == CT_ANGLE_OPEN)
      {
         check_template(pc);
      }
      if ((pc->type == CT_ANGLE_CLOSE) && (pc->parent_type != CT_TEMPLATE))
      {
         pc->type = CT_COMPARE;
      }

      /* TODO: determine other stuff here */

      prev = pc;
      pc   = next;
      next = chunk_get_next_ncnl(pc);
   }
}


/**
 * If there is nothing but CT_WORD and CT_MEMBER, then it's probably a
 * template thingy.  Otherwise, it's likely a comparison.
 */
static void check_template(chunk_t *start)
{
   chunk_t *pc;
   chunk_t *end;

   for (pc = chunk_get_next_ncnl(start); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if ((pc->type != CT_WORD) && (pc->type != CT_MEMBER))
      {
         break;
      }
   }

   end = pc;
   if (end != NULL)
   {
      if (end->type == CT_ANGLE_CLOSE)
      {
         for (pc = start; pc != end; pc = chunk_get_next_ncnl(pc))
         {
            pc->parent_type = CT_TEMPLATE;
         }
         end->parent_type = CT_TEMPLATE;
      }
      else
      {
         start->type = CT_COMPARE;
      }
   }
}

