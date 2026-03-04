/**
 * @file combine_tools.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "tokenizer/combine_tools.h"

#include "ChunkStack.h"
#include "unc_ctype.h"
#include "uncrustify.h"


bool can_be_full_param(Chunk *start, Chunk *end)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LFPARAM, "%s(%d): start->text is '%s', type is %s\n",
           __func__, __LINE__, start->GetLogText(), get_token_name(start->GetType()));
   LOG_FMT(LFPARAM, "%s(%d): end->text   is '%s', type is %s\n",
           __func__, __LINE__, end->GetLogText(), get_token_name(end->GetType()));

   int   word_count     = 0;
   int   type_count     = 0;
   Chunk *pc            = Chunk::NullChunkPtr;
   Chunk *first_word    = Chunk::NullChunkPtr;
   bool  first_word_set = false;

   for (pc = start;
        pc->IsNotNullChunk() && pc != end;
        pc = pc->GetNextNcNnl(E_Scope::PREPROC))
   {
      LOG_FMT(LFPARAM, "%s(%d): text is '%s', type is %s\n",
              __func__, __LINE__, pc->GetLogText(), get_token_name(pc->GetType()));

      if (  pc->Is(E_Token::QUALIFIER)
         || pc->Is(E_Token::STRUCT)
         || pc->Is(E_Token::ENUM)
         || pc->Is(E_Token::UNION)
         || pc->Is(E_Token::TYPENAME))
      {
         LOG_FMT(LFPARAM, "%s(%d): <== %s! (yes)\n",
                 __func__, __LINE__, get_token_name(pc->GetType()));
         return(true);
      }

      if (  pc->Is(E_Token::WORD)
         || pc->Is(E_Token::TYPE))
      {
         ++word_count;

         if (!first_word_set)
         {
            first_word     = pc;
            first_word_set = true;
         }

         if (pc->Is(E_Token::TYPE))
         {
            ++type_count;
         }
      }
      else if (  pc->Is(E_Token::MEMBER)
              || pc->Is(E_Token::DC_MEMBER))
      {
         if (word_count > 0)
         {
            --word_count;
         }
      }
      else if (  pc != start
              && pc->IsPointerOperator())
      {
         // chunk is OK
      }
      else if (pc->Is(E_Token::ASSIGN))
      {
         // chunk is OK (default values)
         break;
      }
      else if (pc->Is(E_Token::ANGLE_OPEN))
      {
         LOG_FMT(LFPARAM, "%s(%d): <== template\n",
                 __func__, __LINE__);

         return(true);
      }
      else if (pc->Is(E_Token::ELLIPSIS))
      {
         LOG_FMT(LFPARAM, "%s(%d): <== ellipsis\n",
                 __func__, __LINE__);

         return(true);
      }
      else if (  word_count == 0
              && pc->Is(E_Token::PAREN_OPEN))
      {
         // Check for old-school func proto param '(type)'
         Chunk *tmp1 = pc->GetClosingParen(E_Scope::PREPROC);

         if (tmp1->IsNullChunk())
         {
            return(false);
         }
         Chunk *tmp2 = tmp1->GetNextNcNnl(E_Scope::PREPROC);

         if (tmp2->IsNullChunk())
         {
            return(false);
         }

         if (  tmp2->Is(E_Token::COMMA)
            || tmp2->IsParenClose())
         {
            do
            {
               pc = pc->GetNextNcNnl(E_Scope::PREPROC);

               if (pc->IsNullChunk())
               {
                  return(false);
               }
               LOG_FMT(LFPARAM, "%s(%d): text is '%s', type is %s\n",
                       __func__, __LINE__, pc->GetLogText(), get_token_name(pc->GetType()));
            } while (pc != tmp1);

            // reset some vars to allow [] after parens
            word_count = 1;
            type_count = 1;
         }
         else
         {
            LOG_FMT(LFPARAM, "%s(%d): <== '%s' not fcn type!\n",
                    __func__, __LINE__, get_token_name(pc->GetType()));
            return(false);
         }
      }
      else if (  (  word_count == 1
                 || (word_count == type_count))
              && pc->Is(E_Token::PAREN_OPEN))
      {
         // Check for func proto param 'void (*name)' or 'void (*name)(params)' or 'void (^name)(params)'
         // <name> can be optional
         Chunk *tmp1 = pc->GetNextNcNnl(E_Scope::PREPROC);

         if (tmp1->IsNullChunk())
         {
            return(false);
         }
         Chunk *tmp2 = tmp1->GetNextNcNnl(E_Scope::PREPROC);

         if (tmp2->Is(E_Token::QUALIFIER))
         {
            // tmp2 is the "nullable" qualifier in this case:
            // void (^nullable name)(params)
            // skip the qualifier
            tmp2 = tmp2->GetNextNcNnl(E_Scope::PREPROC);
         }

         if (tmp2->IsNullChunk())
         {
            return(false);
         }
         Chunk *tmp3 = (tmp2->IsString(")")) ? tmp2 : tmp2->GetNextNcNnl(E_Scope::PREPROC);

         if (tmp3->IsNullChunk())
         {
            return(false);
         }

         if (  !tmp3->IsString(")")
            || !(  tmp1->IsString("*")
                || tmp1->IsString("^")) // Issue #2656
            || !(  tmp2->GetType() == E_Token::WORD
                || tmp2->IsString(")")))
         {
            LOG_FMT(LFPARAM, "%s(%d): <== '%s' not fcn type!\n",
                    __func__, __LINE__, get_token_name(pc->GetType()));
            return(false);
         }
         LOG_FMT(LFPARAM, "%s(%d): <skip fcn type>\n",
                 __func__, __LINE__);

         tmp1 = tmp3->GetNextNcNnl(E_Scope::PREPROC);

         if (tmp1->IsNullChunk())
         {
            return(false);
         }

         if (tmp1->IsString("("))
         {
            tmp3 = tmp1->GetClosingParen(E_Scope::PREPROC);
         }
         pc = tmp3;
         LOG_FMT(LFPARAM, "%s(%d): text is '%s', type is %s\n",
                 __func__, __LINE__, pc->GetLogText(), get_token_name(pc->GetType()));

         // reset some vars to allow [] after parens
         word_count = 1;
         type_count = 1;
      }
      else if (pc->Is(E_Token::TSQUARE))
      {
         // ignore it
      }
      else if (  word_count == 1
              && pc->Is(E_Token::SQUARE_OPEN))
      {
         // skip over any array stuff
         pc = pc->GetClosingParen(E_Scope::PREPROC);
         LOG_FMT(LFPARAM, "%s(%d): text is '%s', type is %s\n",
                 __func__, __LINE__, pc->GetLogText(), get_token_name(pc->GetType()));
      }
      else if (  word_count == 2
              && pc->Is(E_Token::SQUARE_OPEN))
      {
         // Bug #671: is it such as: bool foo[FOO_MAX]
         pc = pc->GetClosingParen(E_Scope::PREPROC);
         LOG_FMT(LFPARAM, "%s(%d): text is '%s', type is %s\n",
                 __func__, __LINE__, pc->GetLogText(), get_token_name(pc->GetType()));
      }
      else if (  word_count == 1
              && language_is_set(lang_flag_e::LANG_CPP)
              && pc->IsString("&&"))
      {
         // ignore possible 'move' operator
      }
      else
      {
         LOG_FMT(LFPARAM, "%s(%d): <== type is %s, no way!, type count is %d, word count is %d\n",
                 __func__, __LINE__, get_token_name(pc->GetType()), type_count, word_count);
         return(false);
      }
      LOG_FMT(LFPARAM, "%s(%d): text is '%s', type is %s\n",
              __func__, __LINE__, pc->GetLogText(), get_token_name(pc->GetType()));
   }

   Chunk *last = pc->GetPrevNcNnlNi();   // Issue #2279

   LOG_FMT(LFPARAM, "%s(%d): last->text is '%s', type is %s\n",
           __func__, __LINE__, last->GetLogText(), get_token_name(last->GetType()));

   if (last->IsPointerOperator())
   {
      LOG_FMT(LFPARAM, "%s(%d): <== type is %s, sure!\n",
              __func__, __LINE__, get_token_name(last->GetType()));
      return(true);
   }

   if (  word_count < 2
      && type_count < 1
      && start->GetBraceLevel() > 0)
   {
      LOG_FMT(LFPARAM, "%s(%d): !MVP!\n",
              __func__, __LINE__);
      // Oh, joy, we are in Most Vexing Parse territory
      Chunk *brace =
         start->GetPrevType(E_Token::BRACE_OPEN, start->GetBraceLevel() - 1);

      if (brace->IsNotNullChunk())
      {
         LOG_FMT(LFPARAM, "%s(%d): (matching %s brace at orig line %zu, orig col is %zu)",
                 __func__, __LINE__,
                 get_token_name(brace->GetParentType()), brace->GetOrigLine(), brace->GetOrigCol());
      }

      if (  brace->IsNotNullChunk()
         && (  brace->GetParentType() == E_Token::CLASS
            || brace->GetParentType() == E_Token::STRUCT))
      {
         // A Most Vexing Parse variable declaration cannot occur in the body
         // of a struct/class, so we probably have a function prototype
         LOG_FMT(LFPARAM, "%s(%d): <== type is %s, Likely!\n",
                 __func__, __LINE__, (pc->IsNullChunk() ? "null chunk" : get_token_name(pc->GetType())));
         return(true);
      }
   }
   LOG_FMT(LFPARAM, "%s(%d): text is '%s', word_count is %d, type_count is %d\n",
           __func__, __LINE__, pc->GetLogText(), word_count, type_count);

   if (first_word->IsNotNullChunk())
   {
      LOG_FMT(LFPARAM, "%s(%d): first_word->text is '%s'\n",
              __func__, __LINE__, first_word->GetLogText());
   }
   bool ret = (  word_count >= 2
              || (  word_count == 1
                 && type_count == 1));

   LOG_FMT(LFPARAM, "%s(%d): ret is %s\n",
           __func__, __LINE__, ret ? "TRUE" : "FALSE");

   LOG_FMT(LFPARAM, "%s(%d): text is '%s', ",
           __func__, __LINE__, pc->GetLogText());
   LOG_FMT(LFPARAM, "<== type is %s, ",
           (pc->IsNullChunk() ? "null chunk" : get_token_name(pc->GetType())));

   if (ret)
   {
      LOG_FMT(LFPARAM, "Yup!\n");
   }
   else
   {
      LOG_FMT(LFPARAM, "Unlikely!\n");
   }
   return(ret);
} // can_be_full_param


bool chunk_ends_type(Chunk *start)
{
   LOG_FUNC_ENTRY();

   if (start->TestFlags(PCF_IN_FCN_CTOR))
   {
      return(false);
   }
   Chunk  *pc       = start;
   bool   ret       = false;
   size_t cnt       = 0;
   bool   last_expr = false;
   bool   last_lval = false;

   for ( ; pc->IsNotNullChunk(); pc = pc->GetPrevNcNnlNi()) // Issue #2279
   {
      LOG_FMT(LFTYPE, "%s(%d): type is %s, text '%s', orig line %zu, orig col %zu\n   ",
              __func__, __LINE__, get_token_name(pc->GetType()), pc->GetLogText(),
              pc->GetOrigLine(), pc->GetOrigCol());
      log_pcf_flags(LFTYPE, pc->GetFlags());

      if (  pc->Is(E_Token::WORD)
         || pc->Is(E_Token::TYPE)
         || pc->Is(E_Token::PTR_TYPE)
         || pc->Is(E_Token::STAR)
         || pc->Is(E_Token::STRUCT)
         || pc->Is(E_Token::DC_MEMBER)
         || pc->Is(E_Token::PP)
         || pc->Is(E_Token::QUALIFIER)
         || (  (  language_is_set(lang_flag_e::LANG_CPP)
               || language_is_set(lang_flag_e::LANG_OC))                       // Issue #2727
            && pc->GetParentType() == E_Token::TEMPLATE
            && (  pc->Is(E_Token::ANGLE_OPEN)
               || pc->Is(E_Token::ANGLE_CLOSE)))
         || (  (  language_is_set(lang_flag_e::LANG_CS)
               || language_is_set(lang_flag_e::LANG_VALA))
            && (pc->Is(E_Token::MEMBER))))
      {
         cnt++;
         last_expr = pc->TestFlags(PCF_EXPR_START)
                     && !pc->TestFlags(PCF_IN_FCN_CALL);
         last_lval = pc->TestFlags(PCF_LVALUE);
         continue;
      }
      /* If a comma is encountered within a template, it must be
       * considered within the context of its immediate parent
       * template (i.e. argument list nest level)
       */

      if (  (  pc->IsSemicolon()
            && !pc->TestFlags(PCF_IN_FOR))
         || pc->Is(E_Token::TYPEDEF)
         || pc->Is(E_Token::BRACE_OPEN)
         || pc->IsBraceClose()
         || pc->Is(E_Token::FPAREN_CLOSE)
         || pc->IsOCForinOpenParen()
         || pc->Is(E_Token::MACRO)
         || pc->Is(E_Token::PP_IF)
         || pc->Is(E_Token::PP_ELSE)
         || pc->Is(E_Token::PP_ENDIF)
         || pc->GetParentType() == E_Token::PP_INCLUDE                       // Issue #3233
         || (  (  pc->Is(E_Token::COMMA)
               && !pc->TestFlags(PCF_IN_FCN_CALL)
               && get_cpp_template_angle_nest_level(start) ==
                  get_cpp_template_angle_nest_level(pc))
            && last_expr)
         || (  pc->Is(E_Token::SPAREN_OPEN)
            && last_lval))
      {
         ret = cnt > 0;
      }
      break;
   }

   if (pc->IsNullChunk())
   {
      // first token
      ret = true;
   }
   LOG_FMT(LFTYPE, "%s(%d): first token verdict: %s\n",
           __func__, __LINE__, ret ? "yes" : "no");

   return(ret);
} // chunk_ends_type


