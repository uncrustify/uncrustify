/**
 * @file check_template.cpp
 *
 * splitted from tokenize_cleanup.cpp
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


constexpr static auto LCURRENT = LTEMPL;


using namespace uncrustify;


bool invalid_open_angle_template(Chunk *prev)
{
   if (prev->IsNullChunk())
   {
      return(false);
   }
   // A template requires a word/type right before the open angle
   return(  prev->IsNot(CT_WORD)
         && prev->IsNot(CT_TYPE)
         && prev->IsNot(CT_COMMA)
         && prev->IsNot(CT_QUALIFIER)
         && prev->IsNot(CT_OPERATOR_VAL)
         && prev->GetParentType() != CT_OPERATOR);
}


Chunk *handle_double_angle_close(Chunk *pc)
{
   Chunk *next = pc->GetNext();

   if (next->IsNotNullChunk())
   {
      if (  pc->Is(CT_ANGLE_CLOSE)
         && next->Is(CT_ANGLE_CLOSE)
         && pc->GetParentType() == CT_NONE
         && (pc->GetOrigColEnd() + 1) == next->GetOrigCol()
         && next->GetParentType() == CT_NONE)
      {
         pc->Str().append('>');
         pc->SetType(CT_SHIFT);
         pc->SetOrigColEnd(next->GetOrigColEnd());

         Chunk *tmp = next->GetNextNcNnl();
         Chunk::Delete(next);
         next = tmp;
      }
      else
      {
         // bug #663
         pc->SetType(CT_COMPARE);
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

   if (prev->Is(CT_TEMPLATE))
   {
      LOG_FMT(LTEMPL, "%s(%d): CT_TEMPLATE:\n", __func__, __LINE__);

      // We have: "template< ... >", which is a template declaration
      size_t level  = 1;
      size_t parens = 0;

      for (pc = start->GetNextNcNnl(E_Scope::PREPROC);
           pc->IsNotNullChunk();
           pc = pc->GetNextNcNnl(E_Scope::PREPROC))
      {
         LOG_FMT(LTEMPL, "%s(%d): type is %s, level is %zu\n",
                 __func__, __LINE__, get_token_name(pc->GetType()), level);

         if (  (pc->GetStr()[0] == '>')
            && (pc->Len() > 1))
         {
            if (pc->GetStr()[1] == '=')                         // Issue #1462 and #2565
            {
               LOG_FMT(LTEMPL, "%s(%d): do not split '%s' at orig line %zu, orig col %zu\n",
                       __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
            }
            else
            {
               LOG_FMT(LTEMPL, "%s(%d): {split '%s' at orig line %zu, orig col %zu}\n",
                       __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
               split_off_angle_close(pc);
            }
         }

         if (pc->Is(CT_DECLTYPE))
         {
            flag_cpp_decltype(pc);
         }
         else if (pc->Is(CT_PAREN_OPEN))
         {
            ++parens;
         }
         else if (pc->Is(CT_PAREN_CLOSE))
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
       * Finally, if we are inside an 'if' statement and hit a CT_BOOL,
       * then it isn't a template.
       */

      if (invalid_open_angle_template(prev))
      {
         LOG_FMT(LTEMPL, "%s(%d): - after type %s + ( - Not a template\n",
                 __func__, __LINE__, get_token_name(prev->GetType()));
         start->SetType(CT_COMPARE);
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
         if (  (  pc->Is(CT_SEMICOLON)
               && hit_semicolon)
            || pc->Is(CT_SQUARE_CLOSE))
         {
            break;
         }

         if (pc->Is(CT_DECLTYPE))
         {
            flag_cpp_decltype(pc);
         }

         if (pc->Is(CT_BRACE_OPEN))
         {
            if (  !pc->TestFlags(PCF_IN_DECLTYPE)
               || !detect_cpp_braced_init_list(pc->GetPrev(), pc))
            {
               break;
            }
            flag_cpp_braced_init_list(pc->GetPrev(), pc);
         }

         if (  pc->Is(CT_BRACE_CLOSE)
            && pc->GetParentType() != CT_BRACED_INIT_LIST
            && !pc->TestFlags(PCF_IN_DECLTYPE))
         {
            break;
         }

         if (  pc->Is(CT_SEMICOLON)
            && !hit_semicolon)
         {
            hit_semicolon = true;
         }

         if (  (  (  pc->Is(CT_IF)
                  || pc->Is(CT_RETURN)
                  || pc->Is(CT_WHILE)
                  || pc->Is(CT_WHILE_OF_DO))
               && !hit_semicolon)
            || (  pc->Is(CT_FOR)
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
      const int max_token_count = 1024;
      E_Token   tokens[max_token_count];
      size_t    num_tokens = 1;

      tokens[0] = CT_ANGLE_OPEN;

      for (pc = start->GetNextNcNnl(E_Scope::PREPROC);
           pc->IsNotNullChunk();
           pc = pc->GetNextNcNnl(E_Scope::PREPROC))
      {
         LOG_FMT(LTEMPL, "%s(%d): pc orig line is %zu, orig col is %zu, type is %s, num_tokens is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()), num_tokens);

         log_rule_B("tok_split_gte");

         if (pc->Is(CT_BRACE_OPEN))                     // Issue #2886
         {
            // look for the closing brace
            Chunk *A = pc->GetClosingParen();
            LOG_FMT(LTEMPL, "%s(%d): A orig line is %zu, orig col is %zu, type is %s\n",
                    __func__, __LINE__, A->GetOrigLine(), A->GetOrigCol(), get_token_name(A->GetType()));
            pc = A->GetNext();
         }

         if (  (tokens[num_tokens - 1] == CT_ANGLE_OPEN)
            && (pc->GetStr()[0] == '>')
            && (pc->Len() > 1)
            && (  options::tok_split_gte()
               || (  (  pc->IsString(">>")
                     || pc->IsString(">>>"))
                  && (  num_tokens >= 2
                     || (  num_tokens >= 1
                        && in_type_cast)))))
         {
            LOG_FMT(LTEMPL, "%s(%d): {split '%s' at orig line %zu, orig col %zu}\n",
                    __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());

            split_off_angle_close(pc);
         }

         if (pc->IsString("<"))
         {
            if (  num_tokens > 0 && (tokens[num_tokens - 1] == CT_PAREN_OPEN)
               && invalid_open_angle_template(pc->GetPrev()))
            {
               pc->SetType(CT_COMPARE); // Issue #3127
            }
            else
            {
               tokens[num_tokens] = CT_ANGLE_OPEN;
               num_tokens++;
            }
         }
         else if (pc->IsString(">"))
         {
            if (num_tokens > 0 && (tokens[num_tokens - 1] == CT_PAREN_OPEN))
            {
               handle_double_angle_close(pc);
            }
            else if (--num_tokens <= 0)
            {
               break;
            }
            else if (tokens[num_tokens] != CT_ANGLE_OPEN)
            {
               break; // unbalanced parentheses
            }
         }
         else if (  in_if
                 && (  pc->Is(CT_BOOL)
                    || pc->Is(CT_COMPARE)))
         {
            break;
         }
         else if (pc->Is(CT_BRACE_OPEN))
         {
            if (  !pc->TestFlags(PCF_IN_DECLTYPE)
               || !detect_cpp_braced_init_list(pc->GetPrev(), pc))
            {
               break;
            }
            auto brace_open  = pc->GetNextNcNnl();
            auto brace_close = brace_open->GetClosingParen();

            brace_open->SetParentType(CT_BRACED_INIT_LIST);
            brace_close->SetParentType(CT_BRACED_INIT_LIST);
         }
         else if (  pc->Is(CT_BRACE_CLOSE)
                 && pc->GetParentType() != CT_BRACED_INIT_LIST
                 && !pc->TestFlags(PCF_IN_DECLTYPE))
         {
            break;
         }
         else if (pc->Is(CT_SEMICOLON))
         {
            break;
         }
         else if (pc->Is(CT_PAREN_OPEN))
         {
            if (num_tokens >= max_token_count - 1)
            {
               break;
            }
            tokens[num_tokens] = CT_PAREN_OPEN;
            num_tokens++;
         }
         else if (  pc->Is(CT_QUESTION)                    // Issue #2949
                 && language_is_set(lang_flag_e::LANG_CPP))
         {
            break;
         }
         else if (pc->Is(CT_PAREN_CLOSE))
         {
            if (num_tokens == 0)
            {
               fprintf(stderr, "%s(%d): num_tokens is ZERO, cannot be decremented, at line %zu, column %zu\n",
                       __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            num_tokens--;

            if (tokens[num_tokens] != CT_PAREN_OPEN)
            {
               break;  // unbalanced parentheses
            }
         }
      }

      end = pc;
   }

   if (end->Is(CT_ANGLE_CLOSE))
   {
      pc = end->GetNextNcNnl(E_Scope::PREPROC);

      if (  pc->IsNullChunk()
         || pc->IsNot(CT_NUMBER))
      {
         LOG_FMT(LTEMPL, "%s(%d): Template detected\n", __func__, __LINE__);
         LOG_FMT(LTEMPL, "%s(%d):     from orig line %zu, orig col %zu\n",
                 __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol());
         LOG_FMT(LTEMPL, "%s(%d):     to   orig line %zu, orig col %zu\n",
                 __func__, __LINE__, end->GetOrigLine(), end->GetOrigCol());
         start->SetParentType(CT_TEMPLATE);

         check_template_args(start, end);

         end->SetParentType(CT_TEMPLATE);
         end->SetFlagBits(PCF_IN_TEMPLATE);
         return;
      }
   }
   LOG_FMT(LTEMPL, "%s(%d): - Not a template: end = %s\n",
           __func__, __LINE__, (end->IsNotNullChunk() ? get_token_name(end->GetType()) : "<null>"));
   start->SetType(CT_COMPARE);
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

      if (  pc->Is(CT_DECLTYPE)
         || pc->Is(CT_SIZEOF))
      {
         expressionIsNumeric = true;
         break;
      }

      if (next->IsNot(CT_PAREN_OPEN))
      {
         if (  pc->Is(CT_NUMBER)
            || pc->Is(CT_ARITH)
            || pc->Is(CT_SHIFT))
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

         if (  prev->Is(CT_ELLIPSIS)                 // Issue #3309
            && prev2->Is(CT_TYPENAME))
         {
            pc->SetType(CT_PARAMETER_PACK);
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
      case CT_COMMA:

         if (tokens.empty())
         {
            // Check current argument
            check_template_args(start, pc);
            start = pc;
         }
         break;

      case CT_ANGLE_OPEN:
      case CT_PAREN_OPEN:
         tokens.push_back(pc->GetType());
         break;

      case CT_ANGLE_CLOSE:

         if (  !tokens.empty()
            && tokens.back() == CT_ANGLE_OPEN)
         {
            tokens.pop_back();
         }
         break;

      case CT_PAREN_CLOSE:

         if (  !tokens.empty()
            && tokens.back() == CT_PAREN_OPEN)
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
