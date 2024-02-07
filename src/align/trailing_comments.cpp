/**
 * @file trailing_comments.cpp
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align/trailing_comments.h"

#include "align/add.h"
#include "align/tab_column.h"
#include "chunk.h"
#include "ChunkStack.h"
#include "indent.h"
#include "log_rules.h"


constexpr static auto LCURRENT = LALTC;


using namespace uncrustify;


void align_stack(ChunkStack &cs, size_t col, bool align_single, log_sev_t sev)
{
   LOG_FUNC_ENTRY();

   log_rule_B("align_on_tabstop");

   if (options::align_on_tabstop())
   {
      col = align_tab_column(col);
   }

   if (  (cs.Len() > 1)
      || (  align_single
         && (cs.Len() == 1)))
   {
      LOG_FMT(sev, "%s(%d): max_col=%zu\n", __func__, __LINE__, col);
      Chunk *pc;

      while ((pc = cs.Pop_Back())->IsNotNullChunk())
      {
         align_to_column(pc, col);
         pc->SetFlagBits(PCF_WAS_ALIGNED);

         LOG_FMT(sev, "%s(%d): indented [%s] on line %zu to %zu\n",
                 __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetColumn());
      }
   }
   cs.Reset();
} // align_stack


Chunk *align_trailing_comments(Chunk *start)
{
   LOG_FUNC_ENTRY();
   size_t       min_col  = 0;
   size_t       min_orig = 0;
   Chunk        *pc      = start;
   const size_t lvl      = start->GetBraceLevel();
   size_t       nl_count = 0;
   ChunkStack   cs;
   size_t       col;

   log_rule_B("align_right_cmt_at_col");
   size_t intended_col = options::align_right_cmt_at_col();

   log_rule_B("align_right_cmt_same_level");
   const bool      same_level = options::align_right_cmt_same_level();
   comment_align_e cmt_type_cur;
   comment_align_e cmt_type_start = get_comment_align_type(pc);

   LOG_FMT(LALADD, "%s(%d): start on line=%zu\n",
           __func__, __LINE__, pc->GetOrigLine());

   // Find the max column
   log_rule_B("align_right_cmt_span");

   while (  pc->IsNotNullChunk()
         && (nl_count < options::align_right_cmt_span()))
   {
      if (  pc->TestFlags(PCF_RIGHT_COMMENT)
         && pc->GetColumn() > 1)
      {
         if (  same_level
            && pc->GetBraceLevel() != lvl)
         {
            pc = pc->GetPrev();
            break;
         }
         cmt_type_cur = get_comment_align_type(pc);

         if (cmt_type_cur == cmt_type_start)
         {
            LOG_FMT(LALADD, "%s(%d): line=%zu min_col=%zu pc->col=%zu pc->len=%zu %s\n",
                    __func__, __LINE__, pc->GetOrigLine(), min_col, pc->GetColumn(), pc->Len(),
                    get_token_name(pc->GetType()));

            if (  min_orig == 0
               || min_orig > pc->GetColumn())
            {
               min_orig = pc->GetColumn();
            }
            align_add(cs, pc, min_col); // (intended_col < col));
            nl_count = 0;
         }
      }

      if (pc->IsNewline())
      {
         nl_count += pc->GetNlCount();
      }
      pc = pc->GetNext();
   }
   // Start with the minimum original column
   col = min_orig;

   // fall back to the intended column
   if (  intended_col > 0
      && col > intended_col)
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

   if (  cpd.frag_cols > 0
      && cpd.frag_cols <= col)
   {
      col -= cpd.frag_cols;
   }
   align_stack(cs, col, (intended_col != 0), LALTC);

   return(pc->GetNext());
} // align_trailing_comments


comment_align_e get_comment_align_type(Chunk *cmt)
{
   Chunk           *prev;
   comment_align_e cmt_type = comment_align_e::REGULAR;

   log_rule_B("align_right_cmt_mix");

   if (  !options::align_right_cmt_mix()
      && cmt->IsNotNullChunk()
      && ((prev = cmt->GetPrev())->IsNotNullChunk()))
   {
      if (  prev->Is(CT_PP_ENDIF)
         || prev->Is(CT_PP_ELSE)
         || prev->Is(CT_ELSE)
         || prev->Is(CT_BRACE_CLOSE))
      {
         // TODO: make the magic 3 configurable
         if ((cmt->GetColumn() - (prev->GetColumn() + prev->Len())) < 3)
         {
            cmt_type = (prev->Is(CT_PP_ENDIF)) ? comment_align_e::ENDIF : comment_align_e::BRACE;
         }
      }
   }
   return(cmt_type);
} // get_comment_align_type


void align_right_comments()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (  pc->Is(CT_COMMENT)
         || pc->Is(CT_COMMENT_CPP)
         || pc->Is(CT_COMMENT_MULTI))
      {
         if (pc->GetParentType() == CT_COMMENT_END)
         {
            log_rule_B("align_right_cmt_gap");

            if (pc->GetOrigPrevSp() < options::align_right_cmt_gap())
            {
               LOG_FMT(LALTC, "NOT changing END comment on line %zu (%zu < %u)\n",
                       pc->GetOrigLine(), pc->GetOrigPrevSp(),
                       options::align_right_cmt_gap());
            }
            else
            {
               LOG_FMT(LALTC, "Changing END comment on line %zu into a RIGHT-comment\n",
                       pc->GetOrigLine());
               pc->SetFlagBits(PCF_RIGHT_COMMENT);
            }
         }

         // Change certain WHOLE comments into RIGHT-alignable comments
         if (pc->GetParentType() == CT_COMMENT_WHOLE)
         {
            log_rule_B("input_tab_size");
            size_t max_col = pc->GetColumnIndent() + options::input_tab_size();

            // If the comment is further right than the brace level...
            if (pc->GetColumn() >= max_col)
            {
               LOG_FMT(LALTC, "Changing WHOLE comment on line %zu into a RIGHT-comment (col=%zu col_ind=%zu max_col=%zu)\n",
                       pc->GetOrigLine(), pc->GetColumn(), pc->GetColumnIndent(), max_col);

               pc->SetFlagBits(PCF_RIGHT_COMMENT);
            }
         }
      }
   }

   Chunk *pc = Chunk::GetHead();

   while (pc->IsNotNullChunk())
   {
      if (pc->TestFlags(PCF_RIGHT_COMMENT))
      {
         pc = align_trailing_comments(pc);
      }
      else
      {
         pc = pc->GetNext();
      }
   }
} // align_right_comments
