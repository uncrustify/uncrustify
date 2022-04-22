/**
 * @file align_same_func_call_params.cpp
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "align_same_func_call_params.h"

#include "align_stack.h"
#include "log_rules.h"

constexpr static auto LCURRENT = LASFCP;

using namespace uncrustify;


void align_same_func_call_params(void)
{
   LOG_FUNC_ENTRY();

   Chunk             *pc;
   Chunk             *align_root = Chunk::NullChunkPtr;
   Chunk             *align_cur  = Chunk::NullChunkPtr;
   size_t            align_len   = 0;
   size_t            span        = 3;
   size_t            thresh;
   Chunk             *align_fcn;
   unc_text          align_fcn_name;
   unc_text          align_root_name;
   deque<Chunk *>    chunks;
   deque<AlignStack> array_of_AlignStack;
   AlignStack        fcn_as;
   const char        *add_str;

   // Default span is 3 if align_same_func_call_params is true
   log_rule_B("align_same_func_call_params_span");

   if (options::align_same_func_call_params_span() > 0)
   {
      span = options::align_same_func_call_params_span();
   }
   log_rule_B("align_same_func_call_params_thresh");
   thresh = options::align_same_func_call_params_thresh();

   fcn_as.Start(span, thresh);
   LOG_FMT(LAS, "%s(%d): (3): span is %zu, thresh is %zu\n",
           __func__, __LINE__, span, thresh);

   for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (chunk_is_newline(pc))
      {
         LOG_FMT(LAS, "%s(%d): orig_line is %zu, <Newline>\n", __func__, __LINE__, pc->orig_line);
      }
      else
      {
         LOG_FMT(LAS, "%s(%d): orig_line is %zu, orig_col is %zu, pc->Text() '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
      }

      if (chunk_is_not_token(pc, CT_FUNC_CALL))
      {
         if (chunk_is_newline(pc))
         {
            for (auto &as_v : array_of_AlignStack)
            {
               as_v.NewLines(pc->nl_count);
            }

            fcn_as.NewLines(pc->nl_count);
         }
         else
         {
            // if we drop below the brace level that started it, we are done
            if (  align_root->IsNotNullChunk()
               && align_root->brace_level > pc->brace_level)
            {
               LOG_FMT(LASFCP, "  ++ (drop) Ended with %zu fcns\n", align_len);

               // Flush it all!
               fcn_as.Flush();

               for (auto &as_v : array_of_AlignStack)
               {
                  as_v.Flush();
               }

               align_root = Chunk::NullChunkPtr;
            }
         }
         continue;
      }
      // Only align function calls that are right after a newline
      Chunk *prev = pc->GetPrev();

      while (  chunk_is_token(prev, CT_MEMBER)
            || chunk_is_token(prev, CT_DC_MEMBER))
      {
         Chunk *tprev = prev->GetPrev();

         if (chunk_is_not_token(tprev, CT_TYPE))
         {
            prev = tprev;
            break;
         }
         prev = tprev->GetPrev();
      }

      if (!chunk_is_newline(prev))
      {
         continue;
      }
      prev      = prev->GetNext();
      align_fcn = prev;
      align_fcn_name.clear();
      LOG_FMT(LASFCP, "%s(%d):\n", __func__, __LINE__);

      while (prev != pc)
      {
         align_fcn_name += prev->str;
         prev            = prev->GetNext();
      }
      align_fcn_name += pc->str;
      LOG_FMT(LASFCP, "%s(%d): Func Call found at orig_line is %zu, orig_col is %zu, c_str() '%s'\n",
              __func__, __LINE__, align_fcn->orig_line,
              align_fcn->orig_col,
              align_fcn_name.c_str());

      add_str = nullptr;

      if (align_root->IsNotNullChunk())
      {
         // Issue # 1395
         // can only align functions on the same brace level
         // and on the same level
         LOG_FMT(LASFCP, "%s(%d):align_root is not nullptr\n", __func__, __LINE__);

         if (  align_root->brace_level == pc->brace_level
            && align_root->level == pc->level
            && align_fcn_name.equals(align_root_name))
         {
            fcn_as.Add(pc);
            align_cur->align.next = pc;
            align_cur             = pc;
            align_len++;
            add_str = "  Add";
         }
         else
         {
            LOG_FMT(LASFCP, "  ++ Ended with %zu fcns\n", align_len);

            // Flush it all!
            fcn_as.Flush();

            for (auto &as_v : array_of_AlignStack)
            {
               as_v.Flush();
            }

            align_root = Chunk::NullChunkPtr;
         }
      }
      LOG_FMT(LASFCP, "%s(%d):\n", __func__, __LINE__);

      if (align_root->IsNullChunk())
      {
         LOG_FMT(LASFCP, "%s(%d):align_root is null chunk, Add pc '%s'\n", __func__, __LINE__, pc->Text());
         fcn_as.Add(pc);
         align_root      = align_fcn;
         align_root_name = align_fcn_name;
         align_cur       = pc;
         align_len       = 1;
         add_str         = "Start";
      }
      LOG_FMT(LASFCP, "%s(%d):\n", __func__, __LINE__);

      if (add_str != nullptr)
      {
         LOG_FMT(LASFCP, "%s(%d): %s with function '%s', on orig_line %zu, ",
                 __func__, __LINE__, add_str, align_fcn_name.c_str(), pc->orig_line);
         align_params(pc, chunks);
         LOG_FMT(LASFCP, "%zu items:", chunks.size());

         for (size_t idx = 0; idx < chunks.size(); idx++)
         {
            // show the chunk(s)
            LOG_FMT(LASFCP, " [%s]", chunks[idx]->Text());

            if (idx < chunks.size() - 1)
            {
               LOG_FMT(LASFCP, ",");
            }
         }

         LOG_FMT(LASFCP, "\n");

         for (size_t idx = 0; idx < chunks.size(); idx++)
         {
            LOG_FMT(LASFCP, "%s(%d): chunks[%zu] is [%s]\n", __func__, __LINE__, idx, chunks[idx]->Text());
            // Issue #2368

            if (array_of_AlignStack.size() > idx)
            {
               // Issue #2368
               array_of_AlignStack[idx].m_right_align = false;
            }

            if (idx >= array_of_AlignStack.size())
            {
               LOG_FMT(LASFCP, "%s(%d): resize with %zu\n", __func__, __LINE__, idx + 1);
               array_of_AlignStack.resize(idx + 1);
               LOG_FMT(LASFCP, "%s(%d): Start for the new\n", __func__, __LINE__);
               array_of_AlignStack[idx].Start(span, thresh);

               log_rule_B("align_number_right");

               if (!options::align_number_right())
               {
                  if (  chunk_is_token(chunks[idx], CT_NUMBER_FP)
                     || chunk_is_token(chunks[idx], CT_NUMBER)
                     || chunk_is_token(chunks[idx], CT_POS)
                     || chunk_is_token(chunks[idx], CT_NEG))
                  {
                     log_rule_B("align_on_tabstop");
                     array_of_AlignStack[idx].m_right_align = !options::align_on_tabstop();
                  }
               }
            }
            LOG_FMT(LASFCP, "%s(%d): save the chunk %s\n", __func__, __LINE__, chunks[idx]->Text());
            array_of_AlignStack[idx].Add(chunks[idx]);
         }
      }
   }

   if (align_len > 1)
   {
      LOG_FMT(LASFCP, "  ++ Ended with %zu fcns\n", align_len);
      fcn_as.End();

      for (auto &as_v : array_of_AlignStack)
      {
         as_v.End();
      }
   }
} // align_same_func_call_params


void align_params(Chunk *start, deque<Chunk *> &chunks)
{
   LOG_FUNC_ENTRY();

   chunks.clear();

   bool  hit_comma = true;
   Chunk *pc       = start->GetNextType(CT_FPAREN_OPEN, start->level);

   while ((pc = pc->GetNext())->IsNotNullChunk())
   {
      if (  chunk_is_newline(pc)
         || chunk_is_token(pc, CT_SEMICOLON)
         || (  chunk_is_token(pc, CT_FPAREN_CLOSE)
            && pc->level == start->level))
      {
         break;
      }

      if (pc->level == (start->level + 1))
      {
         if (hit_comma)
         {
            chunks.push_back(pc);
            hit_comma = false;
         }
         else if (chunk_is_token(pc, CT_COMMA))
         {
            hit_comma = true;
         }
      }
   }
} // void align_params
