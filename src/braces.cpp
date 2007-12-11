/**
 * @file braces.cpp
 * Adds or removes braces.
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
         cpd.settings[UO_mod_full_brace_function].a |
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

   /* Mark one-liners */
   chunk_t *pc;
   chunk_t *br_open;
   chunk_t *tmp;

   pc = chunk_get_head();
   while ((pc = chunk_get_next_ncnl(pc)) != NULL)
   {
      if (pc->type != CT_BRACE_OPEN)
      {
         continue;
      }
      br_open = pc;

      /* Detect empty bodies */
      tmp = chunk_get_next_ncnl(pc);
      if (tmp->type == CT_BRACE_CLOSE)
      {
         br_open->flags |= PCF_EMPTY_BODY;
         tmp->flags     |= PCF_EMPTY_BODY;
      }

      /* Scan for the brace close or a newline */
      tmp = br_open;
      while ((tmp = chunk_get_next_nc(tmp)) != NULL)
      {
         if (chunk_is_newline(tmp))
         {
            break;
         }
         if ((tmp->type == CT_BRACE_CLOSE) && (br_open->level == tmp->level))
         {
            br_open->flags |= PCF_ONE_LINER;
            tmp->flags     |= PCF_ONE_LINER;
            break;
         }
      }
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
         if ((((pc->parent_type == CT_IF) ||
               (pc->parent_type == CT_ELSE) ||
               (pc->parent_type == CT_ELSEIF)) &&
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

            LOG_FMT(LBRDEL, " [%.*s %d-%d]", pc->len, pc->str, pc->orig_line, semi_count);

            was_fcn = (prev != NULL) && (prev->type == CT_FPAREN_CLOSE);

            if (chunk_is_semicolon(pc) ||
                (pc->type == CT_IF) ||
                (pc->type == CT_ELSEIF) ||
                (pc->type == CT_FOR) ||
                (pc->type == CT_DO) ||
                (pc->type == CT_WHILE) ||
                ((pc->type == CT_BRACE_OPEN) && was_fcn))
            {
               hit_semi |= chunk_is_semicolon(pc);
               if (++semi_count > 1)
               {
                  LOG_FMT(LBRDEL, " bailed on %d because of %.*s on line %d\n",
                          bopen->orig_line, pc->len, pc->str, pc->orig_line);
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
      if (semi_count > 0)
      {
         /* we have a pair of braces with only 1 statement inside */
         remove_brace(bopen);
         remove_brace(pc);
         bopen->type = CT_VBRACE_OPEN;
         bopen->len  = 0;
         bopen->str  = "";
         pc->type    = CT_VBRACE_CLOSE;
         pc->len     = 0;
         pc->str     = "";

         LOG_FMT(LBRDEL, " removing braces on line %d and %d\n",
                 bopen->orig_line, pc->orig_line);
      }
      else
      {
         LOG_FMT(LBRDEL, " empty statement\n");
      }
   }
   else
   {
      LOG_FMT(LBRDEL, " not a close brace? - '%.*s'\n", pc->len, pc->str);
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
         if (chunk_safe_to_del_nl(tmp))
         {
            chunk_del(tmp);
         }
      }
   }
}

static void convert_vbrace_to_brace(void)
{
   chunk_t *pc;
   chunk_t *tmp;
   chunk_t *vbc;

   /* Find every vbrace open */
   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->type != CT_VBRACE_OPEN)
      {
         continue;
      }

      if ((((pc->parent_type == CT_IF) ||
            (pc->parent_type == CT_ELSE) ||
            (pc->parent_type == CT_ELSEIF)) &&
           ((cpd.settings[UO_mod_full_brace_if].a & AV_ADD) != 0))
          ||
          ((pc->parent_type == CT_FOR) &&
           ((cpd.settings[UO_mod_full_brace_for].a & AV_ADD) != 0))
          ||
          ((pc->parent_type == CT_DO) &&
           ((cpd.settings[UO_mod_full_brace_do].a & AV_ADD) != 0))
          ||
          ((pc->parent_type == CT_WHILE) &&
           ((cpd.settings[UO_mod_full_brace_while].a & AV_ADD) != 0))
          ||
          ((pc->parent_type == CT_FUNC_DEF) &&
           ((cpd.settings[UO_mod_full_brace_function].a & AV_ADD) != 0)))
      {
         /* Find the matching vbrace close */
         vbc = NULL;
         tmp = pc;
         while ((tmp = chunk_get_next(tmp)) != NULL)
         {
            if ((tmp->flags & PCF_IN_PREPROC) != (pc->flags & PCF_IN_PREPROC))
            {
               break;
            }
            if ((pc->brace_level == tmp->brace_level) &&
                (tmp->type == CT_VBRACE_CLOSE) &&
                (pc->parent_type == tmp->parent_type))
            {
               vbc = tmp;
               break;
            }
         }
         if (vbc == NULL)
         {
            continue;
         }

         pc->type = CT_BRACE_OPEN;
         pc->len  = 1;
         pc->str  = "{";

         vbc->type = CT_BRACE_CLOSE;
         vbc->len  = 1;
         vbc->str  = "}";

         /* If the next chunk is a comment, followed by a newline, then
          * move the brace after the newline and add another newline after
          * the close brace.
          */
         tmp = chunk_get_next(vbc);
         if (chunk_is_comment(tmp))
         {
            tmp = chunk_get_next(tmp);
            if (chunk_is_newline(tmp))
            {
               chunk_move_after(vbc, tmp);
               newline_add_after(vbc);
            }
         }
      }
   }
}

