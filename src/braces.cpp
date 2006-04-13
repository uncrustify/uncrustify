/**
 * @file braces.c
 * Adds or removes braces.
 *
 * $Id: braces.c 93 2006-03-23 04:11:00Z bengardner $
 */
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "chunk_stack.h"
#include "prototypes.h"
#include <cstdio>
#include <cstdlib>
#include <string.h>
#include <errno.h>
#include <ctype.h>


static void convert_vbrace_to_brace(void);
static void examine_braces(void);
static void examine_brace(chunk_t *bopen);
static void remove_brace(chunk_t *pc);


void do_braces(void)
{
   /* covert vbraces if needed */
   if (((cpd.settings[UO_mod_full_brace_if].a |
         cpd.settings[UO_mod_full_brace_do].a |
         cpd.settings[UO_mod_full_brace_for].a |
         cpd.settings[UO_mod_full_brace_while].a) & AV_ADD) != 0)
   {
      convert_vbrace_to_brace();
   }

   if (((cpd.settings[UO_mod_full_brace_if].a |
         cpd.settings[UO_mod_full_brace_do].a |
         cpd.settings[UO_mod_full_brace_for].a |
         cpd.settings[UO_mod_full_brace_while].a) & AV_REMOVE) != 0)
   {
      examine_braces();
   }
}


/**
 * Go backwards to honor brace newline removal limits
 */
static void examine_braces(void)
{
   chunk_t *pc;

   pc = chunk_get_tail();
   while (pc != NULL)
   {
      if ((pc->type == CT_BRACE_OPEN) &&
          ((pc->flags & PCF_IN_PREPROC) == 0))
      {
         if ((((pc->parent_type == CT_IF) || (pc->parent_type == CT_ELSE)) &&
              ((cpd.settings[UO_mod_full_brace_if].a & AV_REMOVE) != 0)) ||
             ((pc->parent_type == CT_DO) &&
              ((cpd.settings[UO_mod_full_brace_do].a & AV_REMOVE) != 0)) ||
             ((pc->parent_type == CT_FOR) &&
              ((cpd.settings[UO_mod_full_brace_for].a & AV_REMOVE) != 0)) ||
             ((pc->parent_type == CT_WHILE) &&
              ((cpd.settings[UO_mod_full_brace_while].a & AV_REMOVE) != 0)))
         {
            examine_brace(pc);
         }
      }
      pc = chunk_get_prev_type(pc, CT_BRACE_OPEN, -1);
   }
}


/**
 * Step forward and count the number of semi colons at the current level.
 * Abort if more than 1 or if we enter a preprocessor
 */
static void examine_brace(chunk_t *bopen)
{
   chunk_t *pc;
   chunk_t *prev      = NULL;
   int     semi_count = 0;
   int     level      = bopen->level + 1;
   bool    hit_semi   = false;
   bool    was_fcn    = false;
   int     nl_max     = cpd.settings[UO_mod_full_brace_nl].n;
   int     nl_count   = 0;

   LOG_FMT(LBRDEL, "%s: start on %d : ", __func__, bopen->orig_line);

   pc = chunk_get_next_nc(bopen);
   while ((pc != NULL) && (pc->level >= level))
   {
      if ((pc->flags & PCF_IN_PREPROC) != 0)
      {
         LOG_FMT(LBRDEL, " PREPROC\n");
         return;
      }

      if (chunk_is_newline(pc))
      {
         nl_count += pc->nl_count;
         if ((nl_max > 0) && (nl_count > nl_max))
         {
            LOG_FMT(LBRDEL, " exceeded %d newlines\n", nl_max);
            return;
         }
      }
      else
      {
         if (pc->level == level)
         {
            if ((semi_count > 0) && hit_semi)
            {
               /* should have bailed due to close brace level drop */
               LOG_FMT(LBRDEL, " no close brace\n");
               return;
            }

            LOG_FMT(LBRDEL, " [%s %d-%d]", pc->str, pc->orig_line, semi_count);

            was_fcn = (prev != NULL) && (prev->type == CT_FPAREN_CLOSE);

            if ((pc->type == CT_SEMICOLON) ||
                (pc->type == CT_IF) ||
                (pc->type == CT_FOR) ||
                (pc->type == CT_DO) ||
                (pc->type == CT_WHILE) ||
                ((pc->type == CT_BRACE_OPEN) && was_fcn))
            {
               hit_semi |= (pc->type == CT_SEMICOLON);
               if (++semi_count > 1)
               {
                  LOG_FMT(LBRDEL, " bailed on %d because of %s on line %d\n",
                          bopen->orig_line, pc->str, pc->orig_line);
                  return;
               }
            }
         }
      }
      prev = pc;
      pc   = chunk_get_next_nc(pc);
   }

   if (pc == NULL)
   {
      LOG_FMT(LBRDEL, " NULL\n");
      return;
   }

   if (pc->type == CT_BRACE_CLOSE)
   {
      /* we have a pair of braces with only 1 statement inside */
      remove_brace(bopen);
      remove_brace(pc);
      bopen->type = CT_VBRACE_OPEN;
      bopen->len  = 0;
      pc->type    = CT_VBRACE_CLOSE;
      pc->len     = 0;

      LOG_FMT(LBRDEL, " removing braces on line %d and %d\n",
              bopen->orig_line, pc->orig_line);
   }
   else
   {
      LOG_FMT(LBRDEL, " not a close brace? - '%s'\n", pc->str);
   }
}


static void remove_brace(chunk_t *pc)
{
   chunk_t *tmp;

   pc->len = 0;

   if (pc->type == CT_BRACE_OPEN)
   {
      pc->type = CT_VBRACE_OPEN;
      tmp      = chunk_get_prev(pc);
   }
   else
   {
      pc->type = CT_VBRACE_CLOSE;
      tmp      = chunk_get_next(pc);
   }

   if (chunk_is_newline(tmp))
   {
      if (tmp->nl_count > 1)
      {
         tmp->nl_count--;
      }
      else
      {
         chunk_del(tmp);
      }
   }
}


static void convert_vbrace_to_brace(void)
{
   chunk_t *pc;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if ((((pc->parent_type == CT_IF) || (pc->parent_type == CT_ELSE)) &&
           ((cpd.settings[UO_mod_full_brace_if].a & AV_ADD) != 0))
          ||
          ((pc->parent_type == CT_FOR) &&
           ((cpd.settings[UO_mod_full_brace_for].a & AV_ADD) != 0))
          ||
          ((pc->parent_type == CT_DO) &&
           ((cpd.settings[UO_mod_full_brace_do].a & AV_ADD) != 0))
          ||
          ((pc->parent_type == CT_WHILE) &&
           ((cpd.settings[UO_mod_full_brace_while].a & AV_ADD) != 0)))
      {
         if (pc->type == CT_VBRACE_OPEN)
         {
            pc->type = CT_BRACE_OPEN;
            pc->len  = 1;
            pc->str  = "{";
         }
         if (pc->type == CT_VBRACE_CLOSE)
         {
            pc->type = CT_BRACE_CLOSE;
            pc->len  = 1;
            pc->str  = "}";
         }
      }
   }
}