bool chunkstack_match(const ChunkStack &cs, Chunk *pc)
{
   for (size_t idx = 0; idx < cs.Len(); idx++)
   {
      Chunk *tmp = cs.GetChunk(idx);

      if (pc->GetText().equals(tmp->GetText()))
      {
         return(true);
      }
   }

   return(false);
} // chunkstack_match


void flag_series(Chunk *start, Chunk *end, PcfFlags set_flags, PcfFlags clr_flags, E_Scope nav)
{
   LOG_FUNC_ENTRY();

   while (  start->IsNotNullChunk()
         && start != end)
   {
      start->UpdateFlagBits(clr_flags, set_flags);
      log_pcf_flags(LFTYPE, start->GetFlags());

      start = start->GetNext(nav);

      if (start->IsNullChunk())
      {
         return;
      }
   }

   if (end->IsNotNullChunk())
   {
      end->UpdateFlagBits(clr_flags, set_flags);
      log_pcf_flags(LFTYPE, end->GetFlags());
   }
} // flag_series


size_t get_cpp_template_angle_nest_level(Chunk *pc)
{
   LOG_FUNC_ENTRY();
   int nestLevel = 0;

   while (  pc->IsNotNullChunk()
         && pc->TestFlags(PCF_IN_TEMPLATE))
   {
      if (  pc->Is(E_Token::ANGLE_CLOSE)
         && pc->GetParentType() == E_Token::TEMPLATE)
      {
         --nestLevel;
      }
      else if (  pc->Is(E_Token::ANGLE_OPEN)
              && pc->GetParentType() == E_Token::TEMPLATE)
      {
         ++nestLevel;
      }
      pc = pc->GetPrevNcNnlNi();
   }
   return(nestLevel <= 0 ? 0 : size_t(nestLevel));
}


