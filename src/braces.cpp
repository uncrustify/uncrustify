/**
 * @file braces.cpp
 * Adds or removes braces.
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


static void convert_brace(chunk_t *br);
static void convert_vbrace(chunk_t *br);
static void convert_vbrace_to_brace(void);
static void examine_braces(void);
static void examine_brace(chunk_t *bopen);
static void move_case_break(void);
static void mod_case_brace(void);
static void mod_full_brace_if_chain(void);
static bool can_remove_braces(chunk_t *bopen);
static bool should_add_braces(chunk_t *vbopen);


void do_braces(void)
{
   if (((cpd.settings[UO_mod_full_brace_if].a |
         cpd.settings[UO_mod_full_brace_do].a |
         cpd.settings[UO_mod_full_brace_for].a |
         cpd.settings[UO_mod_full_brace_using].a |
         cpd.settings[UO_mod_full_brace_while].a) & AV_REMOVE) != 0)
   {
      examine_braces();
   }

   /* convert vbraces if needed */
   if (((cpd.settings[UO_mod_full_brace_if].a |
         cpd.settings[UO_mod_full_brace_do].a |
         cpd.settings[UO_mod_full_brace_for].a |
         cpd.settings[UO_mod_full_brace_function].a |
         cpd.settings[UO_mod_full_brace_using].a |
         cpd.settings[UO_mod_full_brace_while].a) & AV_ADD) != 0)
   {
      convert_vbrace_to_brace();
   }

   if (cpd.settings[UO_mod_full_brace_if_chain].b)
   {
      mod_full_brace_if_chain();
   }

   /* Mark one-liners */
   chunk_t *pc;
   chunk_t *br_open;
   chunk_t *tmp;
   c_token_t brc_type;

   pc = chunk_get_head();
   while ((pc = chunk_get_next_ncnl(pc)) != NULL)
   {
      if ((pc->type != CT_BRACE_OPEN) &&
          (pc->type != CT_VBRACE_OPEN))
      {
         continue;
      }
      br_open  = pc;
      brc_type = c_token_t(pc->type + 1);

      /* Detect empty bodies */
      tmp = chunk_get_next_ncnl(pc);
      if ((tmp != NULL) && (tmp->type == brc_type))
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
         if ((tmp->type == brc_type) && (br_open->level == tmp->level))
         {
            flag_series(br_open, tmp, PCF_ONE_LINER);
            break;
         }
      }
   }

   if (cpd.settings[UO_mod_case_brace].a != AV_IGNORE)
   {
      mod_case_brace();
   }
   if (cpd.settings[UO_mod_move_case_break].b)
   {
      move_case_break();
   }
}


/**
 * Go backwards to honor brace newline removal limits
 */
static void examine_braces(void)
{
   chunk_t *pc;
   chunk_t *prev;

   pc = chunk_get_tail();
   while (pc != NULL)
   {
      prev = chunk_get_prev_type(pc, CT_BRACE_OPEN, -1);
      if ((pc->type == CT_BRACE_OPEN) &&
          ((pc->flags & PCF_IN_PREPROC) == 0))
      {
         if ((((pc->parent_type == CT_IF) ||
               (pc->parent_type == CT_ELSE) ||
               (pc->parent_type == CT_ELSEIF)) &&
              ((cpd.settings[UO_mod_full_brace_if].a) == AV_REMOVE)) ||
             ((pc->parent_type == CT_DO) &&
              ((cpd.settings[UO_mod_full_brace_do].a) == AV_REMOVE)) ||
             ((pc->parent_type == CT_FOR) &&
              ((cpd.settings[UO_mod_full_brace_for].a) == AV_REMOVE)) ||
             ((pc->parent_type == CT_USING_STMT) &&
              ((cpd.settings[UO_mod_full_brace_using].a) == AV_REMOVE)) ||
             ((pc->parent_type == CT_WHILE) &&
              ((cpd.settings[UO_mod_full_brace_while].a) == AV_REMOVE)))
         {
            examine_brace(pc);
         }
      }
      pc = prev;
   }
}


/**
 * Checks to see if the virtual braces should be converted to real braces.
 *  - over a certain length
 *
 * @param vbopen Virtual Brace Open chunk
 * @return true (convert to real braces) or false (leave alone)
 */
