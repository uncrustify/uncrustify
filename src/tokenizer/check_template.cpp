/**
 * @file check_template.cpp
 *
 * split from tokenize_cleanup.cpp
 *
 * @author  Guy Maurel 2022
 * @license GPL v2+
 */

#include "tokenizer/tokenize_cleanup.h"

#include "chunk.h"
#include "log_rules.h"
#include "tokenizer/check_template.h"
#include "tokenizer/combine.h"
#include "tokenizer/flag_braced_init_list.h"
#include "tokenizer/flag_decltype.h"
#include "uncrustify.h"

#include <cstdio>         // to get fprintf
#include <vector>


constexpr static auto LCURRENT = LTEMPL;


using namespace uncrustify;


bool invalid_open_angle_template(Chunk *prev)
{
   if (prev->IsNullChunk())
   {
      return(false);
   }
   // A template requires a word/type right before the open angle
   // It can also appear after the square brackets of a lambda
   return(  prev->IsNot(E_Token::WORD)
         && prev->IsNot(E_Token::TYPE)
         && prev->IsNot(E_Token::COMMA)
         && prev->IsNot(E_Token::QUALIFIER)
         && prev->IsNot(E_Token::OPERATOR_VAL)
         && prev->IsNot(E_Token::SQUARE_CLOSE)
         && prev->IsNot(E_Token::TSQUARE)
         && prev->GetParentType() != E_Token::OPERATOR);
}


Chunk *handle_double_angle_close(Chunk *pc)
{
   Chunk *next = pc->GetNext();

   if (next->IsNotNullChunk())
   {
      if (  pc->Is(E_Token::ANGLE_CLOSE)
         && next->Is(E_Token::ANGLE_CLOSE)
         && pc->GetParentType() == E_Token::NONE
         && (pc->GetOrigColEnd() + 1) == next->GetOrigCol()
         && next->GetParentType() == E_Token::NONE)
      {
         pc->Text().append('>');
         pc->SetType(E_Token::SHIFT);
         pc->SetOrigColEnd(next->GetOrigColEnd());

         Chunk *tmp = next->GetNextNcNnl();
         Chunk::Delete(next);
         next = tmp;
      }
      else
      {
         // bug #663
         pc->SetType(E_Token::COMPARE);
      }
   }
   return(next);
}