Chunk *get_d_template_types(ChunkStack &cs, Chunk *open_paren)
{
   LOG_FUNC_ENTRY();
   Chunk *tmp       = open_paren->GetNextNcNnl();
   bool  maybe_type = true;

   while (  tmp->IsNullChunk()
         && tmp->GetLevel() > open_paren->GetLevel())
   {
      if (  tmp->Is(E_Token::TYPE)
         || tmp->Is(E_Token::WORD))
      {
         if (maybe_type)
         {
            make_type(tmp);
            cs.Push_Back(tmp);
         }
         maybe_type = false;
      }
      else if (tmp->Is(E_Token::COMMA))
      {
         maybe_type = true;
      }
      tmp = tmp->GetNextNcNnl();
   }
   return(tmp);
} // get_d_template_types


bool go_on(Chunk *pc, Chunk *start)
{
   if (  pc->IsNullChunk()
      || pc->GetLevel() != start->GetLevel())
   {
      return(false);
   }

   if (pc->TestFlags(PCF_IN_FOR))
   {
      return(  (!pc->IsSemicolon())
            && (!(pc->Is(E_Token::COLON))));
   }
   return(!pc->IsSemicolon());
} // go_on


bool is_ucase_str(const char *str, size_t len)
{
   while (len-- > 0)
   {
      if (unc_toupper(*str) != *str)
      {
         return(false);
      }
      str++;
   }
   return(true);
} // is_ucase_str