static bool should_add_braces(chunk_t *vbopen)
{
   chunk_t *pc;
   int     nl_max   = cpd.settings[UO_mod_full_brace_nl].n;
   int     nl_count = 0;

   if (nl_max == 0)
   {
      return(false);
   }

   LOG_FMT(LBRDEL, "%s: start on %d : ", __func__, vbopen->orig_line);
   for (pc = chunk_get_next_nc(vbopen, CNAV_PREPROC);
        (pc != NULL) && (pc->level > vbopen->level);
        pc = chunk_get_next_nc(pc, CNAV_PREPROC))
   {
      if (chunk_is_newline(pc))
      {
         nl_count += pc->nl_count;
      }
   }
   if ((pc != NULL) && (nl_count > nl_max) && (vbopen->pp_level == pc->pp_level))
   {
      LOG_FMT(LBRDEL, " exceeded %d newlines\n", nl_max);
      return(true);
   }
   return(false);
}


/**
 * Checks to see if the braces can be removed.
 *  - less than a certain length
 *  - doesn't mess up if/else stuff
 */
static bool can_remove_braces(chunk_t *bopen)
{
   chunk_t *pc;
   chunk_t *prev      = NULL;
   int     semi_count = 0;
   int     level      = bopen->level + 1;
   bool    hit_semi   = false;
   bool    was_fcn    = false;
   int     nl_max     = cpd.settings[UO_mod_full_brace_nl].n;
   int     nl_count   = 0;
   int     if_count   = 0;
   int     br_count   = 0;

   /* Cannot remove braces inside a preprocessor */
   if (bopen->flags & PCF_IN_PREPROC)
   {
      return(false);
   }
   pc = chunk_get_next_ncnl(bopen, CNAV_PREPROC);
   if ((pc != NULL) && (pc->type == CT_BRACE_CLOSE))
   {
      /* Can't remove empty statement */
      return(false);
   }

   LOG_FMT(LBRDEL, "%s: start on %d : ", __func__, bopen->orig_line);

   pc = chunk_get_next_nc(bopen, CNAV_PREPROC);
   while ((pc != NULL) && (pc->level >= level))
   {
      if (chunk_is_newline(pc))
      {
         nl_count += pc->nl_count;
         if ((nl_max > 0) && (nl_count > nl_max))
         {
            LOG_FMT(LBRDEL, " exceeded %d newlines\n", nl_max);
            return(false);
         }
      }
      else
      {
         if (pc->type == CT_BRACE_OPEN)
         {
            br_count++;
         }
         else if (pc->type == CT_BRACE_CLOSE)
         {
            br_count--;
         }
         else if ((pc->type == CT_IF) || (pc->type == CT_ELSEIF))
         {
            if (br_count == 0)
            {
               if_count++;
            }
         }

         if (pc->level == level)
         {
            if ((semi_count > 0) && hit_semi)
            {
               /* should have bailed due to close brace level drop */
               LOG_FMT(LBRDEL, " no close brace\n");
               return(false);
            }

            LOG_FMT(LBRDEL, " [%s %d-%d]", pc->str.c_str(), pc->orig_line, semi_count);

            if (pc->type == CT_ELSE)
            {
               LOG_FMT(LBRDEL, " bailed on %s on line %d\n",
                       pc->str.c_str(), pc->orig_line);
               return(false);
            }

            was_fcn = (prev != NULL) && (prev->type == CT_FPAREN_CLOSE);

            if (chunk_is_semicolon(pc) ||
                (pc->type == CT_IF) ||
                (pc->type == CT_ELSEIF) ||
                (pc->type == CT_FOR) ||
                (pc->type == CT_DO) ||
                (pc->type == CT_WHILE) ||
                (pc->type == CT_USING_STMT) ||
                ((pc->type == CT_BRACE_OPEN) && was_fcn))
            {
               hit_semi |= chunk_is_semicolon(pc);
               if (++semi_count > 1)
               {
                  LOG_FMT(LBRDEL, " bailed on %d because of %s on line %d\n",
                          bopen->orig_line, pc->str.c_str(), pc->orig_line);
                  return(false);
               }
            }
         }
      }
      prev = pc;
      pc   = chunk_get_next_nc(pc, CNAV_PREPROC);
   }

   if (pc == NULL)
   {
      LOG_FMT(LBRDEL, " NULL\n");
      return(false);
   }

   if ((pc->type == CT_BRACE_CLOSE) && (pc->parent_type == CT_IF))
   {
      chunk_t *next = chunk_get_next_ncnl(pc, CNAV_PREPROC);

      prev = chunk_get_prev_ncnl(pc, CNAV_PREPROC);

      if ((next != NULL) && (next->type == CT_ELSE) &&
          ((prev->type == CT_BRACE_CLOSE) || (prev->type == CT_VBRACE_CLOSE)) &&
          (prev->parent_type == CT_IF))
      {
         LOG_FMT(LBRDEL, " - bailed on '%s'[%s] on line %d due to 'if' and 'else' sequence\n",
                 get_token_name(pc->type), get_token_name(pc->parent_type),
                 pc->orig_line);
         return(false);
      }
   }

   LOG_FMT(LBRDEL, " - end on '%s' on line %d. if_count=%d semi_count=%d\n",
           get_token_name(pc->type), pc->orig_line, if_count, semi_count);

   return((pc->type == CT_BRACE_CLOSE) && (pc->pp_level == bopen->pp_level));
}


