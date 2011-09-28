/**
 * @file parens.cpp
 * Adds or removes parens.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "prototypes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "unc_ctype.h"


static void add_parens_between(chunk_t *first, chunk_t *last);
static void check_bool_parens(chunk_t *popen, chunk_t *pclose, int nest);


void do_parens(void)
{
   chunk_t *pc;
   chunk_t *pclose;

   if (cpd.settings[UO_mod_full_paren_if_bool].b)
   {
      pc = chunk_get_head();
      while ((pc = chunk_get_next_ncnl(pc)) != NULL)
      {
         if ((pc->type != CT_SPAREN_OPEN) ||
             ((pc->parent_type != CT_IF) &&
              (pc->parent_type != CT_ELSEIF) &&
              (pc->parent_type != CT_SWITCH)))
         {
            continue;
         }

         /* Grab the close sparen */
         pclose = chunk_get_next_type(pc, CT_SPAREN_CLOSE, pc->level, CNAV_PREPROC);
         if (pclose != NULL)
         {
            check_bool_parens(pc, pclose, 0);
            pc = pclose;
         }
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

   LOG_FMT(LPARADD, "%s: line %d between %s [lvl=%d] and %s [lvl=%d]\n",
           __func__, first->orig_line,
           first->str.c_str(), first->level,
           last->str.c_str(), last->level);

   /* Don't do anything if we have a bad sequence, ie "&& )" */
   first_n = chunk_get_next_ncnl(first);
   if (first_n == last)
   {
      return;
   }

   pc.type        = CT_PAREN_OPEN;
   pc.str         = "(";
   pc.flags       = first_n->flags & PCF_COPY_FLAGS;
   pc.level       = first_n->level;
   pc.pp_level    = first_n->pp_level;
   pc.brace_level = first_n->brace_level;

   chunk_add_before(&pc, first_n);

   last_p         = chunk_get_prev_ncnl(last, CNAV_PREPROC);
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
 * Scans between two parens and adds additional parens if needed.
 * This function is recursive. If it hits another open paren, it'll call itself
 * with the new bounds.
 *
 * Adds optional parens in an IF or SWITCH conditional statement.
 *
 * This basically just checks for a CT_COMPARE that isn't surrounded by parens.
 * The edges for the compare are the open, close and any CT_BOOL tokens.
 *
 * This only handleds VERY simple patterns:
 *   (!a && b)         => (!a && b)          -- no change
 *   (a && b == 1)     => (a && (b == 1))
 *   (a == 1 || b > 2) => ((a == 1) || (b > 2))
 *
 * FIXME: we really should bail if we transition between a preprocessor and
 *        a non-preprocessor
 */
static void check_bool_parens(chunk_t *popen, chunk_t *pclose, int nest)
{
   chunk_t *pc;
   chunk_t *ref = popen;
   chunk_t *next;
   bool    hit_compare = false;

   LOG_FMT(LPARADD, "%s(%d): popen on %d, col %d, pclose on %d, col %d, level=%d\n",
           __func__, nest,
           popen->orig_line, popen->orig_col,
           pclose->orig_line, pclose->orig_col,
           popen->level);

   pc = popen;
   while (((pc = chunk_get_next_ncnl(pc)) != NULL) && (pc != pclose))
   {
      if (pc->flags & PCF_IN_PREPROC)
      {
         LOG_FMT(LPARADD2, " -- bail on PP %s [%s] at line %d col %d, level %d\n",
                 get_token_name(pc->type),
                 pc->str.c_str(), pc->orig_line, pc->orig_col, pc->level);
         return;
      }

      if ((pc->type == CT_BOOL) ||
          (pc->type == CT_QUESTION) ||
          (pc->type == CT_COND_COLON) ||
          (pc->type == CT_COMMA))
      {
         LOG_FMT(LPARADD2, " -- %s [%s] at line %d col %d, level %d\n",
                 get_token_name(pc->type),
                 pc->str.c_str(), pc->orig_line, pc->orig_col, pc->level);
         if (hit_compare)
         {
            hit_compare = false;
            add_parens_between(ref, pc);
         }
         ref = pc;
      }
      else if (pc->type == CT_COMPARE)
      {
         LOG_FMT(LPARADD2, " -- compare [%s] at line %d col %d, level %d\n",
                 pc->str.c_str(), pc->orig_line, pc->orig_col, pc->level);
         hit_compare = true;
      }
      else if (chunk_is_paren_open(pc))
      {
         next = chunk_skip_to_match(pc);
         if (next != NULL)
         {
            check_bool_parens(pc, next, nest + 1);
            pc = next;
         }
      }
      else if ((pc->type == CT_BRACE_OPEN) ||
               (pc->type == CT_SQUARE_OPEN) ||
               (pc->type == CT_ANGLE_OPEN))
      {
         /* Skip [], {}, and <> */
         pc = chunk_skip_to_match(pc);
      }
   }

   if (hit_compare && (ref != popen))
   {
      add_parens_between(ref, pclose);
   }
}
