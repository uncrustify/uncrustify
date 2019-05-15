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
#include "chunk_list.h"

using namespace uncrustify;


void align_same_func_call_params(void)
{
   LOG_FUNC_ENTRY();

   chunk_t           *pc;
   chunk_t           *align_root = nullptr;
   chunk_t           *align_cur  = nullptr;
   size_t            align_len   = 0;
   size_t            span        = 3;
   size_t            thresh;
   chunk_t           *align_fcn;
   unc_text          align_fcn_name;
   unc_text          align_root_name;
   deque<chunk_t *>  chunks;
   deque<AlignStack> as;
   AlignStack        fcn_as;
   const char        *add_str;

   // Default span is 3 if align_same_func_call_params is true
   if (options::align_same_func_call_params_span() > 0)
   {
      span = options::align_same_func_call_params_span();
   }

   thresh = options::align_same_func_call_params_thresh();

   fcn_as.Start(span, thresh);

   for (pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      if (pc->type != CT_FUNC_CALL)
      {
         if (chunk_is_newline(pc))
         {
            for (auto &as_v : as)
            {
               as_v.NewLines(pc->nl_count);
            }
            fcn_as.NewLines(pc->nl_count);
         }
         else
         {
            // if we drop below the brace level that started it, we are done
            if (align_root && align_root->brace_level > pc->brace_level)
            {
               LOG_FMT(LASFCP, "  ++ (drop) Ended with %zu fcns\n", align_len);

               // Flush it all!
               fcn_as.Flush();
               for (auto &as_v : as)
               {
                  as_v.Flush();
               }
               align_root = nullptr;
            }
         }
         continue;
      }

      // Only align function calls that are right after a newline
      chunk_t *prev = chunk_get_prev(pc);
      while (  chunk_is_token(prev, CT_MEMBER)
            || chunk_is_token(prev, CT_DC_MEMBER))
      {
         chunk_t *tprev = chunk_get_prev(prev);
         if (!chunk_is_token(tprev, CT_TYPE))
         {
            prev = tprev;
            break;
         }
         prev = chunk_get_prev(tprev);
      }
      if (!chunk_is_newline(prev))
      {
         continue;
      }
      prev      = chunk_get_next(prev);
      align_fcn = prev;
      align_fcn_name.clear();
      LOG_FMT(LASFCP, "%s(%d): align_fnc_name '%s'\n", __func__, __LINE__, align_fcn_name.c_str());
      while (prev != pc)
      {
         LOG_FMT(LASFCP, "%s(%d): align_fnc_name '%s'\n", __func__, __LINE__, align_fcn_name.c_str());
         align_fcn_name += prev->str;
         LOG_FMT(LASFCP, "%s(%d): align_fnc_name '%s'\n", __func__, __LINE__, align_fcn_name.c_str());
         prev = chunk_get_next(prev);
      }
      LOG_FMT(LASFCP, "%s(%d): align_fnc_name '%s'\n", __func__, __LINE__, align_fcn_name.c_str());
      align_fcn_name += pc->str;
      LOG_FMT(LASFCP, "%s(%d): align_fnc_name '%s'\n", __func__, __LINE__, align_fcn_name.c_str());
      LOG_FMT(LASFCP, "%s(%d): Func Call @ orig_line is %zu, orig_col is %zu, c_str() '%s'\n",
              __func__, __LINE__, align_fcn->orig_line,
              align_fcn->orig_col,
              align_fcn_name.c_str());

      add_str = nullptr;
      if (align_root != nullptr)
      {
         // Issue # 1395
         // can only align functions on the same brace level
         // and on the same level
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
            for (auto &as_v : as)
            {
               as_v.Flush();
            }
            align_root = nullptr;
         }
      }

      if (align_root == nullptr)
      {
         fcn_as.Add(pc);
         align_root      = align_fcn;
         align_root_name = align_fcn_name;
         align_cur       = pc;
         align_len       = 1;
         add_str         = "Start";
      }

      if (add_str != nullptr)
      {
         LOG_FMT(LASFCP, "%s(%d): %s '%s' on line %zu -",
                 __func__, __LINE__, add_str, align_fcn_name.c_str(), pc->orig_line);
         align_params(pc, chunks);
         LOG_FMT(LASFCP, " %d items:", (int)chunks.size());

         for (size_t idx = 0; idx < chunks.size(); idx++)
         {
            LOG_FMT(LASFCP, " [%s]", chunks[idx]->text());
            if (idx >= as.size())
            {
               as.resize(idx + 1);
               as[idx].Start(span, thresh);
               if (!options::align_number_right())
               {
                  if (  chunk_is_token(chunks[idx], CT_NUMBER_FP)
                     || chunk_is_token(chunks[idx], CT_NUMBER)
                     || chunk_is_token(chunks[idx], CT_POS)
                     || chunk_is_token(chunks[idx], CT_NEG))
                  {
                     as[idx].m_right_align = !options::align_on_tabstop();
                  }
               }
            }
            as[idx].Add(chunks[idx]);
         }
         LOG_FMT(LASFCP, "\n");
      }
   }

   if (align_len > 1)
   {
      LOG_FMT(LASFCP, "  ++ Ended with %zu fcns\n", align_len);
      fcn_as.End();
      for (auto &as_v : as)
      {
         as_v.End();
      }
   }
} // align_same_func_call_params


void align_params(chunk_t *start, deque<chunk_t *> &chunks)
{
   LOG_FUNC_ENTRY();

   chunks.clear();

   bool    hit_comma = true;
   chunk_t *pc       = chunk_get_next_type(start, CT_FPAREN_OPEN, start->level);
   while ((pc = chunk_get_next(pc)) != nullptr)
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
