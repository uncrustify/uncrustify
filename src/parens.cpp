/**
 * @file parens.cpp
 * Adds or removes parens.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 *
 * $Id$
 */
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "prototypes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cctype>


static void add_parens_between(chunk_t *first, chunk_t *last);
static chunk_t *add_bool_parens(chunk_t *popen);


void do_parens(void)
{
   chunk_t *pc;

   if (cpd.settings[UO_mod_full_paren_if_bool].b)
   {
      pc = chunk_get_head();
      while ((pc = chunk_get_next_ncnl(pc)) != NULL)
      {
         if ((pc->type != CT_SPAREN_OPEN) ||
             ((pc->parent_type != CT_IF) && (pc->parent_type != CT_SWITCH)))
         {
            continue;
         }
         pc = add_bool_parens(pc);
      }
   }
}


/**
 * Add an open paren after first and add a close paren before the last
 */
static void add_parens_between(chunk_t *first, chunk_t *last)
{
   chunk_t pc;
   chunk_t *first_n;
   chunk_t *last_p;
   chunk_t *tmp;

   LOG_FMT(LPARADD, "%s: line %d between %.*s and %.*s\n", __func__,
           first->orig_line, first->len, first->str, last->len, last->str);

   /* Don't do anything if we have a bad sequence, ie "&& )" */
   first_n = chunk_get_next_ncnl(first);
   if (first_n == last)
   {
      return;
   }

   memset(&pc, 0, sizeof(pc));

   pc.type        = CT_PAREN_OPEN;
   pc.str         = "(";
   pc.len         = 1;
   pc.flags       = first_n->flags & PCF_COPY_FLAGS;
   pc.level       = first_n->level;
   pc.pp_level    = first_n->pp_level;
   pc.brace_level = first_n->brace_level;

   chunk_add_before(&pc, first_n);

   last_p         = chunk_get_prev_ncnl(last);
   pc.type        = CT_PAREN_CLOSE;
   pc.str         = ")";
   pc.flags       = last_p->flags & PCF_COPY_FLAGS;
   pc.level       = last_p->level;
   pc.pp_level    = last_p->pp_level;
   pc.brace_level = last_p->brace_level;

   chunk_add_after(&pc, last_p);

   for (tmp = first_n;
        tmp != last_p;
        tmp = chunk_get_next_ncnl(tmp))
   {
      tmp->level++;
   }
   last_p->level++;
}


/**
 * Step forward and count the number of semi colons at the current level.
 * Abort if more than 1 or if we enter a preprocessor
 */
static chunk_t *add_bool_parens(chunk_t *popen)
{
   chunk_t *pc;
   chunk_t *ref;
   chunk_t *prev;


   LOG_FMT(LPARADD, "%s: start on %d : \n", __func__, popen->orig_line);

   ref = popen;
   pc  = popen;

   while (((pc = chunk_get_next_nc(pc)) != NULL) && (pc->level > popen->level))
   {
      if (pc->type != CT_BOOL)
      {
         continue;
      }

      prev = chunk_get_prev_ncnl(pc);
      if (prev->type != CT_PAREN_CLOSE)
      {
         /* Add a paren set */
         add_parens_between(ref, pc);
         ref = pc;
      }
   }

   if ((pc != NULL) && (pc->type == CT_SPAREN_CLOSE) && (ref != popen))
   {
      add_parens_between(ref, pc);
   }
   return(pc);
}