/**
 * Step forward and count the number of semi colons at the current level.
 * Abort if more than 1 or if we enter a preprocessor
 */
static void examine_brace(chunk_t *bopen)
{
   chunk_t *pc;
   chunk_t *next;
   chunk_t *prev      = NULL;
   int     semi_count = 0;
   int     level      = bopen->level + 1;
   bool    hit_semi   = false;
   bool    was_fcn    = false;
   int     nl_max     = cpd.settings[UO_mod_full_brace_nl].n;
   int     nl_count   = 0;
   int     if_count   = 0;
   int     br_count   = 0;

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
         if (pc->type == CT_BRACE_OPEN)
         {
            br_count++;
         }
         else if (pc->type == CT_BRACE_CLOSE)
         {
            br_count--;
            if (br_count == 0)
            {
               next = chunk_get_next_ncnl(pc, CNAV_PREPROC);
               if ((next == NULL) || (next->type != CT_BRACE_CLOSE))
               {
                  LOG_FMT(LBRDEL, " junk after close brace\n");
                  return;
               }
            }
         }
         else if ((pc->type == CT_IF) || (pc->type == CT_ELSEIF))
         {
            if (br_count == 0)
            {
               if_count++;
            }
         }

         if (pc->level == level)
         {
            if ((semi_count > 0) && hit_semi)
            {
               /* should have bailed due to close brace level drop */
               LOG_FMT(LBRDEL, " no close brace\n");
               return;
            }

            LOG_FMT(LBRDEL, " [%s %d-%d]", pc->str.c_str(), pc->orig_line, semi_count);

            if (pc->type == CT_ELSE)
            {
               LOG_FMT(LBRDEL, " bailed on %s on line %d\n",
                       pc->str.c_str(), pc->orig_line);
               return;
            }

            was_fcn = (prev != NULL) && (prev->type == CT_FPAREN_CLOSE);

            if (chunk_is_semicolon(pc) ||
                (pc->type == CT_IF) ||
                (pc->type == CT_ELSEIF) ||
                (pc->type == CT_FOR) ||
                (pc->type == CT_DO) ||
                (pc->type == CT_WHILE) ||
                (pc->type == CT_USING_STMT) ||
                ((pc->type == CT_BRACE_OPEN) && was_fcn))
            {
               hit_semi |= chunk_is_semicolon(pc);
               if (++semi_count > 1)
               {
                  LOG_FMT(LBRDEL, " bailed on %d because of %s on line %d\n",
                          bopen->orig_line, pc->str.c_str(), pc->orig_line);
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

   LOG_FMT(LBRDEL, " - end on '%s' on line %d. if_count=%d semi_count=%d\n",
           get_token_name(pc->type), pc->orig_line, if_count, semi_count);

   if (pc->type == CT_BRACE_CLOSE)
   {
      next = chunk_get_next_ncnl(pc);
      while ((next != NULL) && (next->type == CT_VBRACE_CLOSE))
      {
         next = chunk_get_next_ncnl(next);
      }
      LOG_FMT(LBRDEL, " next is '%s'\n", get_token_name(next->type));
      if ((if_count > 0) &&
          ((next->type == CT_ELSE) || (next->type == CT_ELSEIF)))
      {
         LOG_FMT(LBRDEL, " bailed on because 'else' is next and %d ifs\n", if_count);
         return;
      }

      if (semi_count > 0)
      {
         if (bopen->parent_type == CT_ELSE)
         {
            next = chunk_get_next_ncnl(bopen);
            if (next->type == CT_IF)
            {
               prev = chunk_get_prev_ncnl(bopen);
               LOG_FMT(LBRDEL, " else-if removing braces on line %d and %d\n",
                       bopen->orig_line, pc->orig_line);

               chunk_del(bopen);
               chunk_del(pc);
               newline_del_between(prev, next);
               if (cpd.settings[UO_nl_else_if].a & AV_ADD)
               {
                  newline_add_between(prev, next);
               }
               return;
            }
         }

         /* we have a pair of braces with only 1 statement inside */
         convert_brace(bopen);
         convert_brace(pc);

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
      LOG_FMT(LBRDEL, " not a close brace? - '%s'\n", pc->str.c_str());
   }
}


/**
 * Converts a single brace into a virtual brace
 */
static void convert_brace(chunk_t *br)
{
   chunk_t *tmp;

   if (br == NULL)
   {
      return;
   }
   else if (br->type == CT_BRACE_OPEN)
   {
      br->type = CT_VBRACE_OPEN;
      br->str.clear();
      tmp = chunk_get_prev(br);
   }
   else if (br->type == CT_BRACE_CLOSE)
   {
      br->type = CT_VBRACE_CLOSE;
      br->str.clear();
      tmp = chunk_get_next(br);
   }
   else
   {
      return;
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


/**
 * Converts a single virtual brace into a brace
 */
static void convert_vbrace(chunk_t *vbr)
{
   if (vbr == NULL)
   {
      return;
   }
   else if (vbr->type == CT_VBRACE_OPEN)
   {
      vbr->type = CT_BRACE_OPEN;
      vbr->str  = "{";

      /* If the next chunk is a preprocessor, then move the open brace after the
       * preprocessor.
       */
      chunk_t *tmp = chunk_get_next(vbr);
      if ((tmp != NULL) && (tmp->type == CT_PREPROC))
      {
         tmp = chunk_get_next(vbr, CNAV_PREPROC);
         chunk_move_after(vbr, tmp);
         newline_add_after(vbr);
      }
   }
   else if (vbr->type == CT_VBRACE_CLOSE)
   {
      vbr->type = CT_BRACE_CLOSE;
      vbr->str  = "}";

      /* If the next chunk is a comment, followed by a newline, then
       * move the brace after the newline and add another newline after
       * the close brace.
       */
      chunk_t *tmp = chunk_get_next(vbr);
      if (chunk_is_comment(tmp))
      {
         tmp = chunk_get_next(tmp);
         if (chunk_is_newline(tmp))
         {
            chunk_move_after(vbr, tmp);
            newline_add_after(vbr);
         }
      }
   }
}


static void convert_vbrace_to_brace(void)
{
   chunk_t *pc;
   chunk_t *tmp;
   chunk_t *vbc;
   bool    in_preproc;

   /* Find every vbrace open */
   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->type != CT_VBRACE_OPEN)
      {
         continue;
      }

      in_preproc = (pc->flags & PCF_IN_PREPROC) != 0;

      if ((((pc->parent_type == CT_IF) ||
            (pc->parent_type == CT_ELSE) ||
            (pc->parent_type == CT_ELSEIF)) &&
           ((cpd.settings[UO_mod_full_brace_if].a & AV_ADD) != 0) &&
            !cpd.settings[UO_mod_full_brace_if_chain].b)
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
          ((pc->parent_type == CT_USING_STMT) &&
           ((cpd.settings[UO_mod_full_brace_using].a & AV_ADD) != 0))
          ||
          ((pc->parent_type == CT_FUNC_DEF) &&
           ((cpd.settings[UO_mod_full_brace_function].a & AV_ADD) != 0)))
      {
         /* Find the matching vbrace close */
         vbc = NULL;
         tmp = pc;
         while ((tmp = chunk_get_next(tmp)) != NULL)
         {
            if (in_preproc && ((tmp->flags & PCF_IN_PREPROC) == 0))
            {
               /* Can't leave a preprocessor */
               break;
            }
            if ((pc->brace_level == tmp->brace_level) &&
                (tmp->type == CT_VBRACE_CLOSE) &&
                (pc->parent_type == tmp->parent_type) &&
                ((tmp->flags & PCF_IN_PREPROC) == (pc->flags & PCF_IN_PREPROC)))
            {
               vbc = tmp;
               break;
            }
         }
         if (vbc == NULL)
         {
            continue;
         }

         convert_vbrace(pc);
         convert_vbrace(vbc);
      }
   }
}


/**
 * Adds a comment after the ref chunk
 * Returns the added chunk or NULL
 */
chunk_t *insert_comment_after(chunk_t *ref, c_token_t cmt_type,
                              const unc_text& cmt_text)
{
   chunk_t new_cmt;

   new_cmt      = *ref;
   new_cmt.prev = NULL;
   new_cmt.next = NULL;

   new_cmt.flags = (ref->flags & PCF_COPY_FLAGS);
   new_cmt.type  = cmt_type;

   new_cmt.str.clear();
   if (cmt_type == CT_COMMENT_CPP)
   {
      new_cmt.str.append("// ");
      new_cmt.str.append(cmt_text);
   }
   else
   {
      new_cmt.str.append("/* ");
      new_cmt.str.append(cmt_text);
      new_cmt.str.append(" */");
   }
   /* TODO: expand comment type to cover other comment styles? */

   new_cmt.column   = ref->column + ref->len() + 1;
   new_cmt.orig_col = new_cmt.column;

   return(chunk_add_after(&new_cmt, ref));
}


/*
 * See also it's preprocessor counterpart
 *   add_long_preprocessor_conditional_block_comment
 * in defines.cpp
 */
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
      if ((pc->type == CT_FUNC_DEF) ||
          (pc->type == CT_OC_MSG_DECL))
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
            if ((tmp == NULL) || chunk_is_newline(tmp))
            {
               int     nl_min  = 0;
               chunk_t *tag_pc = NULL;

               if (br_open->parent_type == CT_SWITCH)
               {
                  nl_min = cpd.settings[UO_mod_add_long_switch_closebrace_comment].n;
                  tag_pc = sw_pc;
               }
               else if ((br_open->parent_type == CT_FUNC_DEF) ||
                        (br_open->parent_type == CT_OC_MSG_DECL))
               {
                  nl_min = cpd.settings[UO_mod_add_long_function_closebrace_comment].n;
                  tag_pc = fcn_pc;
               }

               if ((nl_min > 0) && (nl_count >= nl_min) && (tag_pc != NULL))
               {
                  /* determine the added comment style */
                  c_token_t style = (cpd.lang_flags & (LANG_CPP | LANG_CS)) ?
                                    CT_COMMENT_CPP : CT_COMMENT;

                  /* Add a comment after the close brace */
                  insert_comment_after(br_close, style, tag_pc->str);
               }
            }
            break;
         }
      }
   }
}


