/**
 * @file var_def_blk.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "newlines/var_def_blk.h"

#include "log_rules.h"
#include "mark_change.h"
#include "newlines/is_func_call_or_def.h"
#include "newlines/is_var_def.h"
#include "newlines/min_after.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


// Put newline(s) before and/or after a block of variable definitions
Chunk *newline_var_def_blk(Chunk *start)
{
   LOG_FUNC_ENTRY();

   Chunk *pc           = start;
   Chunk *prev         = start->GetPrevNcNnlNi(); // Issue #2279
   bool  did_this_line = false;
   bool  fn_top        = false;
   bool  var_blk       = false;
   bool  first_var_blk = true;

   LOG_FMT(LVARDFBLK, "%s(%d): start orig line is %zu, orig col is %zu, text is '%s'\n",
           __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol(), start->GetLogText());

   if (start->Is(E_Token::BRACE_OPEN))
   {
      // can't be any variable definitions in a "= {" block
      if (  prev->IsNotNullChunk()
         && prev->Is(E_Token::ASSIGN))
      {
         Chunk *tmp = start->GetClosingParen();
         return(tmp->GetNextNcNnl());
      }
      // check if we're at the top of a function definition, or function call with a
      // possible variable block
      fn_top = is_func_call_or_def(start);
      // opening brace is processed, start with next chunk
      pc = pc->GetNext();
   }

   while (  pc->IsNotNullChunk()
         && (  pc->GetLevel() >= start->GetLevel()
            || pc->GetLevel() == 0))
   {
      LOG_CHUNK(LTOK, pc);

      Chunk *next_pc = pc->GetNext();
      LOG_FMT(LVARDFBLK, "%s(%d): next_pc orig line is %zu, orig col is %zu, type is %s, text is '%s'\n",
              __func__, __LINE__, next_pc->GetOrigLine(), next_pc->GetOrigCol(), get_token_name(next_pc->GetType()), next_pc->GetLogText());

      // If next_pc token is E_Token::DC_MEMBER, skip it
      if (next_pc->Is(E_Token::DC_MEMBER))
      {
         pc = pc->SkipDcMember();
      }

      // skip qualifiers
      if (pc->Is(E_Token::QUALIFIER))
      {
         pc = pc->GetNext();
         continue;
      }

      if (pc->IsComment())
      {
         pc = pc->GetNext();
         continue;
      }

      // process nested braces
      if (pc->Is(E_Token::BRACE_OPEN))
      {
         pc = newline_var_def_blk(pc);
         continue;
      }

      // Done with this brace set?
      if (pc->Is(E_Token::BRACE_CLOSE))
      {
         pc = pc->GetNext();
         break;
      }

      // skip vbraces
      if (pc->Is(E_Token::VBRACE_OPEN))
      {
         pc = pc->GetNextType(E_Token::VBRACE_CLOSE, pc->GetLevel());
         pc = pc->GetNext();
         continue;
      }

      // Ignore stuff inside parenthesis/squares/angles
      if (pc->GetLevel() > pc->GetBraceLevel())
      {
         pc = pc->GetNext();
         continue;
      }

      if (pc->IsNewline())
      {
         did_this_line = false;
         pc            = pc->GetNext();
         continue;
      }

      // Determine if this is a variable definition or code
      if (  !did_this_line
         && pc->IsNot(E_Token::FUNC_CLASS_PROTO)
         && (  (pc->GetLevel() == (start->GetLevel() + 1))
            || pc->GetLevel() == 0))
      {
         // Find the "next" chunk for is_var_def()
         Chunk *next = pc->GetNextNcNnl();

         LOG_FMT(LVARDFBLK, "%s(%d): next orig line is %zu, orig col is %zu, type is %s, text is '%s'\n",
                 __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), get_token_name(next->GetType()), next->GetLogText());

         // skip over all other type-like things
         while (  next->Is(E_Token::PTR_TYPE)  // Issue #2692
               || next->Is(E_Token::BYREF)     // Issue #3018
               || next->Is(E_Token::QUALIFIER)
               || next->Is(E_Token::TSQUARE))
         {
            next = next->GetNextNcNnl();
            LOG_FMT(LVARDFBLK, "%s(%d): next orig line is %zu, orig col is %zu, text is '%s'\n",
                    __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), next->GetLogText());
         }

         if (next->IsNullChunk())
         {
            break;
         }
         LOG_FMT(LVARDFBLK, "%s(%d): next orig line is %zu, orig col is %zu, type is %s, text is '%s'\n",
                 __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), get_token_name(next->GetType()), next->GetLogText());

         // Find the end of the previous block
         LOG_FMT(LVARDFBLK, "%s(%d): pc orig line is %zu, orig col is %zu, type is %s, text is '%s'\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()), pc->GetLogText());

         prev = pc->GetPrevNcNnl();

         LOG_FMT(LVARDFBLK, "%s(%d): prev orig line is %zu, orig col is %zu, type is %s, text is '%s'\n",
                 __func__, __LINE__, prev->GetOrigLine(), prev->GetOrigCol(), get_token_name(prev->GetType()), prev->GetLogText());

         while (  prev->Is(E_Token::DC_MEMBER)
               || prev->Is(E_Token::QUALIFIER)
               || prev->Is(E_Token::TYPE))
         {
            prev = prev->GetPrevNcNnl();
         }

         if (!(  prev->IsBraceOpen()
              || prev->IsBraceClose()))
         {
            prev = pc->GetPrevType(E_Token::SEMICOLON, pc->GetLevel());
         }

         if (prev->IsNullChunk())
         {
            prev = pc->GetPrevType(E_Token::BRACE_OPEN, pc->GetLevel() - 1);      // Issue #2692
         }

         if (  prev->Is(E_Token::STRING)
            && prev->GetParentType() == E_Token::EXTERN
            && prev->GetPrev()->Is(E_Token::EXTERN))
         {
            prev = prev->GetPrev()->GetPrevNcNnlNi();   // Issue #2279
         }
         LOG_FMT(LVARDFBLK, "%s(%d): prev orig line is %zu, orig col is %zu, type is %s, text is '%s'\n",
                 __func__, __LINE__, prev->GetOrigLine(), prev->GetOrigCol(), get_token_name(prev->GetType()), prev->GetLogText());

         if (pc->Is(E_Token::FUNC_CLASS_DEF))
         {
            log_rule_B("nl_var_def_blk_end");

            if (  var_blk
               && options::nl_var_def_blk_end() > 0)
            {
               prev = prev->GetPrev();
               newline_min_after(prev, options::nl_var_def_blk_end() + 1, PCF_VAR_DEF);
               pc            = pc->GetNext();
               first_var_blk = false;
               var_blk       = false;
            }
         }
         else if (is_var_def(pc, next))
         {
            LOG_FMT(LVARDFBLK, "%s(%d): 'typ==var' found: '%s %s' at line %zu\n",
                    __func__, __LINE__, pc->GetLogText(), next->GetLogText(), pc->GetOrigLine());
            LOG_FMT(LBLANKD, "%s(%d): var_blk %s, first_var_blk %s, fn_top %s\n",
                    __func__, __LINE__, var_blk ? "TRUE" : "FALSE",
                    first_var_blk ? "TRUE" : "FALSE", fn_top ? "TRUE" : "FALSE");
            // Put newline(s) before a block of variable definitions
            log_rule_B("nl_var_def_blk_start");

            if (  !var_blk
               && !first_var_blk
               && options::nl_var_def_blk_start() > 0)
            {
               LOG_FMT(LVARDFBLK, "%s(%d): pc is '%s', orig line is %zu\n",
                       __func__, __LINE__, pc->GetLogText(), pc->GetOrigLine());

               if (prev->IsNullChunk())
               {
                  LOG_FMT(LVARDFBLK, "%s(%d): prev is a null chunk\n", __func__, __LINE__);
               }
               else
               {
                  LOG_FMT(LVARDFBLK, "%s(%d): prev is '%s', orig line is %zu\n",
                          __func__, __LINE__, prev->GetLogText(), prev->GetOrigLine());

                  if (!prev->IsBraceOpen())
                  {
                     newline_min_after(prev, options::nl_var_def_blk_start() + 1, PCF_VAR_DEF);
                  }
               }
            }
            // set newlines within var def block
            log_rule_B("nl_var_def_blk_in");

            if (  var_blk
               && (options::nl_var_def_blk_in() > 0))
            {
               prev = pc->GetPrev();
               LOG_FMT(LVARDFBLK, "%s(%d): prev orig line is %zu, orig col is %zu, text is '%s'\n",
                       __func__, __LINE__, prev->GetOrigLine(), prev->GetOrigCol(), prev->GetLogText());

               if (prev->IsNewline())
               {
                  if (prev->GetNlCount() > options::nl_var_def_blk_in())
                  {
                     prev->SetNlCount(options::nl_var_def_blk_in());
                     MARK_CHANGE();
                  }
               }
            }
            pc      = pc->GetNextType(E_Token::SEMICOLON, pc->GetLevel());
            var_blk = true;
         }
         else if (var_blk)
         {
            LOG_FMT(LVARDFBLK, "%s(%d): var_blk %s, first_var_blk %s, fn_top %s\n",
                    __func__, __LINE__, /* var_blk ? */ "TRUE" /* : "FALSE" */,
                    first_var_blk ? "TRUE" : "FALSE", fn_top ? "TRUE" : "FALSE");
            log_rule_B("nl_var_def_blk_end_func_top");
            log_rule_B("nl_var_def_blk_end");

            if (  first_var_blk
               && fn_top)
            {
               // set blank lines after first var def block at the top of a function
               if (options::nl_var_def_blk_end_func_top() > 0)
               {
                  LOG_FMT(LVARDFBLK, "%s(%d): nl_var_def_blk_end_func_top at line %zu\n",
                          __func__, __LINE__, prev->GetOrigLine());
                  newline_min_after(prev, options::nl_var_def_blk_end_func_top() + 1, PCF_VAR_DEF);
               }
            }
            else if (  !pc->IsPreproc()
                    && options::nl_var_def_blk_end() > 0)
            {
               // set blank lines after other var def blocks
               LOG_FMT(LVARDFBLK, "%s(%d): nl_var_def_blk_end at line %zu\n",
                       __func__, __LINE__, prev->GetOrigLine());
               // Issue #3516
               newline_min_after(prev, options::nl_var_def_blk_end() + 1, PCF_VAR_DEF);
            }
            // reset the variables for the next block
            first_var_blk = false;
            var_blk       = false;
         }
         else
         {
            first_var_blk = false;
            var_blk       = false;
         }
      }
      did_this_line = true;
      pc            = pc->GetNext();
   }
   LOG_FMT(LVARDFBLK, "%s(%d): pc orig line is %zu, orig col is %zu, text is '%s', level is %zu\n",
           __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLogText(), pc->GetLevel());
   LOG_FMT(LVARDFBLK, "%s(%d): start orig line is %zu, orig col is %zu, text is '%s', level is %zu\n",
           __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol(), start->GetLogText(), start->GetLevel());
   return(pc);
} // newline_var_def_blk
