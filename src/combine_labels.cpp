/**
 * @file combine_labels.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.h
 */

#include "combine_labels.h"

#include "chunk_list.h"
#include "cs_top_is_question.h"
#include "uncrustify.h"


chunk_t *chunk_get_next_local(chunk_t *pc, scope_e scope = scope_e::ALL)
{
   chunk_t *tmp = pc;

   do
   {
      tmp = chunk_get_next(tmp, scope);
   } while (  tmp != nullptr
           && (  chunk_is_comment(tmp)
              || chunk_is_token(tmp, CT_NOEXCEPT)));

   return(tmp);
}


chunk_t *chunk_get_prev_local(chunk_t *pc, scope_e scope = scope_e::ALL)
{
   chunk_t *tmp = pc;

   do
   {
      tmp = chunk_get_prev(tmp, scope);
   } while (  tmp != nullptr
           && (  chunk_is_comment(tmp)
              || chunk_is_newline(tmp)
              || chunk_is_token(tmp, CT_NOEXCEPT)));

   return(tmp);
}


void combine_labels(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *cur;
   chunk_t *prev;
   chunk_t *next;
   chunk_t *tmp;
   bool    hit_case  = false;
   bool    hit_class = false;

   cpd.unc_stage = unc_stage_e::COMBINE_LABELS;

   // stack to handle nesting inside of OC messages, which reset the scope
   ChunkStack cs;

   prev = chunk_get_head();
   if (prev == nullptr)
   {
      return;
   }
   cur = chunk_get_next_nc(prev);
   if (cur == nullptr)
   {
      return;
   }
   next = chunk_get_next_nc(cur);

   // unlikely that the file will start with a label...
   // prev cur next
   while (next != nullptr)
   {
      if (chunk_is_token(next, CT_NEWLINE))
      {
         LOG_FMT(LFCN, "%s(%d): next->orig_line is %zu, next->orig_col is %zu, <Newline>, nl is %zu\n",
                 __func__, __LINE__, next->orig_line, next->orig_col, next->nl_count);
      }
      else if (chunk_is_token(next, CT_VBRACE_OPEN))
      {
         LOG_FMT(LFCN, "%s(%d): next->orig_line is %zu, next->orig_col is %zu, VBRACE_OPEN\n",
                 __func__, __LINE__, next->orig_line, next->orig_col);
      }
      else if (chunk_is_token(next, CT_VBRACE_CLOSE))
      {
         LOG_FMT(LFCN, "%s(%d): next->orig_line is %zu, next->orig_col is %zu, VBRACE_CLOSE\n",
                 __func__, __LINE__, next->orig_line, next->orig_col);
      }
      else
      {
         LOG_FMT(LFCN, "%s(%d): next->orig_line is %zu, next->orig_col is %zu, text() '%s'\n",
                 __func__, __LINE__, next->orig_line, next->orig_col, next->text());
      }

      if (  !(next->flags & PCF_IN_OC_MSG)  // filter OC case of [self class] msg send
         && (  chunk_is_token(next, CT_CLASS)
            || chunk_is_token(next, CT_OC_CLASS)
            || chunk_is_token(next, CT_TEMPLATE)))
      {
         hit_class = true;
      }
      if (chunk_is_semicolon(next) || chunk_is_token(next, CT_BRACE_OPEN))
      {
         hit_class = false;
      }

      if (chunk_is_token(prev, CT_SQUARE_OPEN) && prev->parent_type == CT_OC_MSG)
      {
         cs.Push_Back(prev);
      }
      else if (chunk_is_token(next, CT_SQUARE_CLOSE) && next->parent_type == CT_OC_MSG)
      {
         // pop until we hit '['
         while (!cs.Empty())
         {
            chunk_t *t2 = cs.Top()->m_pc;
            cs.Pop_Back();
            if (chunk_is_token(t2, CT_SQUARE_OPEN))
            {
               break;
            }
         }
      }

      if ((chunk_is_token(next, CT_QUESTION)) && ((next->flags & PCF_IN_TEMPLATE) == 0))
      {
         cs.Push_Back(next);
      }
      else if (chunk_is_token(next, CT_CASE))
      {
         if (chunk_is_token(cur, CT_GOTO))
         {
            // handle "goto case x;"
            set_chunk_type(next, CT_QUALIFIER);
         }
         else
         {
            hit_case = true;
         }
      }
      else if (  chunk_is_token(next, CT_COLON)
              || (  chunk_is_token(next, CT_OC_COLON)
                 && cs_top_is_question(cs, next->level)))
      {
         if (chunk_is_token(cur, CT_DEFAULT))
         {
            set_chunk_type(cur, CT_CASE);
            hit_case = true;
         }
         if (cs_top_is_question(cs, next->level))
         {
            set_chunk_type(next, CT_COND_COLON);
            cs.Pop_Back();
         }
         else if (hit_case)
         {
            hit_case = false;
            set_chunk_type(next, CT_CASE_COLON);
            tmp = chunk_get_next_ncnlnp(next);                // Issue #2150
            if (chunk_is_token(tmp, CT_BRACE_OPEN))
            {
               set_chunk_parent(tmp, CT_CASE);
               tmp = chunk_get_next_type(tmp, CT_BRACE_CLOSE, tmp->level);
               if (tmp != nullptr)
               {
                  set_chunk_parent(tmp, CT_CASE);
               }
            }
         }
         else if (cur->flags & PCF_IN_WHERE_SPEC)
         {
            /* leave colons in where-constraint clauses alone */
         }
         else
         {
            LOG_FMT(LFCN, "\n%s(%d): prev->text() is '%s', orig_line is %zu, orig_col is %zu\n",
                    __func__, __LINE__, prev->text(), prev->orig_line, prev->orig_col);
            LOG_FMT(LFCN, "%s(%d): cur->text() is '%s', orig_line is %zu, orig_col is %zu\n",
                    __func__, __LINE__, cur->text(), cur->orig_line, cur->orig_col);
            LOG_FMT(LFCN, "%s(%d): next->text() is '%s', orig_line is %zu, orig_col is %zu\n\n",
                    __func__, __LINE__, next->text(), next->orig_line, next->orig_col);
            chunk_t *nextprev = chunk_get_prev_local(next);   // Issue #2279
            if (nextprev == nullptr)
            {
               return;
            }
            if (language_is_set(LANG_PAWN))
            {
               if (chunk_is_token(cur, CT_WORD) || chunk_is_token(cur, CT_BRACE_CLOSE))
               {
                  c_token_t new_type = CT_TAG;

                  tmp = chunk_get_next_nc(next);
                  if (tmp == nullptr)
                  {
                     return;
                  }
                  if (chunk_is_newline(prev) && chunk_is_newline(tmp))
                  {
                     new_type = CT_LABEL;
                     set_chunk_type(next, CT_LABEL_COLON);
                  }
                  else
                  {
                     set_chunk_type(next, CT_TAG_COLON);
                  }
                  if (chunk_is_token(cur, CT_WORD))
                  {
                     set_chunk_type(cur, new_type);
                  }
               }
            }
            else if (next->flags & PCF_IN_ARRAY_ASSIGN)
            {
               set_chunk_type(next, CT_D_ARRAY_COLON);
            }
            else if (next->flags & PCF_IN_FOR)
            {
               set_chunk_type(next, CT_FOR_COLON);
            }
            else if (next->flags & PCF_OC_BOXED)
            {
               set_chunk_type(next, CT_OC_DICT_COLON);
            }
            else if (chunk_is_token(cur, CT_WORD))
            {
               tmp = chunk_get_next_nc(next, scope_e::PREPROC);
               // Issue #1187
               if (tmp == nullptr)
               {
                  return;
               }
               LOG_FMT(LFCN, "%s(%d): orig_line is %zu, orig_col is %zu, tmp '%s': ",
                       __func__, __LINE__, tmp->orig_line, tmp->orig_col, (tmp->type == CT_NEWLINE) ? "<Newline>" : tmp->text());
               log_pcf_flags(LGUY, tmp->flags);
               if (next->flags & PCF_IN_FCN_CALL)
               {
                  // Must be a macro thingy, assume some sort of label
                  set_chunk_type(next, CT_LABEL_COLON);
               }
               else if (  tmp == nullptr
                       || (  tmp->type != CT_NUMBER
                          && tmp->type != CT_DECLTYPE
                          && tmp->type != CT_SIZEOF
                          && tmp->parent_type != CT_SIZEOF
                          && !(tmp->flags & (PCF_IN_STRUCT | PCF_IN_CLASS)))
                       || chunk_is_token(tmp, CT_NEWLINE))
               {
                  /*
                   * the CT_SIZEOF isn't great - test 31720 happens to use a sizeof expr,
                   * but this really should be able to handle any constant expr
                   */
                  // Fix for #1242
                  // For MIDL_INTERFACE classes class name is tokenized as Label.
                  // Corrected the identification of Label in c style languages.
                  if (  language_is_set(LANG_C | LANG_CPP | LANG_CS)
                     && (!language_is_set(LANG_OC)))
                  {
                     chunk_t *labelPrev = prev;
                     if (chunk_is_token(labelPrev, CT_NEWLINE))
                     {
                        labelPrev = chunk_get_prev_ncnlni(prev);   // Issue #2279
                     }
                     if (labelPrev->type != CT_FPAREN_CLOSE)
                     {
                        set_chunk_type(cur, CT_LABEL);
                        set_chunk_type(next, CT_LABEL_COLON);
                     }
                  }
                  else
                  {
                     set_chunk_type(cur, CT_LABEL);
                     set_chunk_type(next, CT_LABEL_COLON);
                  }
               }
               else if (next->flags & (PCF_IN_STRUCT | PCF_IN_CLASS | PCF_IN_TYPEDEF))
               {
                  set_chunk_type(next, CT_BIT_COLON);

                  tmp = chunk_get_next(next);
                  if (tmp == nullptr)
                  {
                     return;
                  }
                  while ((tmp = chunk_get_next(tmp)) != nullptr)
                  {
                     if (chunk_is_token(tmp, CT_SEMICOLON))
                     {
                        break;
                     }
                     if (chunk_is_token(tmp, CT_COLON))
                     {
                        set_chunk_type(tmp, CT_BIT_COLON);
                     }
                  }
               }
            }
            else if (chunk_is_token(nextprev, CT_FPAREN_CLOSE))
            {
               LOG_FMT(LFCN, "%s(%d): nextprev->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                       __func__, __LINE__, nextprev->text(), nextprev->orig_line, nextprev->orig_col,
                       get_token_name(nextprev->type));
               LOG_FMT(LFCN, "%s(%d): next->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                       __func__, __LINE__, next->text(), next->orig_line, next->orig_col,
                       get_token_name(next->type));
               // Issue #2172
               if (next->parent_type == CT_FUNC_DEF)
               {
                  LOG_FMT(LFCN, "%s(%d): it's a construct colon\n", __func__, __LINE__);
                  // it's a construct colon
                  set_chunk_type(next, CT_CONSTR_COLON);
               }
               else
               {
                  LOG_FMT(LFCN, "%s(%d): it's a class colon\n", __func__, __LINE__);
                  // it's a class colon
                  set_chunk_type(next, CT_CLASS_COLON);
               }
            }
            else if (next->level > next->brace_level)
            {
               // ignore it, as it is inside a paren
            }
            else if (chunk_is_token(cur, CT_TYPE))
            {
               set_chunk_type(next, CT_BIT_COLON);
            }
            else if (chunk_is_token(nextprev, CT_TYPE))
            {
               set_chunk_type(next, CT_BIT_COLON);
            }
            else if (  chunk_is_token(cur, CT_ENUM)
                    || chunk_is_token(cur, CT_ACCESS)
                    || chunk_is_token(cur, CT_QUALIFIER)
                    || cur->parent_type == CT_ALIGN)
            {
               // ignore it - bit field, align or public/private, etc
            }
            else if (chunk_is_token(cur, CT_ANGLE_CLOSE) || hit_class)
            {
               // ignore it - template thingy
            }
            else if (cur->parent_type == CT_SQL_EXEC)
            {
               // ignore it - SQL variable name
            }
            else if (next->parent_type == CT_ASSERT)
            {
               // ignore it - Java assert thing
            }
            else
            {
               tmp = chunk_get_next_ncnl(next);
               //tmp = chunk_get_next_local(next);
               if (tmp != nullptr)

               {
                  LOG_FMT(LFCN, "%s(%d): tmp->text() is '%s', orig_line is %zu, orig_col is %zu, type is %s\n",
                          __func__, __LINE__, tmp->text(), tmp->orig_line, tmp->orig_col,
                          get_token_name(tmp->type));
                  if (chunk_is_token(tmp, CT_BASE) || chunk_is_token(tmp, CT_THIS))
                  {
                     // ignore it, as it is a C# base thingy
                  }
                  else if (language_is_set(LANG_CS))
                  {
                     // there should be a better solution for that
                  }
                  else
                  {
                     LOG_FMT(LWARN, "%s(%d): %s:%zu unexpected colon in col %zu n-parent=%s c-parent=%s l=%zu bl=%zu\n",
                             __func__, __LINE__,
                             cpd.filename.c_str(), next->orig_line, next->orig_col,
                             get_token_name(next->parent_type),
                             get_token_name(cur->parent_type),
                             next->level, next->brace_level);
                     cpd.error_count++;
                  }
               }
            }
         }
      }

      prev = cur;
      cur  = next;
      next = chunk_get_next_local(next);
   }
} // combine_labels
