/**
 * @file combine_labels.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "tokenizer/combine_labels.h"

#include "chunk.h"
#include "ChunkStack.h"
#include "tokenizer/cs_top_is_question.h"
#include "uncrustify.h"


Chunk *chunk_get_next_local(Chunk *pc, E_Scope scope = E_Scope::ALL)
{
   Chunk *tmp = pc;

   do
   {
      tmp = tmp->GetNext(scope);
   } while (  tmp->IsNotNullChunk()
           && (  tmp->IsComment()
              || tmp->Is(CT_NOEXCEPT)));

   return(tmp);
}


Chunk *chunk_get_prev_local(Chunk *pc, E_Scope scope = E_Scope::ALL)
{
   Chunk *tmp = pc;

   do
   {
      tmp = tmp->GetPrev(scope);
   } while (  tmp->IsNotNullChunk()
           && (  tmp->IsCommentOrNewline()
              || tmp->Is(CT_NOEXCEPT)));

   return(tmp);
}


void combine_labels()
{
   LOG_FUNC_ENTRY();
   bool hit_case  = false;
   bool hit_class = false;

   cpd.unc_stage = unc_stage_e::COMBINE_LABELS;

   // stack to handle nesting inside of OC messages, which reset the scope
   ChunkStack cs;

   Chunk      *prev = Chunk::GetHead();

   if (prev->IsNullChunk())
   {
      return;
   }
   Chunk *cur = prev->GetNextNc();

   if (cur->IsNullChunk())
   {
      return;
   }
   Chunk *next = cur->GetNextNc();

   // unlikely that the file will start with a label...
   // prev cur next
   while (next->IsNotNullChunk())
   {
      if (next->Is(CT_NEWLINE))
      {
         LOG_FMT(LFCN, "%s(%d): next orig line is %zu, orig col is %zu, <Newline>, nl is %zu\n",
                 __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), next->GetNlCount());
      }
      else if (next->Is(CT_VBRACE_OPEN))
      {
         LOG_FMT(LFCN, "%s(%d): next orig line is %zu, orig col is %zu, VBRACE_OPEN\n",
                 __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol());
      }
      else if (next->Is(CT_VBRACE_CLOSE))
      {
         LOG_FMT(LFCN, "%s(%d): next orig line is %zu, orig col is %zu, VBRACE_CLOSE\n",
                 __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol());
      }
      else
      {
         LOG_FMT(LFCN, "%s(%d): next orig line is %zu, orig col is %zu, Text() '%s', type is %s\n",
                 __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), next->Text(), get_token_name(next->GetType()));
      }
      // this is a regression, have to be repaired latter
      // see also Issue #4081
      //E_Token pt = next->GetParentType();                         // Issue #4042

      //if (  pt == CT_UNION
      //   && next->Is(CT_BRACE_OPEN))
      //{
      //   Chunk *close_brace = next->GetClosingParen();
      //   next = close_brace->GetNextNc();
      //   continue;
      //}

      if (  !next->TestFlags(PCF_IN_OC_MSG) // filter OC case of [self class] msg send
         && (  next->Is(CT_CLASS)
            || next->Is(CT_OC_CLASS)
            || next->Is(CT_TEMPLATE)))
      {
         hit_class = true;
      }

      if (  next->IsSemicolon()
         || next->Is(CT_BRACE_OPEN))
      {
         hit_class = false;
      }

      if (  prev->Is(CT_SQUARE_OPEN)
         && prev->GetParentType() == CT_OC_MSG)
      {
         cs.Push_Back(prev);
      }
      else if (  next->Is(CT_SQUARE_CLOSE)
              && next->GetParentType() == CT_OC_MSG)
      {
         // pop until we hit '['
         while (!cs.Empty())
         {
            Chunk *t2 = cs.Top()->m_pc;
            cs.Pop_Back();

            if (t2->Is(CT_SQUARE_OPEN))
            {
               break;
            }
         }
      }

      if (  next->Is(CT_QUESTION)
         && !next->TestFlags(PCF_IN_TEMPLATE))
      {
         cs.Push_Back(next);
      }
      else if (next->Is(CT_CASE))
      {
         if (cur->Is(CT_GOTO))
         {
            // handle "goto case x;"
            next->SetType(CT_QUALIFIER);
         }
         else
         {
            hit_case = true;
         }
      }
      else if (  next->Is(CT_COLON)
              || (  next->Is(CT_OC_COLON)
                 && cs_top_is_question(cs, next->GetLevel())))
      {
         if (cur->Is(CT_DEFAULT))
         {
            cur->SetType(CT_CASE);
            hit_case = true;
         }

         if (  cs_top_is_question(cs, next->GetLevel())
            && !cur->Is(CT_OC_MSG_NAME)
            && next->TestFlags(PCF_IN_CONDITIONAL))             // Issue #3558
         {
            //log_pcf_flags(LFCN, next->GetFlags());
            next->SetType(CT_COND_COLON);
            cs.Pop_Back();
         }
         else if (hit_case)
         {
            hit_case = false;
            next->SetType(CT_CASE_COLON);
            Chunk *tmp = next->GetNextNcNnlNpp();                // Issue #2150

            if (tmp->Is(CT_BRACE_OPEN))
            {
               tmp->SetParentType(CT_CASE);
               tmp = tmp->GetNextType(CT_BRACE_CLOSE, tmp->GetLevel());

               if (tmp->IsNotNullChunk())
               {
                  tmp->SetParentType(CT_CASE);
               }
            }

            if (  cur->Is(CT_NUMBER)
               && prev->Is(CT_ELLIPSIS))
            {
               Chunk *pre_elipsis = prev->GetPrevNcNnlNpp();

               if (pre_elipsis->Is(CT_NUMBER))
               {
                  prev->SetType(CT_CASE_ELLIPSIS);
               }
            }
         }
         else if (cur->TestFlags(PCF_IN_WHERE_SPEC))
         {
            /* leave colons in where-constraint clauses alone */
         }
         else
         {
            LOG_FMT(LFCN, "%s(%d): prev->Text() is '%s', orig line is %zu, orig col is %zu\n",
                    __func__, __LINE__, prev->Text(), prev->GetOrigLine(), prev->GetOrigCol());
            LOG_FMT(LFCN, "%s(%d): cur->Text() is '%s', orig line is %zu, orig col is %zu\n",
                    __func__, __LINE__, cur->Text(), cur->GetOrigLine(), cur->GetOrigCol());
            LOG_FMT(LFCN, "%s(%d): next->Text() is '%s', orig line is %zu, orig col is %zu\n",
                    __func__, __LINE__, next->Text(), next->GetOrigLine(), next->GetOrigCol());
            Chunk *nextprev = chunk_get_prev_local(next);   // Issue #2279

            if (nextprev->IsNullChunk())
            {
               return;
            }

            if (language_is_set(lang_flag_e::LANG_PAWN))
            {
               if (  cur->Is(CT_WORD)
                  || cur->Is(CT_BRACE_CLOSE))
               {
                  E_Token new_type = CT_TAG;

                  Chunk   *tmp = next->GetNextNc();

                  if (tmp->IsNullChunk())
                  {
                     return;
                  }

                  if (  prev->IsNewline()
                     && tmp->IsNewline())
                  {
                     new_type = CT_LABEL;
                     next->SetType(CT_LABEL_COLON);
                  }
                  else
                  {
                     next->SetType(CT_TAG_COLON);
                  }

                  if (cur->Is(CT_WORD))
                  {
                     cur->SetType(new_type);
                  }
               }
            }
            else if (next->TestFlags(PCF_IN_ARRAY_ASSIGN))
            {
               next->SetType(CT_D_ARRAY_COLON);
            }
            else if (next->TestFlags(PCF_IN_FOR))
            {
               next->SetType(CT_FOR_COLON);
            }
            else if (next->TestFlags(PCF_OC_BOXED))
            {
               next->SetType(CT_OC_DICT_COLON);
            }
            else if (cur->Is(CT_WORD))
            {
               Chunk *tmp = next->GetNextNc(E_Scope::PREPROC);

               // Issue #1187
               if (tmp->IsNullChunk())
               {
                  return;
               }
               LOG_FMT(LFCN, "%s(%d): orig line is %zu, orig col is %zu, tmp '%s': ",
                       __func__, __LINE__, tmp->GetOrigLine(), tmp->GetOrigCol(),
                       (tmp->Is(CT_NEWLINE)) ? "<Newline>" : tmp->Text());
               log_pcf_flags(LFCN, tmp->GetFlags());

               if (next->TestFlags(PCF_IN_FCN_CALL))
               {
                  // Must be a macro thingy, assume some sort of label
                  next->SetType(CT_LABEL_COLON);
               }
               else if (  tmp->IsNullChunk()
                       || (  tmp->IsNot(CT_NUMBER)
                          && tmp->IsNot(CT_DECLTYPE)
                          && tmp->IsNot(CT_SIZEOF)
                          && tmp->GetParentType() != CT_SIZEOF
                          && !tmp->GetFlags().test_any(PCF_IN_STRUCT | PCF_IN_CLASS))
                       || tmp->Is(CT_NEWLINE))
               {
                  /*
                   * the CT_SIZEOF isn't great - test 31720 happens to use a sizeof expr,
                   * but this really should be able to handle any constant expr
                   */
                  // Fix for #1242
                  // For MIDL_INTERFACE classes class name is tokenized as Label.
                  // Corrected the identification of Label in c style languages.
                  if (  (  language_is_set(lang_flag_e::LANG_C)
                        || language_is_set(lang_flag_e::LANG_CPP)
                        || language_is_set(lang_flag_e::LANG_CS))
                     && (!language_is_set(lang_flag_e::LANG_OC)))
                  {
                     Chunk *labelPrev = prev;

                     if (labelPrev->Is(CT_NEWLINE))
                     {
                        labelPrev = prev->GetPrevNcNnlNi();   // Issue #2279
                     }

                     if (  labelPrev->IsNotNullChunk()
                        && labelPrev->IsNot(CT_FPAREN_CLOSE))
                     {
                        cur->SetType(CT_LABEL);
                        next->SetType(CT_LABEL_COLON);
                     }
                  }
                  else
                  {
                     cur->SetType(CT_LABEL);
                     next->SetType(CT_LABEL_COLON);
                  }
               }
               else if (next->GetFlags().test_any(PCF_IN_STRUCT | PCF_IN_CLASS | PCF_IN_TYPEDEF))
               {
                  next->SetType(CT_BIT_COLON);

                  Chunk *nnext = next->GetNext();

                  if (nnext->IsNullChunk())
                  {
                     return;
                  }

                  while ((nnext = nnext->GetNext())->IsNotNullChunk())
                  {
                     if (nnext->Is(CT_SEMICOLON))
                     {
                        break;
                     }

                     if (nnext->Is(CT_COLON))
                     {
                        nnext->SetType(CT_BIT_COLON);
                     }
                  }
               }
            }
            else if (nextprev->Is(CT_FPAREN_CLOSE))
            {
               LOG_FMT(LFCN, "%s(%d): nextprev->Text() is '%s', orig line is %zu, orig col is %zu, type is %s\n",
                       __func__, __LINE__, nextprev->Text(), nextprev->GetOrigLine(), nextprev->GetOrigCol(),
                       get_token_name(nextprev->GetType()));
               LOG_FMT(LFCN, "%s(%d): next->Text() is '%s', orig line is %zu, orig col is %zu, type is %s\n",
                       __func__, __LINE__, next->Text(), next->GetOrigLine(), next->GetOrigCol(),
                       get_token_name(next->GetType()));

               // Issue #2172
               if (next->GetParentType() == CT_FUNC_DEF)
               {
                  LOG_FMT(LFCN, "%s(%d): it's a construct colon\n", __func__, __LINE__);
                  // it's a construct colon
                  next->SetType(CT_CONSTR_COLON);
               }
               else
               {
                  LOG_FMT(LFCN, "%s(%d): it's a class colon\n", __func__, __LINE__);
                  // it's a class colon
                  next->SetType(CT_CLASS_COLON);
               }
            }
            else if (next->GetLevel() > next->GetBraceLevel())
            {
               // ignore it, as it is inside a paren
            }
            else if (  cur->Is(CT_TYPE)
                    || cur->Is(CT_ENUM)       // Issue #2584
                    || nextprev->Is(CT_TYPE)
                    || nextprev->Is(CT_ENUM)) // Issue #2584
            {
               next->SetType(CT_BIT_COLON);
            }
            else if (  cur->Is(CT_ENUM)
                    || cur->Is(CT_ACCESS)
                    || cur->Is(CT_QUALIFIER)
                    || cur->GetParentType() == CT_ALIGN)
            {
               // ignore it - bit field, align or public/private, etc
            }
            else if (  cur->Is(CT_ANGLE_CLOSE)
                    || hit_class)
            {
               // ignore it - template thingy
            }
            else if (cur->GetParentType() == CT_SQL_EXEC)
            {
               // ignore it - SQL variable name
            }
            else if (next->GetParentType() == CT_ASSERT)
            {
               // ignore it - Java assert thing
            }
            else if (next->GetParentType() == CT_STRUCT)
            {
               // ignore it
            }
            else
            {
               Chunk *tmp = next->GetNextNcNnl();

               //tmp = chunk_get_next_local(next);
               if (tmp->IsNotNullChunk())
               {
                  LOG_FMT(LFCN, "%s(%d): tmp->Text() is '%s', orig line is %zu, orig col is %zu, type is %s\n",
                          __func__, __LINE__, tmp->Text(), tmp->GetOrigLine(), tmp->GetOrigCol(),
                          get_token_name(tmp->GetType()));

                  if (  tmp->Is(CT_BASE)
                     || tmp->Is(CT_THIS))
                  {
                     // ignore it, as it is a C# base thingy
                  }
                  else if (  language_is_set(lang_flag_e::LANG_CS)
                          || language_is_set(lang_flag_e::LANG_D))
                  {
                     // there should be a better solution for that
                  }
                  else
                  {
                     LOG_FMT(LWARN, "%s(%d): %s:%zu unexpected colon in col %zu n-parent=%s c-parent=%s l=%zu bl=%zu\n",
                             __func__, __LINE__,
                             cpd.filename.c_str(), next->GetOrigLine(), next->GetOrigCol(),
                             get_token_name(next->GetParentType()),
                             get_token_name(cur->GetParentType()),
                             next->GetLevel(), next->GetBraceLevel());

                     if (language_is_set(lang_flag_e::LANG_OC))
                     {
                        // TO DO: what is to do? any expert?
                     }
                     else
                     {
                        exit(EX_SOFTWARE);
                     }
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