/**
 * Adds a comment after the ref chunk
 * Returns the added chunk or NULL
 */
chunk_t *insert_comment_after(chunk_t *ref, c_token_t cmt_type,
                              int cmt_len, const char *cmt_text)
{
   chunk_t new_cmt;
   char    *txt;
   int     txt_len;

   if (cmt_len <= 0)
   {
      cmt_len = strlen(cmt_text);
   }
   txt_len = cmt_len + 8; /* 8 is big enough for all types */

   memset(&new_cmt, 0, sizeof(new_cmt));
   new_cmt.flags = (ref->flags & PCF_COPY_FLAGS) | PCF_OWN_STR;
   new_cmt.type  = cmt_type;

   /* allocate memory for the string */
   txt = new char[txt_len];
   if (txt == NULL)
   {
      return(NULL);
   }

   new_cmt.str = txt;
   if (cmt_type == CT_COMMENT_CPP)
   {
      new_cmt.len = snprintf(txt, txt_len, "// %.*s", cmt_len, cmt_text);
   }
   else
   {
      new_cmt.len = snprintf(txt, txt_len, "/* %.*s */", cmt_len, cmt_text);
   }
   /* TODO: expand comment type to cover other comment styles? */

   new_cmt.column   = ref->column + ref->len + 1;
   new_cmt.orig_col = new_cmt.column;

   return(chunk_add_after(&new_cmt, ref));
}

void add_long_closebrace_comment(void)
{
   chunk_t *pc;
   chunk_t *tmp;
   chunk_t *br_open;
   chunk_t *br_close;
   chunk_t *fcn_pc = NULL;
   chunk_t *sw_pc  = NULL;
   int     nl_count;

   pc = chunk_get_head();
   while ((pc = chunk_get_next_ncnl(pc)) != NULL)
   {
      if (pc->type == CT_FUNC_DEF)
      {
         fcn_pc = pc;
      }
      else if (pc->type == CT_SWITCH)
      {
         /* kinda pointless, since it always has the text "switch" */
         sw_pc = pc;
      }
      if ((pc->type != CT_BRACE_OPEN) || ((pc->flags & PCF_IN_PREPROC) != 0))
      {
         continue;
      }

      br_open  = pc;
      nl_count = 0;

      tmp = pc;
      while ((tmp = chunk_get_next(tmp)) != NULL)
      {
         if (chunk_is_newline(tmp))
         {
            nl_count += tmp->nl_count;
         }
         else if ((tmp->level == br_open->level) &&
                  (tmp->type == CT_BRACE_CLOSE))
         {
            br_close = tmp;

            //LOG_FMT(LSYS, "found brace pair on lines %d and %d, nl_count=%d\n",
            //        br_open->orig_line, br_close->orig_line, nl_count);

            /* Found the matching close brace - make sure a newline is next */
            tmp = chunk_get_next(tmp);
            if (chunk_is_newline(tmp))
            {
               int     nl_min  = 0;
               chunk_t *tag_pc = NULL;

               if (br_open->parent_type == CT_SWITCH)
               {
                  nl_min = cpd.settings[UO_mod_add_long_switch_closebrace_comment].n;
                  tag_pc = sw_pc;
               }
               else if (br_open->parent_type == CT_FUNC_DEF)
               {
                  nl_min = cpd.settings[UO_mod_add_long_function_closebrace_comment].n;
                  tag_pc = fcn_pc;
               }

               if ((nl_min > 0) && (nl_count >= nl_min) && (tag_pc != NULL))
               {
                  /* TODO: determine the added comment style */

                  /* Add a comment after the close brace */
                  insert_comment_after(br_close, CT_COMMENT,
                                       tag_pc->len, tag_pc->str);
               }
            }
            break;
         }
      }
   }
}