void check_template(Chunk *start, bool in_type_cast)
{
   LOG_FMT(LTEMPL, "%s(%d): orig line %zu, orig col %zu:\n",
           __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol());

   Chunk *prev = start->GetPrevNcNnl(E_Scope::PREPROC);

   if (prev->IsNullChunk())
   {
      return;
   }
   Chunk *end;
   Chunk *pc;

   if (prev->Is(E_Token::TEMPLATE))
   {
      LOG_FMT(LTEMPL, "%s(%d): E_Token::TEMPLATE:\n", __func__, __LINE__);

      // We have: "template< ... >", which is a template declaration
      size_t level  = 1;
      size_t parens = 0;

      for (pc = start->GetNextNcNnl(E_Scope::PREPROC);
           pc->IsNotNullChunk();
           pc = pc->GetNextNcNnl(E_Scope::PREPROC))
      {
         LOG_FMT(LTEMPL, "%s(%d): type is %s, level is %zu\n",
                 __func__, __LINE__, get_token_name(pc->GetType()), level);

         if (  (pc->GetText()[0] == '>')
            && (pc->Len() > 1))
         {
            if (pc->GetText()[1] == '=')                         // Issue #1462 and #2565
            {
               LOG_FMT(LTEMPL, "%s(%d): do not split '%s' at orig line %zu, orig col %zu\n",
                       __func__, __LINE__, pc->GetLogText(), pc->GetOrigLine(), pc->GetOrigCol());
            }
            else
            {
               LOG_FMT(LTEMPL, "%s(%d): {split '%s' at orig line %zu, orig col %zu}\n",
                       __func__, __LINE__, pc->GetLogText(), pc->GetOrigLine(), pc->GetOrigCol());
               split_off_angle_close(pc);
            }
         }

         if (pc->Is(E_Token::DECLTYPE))
         {
            flag_cpp_decltype(pc);
         }
         else if (pc->Is(E_Token::PAREN_OPEN))
         {
            ++parens;
         }
         else if (pc->Is(E_Token::PAREN_CLOSE))
         {
            --parens;
         }

         if (parens == 0)
         {
            if (pc->IsString("<"))
            {
               level++;
            }
            else if (pc->IsString(">"))
            {
               if (level == 0)
               {
                  fprintf(stderr, "%s(%d): level is ZERO, cannot be decremented, at line %zu, column %zu\n",
                          __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
                  log_flush(true);
                  exit(EX_SOFTWARE);
               }
               level--;

               if (level == 0)
               {
                  break;
               }
            }
         }
      }

      end = pc;
   }
   else
   {
      /*
       * We may have something like "a< ... >", which is a template where
       * '...' may consist of anything except a semicolon, unbalanced
       * parens, or braces (with one exception being braced initializers
       * embedded within decltypes).
       *
       * For example, braces may be encountered as such in the following
       * snippet of valid C++ code:
       *
       * template<typename T,
       *          typename = enable_if_t<is_same<typename decay<T>::type,
       *                                          decltype (make_index_sequence<5> { })>::value>>
       * void foo(T &&arg)
       * {
       *
       * }
       *
       * Finally, if we are inside an 'if' statement and hit a E_Token::BOOL,
       * then it isn't a template.
       */

      if (invalid_open_angle_template(prev))
      {
         LOG_FMT(LTEMPL, "%s(%d): - after type %s + ( - Not a template\n",
                 __func__, __LINE__, get_token_name(prev->GetType()));
         start->SetType(E_Token::COMPARE);
         return;
      }
      LOG_FMT(LTEMPL, "%s(%d): - prev->GetType() is %s -\n",
              __func__, __LINE__, get_token_name(prev->GetType()));

      // Scan back and make sure we aren't inside square parenthesis
      bool in_if         = false;
      bool hit_semicolon = false;
      pc = start->GetPrevNcNnl(E_Scope::PREPROC);

      while (pc->IsNotNullChunk())
      {
         if (  (  pc->Is(E_Token::SEMICOLON)
               && hit_semicolon)
            || pc->Is(E_Token::SQUARE_CLOSE))
         {
            break;
         }

         if (pc->Is(E_Token::DECLTYPE))
         {
            flag_cpp_decltype(pc);
         }

         if (pc->Is(E_Token::BRACE_OPEN))
         {
            if (  !pc->TestFlags(PCF_IN_DECLTYPE)
               || !detect_cpp_braced_init_list(pc->GetPrev(), pc))
            {
               break;
            }
            flag_cpp_braced_init_list(pc->GetPrev(), pc);
         }

         if (  pc->Is(E_Token::BRACE_CLOSE)
            && pc->GetParentType() != E_Token::BRACED_INIT_LIST
            && !pc->TestFlags(PCF_IN_DECLTYPE))
         {
            break;
         }

         if (  pc->Is(E_Token::SEMICOLON)
            && !hit_semicolon)
         {
            hit_semicolon = true;
         }

         if (  (  (  pc->Is(E_Token::IF)
                  || pc->Is(E_Token::RETURN)
                  || pc->Is(E_Token::WHILE)
                  || pc->Is(E_Token::WHILE_OF_DO))
               && !hit_semicolon)
            || (  pc->Is(E_Token::FOR)
               && hit_semicolon))
         {
            in_if = true;
            break;
         }
         pc = pc->GetPrevNcNnl(E_Scope::PREPROC);
      }
      /*
       * Scan forward to the angle close
       * If we have a comparison in there, then it can't be a template.
       */
      constexpr size_t MAX_TOKEN_COUNT{ 1024 };
      E_Token          tokens[MAX_TOKEN_COUNT];
      size_t           num_tokens = 1;

      tokens[0] = E_Token::ANGLE_OPEN;

      for (pc = start->GetNextNcNnl(E_Scope::PREPROC);
           pc->IsNotNullChunk();
           pc = pc->GetNextNcNnl(E_Scope::PREPROC))
      {
         LOG_FMT(LTEMPL, "%s(%d): pc orig line is %zu, orig col is %zu, type is %s, num_tokens is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()), num_tokens);

         log_rule_B("tok_split_gte");

         if (pc->Is(E_Token::BRACE_OPEN))                     // Issue #2886
         {
            // look for the closing brace
            Chunk *A = pc->GetClosingParen();
            LOG_FMT(LTEMPL, "%s(%d): A orig line is %zu, orig col is %zu, type is %s\n",
                    __func__, __LINE__, A->GetOrigLine(), A->GetOrigCol(), get_token_name(A->GetType()));
            pc = A->GetNext();
         }

         if (  num_tokens > 0
            && (tokens[num_tokens - 1] == E_Token::ANGLE_OPEN)
            && (pc->GetText()[0] == '>')
            && (pc->Len() > 1)
            && (  options::tok_split_gte()
               || (  (  pc->IsString(">>")
                     || pc->IsString(">>>"))
                  && (  num_tokens >= 2
                     || (  num_tokens >= 1
                        && in_type_cast)))))
         {
            LOG_FMT(LTEMPL, "%s(%d): {split '%s' at orig line %zu, orig col %zu}\n",
                    __func__, __LINE__, pc->GetLogText(), pc->GetOrigLine(), pc->GetOrigCol());

            split_off_angle_close(pc);
         }

         if (pc->IsString("<"))
         {
            if (  num_tokens > 0 && (tokens[num_tokens - 1] == E_Token::PAREN_OPEN)
               && invalid_open_angle_template(pc->GetPrev()))
            {
               pc->SetType(E_Token::COMPARE); // Issue #3127
            }
            else
            {
               tokens[num_tokens] = E_Token::ANGLE_OPEN;
               num_tokens++;

               if (num_tokens >= MAX_TOKEN_COUNT)
               {
                  fprintf(stderr, "FATAL(1): The variable 'tokens' is too small,\n");
                  fprintf(stderr, "   it should be bigger than %zu.\n", num_tokens);
                  fprintf(stderr, "Please report this to the uncrustify project.\n");
                  fflush(stderr);
                  exit(EX_SOFTWARE);
               }
            }
         }
         else if (pc->IsString(">"))
         {
            if (  num_tokens > 0
               && (tokens[num_tokens - 1] == E_Token::PAREN_OPEN))
            {
               handle_double_angle_close(pc);
            }
            else if (  num_tokens > 0
                    && --num_tokens == 0)
            {
               break;
            }
            else if (tokens[num_tokens] != E_Token::ANGLE_OPEN)
            {
               break; // unbalanced parentheses
            }
         }
         else if (  in_if
                 && (  pc->Is(E_Token::BOOL)
                    || pc->Is(E_Token::COMPARE)))
         {
            break;
         }
         else if (pc->Is(E_Token::BRACE_OPEN))
         {
            if (  !pc->TestFlags(PCF_IN_DECLTYPE)
               || !detect_cpp_braced_init_list(pc->GetPrev(), pc))
            {
               break;
            }
            auto brace_open  = pc->GetNextNcNnl();
            auto brace_close = brace_open->GetClosingParen();

            brace_open->SetParentType(E_Token::BRACED_INIT_LIST);
            brace_close->SetParentType(E_Token::BRACED_INIT_LIST);
         }
         else if (  pc->Is(E_Token::BRACE_CLOSE)
                 && pc->GetParentType() != E_Token::BRACED_INIT_LIST
                 && !pc->TestFlags(PCF_IN_DECLTYPE))
         {
            break;
         }
         else if (pc->Is(E_Token::SEMICOLON))
         {
            break;
         }
         else if (pc->Is(E_Token::PAREN_OPEN))
         {
            tokens[num_tokens] = E_Token::PAREN_OPEN;
            num_tokens++;

            if (num_tokens >= MAX_TOKEN_COUNT)
            {
               fprintf(stderr, "FATAL(2): The variable 'tokens' is too small,\n");
               fprintf(stderr, "   it should be bigger than %zu.\n", num_tokens);
               fprintf(stderr, "Please report this to the uncrustify project.\n");
               fflush(stderr);
               exit(EX_SOFTWARE);
            }
         }
         else if (  pc->Is(E_Token::QUESTION)                    // Issue #2949
                 && language_is_set(lang_flag_e::LANG_CPP))
         {
            break;
         }
         else if (pc->Is(E_Token::PAREN_CLOSE))
         {
            if (num_tokens == 0)
            {
               fprintf(stderr, "%s(%d): num_tokens is ZERO, cannot be decremented, at line %zu, column %zu\n",
                       __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
               log_flush(true);
               exit(EX_SOFTWARE);
            }

            if (tokens[--num_tokens] != E_Token::PAREN_OPEN)
            {
               break;  // unbalanced parentheses
            }
         }
      }

      end = pc;
   }

   if (end->Is(E_Token::ANGLE_CLOSE))
   {
      pc = end->GetNextNcNnl(E_Scope::PREPROC);

      if (  pc->IsNullChunk()
         || pc->IsNot(E_Token::NUMBER))
      {
         LOG_FMT(LTEMPL, "%s(%d): Template detected\n", __func__, __LINE__);
         LOG_FMT(LTEMPL, "%s(%d):     from orig line %zu, orig col %zu\n",
                 __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol());
         LOG_FMT(LTEMPL, "%s(%d):     to   orig line %zu, orig col %zu\n",
                 __func__, __LINE__, end->GetOrigLine(), end->GetOrigCol());
         start->SetParentType(E_Token::TEMPLATE);

         check_template_args(start, end);

         end->SetParentType(E_Token::TEMPLATE);
         end->SetFlagBits(PCF_IN_TEMPLATE);
         return;
      }
   }
   LOG_FMT(LTEMPL, "%s(%d): - Not a template: end = %s\n",
           __func__, __LINE__, (end->IsNotNullChunk() ? get_token_name(end->GetType()) : "<null>"));
   start->SetType(E_Token::COMPARE);
} // check_template


void check_template_arg(Chunk *start, Chunk *end)
{
   LOG_FMT(LTEMPL, "%s(%d): Template argument detected\n", __func__, __LINE__);
   LOG_FMT(LTEMPL, "%s(%d):     from orig line %zu, orig col %zu\n",
           __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol());
   LOG_FMT(LTEMPL, "%s(%d):     to   orig line %zu, orig col %zu\n",
           __func__, __LINE__, end->GetOrigLine(), end->GetOrigCol());

   // Issue #1127
   // MyFoo<mySize * 2> foo1;
   // MyFoo<2*mySize * 2> foo1;
   // Issue #1346
   // use it as ONE line:
   //   typename std::enable_if<!std::is_void<T>::value,
   //   QVector<T> >::type dummy(const std::function<T*(const S&)>&
   //   pFunc, const QVector<S>& pItems)
   // we need two runs
   // 1. run to test if expression is numeric
   bool  expressionIsNumeric = false;
   Chunk *pc                 = start;

   while (pc != end)
   {
      Chunk *next = pc->GetNextNcNnl(E_Scope::PREPROC);
      pc->SetFlagBits(PCF_IN_TEMPLATE);

      if (  pc->Is(E_Token::DECLTYPE)
         || pc->Is(E_Token::SIZEOF))
      {
         expressionIsNumeric = true;
         break;
      }

      if (next->IsNot(E_Token::PAREN_OPEN))
      {
         if (  pc->Is(E_Token::NUMBER)
            || pc->Is(E_Token::ARITH)
            || pc->Is(E_Token::SHIFT))
         {
            expressionIsNumeric = true;
            break;
         }
      }
      pc = next;
   }
   LOG_FMT(LTEMPL, "%s(%d): expressionIsNumeric is %s\n",
           __func__, __LINE__, expressionIsNumeric ? "TRUE" : "FALSE");

   // 2. run to do the work
   if (!expressionIsNumeric)
   {
      pc = start;

      while (pc != end)
      {
         Chunk *next = pc->GetNextNcNnl(E_Scope::PREPROC);
         pc->SetFlagBits(PCF_IN_TEMPLATE);

         Chunk *prev  = pc->GetPrevNcNnl(E_Scope::PREPROC);
         Chunk *prev2 = prev->GetPrevNcNnl(E_Scope::PREPROC);

         if (  prev->Is(E_Token::ELLIPSIS)                 // Issue #3309
            && prev2->Is(E_Token::TYPENAME))
         {
            pc->SetType(E_Token::PARAMETER_PACK);
         }
         else
         {
            make_type(pc);
         }
         pc = next;
      }
   }
} // check_template_arg


void check_template_args(Chunk *start, Chunk *end)
{
   std::vector<E_Token> tokens;

   // Scan for commas
   Chunk *pc;

   for (pc = start->GetNextNcNnl(E_Scope::PREPROC);
        pc->IsNotNullChunk() && pc != end;
        pc = pc->GetNextNcNnl(E_Scope::PREPROC))
   {
      switch (pc->GetType())
      {
      case E_Token::COMMA:

         if (tokens.empty())
         {
            // Check current argument
            check_template_args(start, pc);
            start = pc;
         }
         break;

      case E_Token::ANGLE_OPEN:
      case E_Token::PAREN_OPEN:
         tokens.push_back(pc->GetType());
         break;

      case E_Token::ANGLE_CLOSE:

         if (  !tokens.empty()
            && tokens.back() == E_Token::ANGLE_OPEN)
         {
            tokens.pop_back();
         }
         break;

      case E_Token::PAREN_CLOSE:

         if (  !tokens.empty()
            && tokens.back() == E_Token::PAREN_OPEN)
         {
            tokens.pop_back();
         }
         break;

      default:
         break;
      }
   }

   // Check whatever is left
   check_template_arg(start, end);
} // check_template_args
