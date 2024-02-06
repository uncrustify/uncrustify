/**
 * @file oc_msg_colons.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "align/oc_msg_colons.h"

#include "align/stack.h"
#include "log_rules.h"


constexpr static auto LCURRENT = LOCMSG;


using namespace uncrustify;


void align_oc_msg_colon(Chunk *so)
{
   LOG_FUNC_ENTRY();

   AlignStack nas;   // for the parameter tag

   nas.Start(1);
   nas.Reset();
   log_rule_B("align_on_tabstop");
   nas.m_right_align = !options::align_on_tabstop();

   AlignStack cas;   // for the colons

   log_rule_B("align_oc_msg_colon_span");
   size_t span = options::align_oc_msg_colon_span();

   cas.Start(span);

   size_t level = so->GetLevel();
   Chunk  *pc   = so->GetNextNcNnl(E_Scope::PREPROC);

   bool   did_line   = false;
   bool   has_colon  = false;
   size_t lcnt       = 0;  // line count with no colon for span
   bool   first_line = true;

   while (  pc->IsNotNullChunk()
         && pc->GetLevel() > level)
   {
      if (pc->GetLevel() > (level + 1))
      {
         // do nothing
      }
      else if (pc->IsNewline())
      {
         if (!has_colon)
         {
            ++lcnt;
         }
         did_line = false;

         log_rule_B("align_oc_msg_colon_xcode_like");

         if (  options::align_oc_msg_colon_xcode_like()
            && first_line
            && !has_colon)
         {
            span = 0;
         }
         has_colon  = !has_colon;
         first_line = false;
      }
      else if (  !did_line
              && (lcnt < span + 1)
              && pc->Is(CT_OC_COLON))
      {
         has_colon = true;
         cas.Add(pc);
         Chunk *tmp = pc->GetPrev();

         if (  tmp->IsNotNullChunk()
            && (  tmp->Is(CT_OC_MSG_FUNC)
               || tmp->Is(CT_OC_MSG_NAME)))
         {
            nas.Add(tmp);
            tmp->SetFlagBits(PCF_DONT_INDENT);
         }
         did_line = true;
      }
      pc = pc->GetNext(E_Scope::PREPROC);
   }
   log_rule_B("align_oc_msg_colon_first");
   nas.m_skip_first = !options::align_oc_msg_colon_first();
   cas.m_skip_first = !options::align_oc_msg_colon_first();

   // find the longest args that isn't the first one
   size_t first_len = 0;
   size_t mlen      = 0;
   Chunk  *longest  = Chunk::NullChunkPtr;

   size_t len = nas.m_aligned.Len();

   for (size_t idx = 0; idx < len; idx++)
   {
      Chunk *tmp = nas.m_aligned.GetChunk(idx);

      if (tmp->IsNotNullChunk())
      {
         size_t tlen = tmp->GetStr().size();

         if (tlen > mlen)
         {
            mlen = tlen;

            if (idx != 0)
            {
               longest = tmp;
            }
         }

         if (idx == 0)
         {
            first_len = tlen + 1;
         }
      }
   }

   // add spaces before the longest arg
   log_rule_B("indent_oc_msg_colon");
   len = options::indent_oc_msg_colon();
   size_t len_diff = mlen - first_len;

   log_rule_B("indent_columns");
   size_t indent_size = options::indent_columns();

   // Align with first colon if possible by removing spaces
   log_rule_B("indent_oc_msg_prioritize_first_colon");

   if (  longest->IsNotNullChunk()
      && options::indent_oc_msg_prioritize_first_colon()
      && len_diff > 0
      && (  (longest->GetColumn() >= len_diff)
         && (longest->GetColumn() - len_diff) > (longest->GetBraceLevel() * indent_size)))
   {
      longest->SetColumn(longest->GetColumn() - len_diff);
   }
   else if (  longest->IsNotNullChunk()
           && len > 0)
   {
      Chunk chunk;

      chunk.SetType(CT_SPACE);
      chunk.SetParentType(CT_NONE);
      chunk.SetOrigLine(longest->GetOrigLine());
      chunk.SetOrigCol(longest->GetOrigCol());
      chunk.SetLevel(longest->GetLevel());
      chunk.SetBraceLevel(longest->GetBraceLevel());
      chunk.SetFlags(longest->GetFlags() & PCF_COPY_FLAGS);

      // start at one since we already indent for the '['
      for (size_t idx = 1; idx < len; idx++)
      {
         chunk.Str().append(' ');
      }

      chunk.CopyAndAddBefore(longest);
   }
   nas.End();
   cas.End();
} // align_oc_msg_colon


void align_oc_msg_colons()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (  pc->Is(CT_SQUARE_OPEN)
         && pc->GetParentType() == CT_OC_MSG)
      {
         align_oc_msg_colon(pc);
      }
   }
} // align_oc_msg_colons