void make_type(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc->IsNotNullChunk())
   {
      if (pc->Is(E_Token::WORD))
      {
         pc->SetType(E_Token::TYPE);
      }
      else if (  (  pc->IsStar()
                 || pc->IsMsRef()
                 || pc->IsNullable())
              && pc->GetPrev()->IsTypeDefinition())               // Issue # 2640
      {
         pc->SetType(E_Token::PTR_TYPE);
      }
      else if (  pc->IsAddress()
              && pc->GetPrev()->IsNot(E_Token::SQUARE_OPEN))            // Issue # 2166
      {
         pc->SetType(E_Token::BYREF);
      }
   }
} // make_type


Chunk *set_paren_parent(Chunk *start, E_Token parent_type)
{
   LOG_FUNC_ENTRY();
   Chunk *end;

   end = start->GetClosingParen(E_Scope::PREPROC);

   if (end->IsNotNullChunk())
   {
      LOG_FMT(LFLPAREN, "%s(%d): %zu:%zu '%s' and %zu:%zu '%s' type is %s, parent type is %s",
              __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol(), start->GetLogText(),
              end->GetOrigLine(), end->GetOrigCol(), end->GetLogText(),
              get_token_name(start->GetType()), get_token_name(parent_type));
      log_func_stack_inline(LFLPAREN);
      start->SetParentType(parent_type);
      end->SetParentType(parent_type);
      LOG_FMT(LFLPAREN, "%s(%d):\n", __func__, __LINE__);
      return(end->GetNextNcNnl(E_Scope::PREPROC));
   }
   LOG_FMT(LFLPAREN, "%s(%d):\n", __func__, __LINE__);
   return(Chunk::NullChunkPtr);
} // set_paren_parent