static void move_case_break(void)
{
   chunk_t *pc;
   chunk_t *prev = NULL;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if ((pc->type == CT_BREAK) &&
          (prev != NULL) &&
          (prev->type == CT_BRACE_CLOSE) &&
          (prev->parent_type == CT_CASE))
      {
         if (chunk_is_newline(chunk_get_prev(pc)) &&
             chunk_is_newline(chunk_get_prev(prev)))
         {
            chunk_swap_lines(prev, pc);
         }
      }
      prev = pc;
   }
}


/**
 * Remove the case brace, if allowable.
 */
static chunk_t *mod_case_brace_remove(chunk_t *br_open)
{
   chunk_t *pc;
   chunk_t *br_close;
   chunk_t *next = chunk_get_next_ncnl(br_open, CNAV_PREPROC);

   LOG_FMT(LMCB, "%s: line %d", __func__, br_open->orig_line);

   /* Find the matching brace close */
   br_close = chunk_get_next_type(br_open, CT_BRACE_CLOSE, br_open->level, CNAV_PREPROC);
   if (br_close == NULL)
   {
      LOG_FMT(LMCB, " - no close\n");
      return(next);
   }

   /* Make sure 'break', 'return', 'goto', 'case' or '}' is after the close brace */
   pc = chunk_get_next_ncnl(br_close, CNAV_PREPROC);
   if ((pc == NULL) ||
       ((pc->type != CT_BREAK) &&
        (pc->type != CT_RETURN) &&
        (pc->type != CT_CASE) &&
        (pc->type != CT_GOTO) &&
        (pc->type != CT_BRACE_CLOSE)))
   {
      LOG_FMT(LMCB, " - after '%s'\n",
              (pc == NULL) ? "<null>" : get_token_name(pc->type));
      return(next);
   }

   /* scan to make sure there are no definitions at brace level between braces */
   for (pc = br_open; pc != br_close; pc = chunk_get_next_ncnl(pc, CNAV_PREPROC))
   {
      if ((pc->level == (br_open->level + 1)) && (pc->flags & PCF_VAR_DEF))
      {
         LOG_FMT(LMCB, " - vardef on line %d: '%s'\n", pc->orig_line, pc->str.c_str());
         return(next);
      }
   }
   LOG_FMT(LMCB, " - removing braces on lines %d and %d\n",
           br_open->orig_line, br_close->orig_line);

   for (pc = br_open; pc != br_close; pc = chunk_get_next_ncnl(pc, CNAV_PREPROC))
   {
      pc->brace_level--;
      pc->level--;
   }
   next = chunk_get_prev(br_open, CNAV_PREPROC);
   chunk_del(br_open);
   chunk_del(br_close);
   return(chunk_get_next(next, CNAV_PREPROC));
}


