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
              || tmp->Is(E_Token::NOEXCEPT)));

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
              || tmp->Is(E_Token::NOEXCEPT)));

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
      if (next->Is(E_Token::NEWLINE))
      {
         LOG_FMT(LFCN, "%s(%d): next orig line is %zu, orig col is %zu, <Newline>, nl is %zu\n",
                 __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), next->GetNlCount());
      }
      else if (next->Is(E_Token::VBRACE_OPEN))
      {
         LOG_FMT(LFCN, "%s(%d): next orig line is %zu, orig col is %zu, VBRACE_OPEN\n",
                 __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol());
      }
      else if (next->Is(E_Token::VBRACE_CLOSE))
      {
         LOG_FMT(LFCN, "%s(%d): next orig line is %zu, orig col is %zu, VBRACE_CLOSE\n",
                 __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol());
      }
      else
      {
         LOG_FMT(LFCN, "%s(%d): next orig line is %zu, orig col is %zu, text '%s', type is %s\n",
                 __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), next->GetLogText(), get_token_name(next->GetType()));
      }
      // this is a regression, have to be repaired latter
      // see also Issue #4081
      //E_Token pt = next->GetParentType();                         // Issue #4042

      //if (  pt == E_Token::UNION
      //   && next->Is(E_Token::BRACE_OPEN))
      //{
      //   Chunk *close_brace = next->GetClosingParen();
      //   next = close_brace->GetNextNc();
      //   continue;
      //}

      if (  !next->TestFlags(PCF_IN_OC_MSG) // filter OC case of [self class] msg send
         && (  next->Is(E_Token::CLASS)
            || next->Is(E_Token::OC_CLASS)
            || next->Is(E_Token::TEMPLATE)))
      {
         hit_class = true;
      }

      if (  next->IsSemicolon()
         || next->Is(E_Token::BRACE_OPEN))
      {
         hit_class = false;
      }

      if (  prev->Is(E_Token::SQUARE_OPEN)
         && prev->GetParentType() == E_Token::OC_MSG)
      {
         cs.Push_Back(prev);
      }
      else if (  next->Is(E_Token::SQUARE_CLOSE)
              && next->GetParentType() == E_Token::OC_MSG)
      {
         // pop until we hit '['
         while (!cs.Empty())
         {
            Chunk *t2 = cs.Top()->m_pc;
            cs.Pop_Back();

            if (t2->Is(E_Token::SQUARE_OPEN))
            {
               break;
            }
         }
      }

      if (  next->Is(E_Token::QUESTION)
         && !next->TestFlags(PCF_IN_TEMPLATE))
      {
         cs.Push_Back(next);
      }
      else if (next->Is(E_Token::CASE))
      {
         if (cur->Is(E_Token::GOTO))
         {
            // handle "goto case x;"
            next->SetType(E_Token::QUALIFIER);
         }
         else
         {
            hit_case = true;
         }
      }
      else if (  next->Is(E_Token::COND_COLON)
              && cs_top_is_question(cs, next->GetLevel()))
      {
         // Pop the question from the stack when we encounter a E_Token::COND_COLON
         // that was already marked by mark_question_colon (e.g., elvis operator ?:)
         cs.Pop_Back();
      }
      else if (  next->Is(E_Token::COLON)
              || (  next->Is(E_Token::OC_COLON)
                 && cs_top_is_question(cs, next->GetLevel())))
      {
         if (cur->Is(E_Token::DEFAULT))
         {
            cur->SetType(E_Token::CASE);
            hit_case = true;
         }

         if (  cs_top_is_question(cs, next->GetLevel())
            && !cur->Is(E_Token::OC_MSG_NAME)
            && next->TestFlags(PCF_IN_CONDITIONAL))             // Issue #3558
         {
            //log_pcf_flags(LFCN, next->GetFlags());
            next->SetType(E_Token::COND_COLON);
            cs.Pop_Back();
         }
         else if (hit_case)
         {
            hit_case = false;
            next->SetType(E_Token::CASE_COLON);
            Chunk *tmp = next->GetNextNcNnlNpp();                // Issue #2150

            if (tmp->Is(E_Token::BRACE_OPEN))
            {
               tmp->SetParentType(E_Token::CASE);
               tmp = tmp->GetNextType(E_Token::BRACE_CLOSE, tmp->GetLevel());

               if (tmp->IsNotNullChunk())
               {
                  tmp->SetParentType(E_Token::CASE);
               }
            }

            if (  cur->Is(E_Token::NUMBER)
               && prev->Is(E_Token::ELLIPSIS))
            {
               Chunk *pre_ellipsis = prev->GetPrevNcNnlNpp();

               if (pre_ellipsis->Is(E_Token::NUMBER))
               {
                  prev->SetType(E_Token::CASE_ELLIPSIS);
               }
            }
         }
         else if (cur->TestFlags(PCF_IN_WHERE_SPEC))
         {
            /* leave colons in where-constraint clauses alone */
         }
         else
         {
            LOG_FMT(LFCN, "%s(%d): prev->text is '%s', orig line is %zu, orig col is %zu\n",
                    __func__, __LINE__, prev->GetLogText(), prev->GetOrigLine(), prev->GetOrigCol());
            LOG_FMT(LFCN, "%s(%d): cur->text is '%s', orig line is %zu, orig col is %zu\n",
                    __func__, __LINE__, cur->GetLogText(), cur->GetOrigLine(), cur->GetOrigCol());
            LOG_FMT(LFCN, "%s(%d): next->text is '%s', orig line is %zu, orig col is %zu\n",
                    __func__, __LINE__, next->GetLogText(), next->GetOrigLine(), next->GetOrigCol());
            Chunk *nextprev = chunk_get_prev_local(next);   // Issue #2279

            if (nextprev->IsNullChunk())
            {
               return;
            }

            if (language_is_set(lang_flag_e::LANG_PAWN))
            {
               if (  cur->Is(E_Token::WORD)
                  || cur->Is(E_Token::BRACE_CLOSE))
               {
                  E_Token new_type = E_Token::TAG;

                  Chunk   *tmp = next->GetNextNc();

                  if (tmp->IsNullChunk())
                  {
                     return;
                  }

                  if (  prev->IsNewline()
                     && tmp->IsNewline())
                  {
                     new_type = E_Token::LABEL;
                     next->SetType(E_Token::LABEL_COLON);
                  }
                  else
                  {
                     next->SetType(E_Token::TAG_COLON);
                  }

                  if (cur->Is(E_Token::WORD))
                  {
                     cur->SetType(new_type);
                  }
               }
            }
            else if (next->TestFlags(PCF_IN_ARRAY_ASSIGN))
            {
               next->SetType(E_Token::D_ARRAY_COLON);
            }
            else if (next->TestFlags(PCF_IN_FOR))
            {
               next->SetType(E_Token::FOR_COLON);
            }
            else if (next->TestFlags(PCF_OC_BOXED))
            {
               next->SetType(E_Token::OC_DICT_COLON);
            }
            else if (cur->Is(E_Token::WORD))
            {
               Chunk *tmp = next->GetNextNc(E_Scope::PREPROC);

               // Issue #1187
               if (tmp->IsNullChunk())
               {
                  return;
               }
               LOG_FMT(LFCN, "%s(%d): orig line is %zu, orig col is %zu, tmp '%s': ",
                       __func__, __LINE__, tmp->GetOrigLine(), tmp->GetOrigCol(),
                       (tmp->Is(E_Token::NEWLINE)) ? "<Newline>" : tmp->GetLogText());
               log_pcf_flags(LFCN, tmp->GetFlags());

               if (next->TestFlags(PCF_IN_FCN_CALL))
               {
                  // Must be a macro thingy, assume some sort of label
                  next->SetType(E_Token::LABEL_COLON);
               }
               else if (  (  tmp->IsNot(E_Token::NUMBER)
                          && tmp->IsNot(E_Token::DECLTYPE)
                          && tmp->IsNot(E_Token::SIZEOF)
                          && tmp->GetParentType() != E_Token::SIZEOF
                          && !tmp->GetFlags().test_any(PCF_IN_STRUCT | PCF_IN_CLASS))
                       || tmp->Is(E_Token::NEWLINE))
               {
                  /*
                   * the E_Token::SIZEOF isn't great - test 31720 happens to use a sizeof expr,
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

                     if (labelPrev->Is(E_Token::NEWLINE))
                     {
                        labelPrev = prev->GetPrevNcNnlNi();   // Issue #2279
                     }

                     if (  labelPrev->IsNotNullChunk()
                        && labelPrev->IsNot(E_Token::FPAREN_CLOSE))
                     {
                        cur->SetType(E_Token::LABEL);
                        next->SetType(E_Token::LABEL_COLON);
                     }
                  }
                  else
                  {
                     cur->SetType(E_Token::LABEL);
                     next->SetType(E_Token::LABEL_COLON);
                  }
               }
               else if (next->GetFlags().test_any(PCF_IN_STRUCT | PCF_IN_CLASS | PCF_IN_TYPEDEF))
               {
                  next->SetType(E_Token::BIT_COLON);

                  Chunk *nnext = next->GetNext();

                  if (nnext->IsNullChunk())
                  {
                     return;
                  }

                  while ((nnext = nnext->GetNext())->IsNotNullChunk())
                  {
                     if (nnext->Is(E_Token::SEMICOLON))
                     {
                        break;
                     }

                     if (nnext->Is(E_Token::COLON))
                     {
                        nnext->SetType(E_Token::BIT_COLON);
                     }
                  }
               }
            }
            else if (nextprev->Is(E_Token::FPAREN_CLOSE))
            {
               LOG_FMT(LFCN, "%s(%d): nextprev->text is '%s', orig line is %zu, orig col is %zu, type is %s\n",
                       __func__, __LINE__, nextprev->GetLogText(), nextprev->GetOrigLine(), nextprev->GetOrigCol(),
                       get_token_name(nextprev->GetType()));
               LOG_FMT(LFCN, "%s(%d): next->text is '%s', orig line is %zu, orig col is %zu, type is %s\n",
                       __func__, __LINE__, next->GetLogText(), next->GetOrigLine(), next->GetOrigCol(),
                       get_token_name(next->GetType()));

               // Issue #2172
               if (next->GetParentType() == E_Token::FUNC_DEF)
               {
                  LOG_FMT(LFCN, "%s(%d): it's a construct colon\n", __func__, __LINE__);
                  // it's a construct colon
                  next->SetType(E_Token::CONSTR_COLON);
               }
               else
               {
                  LOG_FMT(LFCN, "%s(%d): it's a class colon\n", __func__, __LINE__);
                  // it's a class colon
                  next->SetType(E_Token::CLASS_COLON);
               }
            }
            else if (next->GetLevel() > next->GetBraceLevel())
            {
               // ignore it, as it is inside a paren
            }
            else if (  cur->Is(E_Token::TYPE)
                    || cur->Is(E_Token::ENUM)       // Issue #2584
                    || nextprev->Is(E_Token::TYPE)
                    || nextprev->Is(E_Token::ENUM)) // Issue #2584
            {
               next->SetType(E_Token::BIT_COLON);
            }
            else if (  cur->Is(E_Token::ENUM)
                    || cur->Is(E_Token::ACCESS)
                    || cur->Is(E_Token::QUALIFIER)
                    || cur->GetParentType() == E_Token::ALIGN)
            {
               // ignore it - bit field, align or public/private, etc
            }
            else if (  cur->Is(E_Token::ANGLE_CLOSE)
                    || hit_class)
            {
               // ignore it - template thingy
            }
            else if (cur->GetParentType() == E_Token::SQL_EXEC)
            {
               // ignore it - SQL variable name
            }
            else if (next->GetParentType() == E_Token::ASSERT)
            {
               // ignore it - Java assert thing
            }
            else if (next->GetParentType() == E_Token::STRUCT)
            {
               // ignore it
            }
            else
            {
               Chunk *tmp = next->GetNextNcNnl();

               //tmp = chunk_get_next_local(next);
               if (tmp->IsNotNullChunk())
               {
                  LOG_FMT(LFCN, "%s(%d): tmp->text is '%s', orig line is %zu, orig col is %zu, type is %s\n",
                          __func__, __LINE__, tmp->GetLogText(), tmp->GetOrigLine(), tmp->GetOrigCol(),
                          get_token_name(tmp->GetType()));

                  if (  tmp->Is(E_Token::BASE)
                     || tmp->Is(E_Token::THIS))
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
