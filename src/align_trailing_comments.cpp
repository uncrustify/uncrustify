/**
 * @file align_trailing_comments.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_trailing_comments.h"

#include "align_add.h"
#include "align_tab_column.h"
#include "indent.h"
#include "uncrustify.h"

using namespace uncrustify;


void align_stack(ChunkStack &cs, size_t col, bool align_single, log_sev_t sev)
{
   LOG_FUNC_ENTRY();

   if (options::align_on_tabstop())
   {
      col = align_tab_column(col);
   }

   if (  (cs.Len() > 1)
      || (align_single && (cs.Len() == 1)))
   {
      LOG_FMT(sev, "%s(%d): max_col=%zu\n", __func__, __LINE__, col);
      chunk_t *pc;
      while ((pc = cs.Pop_Back()) != nullptr)
      {
         align_to_column(pc, col);
         chunk_flags_set(pc, PCF_WAS_ALIGNED);

         LOG_FMT(sev, "%s(%d): indented [%s] on line %zu to %zu\n",
                 __func__, __LINE__, pc->text(), pc->orig_line, pc->column);
      }
   }
   cs.Reset();
} // align_stack


chunk_t *align_trailing_comments(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   size_t          min_col  = 0;
   size_t          min_orig = 0;
   chunk_t         *pc      = start;
   const size_t    lvl      = start->brace_level;
   size_t          nl_count = 0;
   ChunkStack      cs;
   size_t          col;
   size_t          intended_col = options::align_right_cmt_at_col();
   const bool      same_level   = options::align_right_cmt_same_level();
   comment_align_e cmt_type_cur;
   comment_align_e cmt_type_start = get_comment_align_type(pc);

   LOG_FMT(LALADD, "%s(%d): start on line=%zu\n",
           __func__, __LINE__, pc->orig_line);

   // Find the max column
   while (  pc != nullptr
         && (nl_count < options::align_right_cmt_span()))
   {
      if ((pc->flags & PCF_RIGHT_COMMENT) && pc->column > 1)
      {
         if (same_level && pc->brace_level != lvl)
         {
            pc = chunk_get_prev(pc);
            break;
         }

         cmt_type_cur = get_comment_align_type(pc);

         if (cmt_type_cur == cmt_type_start)
         {
            LOG_FMT(LALADD, "%s(%d): line=%zu min_col=%zu pc->col=%zu pc->len=%zu %s\n",
                    __func__, __LINE__, pc->orig_line, min_col, pc->column, pc->len(),
                    get_token_name(pc->type));
            if (min_orig == 0 || min_orig > pc->column)
            {
               min_orig = pc->column;
            }
            align_add(cs, pc, min_col); // (intended_col < col));
            nl_count = 0;
         }
      }
      if (chunk_is_newline(pc))
      {
         nl_count += pc->nl_count;
      }
      pc = chunk_get_next(pc);
   }

   // Start with the minimum original column
   col = min_orig;
   // fall back to the intended column
   if (intended_col > 0 && col > intended_col)
   {
      col = intended_col;
   }
   // if less than allowed, bump it out
   if (col < min_col)
   {
      col = min_col;
   }
   // bump out to the intended column
   if (col < intended_col)
   {
      col = intended_col;
   }
   LOG_FMT(LALADD, "%s(%d):  -- min_orig=%zu intended_col=%zu min_allowed=%zu ==> col=%zu\n",
           __func__, __LINE__, min_orig, intended_col, min_col, col);
   if (cpd.frag_cols > 0 && cpd.frag_cols <= col)
   {
      col -= cpd.frag_cols;
   }
   align_stack(cs, col, (intended_col != 0), LALTC);

   return(chunk_get_next(pc));
} // align_trailing_comments


comment_align_e get_comment_align_type(chunk_t *cmt)
{
   chunk_t         *prev;
   comment_align_e cmt_type = comment_align_e::REGULAR;

   if (  !options::align_right_cmt_mix()
      && ((prev = chunk_get_prev(cmt)) != nullptr))
   {
      if (  chunk_is_token(prev, CT_PP_ENDIF)
         || chunk_is_token(prev, CT_PP_ELSE)
         || chunk_is_token(prev, CT_ELSE)
         || chunk_is_token(prev, CT_BRACE_CLOSE))
      {
         // TODO: make the magic 3 configurable
         if ((cmt->column - (prev->column + prev->len())) < 3)
         {
            cmt_type = (chunk_is_token(prev, CT_PP_ENDIF)) ? comment_align_e::ENDIF : comment_align_e::BRACE;
         }
      }
   }
   return(cmt_type);
} // get_comment_align_type


void align_right_comments(void)
{
   LOG_FUNC_ENTRY();

   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (  chunk_is_token(pc, CT_COMMENT)
         || chunk_is_token(pc, CT_COMMENT_CPP)
         || chunk_is_token(pc, CT_COMMENT_MULTI))
      {
         if (pc->parent_type == CT_COMMENT_END)
         {
            chunk_t *prev = chunk_get_prev(pc);

            if (pc->orig_col < prev->orig_col_end + options::align_right_cmt_gap())
            {
               LOG_FMT(LALTC, "NOT changing END comment on line %zu (%zu <= %zu + %u)\n",
                       pc->orig_line, pc->orig_col, prev->orig_col_end,
                       options::align_right_cmt_gap());
            }
            else
            {
               LOG_FMT(LALTC, "Changing END comment on line %zu into a RIGHT-comment\n",
                       pc->orig_line);
               chunk_flags_set(pc, PCF_RIGHT_COMMENT);
            }
         }

         // Change certain WHOLE comments into RIGHT-alignable comments
         if (pc->parent_type == CT_COMMENT_WHOLE)
         {
            size_t max_col = pc->column_indent + options::input_tab_size();

            // If the comment is further right than the brace level...
            if (pc->column >= max_col)
            {
               LOG_FMT(LALTC, "Changing WHOLE comment on line %zu into a RIGHT-comment (col=%zu col_ind=%zu max_col=%zu)\n",
                       pc->orig_line, pc->column, pc->column_indent, max_col);

               chunk_flags_set(pc, PCF_RIGHT_COMMENT);
            }
         }
      }
   }

   chunk_t *pc = chunk_get_head();
   while (pc != nullptr)
   {
      if (pc->flags & PCF_RIGHT_COMMENT)
      {
         pc = align_trailing_comments(pc);
      }
      else
      {
         pc = chunk_get_next(pc);
      }
   }
} // align_right_comments