/**
 * Add the case brace, if allowable.
 */
static chunk_t *mod_case_brace_add(chunk_t *cl_colon)
{
   chunk_t *pc   = cl_colon;
   chunk_t *last = NULL;
   chunk_t *next = chunk_get_next_ncnl(cl_colon, CNAV_PREPROC);
   chunk_t *br_open;
   chunk_t *br_close;
   chunk_t chunk;

   LOG_FMT(LMCB, "%s: line %d", __func__, pc->orig_line);

   while ((pc = chunk_get_next_ncnl(pc, CNAV_PREPROC)) != NULL)
   {
      if (pc->level < cl_colon->level)
      {
         LOG_FMT(LMCB, " - level drop\n");
         return(next);
      }

      if ((pc->level == cl_colon->level) &&
          ((pc->type == CT_CASE) ||
           (pc->type == CT_BREAK)))
      {
         last = pc;
         //if (pc->type == CT_BREAK)
         //{
         //   /* Step past the semicolon */
         //   last = chunk_get_next_ncnl(chunk_get_next_ncnl(last));
         //}
         break;
      }
   }

   if (last == NULL)
   {
      LOG_FMT(LMCB, " - NULL last\n");
      return(next);
   }

   LOG_FMT(LMCB, " - adding before '%s' on line %d\n", last->str.c_str(), last->orig_line);

   chunk.type        = CT_BRACE_OPEN;
   chunk.orig_line   = cl_colon->orig_line;
   chunk.parent_type = CT_CASE;
   chunk.level       = cl_colon->level;
   chunk.brace_level = cl_colon->brace_level;
   chunk.flags       = pc->flags & PCF_COPY_FLAGS;
   chunk.str         = "{";

   br_open = chunk_add_after(&chunk, cl_colon);

   chunk.type      = CT_BRACE_CLOSE;
   chunk.orig_line = last->orig_line;
   chunk.str       = "}";

   br_close = chunk_add_before(&chunk, last);
   newline_add_before(last);

   for (pc = chunk_get_next(br_open, CNAV_PREPROC);
        pc != br_close;
        pc = chunk_get_next(pc, CNAV_PREPROC))
   {
      pc->level++;
      pc->brace_level++;
   }

   return(br_open);
}


static void mod_case_brace(void)
{
   chunk_t *pc = chunk_get_head();
   chunk_t *next;

   while (pc != NULL)
   {
      next = chunk_get_next_ncnl(pc, CNAV_PREPROC);
      if (next == NULL)
      {
         return;
      }

      if ((cpd.settings[UO_mod_case_brace].a == AV_REMOVE) &&
          (pc->type == CT_BRACE_OPEN) &&
          (pc->parent_type == CT_CASE))
      {
         pc = mod_case_brace_remove(pc);
      }
      else if ((cpd.settings[UO_mod_case_brace].a & AV_ADD) &&
               (pc->type == CT_CASE_COLON) &&
               (next->type != CT_BRACE_OPEN) &&
               (next->type != CT_BRACE_CLOSE) &&
               (next->type != CT_CASE))
      {
         pc = mod_case_brace_add(pc);
      }
      else
      {
         pc = chunk_get_next_ncnl(pc, CNAV_PREPROC);
      }
   }
}


/**
 * Traverse the if chain and see if all can be removed
 */
static void process_if_chain(chunk_t *br_start)
{
   chunk_t *braces[256];
   chunk_t *br_close;
   int     br_cnt = 0;
   chunk_t *pc;
   bool    must_have_braces = false;
   bool    tmp;

   pc = br_start;

   LOG_FMT(LBRCH, "%s: if starts on line %d\n", __func__, br_start->orig_line);

   while (pc != NULL)
   {
      if (pc->type == CT_BRACE_OPEN)
      {
         tmp = can_remove_braces(pc);
         LOG_FMT(LBRCH, "  [%d] line %d - can%s remove %s\n",
                 br_cnt, pc->orig_line, tmp ? "" : "not",
                 get_token_name(pc->type));
         if (!tmp)
         {
            must_have_braces = true;
         }
      }
      else
      {
         tmp = should_add_braces(pc);
         if (tmp)
         {
            must_have_braces = true;
         }
         LOG_FMT(LBRCH, "  [%d] line %d - %s %s\n",
                 br_cnt, pc->orig_line, tmp ? "should add" : "ignore",
                 get_token_name(pc->type));
      }

      braces[br_cnt++] = pc;
      br_close         = chunk_skip_to_match(pc, CNAV_PREPROC);
      if (br_close == NULL)
      {
         break;
      }
      braces[br_cnt++] = br_close;

      pc = chunk_get_next_ncnl(br_close, CNAV_PREPROC);
      if ((pc == NULL) || (pc->type != CT_ELSE))
      {
         break;
      }
      pc = chunk_get_next_ncnl(pc, CNAV_PREPROC);
      if ((pc != NULL) && (pc->type == CT_ELSEIF))
      {
         while ((pc != NULL) && (pc->type != CT_VBRACE_OPEN) && (pc->type != CT_BRACE_OPEN))
         {
            pc = chunk_get_next_ncnl(pc, CNAV_PREPROC);
         }
      }
      if (pc == NULL)
      {
         break;
      }
      if ((pc->type != CT_BRACE_OPEN) && (pc->type != CT_VBRACE_OPEN))
      {
         break;
      }
   }

   if (must_have_braces)
   {
      LOG_FMT(LBRCH, "%s: add braces on lines[%d]:", __func__, br_cnt);
      while (--br_cnt >= 0)
      {
         if ((braces[br_cnt]->type == CT_VBRACE_OPEN) ||
             (braces[br_cnt]->type == CT_VBRACE_CLOSE))
         {
            LOG_FMT(LBRCH, " %d", braces[br_cnt]->orig_line);
            convert_vbrace(braces[br_cnt]);
         }
         else
         {
            LOG_FMT(LBRCH, " {%d}", braces[br_cnt]->orig_line);
         }
         braces[br_cnt] = NULL;
      }
      LOG_FMT(LBRCH, "\n");
   }
   else
   {
      LOG_FMT(LBRCH, "%s: remove braces on lines[%d]:", __func__, br_cnt);
      while (--br_cnt >= 0)
      {
         if ((braces[br_cnt]->type == CT_BRACE_OPEN) ||
             (braces[br_cnt]->type == CT_BRACE_CLOSE))
         {
            LOG_FMT(LBRCH, " {%d}", braces[br_cnt]->orig_line);
            convert_brace(braces[br_cnt]);
         }
         else
         {
            LOG_FMT(LBRCH, " %d", braces[br_cnt]->orig_line);
         }
         braces[br_cnt] = NULL;
      }
      LOG_FMT(LBRCH, "\n");
   }
}


static void mod_full_brace_if_chain(void)
{
   chunk_t *pc;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if (((pc->type == CT_BRACE_OPEN) || (pc->type == CT_VBRACE_OPEN)) &&
          (pc->parent_type == CT_IF))
      {
         process_if_chain(pc);
      }
   }
}
