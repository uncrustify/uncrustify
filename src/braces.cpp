/**
 * @file braces.cpp
 * Adds or removes braces.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "braces.h"

#include "calculate_closing_brace_position.h"
#include "newlines/add.h"
#include "newlines/between.h"
#include "newlines/del_between.h"
#include "prototypes.h"
#include "tokenizer/combine_tools.h"


constexpr static auto LCURRENT = LBR;


using std::vector;
using namespace uncrustify;


//! Converts a single brace into a virtual brace
static void convert_brace(Chunk *br);


//! Converts a single virtual brace into a real brace
static void convert_vbrace(Chunk *br);


static void convert_vbrace_to_brace();


//! Go backwards to honor brace newline removal limits
static void examine_braces();


/**
 * Step forward and count the number of semi colons at the current level.
 * Abort if more than 1 or if we enter a preprocessor
 */
static void examine_brace(Chunk *bopen);


static void move_case_break();


static void move_case_return();


static void mod_case_brace();


static void mod_full_brace_if_chain();


/**
 * Checks to see if the braces can be removed.
 *  - less than a certain length
 *  - doesn't mess up if/else stuff
 */
static bool can_remove_braces(Chunk *bopen);


/**
 * Checks to see if the virtual braces should be converted to real braces.
 *  - over a certain length
 *
 * @param vbopen  Virtual Brace Open chunk
 *
 * @return true (convert to real braces) or false (leave alone)
 */
static bool should_add_braces(Chunk *vbopen);


/**
 * Collect the text into txt that contains the full tag name.
 * Mainly for collecting namespace 'a.b.c' or function 'foo::bar()' names.
 */
static void append_tag_name(UncText &txt, Chunk *pc);


//! Remove the case brace, if allowable.
static Chunk *mod_case_brace_remove(Chunk *br_open);


//! Add the case brace, if allowable.
static Chunk *mod_case_brace_add(Chunk *cl_colon);


/**
 * Traverse the if chain and see if all can be removed
 *
 * @param br_start  chunk pointing to opening brace of if clause
 */
static void process_if_chain(Chunk *br_start);


/**
 * Check if parenthesis pair that comes before a brace spans multiple lines
 *
 *
 * @param brace  the brace chunk whose predecessing parenthesis will be checked
 *
 * @pre   it needs to be of type CT_BRACE_OPEN or CT_BRACE_CLOSE,
 *        its parent type needs to be one of this types:
 *            CT_IF, CT_ELSEIF, CT_FOR, CT_USING_STMT, CT_WHILE,
 *            CT_FUNC_CLASS_DEF, CT_FUNC_DEF
 *
 * @return  false: if preconditions are not met,
 *                 if an error occurs while  counting the newline between the
 *                 parenthesis or
 *                 when no newlines are found between the parenthesis
 */
static bool paren_multiline_before_brace(Chunk *brace)
{
   if (  (  brace->IsNot(CT_BRACE_OPEN)
         && brace->IsNot(CT_BRACE_CLOSE))
      || (  brace->GetParentType() != CT_IF
         && brace->GetParentType() != CT_ELSEIF
         && brace->GetParentType() != CT_FOR
         && brace->GetParentType() != CT_USING_STMT
         && brace->GetParentType() != CT_WHILE
         && brace->GetParentType() != CT_FUNC_CLASS_DEF
         && brace->GetParentType() != CT_FUNC_DEF))
   {
      return(false);
   }
   const auto paren_t = CT_SPAREN_CLOSE;

   // find parenthesis pair of the if/for/while/...
   auto paren_close = brace->GetPrevType(paren_t, brace->GetLevel(), E_Scope::ALL);
   auto paren_open  = paren_close->GetOpeningParen();

   if (  paren_close->IsNullChunk()
      || paren_open->IsNullChunk()
      || paren_close == brace
      || paren_open == paren_close)
   {
      return(false);
   }
   // determine number of lines in the parenthesis pair spans
   auto       nl_count = size_t{};
   const auto ret_flag = newlines_between(paren_open, paren_close, nl_count);

   if (!ret_flag)
   {
      LOG_FMT(LERR, "%s(%d): newlines_between error\n", __func__, __LINE__);
      return(false);
   }
   // nl_count = 0 -> 1 line
   return(nl_count > 0);
}


void do_braces()
{
   LOG_FUNC_ENTRY();
   // Mark one-liners
   // Issue #2232 put this at the beginning
   Chunk *pc = Chunk::GetHead()->GetNextNcNnl();

   while (pc->IsNotNullChunk())
   {
      if (  pc->IsNot(CT_BRACE_OPEN)
         && pc->IsNot(CT_VBRACE_OPEN))
      {
         pc = pc->GetNextNcNnl();
         continue;
      }
      Chunk         *br_open = pc;
      const E_Token brc_type = E_Token(pc->GetType() + 1); // corresponds to closing type
      // Detect empty bodies
      Chunk         *tmp = pc->GetNextNcNnl();

      if (tmp->Is(brc_type))
      {
         br_open->SetFlagBits(PCF_EMPTY_BODY);
         tmp->SetFlagBits(PCF_EMPTY_BODY);
      }
      // Scan for the brace close or a newline
      tmp = br_open->GetNextNc();

      while (tmp->IsNotNullChunk())
      {
         if (tmp->IsNewline())
         {
            break;
         }

         if (  tmp->Is(brc_type)
            && br_open->GetLevel() == tmp->GetLevel())
         {
            flag_series(br_open, tmp, PCF_ONE_LINER);
            break;
         }
         tmp = tmp->GetNextNc();
      }
      pc = pc->GetNextNcNnl();
   }
   log_rule_B("mod_full_brace_if_chain");
   log_rule_B("mod_full_brace_if_chain_only");

   if (  options::mod_full_brace_if_chain()
      || options::mod_full_brace_if_chain_only())
   {
      mod_full_brace_if_chain();
   }
   log_rule_B("mod_full_brace_if");
   log_rule_B("mod_full_brace_do");
   log_rule_B("mod_full_brace_for");
   log_rule_B("mod_full_brace_using");
   log_rule_B("mod_full_brace_while");

   if ((options::mod_full_brace_if() |
        options::mod_full_brace_do() |
        options::mod_full_brace_for() |
        options::mod_full_brace_using() |
        options::mod_full_brace_while()) & IARF_REMOVE)
   {
      examine_braces();
   }
   // convert vbraces if needed
   log_rule_B("mod_full_brace_if");
   log_rule_B("mod_full_brace_do");
   log_rule_B("mod_full_brace_for");
   log_rule_B("mod_full_brace_function");
   log_rule_B("mod_full_brace_using");
   log_rule_B("mod_full_brace_while");

   if ((options::mod_full_brace_if() |
        options::mod_full_brace_do() |
        options::mod_full_brace_for() |
        options::mod_full_brace_function() |
        options::mod_full_brace_using() |
        options::mod_full_brace_while()) & IARF_ADD)
   {
      convert_vbrace_to_brace();
   }
   log_rule_B("mod_case_brace");

   if (options::mod_case_brace() != IARF_IGNORE)
   {
      mod_case_brace();
   }
   log_rule_B("mod_move_case_break");

   if (options::mod_move_case_break())
   {
      move_case_break();
   }
   log_rule_B("mod_move_case_return");

   if (options::mod_move_case_return())
   {
      move_case_return();
   }
} // do_braces


static void examine_braces()
{
   LOG_FUNC_ENTRY();

   log_rule_B("mod_full_brace_nl_block_rem_mlcond");
   const auto multiline_block = options::mod_full_brace_nl_block_rem_mlcond();

   log_rule_B("mod_full_brace_if");
   log_rule_B("mod_full_brace_do");
   log_rule_B("mod_full_brace_for");
   log_rule_B("mod_full_brace_using");
   log_rule_B("mod_full_brace_while");

   for (Chunk *pc = Chunk::GetTail(); pc->IsNotNullChunk();)
   {
      Chunk *prev = pc->GetPrevType(CT_BRACE_OPEN);

      if (  pc->Is(CT_BRACE_OPEN)
         && !pc->TestFlags(PCF_IN_PREPROC)
         && (  (  (  pc->GetParentType() == CT_IF
                  || pc->GetParentType() == CT_ELSE
                  || pc->GetParentType() == CT_ELSEIF)
               && options::mod_full_brace_if() == IARF_REMOVE)
            || (  pc->GetParentType() == CT_DO
               && options::mod_full_brace_do() == IARF_REMOVE)
            || (  pc->GetParentType() == CT_FOR
               && options::mod_full_brace_for() == IARF_REMOVE)
            || (  pc->GetParentType() == CT_USING_STMT
               && options::mod_full_brace_using() == IARF_REMOVE)
            || (  pc->GetParentType() == CT_WHILE
               && options::mod_full_brace_while() == IARF_REMOVE)))
      {
         if (  multiline_block
            && paren_multiline_before_brace(pc))
         {
            pc = prev;
            continue;
         }
         examine_brace(pc);
      }
      pc = prev;
   }
} // examine_braces


static bool should_add_braces(Chunk *vbopen)
{
   LOG_FUNC_ENTRY();
   log_rule_B("mod_full_brace_nl");
   const size_t nl_max = options::mod_full_brace_nl();

   if (nl_max == 0)
   {
      return(false);
   }
   LOG_FMT(LBRDEL, "%s(%d): start on %zu:\n",
           __func__, __LINE__, vbopen->GetOrigLine());

   size_t nl_count = 0;

   Chunk  *pc = Chunk::NullChunkPtr;

   for (pc = vbopen->GetNextNc(E_Scope::PREPROC);
        (pc->IsNotNullChunk() && pc->GetLevel() > vbopen->GetLevel());
        pc = pc->GetNextNc(E_Scope::PREPROC))
   {
      if (pc->IsNewline())
      {
         nl_count += pc->GetNlCount();
      }
   }

   if (  pc->IsNotNullChunk()
      && nl_count > nl_max
      && vbopen->GetPpLevel() == pc->GetPpLevel())
   {
      LOG_FMT(LBRDEL, "%s(%d): exceeded %zu newlines\n",
              __func__, __LINE__, nl_max);
      return(true);
   }
   return(false);
}


static bool can_remove_braces(Chunk *bopen)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LBRDEL, "%s(%d): start on line %zu:\n",
           __func__, __LINE__, bopen->GetOrigLine());

   // Cannot remove braces inside a preprocessor
   if (bopen->TestFlags(PCF_IN_PREPROC))
   {
      return(false);
   }
   Chunk *pc = bopen->GetNextNcNnl(E_Scope::PREPROC);

   if (pc->Is(CT_BRACE_CLOSE))
   {
      // Can't remove empty statement
      return(false);
   }
   const size_t level = bopen->GetLevel() + 1;

   log_rule_B("mod_full_brace_nl");
   const size_t nl_max = options::mod_full_brace_nl();
   Chunk        *prev  = Chunk::NullChunkPtr;

   size_t       semi_count = 0;
   bool         hit_semi   = false;
   size_t       nl_count   = 0;
   size_t       if_count   = 0;
   int          br_count   = 0;

   pc = bopen->GetNextNc(E_Scope::ALL);
   LOG_FMT(LBRDEL, "%s(%d):  - begin with token '%s', orig line is %zu, orig col is %zu\n",
           __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());

   while (  pc->IsNotNullChunk()
         && pc->GetLevel() >= level)
   {
      LOG_FMT(LBRDEL, "%s(%d): test token '%s', orig line is %zu, orig col is %zu\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());

      if (pc->TestFlags(PCF_IN_PREPROC))
      {
         // Cannot remove braces that contain a preprocessor
         return(false);
      }

      if (pc->IsNewline())
      {
         nl_count += pc->GetNlCount();

         if (  nl_max > 0
            && nl_count > nl_max)
         {
            LOG_FMT(LBRDEL, "%s(%d):  exceeded %zu newlines\n",
                    __func__, __LINE__, nl_max);
            return(false);
         }
      }
      else
      {
         if (pc->Is(CT_BRACE_OPEN))
         {
            br_count++;
         }
         else if (pc->Is(CT_BRACE_CLOSE))
         {
            if (br_count == 0)
            {
               fprintf(stderr, "%s(%d): br_count is ZERO, cannot be decremented, at line %zu, column %zu\n",
                       __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            br_count--;

            if (pc->GetLevel() == level)
            {
               // mean a statement in a braces { stmt; }
               // as a statement with a semicolon { stmt; };
               ++semi_count;
               hit_semi = true;
            }
         }
         else if (  (  pc->Is(CT_IF)
                    || pc->Is(CT_ELSEIF))
                 && br_count == 0)
         {
            if_count++;
         }

         if (pc->GetLevel() == level)
         {
            if (  semi_count > 0
               && hit_semi)
            {
               // should have bailed due to close brace level drop
               LOG_FMT(LBRDEL, "%s(%d):  no close brace\n", __func__, __LINE__);
               return(false);
            }
            LOG_FMT(LBRDEL, "%s(%d): Text() '%s', orig line is %zu, semi_count is %zu\n",
                    __func__, __LINE__, pc->Text(), pc->GetOrigLine(), semi_count);

            if (pc->Is(CT_ELSE))
            {
               LOG_FMT(LBRDEL, "%s(%d):  bailed on '%s' on line %zu\n",
                       __func__, __LINE__, pc->Text(), pc->GetOrigLine());
               return(false);
            }

            if (  pc->IsSemicolon()
               || pc->Is(CT_IF)
               || pc->Is(CT_ELSEIF)
               || pc->Is(CT_FOR)
               || pc->Is(CT_DO)
               || pc->Is(CT_WHILE)
               || pc->Is(CT_USING_STMT)
               || (  pc->Is(CT_BRACE_OPEN)
                  && prev->Is(CT_FPAREN_CLOSE)))
            {
               hit_semi |= pc->IsSemicolon();

               if (++semi_count > 1)
               {
                  LOG_FMT(LBRDEL, "%s(%d):  bailed on %zu because of '%s' on line %zu\n",
                          __func__, __LINE__, bopen->GetOrigLine(), pc->Text(), pc->GetOrigLine());
                  return(false);
               }
            }
         }
      }
      prev = pc;
      pc   = pc->GetNextNc();
   }

   if (pc->IsNullChunk())
   {
      LOG_FMT(LBRDEL, "%s(%d):  pc is null chunk\n", __func__, __LINE__);
      return(false);
   }

   if (  pc->Is(CT_BRACE_CLOSE)
      && pc->GetParentType() == CT_IF)
   {
      Chunk *next     = pc->GetNextNcNnl(E_Scope::PREPROC);
      Chunk *tmp_prev = pc->GetPrevNcNnl(E_Scope::PREPROC);

      if (  next->Is(CT_ELSE)
         && tmp_prev->IsBraceClose()
         && tmp_prev->GetParentType() == CT_IF)
      {
         LOG_FMT(LBRDEL, "%s(%d):  - bailed on '%s'[%s] on line %zu due to 'if' and 'else' sequence\n",
                 __func__, __LINE__, get_token_name(pc->GetType()), get_token_name(pc->GetParentType()),
                 pc->GetOrigLine());
         return(false);
      }
   }
   LOG_FMT(LBRDEL, "%s(%d):  - end on '%s' on line %zu. if_count is %zu semi_count is %zu\n",
           __func__, __LINE__, get_token_name(pc->GetType()), pc->GetOrigLine(), if_count, semi_count);

   return(  pc->Is(CT_BRACE_CLOSE)
         && pc->GetPpLevel() == bopen->GetPpLevel());
} // can_remove_braces


static void examine_brace(Chunk *bopen)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LBRDEL, "%s(%d): start on orig line %zu, bopen->GetLevel() is %zu\n",
           __func__, __LINE__, bopen->GetOrigLine(), bopen->GetLevel());

   const size_t level = bopen->GetLevel() + 1;

   log_rule_B("mod_full_brace_nl");
   const size_t nl_max = options::mod_full_brace_nl();

   Chunk        *prev      = Chunk::NullChunkPtr;
   size_t       semi_count = 0;
   bool         hit_semi   = false;
   size_t       nl_count   = 0;
   size_t       if_count   = 0;
   int          br_count   = 0;

   Chunk        *pc = bopen->GetNextNc();

   while (  pc->IsNotNullChunk()
         && pc->GetLevel() >= level)
   {
      if (pc->Is(CT_NEWLINE))
      {
         LOG_FMT(LBRDEL, "%s(%d): orig line is %zu, orig col is %zu, <Newline>\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
      }
      else
      {
         LOG_FMT(LBRDEL, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s'\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
      }

      if (pc->TestFlags(PCF_IN_PREPROC))
      {
         // Cannot remove braces that contain a preprocessor
         LOG_FMT(LBRDEL, "%s(%d):  PREPROC\n", __func__, __LINE__);
         return;
      }

      if (pc->IsNewline())
      {
         nl_count += pc->GetNlCount();

         if (  nl_max > 0
            && nl_count > nl_max)
         {
            LOG_FMT(LBRDEL, "%s(%d):  exceeded %zu newlines\n",
                    __func__, __LINE__, nl_max);
            return;
         }
      }
      else
      {
         LOG_FMT(LBRDEL, "%s(%d): for pc->Text() '%s', pc->GetLevel() is %zu,  bopen->GetLevel() is %zu\n",
                 __func__, __LINE__, pc->Text(), pc->GetLevel(), bopen->GetLevel());

         if (  pc->Is(CT_BRACE_OPEN)
            && pc->GetLevel() == bopen->GetLevel())
         {
            br_count++;
            LOG_FMT(LBRDEL, "%s(%d): br_count is now %d, pc->GetLevel() is %zu,  bopen->GetLevel() is %zu\n",
                    __func__, __LINE__, br_count, pc->GetLevel(), bopen->GetLevel());
         }
         else if (  pc->Is(CT_BRACE_CLOSE)
                 && pc->GetLevel() == bopen->GetLevel())
         {
            if (br_count == 0)
            {
               fprintf(stderr, "%s(%d): br_count is ZERO, cannot be decremented, at line %zu, column %zu\n",
                       __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol());
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            br_count--;
            LOG_FMT(LBRDEL, "%s(%d): br_count is now %d, pc->GetLevel() is %zu,  bopen->GetLevel() is %zu\n",
                    __func__, __LINE__, br_count, pc->GetLevel(), bopen->GetLevel());

            if (br_count == 0)
            {
               Chunk *next = pc->GetNextNcNnl(E_Scope::PREPROC);

               if (  next->IsNullChunk()
                  || next->IsNot(CT_BRACE_CLOSE))
               {
                  LOG_FMT(LBRDEL, "%s(%d):  junk after close brace\n", __func__, __LINE__);
                  return;
               }
            }
         }
         else if (  (  pc->Is(CT_IF)
                    || pc->Is(CT_ELSEIF))
                 && br_count == 0)
         {
            if_count++;
         }
         LOG_FMT(LBRDEL, "%s(%d): pc->GetLevel() is %zu, level is %zu\n",
                 __func__, __LINE__, pc->GetLevel(), level);

         if (pc->GetLevel() == level)
         {
            if (  semi_count > 0
               && hit_semi)
            {
               // should have bailed due to close brace level drop
               LOG_FMT(LBRDEL, "%s(%d):  no close brace\n", __func__, __LINE__);
               return;
            }
            LOG_FMT(LBRDEL, "%s(%d): Text() '%s', orig line is %zu, semi_count is %zu\n",
                    __func__, __LINE__, pc->Text(), pc->GetOrigLine(), semi_count);

            if (pc->Is(CT_ELSE))
            {
               LOG_FMT(LBRDEL, "%s(%d):  bailed on '%s' on line %zu\n",
                       __func__, __LINE__, pc->Text(), pc->GetOrigLine());
               return;
            }

            if (prev->IsNotNullChunk())
            {
               LOG_FMT(LBRDEL, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', prev->Text '%s', prev->GetType() %s\n",
                       __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), prev->Text(), get_token_name(prev->GetType()));
            }
            else
            {
               LOG_FMT(LBRDEL, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', prev is a null chunk\n",
                       __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
            }
            LOG_FMT(LBRDEL, "%s(%d): for pc->Text() '%s', pc->GetLevel() is %zu,  bopen->GetLevel() is %zu\n",
                    __func__, __LINE__, pc->Text(), pc->GetLevel(), bopen->GetLevel());

            if (  pc->IsSemicolon()
               || pc->Is(CT_IF)
               || pc->Is(CT_ELSEIF)
               || pc->Is(CT_FOR)
               || pc->Is(CT_DO)
               || pc->Is(CT_WHILE)
               || pc->Is(CT_SWITCH)
               || pc->Is(CT_USING_STMT)
               || (  pc->Is(CT_BRACE_OPEN)
                  && pc->GetLevel() == bopen->GetLevel())) // Issue #1758
            {
               LOG_FMT(LBRDEL, "%s(%d): pc->Text() '%s', orig line is %zu, orig col is %zu, level is %zu\n",
                       __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetLevel());
               hit_semi |= pc->IsSemicolon();
               semi_count++;
               LOG_FMT(LBRDEL, "%s(%d): semi_count is %zu\n",
                       __func__, __LINE__, semi_count);

               if (semi_count > 1)
               {
                  LOG_FMT(LBRDEL, "%s(%d):  bailed on %zu because of '%s' on line %zu\n",
                          __func__, __LINE__, bopen->GetOrigLine(), pc->Text(), pc->GetOrigLine());
                  return;
               }
            }
         }
      }
      prev = pc;
      pc   = pc->GetNext();                  // Issue #1907
   }

   if (pc->IsNullChunk())
   {
      LOG_FMT(LBRDEL, "%s(%d): pc is a null chunk\n", __func__, __LINE__);
      return;
   }
   LOG_FMT(LBRDEL, "%s(%d):  - end on '%s' on line %zu. if_count is %zu, semi_count is %zu\n",
           __func__, __LINE__, get_token_name(pc->GetType()), pc->GetOrigLine(), if_count, semi_count);

   if (pc->Is(CT_BRACE_CLOSE))
   {
      Chunk *next = pc->GetNextNcNnl();

      if (next->IsNotNullChunk())
      {
         while (next->Is(CT_VBRACE_CLOSE))
         {
            next = next->GetNextNcNnl();
         }

         if (next->IsNotNullChunk())
         {
            LOG_FMT(LBRDEL, "%s(%d): orig line is %zu, orig col is %zu, next is '%s'\n",
                    __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), get_token_name(next->GetType()));
         }

         if (  if_count > 0
            && (  next->Is(CT_ELSE)
               || next->Is(CT_ELSEIF)))
         {
            LOG_FMT(LBRDEL, "%s(%d):  bailed on because 'else' is next and %zu ifs\n",
                    __func__, __LINE__, if_count);
            return;
         }
      }
      LOG_FMT(LBRDEL, "%s(%d): semi_count is %zu\n",
              __func__, __LINE__, semi_count);

      if (semi_count > 0)
      {
         LOG_FMT(LBRDEL, "%s(%d): bopen->GetParentType() is %s\n",
                 __func__, __LINE__, get_token_name(bopen->GetParentType()));

         if (bopen->GetParentType() == CT_ELSE)
         {
            Chunk *tmp_next = bopen->GetNextNcNnl();

            if (tmp_next->Is(CT_IF))
            {
               Chunk *tmp_prev = bopen->GetPrevNcNnl();
               LOG_FMT(LBRDEL, "%s(%d):  else-if removing braces on line %zu and %zu\n",
                       __func__, __LINE__, bopen->GetOrigLine(), pc->GetOrigLine());

               Chunk::Delete(bopen);
               Chunk::Delete(pc);
               newline_del_between(tmp_prev, tmp_next);

               log_rule_B("nl_else_if");

               if (options::nl_else_if() & IARF_ADD)
               {
                  newline_add_between(tmp_prev, tmp_next);
               }
               return;
            }
         }
         // we have a pair of braces with only 1 statement inside
         LOG_FMT(LBRDEL, "%s(%d): we have a pair of braces with only 1 statement inside\n",
                 __func__, __LINE__);
         LOG_FMT(LBRDEL, "%s(%d): removing braces on line %zu and %zu\n",
                 __func__, __LINE__, bopen->GetOrigLine(), pc->GetOrigLine());
         convert_brace(bopen);
         convert_brace(pc);
      }
      else
      {
         LOG_FMT(LBRDEL, "%s(%d):  empty statement\n", __func__, __LINE__);
      }
   }
   else
   {
      LOG_FMT(LBRDEL, "%s(%d):  not a close brace? - '%s'\n",
              __func__, __LINE__, pc->Text());
   }
} // examine_brace


static void convert_brace(Chunk *br)
{
   LOG_FUNC_ENTRY();

   if (br->TestFlags(PCF_KEEP_BRACE))
   {
      return;
   }
   Chunk *tmp;

   if (br->Is(CT_BRACE_OPEN))
   {
      br->SetType(CT_VBRACE_OPEN);
      br->Str().clear();
      tmp = br->GetPrev();

      if (tmp->IsNullChunk())
      {
         return;
      }
   }
   else if (br->Is(CT_BRACE_CLOSE))
   {
      br->SetType(CT_VBRACE_CLOSE);
      br->Str().clear();
      tmp = br->GetNext();

      if (tmp->IsNullChunk())
      {
         return;
      }
   }
   else
   {
      return;
   }

   if (tmp->IsNewline())
   {
      if (tmp->GetNlCount() > 1)
      {
         if (!br->TestFlags(PCF_ONE_LINER)) // Issue #2232
         {
            tmp->SetNlCount(tmp->GetNlCount() - 1);
            LOG_FMT(LBRDEL, "%s(%d): tmp new line count is %zu\n",
                    __func__, __LINE__, tmp->GetNlCount());
         }
      }
      else
      {
         // Issue #2219
         // look for opening brace
         Chunk *brace = Chunk::NullChunkPtr;

         if (br->Is(CT_VBRACE_OPEN))
         {
            brace = tmp;
         }
         else if (br->Is(CT_VBRACE_CLOSE))
         {
            brace = br->GetOpeningParen();

            if (brace->IsNullChunk())
            {
               brace = br->GetPrevType(CT_BRACE_OPEN, br->GetLevel());
            }
         }

         if (  br->Is(CT_VBRACE_OPEN)
            || (  br->Is(CT_VBRACE_CLOSE)
               && brace->GetOrigLine() < tmp->GetOrigLine()))
         {
            if (tmp->SafeToDeleteNl())
            {
               Chunk::Delete(tmp);
            }
         }
      }
   }
} // convert_brace


static void convert_vbrace(Chunk *vbr)
{
   LOG_FUNC_ENTRY();

   if (vbr->Is(CT_VBRACE_OPEN))
   {
      vbr->SetType(CT_BRACE_OPEN);
      vbr->Str() = "{";

      /*
       * If the next chunk is a preprocessor, then move the open brace after the
       * preprocessor.
       */
      Chunk *tmp = vbr->GetNext();

      if (tmp->Is(CT_PREPROC))
      {
         tmp = vbr->GetNext(E_Scope::PREPROC);
         vbr->MoveAfter(tmp);
         newline_add_after(vbr);
      }
   }
   else if (vbr->Is(CT_VBRACE_CLOSE))
   {
      vbr->SetType(CT_BRACE_CLOSE);
      vbr->Str() = "}";

      /*
       * If the next chunk is a comment, followed by a newline, then
       * move the brace after the newline and add another newline after
       * the close brace, unless we're keeping a one-liner.
       */
      Chunk *tmp = vbr->GetNext();

      if (  tmp->IsComment()
         && (  !vbr->TestFlags(PCF_ONE_LINER)
            || !options::nl_if_leave_one_liners()))
      {
         tmp = tmp->GetNext();

         if (tmp->IsNewline())
         {
            vbr->MoveAfter(tmp);
            newline_add_after(vbr);
         }
      }
   }
} // convert_vbrace


static void convert_vbrace_to_brace()
{
   LOG_FUNC_ENTRY();

   // Find every vbrace open
   log_rule_B("mod_full_brace_if");
   log_rule_B("mod_full_brace_if_chain");
   log_rule_B("mod_full_brace_for");
   log_rule_B("mod_full_brace_do");
   log_rule_B("mod_full_brace_while");
   log_rule_B("mod_full_brace_using");
   log_rule_B("mod_full_brace_function");

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (pc->IsNot(CT_VBRACE_OPEN))
      {
         continue;
      }
      auto const in_preproc = pc->TestFlags(PCF_IN_PREPROC);

      if (  (  (  pc->GetParentType() == CT_IF
               || pc->GetParentType() == CT_ELSE
               || pc->GetParentType() == CT_ELSEIF)
            && (options::mod_full_brace_if() & IARF_ADD)
            && !options::mod_full_brace_if_chain())
         || (  pc->GetParentType() == CT_FOR
            && (options::mod_full_brace_for() & IARF_ADD))
         || (  pc->GetParentType() == CT_DO
            && (options::mod_full_brace_do() & IARF_ADD))
         || (  pc->GetParentType() == CT_WHILE
            && (options::mod_full_brace_while() & IARF_ADD))
         || (  pc->GetParentType() == CT_USING_STMT
            && (options::mod_full_brace_using() & IARF_ADD))
         || (  pc->GetParentType() == CT_FUNC_DEF
            && (options::mod_full_brace_function() & IARF_ADD)))
      {
         // Find the matching vbrace close
         Chunk *vbc = Chunk::NullChunkPtr;
         Chunk *tmp = pc->GetNext();

         while (tmp->IsNotNullChunk())
         {
            if (  in_preproc
               && !tmp->TestFlags(PCF_IN_PREPROC))
            {
               // Can't leave a preprocessor
               break;
            }

            if (  pc->GetBraceLevel() == tmp->GetBraceLevel()
               && tmp->Is(CT_VBRACE_CLOSE)
               && pc->GetParentType() == tmp->GetParentType()
               && ((tmp->GetFlags() & PCF_IN_PREPROC) == (pc->GetFlags() & PCF_IN_PREPROC)))
            {
               vbc = tmp;
               break;
            }
            tmp = tmp->GetNext();
         }

         if (vbc->IsNullChunk())
         {
            continue;
         }
         // if we found a corresponding virtual closing brace
         convert_vbrace(pc);   // convert both the opening
         convert_vbrace(vbc);  // and closing brace
      }
   }
} // convert_vbrace_to_brace


Chunk *insert_comment_after(Chunk *ref, E_Token cmt_type,
                            const UncText &cmt_text)
{
   LOG_FUNC_ENTRY();

   Chunk new_cmt = *ref;

   new_cmt.SetFlags(ref->GetFlags() & PCF_COPY_FLAGS);
   new_cmt.SetType(cmt_type);
   new_cmt.Str().clear();

   if (cmt_type == CT_COMMENT_CPP)
   {
      new_cmt.Str().append("// ");
      new_cmt.Str().append(cmt_text);
   }
   else
   {
      if (ref->Is(CT_PP_ELSE))
      {  // make test c/ 02501 stable
         new_cmt.Str().append(" ");
      }
      new_cmt.Str().append("/* ");
      new_cmt.Str().append(cmt_text);
      new_cmt.Str().append(" */");
   }
   // TODO: expand comment type to cover other comment styles?

   new_cmt.SetColumn(ref->GetColumn() + ref->Len() + 1);
   new_cmt.SetOrigCol(new_cmt.GetColumn());

   return(new_cmt.CopyAndAddAfter(ref));
}


static void append_tag_name(UncText &txt, Chunk *pc)
{
   LOG_FUNC_ENTRY();
   Chunk *tmp = pc->GetPrevNcNnl();

   LOG_FMT(LMCB, "%s(%d): txt is '%s'\n",
           __func__, __LINE__, txt.c_str());

   // step backwards over all a::b stuff
   while (tmp->IsNotNullChunk())
   {
      if (  tmp->IsNot(CT_DC_MEMBER)
         && tmp->IsNot(CT_MEMBER))
      {
         break;
      }
      tmp = tmp->GetPrevNcNnl();
      pc  = tmp;

      if (!tmp->IsWord())
      {
         break;
      }
   }
   txt += pc->GetStr();
   LOG_FMT(LMCB, "%s(%d): txt is '%s'\n",
           __func__, __LINE__, txt.c_str());

   pc = pc->GetNextNcNnl();

   while (pc->IsNotNullChunk())
   {
      if (  pc->IsNot(CT_DC_MEMBER)
         && pc->IsNot(CT_MEMBER))
      {
         break;
      }
      txt += pc->GetStr();
      LOG_FMT(LMCB, "%s(%d): txt is '%s'\n",
              __func__, __LINE__, txt.c_str());
      pc = pc->GetNextNcNnl();

      if (pc->IsNotNullChunk())
      {
         txt += pc->GetStr();
         LOG_FMT(LMCB, "%s(%d): txt is '%s'\n",
                 __func__, __LINE__, txt.c_str());
      }
      pc = pc->GetNextNcNnl();
   }
} // append_tag_name


void add_long_closebrace_comment()
{
   LOG_FUNC_ENTRY();
   Chunk *fcn_pc = Chunk::NullChunkPtr;
   Chunk *sw_pc  = Chunk::NullChunkPtr;
   Chunk *ns_pc  = Chunk::NullChunkPtr;
   Chunk *cl_pc  = Chunk::NullChunkPtr;

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (  pc->Is(CT_FUNC_DEF)
         || pc->Is(CT_OC_MSG_DECL))
      {
         fcn_pc = pc;
      }
      else if (pc->Is(CT_SWITCH))
      {
         // pointless, since it always has the text "switch"
         sw_pc = pc;
      }
      else if (pc->Is(CT_NAMESPACE))
      {
         ns_pc = pc;
      }
      else if (pc->Is(CT_CLASS))
      {
         cl_pc = pc;
      }

      if (  pc->IsNot(CT_BRACE_OPEN)
         || pc->TestFlags(PCF_IN_PREPROC))
      {
         continue;
      }
      Chunk  *br_open = pc;
      size_t nl_count = 0;

      Chunk  *tmp = pc;

      while ((tmp = tmp->GetNext(E_Scope::PREPROC))->IsNotNullChunk())
      {
         if (tmp->IsNewline())
         {
            nl_count += tmp->GetNlCount();
            continue;
         }

         // handle only matching closing braces, skip other chunks
         if (  tmp->GetLevel() != br_open->GetLevel()
            || tmp->IsNot(CT_BRACE_CLOSE))
         {
            continue;
         }
         Chunk *br_close = tmp;

         tmp = tmp->GetNext();

         // check for a possible end semicolon
         if (tmp->Is(CT_SEMICOLON))
         {
            // set br_close to the semi token,
            // as br_close is used to add the comment after it
            br_close = tmp;
            tmp      = tmp->GetNext();
         }

         // make sure a newline follows in order to not overwrite an already
         // existring comment
         if (  tmp->IsNotNullChunk()
            && !tmp->IsNewline())
         {
            break;
         }
         size_t  nl_min  = 0;
         Chunk   *tag_pc = Chunk::NullChunkPtr;
         UncText xstr;

         if (  br_open->GetParentType() == CT_FUNC_DEF
            || br_open->GetParentType() == CT_OC_MSG_DECL)
         {
            log_rule_B("mod_add_long_function_closebrace_comment");
            nl_min = options::mod_add_long_function_closebrace_comment();
            tag_pc = fcn_pc;

            if (tag_pc->IsNotNullChunk())
            {
               append_tag_name(xstr, tag_pc);
               LOG_FMT(LMCB, "%s(%d): xstr is '%s'\n",
                       __func__, __LINE__, xstr.c_str());
            }
         }
         else if (  br_open->GetParentType() == CT_SWITCH
                 && sw_pc->IsNotNullChunk())
         {
            log_rule_B("mod_add_long_switch_closebrace_comment");
            nl_min = options::mod_add_long_switch_closebrace_comment();
            tag_pc = sw_pc;
            xstr   = sw_pc->GetStr();
            LOG_FMT(LMCB, "%s(%d): xstr is '%s'\n",
                    __func__, __LINE__, xstr.c_str());
         }
         else if (  br_open->GetParentType() == CT_NAMESPACE
                 && ns_pc->IsNotNullChunk())
         {
            log_rule_B("mod_add_long_namespace_closebrace_comment");
            nl_min = options::mod_add_long_namespace_closebrace_comment();
            tag_pc = ns_pc;
            xstr   = tag_pc->GetStr();    // add 'namespace' to the string
            LOG_FMT(LMCB, "%s(%d): xstr is '%s'\n",
                    __func__, __LINE__, xstr.c_str());

            // next chunk, normally is going to be the namespace name
            // append it with a space to generate "namespace xyz"
            Chunk *tmp_next = tag_pc->GetNextNcNnl();

            if (tmp_next->IsNot(CT_BRACE_OPEN)) // anonymous namespace -> ignore
            {
               xstr.append(" ");
               LOG_FMT(LMCB, "%s(%d): xstr is '%s'\n",
                       __func__, __LINE__, xstr.c_str());
               append_tag_name(xstr, tmp_next);
               LOG_FMT(LMCB, "%s(%d): xstr is '%s'\n",
                       __func__, __LINE__, xstr.c_str());
            }
         }
         else if (  br_open->GetParentType() == CT_CLASS
                 && cl_pc->IsNotNullChunk()
                 && (  !language_is_set(lang_flag_e::LANG_CPP) // proceed if not C++
                    || br_close->Is(CT_SEMICOLON)))            // else a C++ class needs to end with a semicolon
         {
            log_rule_B("mod_add_long_class_closebrace_comment");
            nl_min = options::mod_add_long_class_closebrace_comment();
            tag_pc = cl_pc;
            xstr   = tag_pc->GetStr();
            LOG_FMT(LMCB, "%s(%d): xstr is '%s'\n",
                    __func__, __LINE__, xstr.c_str());

            Chunk *tmp_next = cl_pc->GetNext();

            if (tag_pc->IsNotNullChunk())
            {
               xstr.append(" ");
               LOG_FMT(LMCB, "%s(%d): xstr is '%s'\n",
                       __func__, __LINE__, xstr.c_str());
               append_tag_name(xstr, tmp_next);
               LOG_FMT(LMCB, "%s(%d): xstr is '%s'\n",
                       __func__, __LINE__, xstr.c_str());
            }
         }

         if (  nl_min > 0
            && nl_count >= nl_min
            && tag_pc->IsNotNullChunk())
         {
            E_Token style;

            if (options::mod_add_force_c_closebrace_comment())
            {
               // force the C comment style
               style = CT_COMMENT;
            }
            else
            {
               // use the comment style that fits to the selected language
               style = (  language_is_set(lang_flag_e::LANG_CPP)
                       || language_is_set(lang_flag_e::LANG_CS))
                          ? CT_COMMENT_CPP : CT_COMMENT;
            }
            // Add a comment after the close brace
            LOG_FMT(LMCB, "%s(%d): xstr is '%s'\n",
                    __func__, __LINE__, xstr.c_str());
            insert_comment_after(br_close, style, xstr);
         }
         break;
      }
   }
} // add_long_closebrace_comment


static void move_case_break()
{
   LOG_FUNC_ENTRY();
   Chunk *prev = Chunk::NullChunkPtr;

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (  pc->Is(CT_BREAK)
         && prev->Is(CT_BRACE_CLOSE)
         && prev->GetParentType() == CT_CASE
         && pc->GetPrev()->IsNewline()
         && prev->GetPrev()->IsNewline())
      {
         prev->SwapLines(pc);
      }
      prev = pc;
   }
}


static void move_case_return()
{
   LOG_FUNC_ENTRY();
   Chunk *prev = Chunk::NullChunkPtr;

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (  pc->Is(CT_RETURN)
         && prev->Is(CT_BRACE_CLOSE)
         && prev->GetParentType() == CT_CASE
         && pc->GetPrev()->IsNewline()
         && prev->GetPrev()->IsNewline())
      {
         // Find the end of the return statement
         while (pc->IsNot(CT_SEMICOLON))
         {
            if (  pc->Is(CT_CASE)
               || pc->Is(CT_BRACE_CLOSE))
            {
               // This may indicate a semicolon was missing in the code to format.
               // Avoid moving the return statement to prevent potential unwanted errors.
               pc = Chunk::NullChunkPtr;
               break;
            }
            pc = pc->GetNext();
         }
         pc = pc->GetNextNl();
         pc = pc->GetNextNcNnl();

         if (pc->IsNotNullChunk())
         {
            // Swap all lines between brace close and current token
            LOG_FMT(LMCB, "%s(%d): move line %zu before line %zu\n",
                    __func__, __LINE__, prev->GetOrigLine(), pc->GetOrigLine());
            Chunk *curr = prev->GetNextNcNnl();

            while (curr != pc)
            {
               prev->SwapLines(curr);
               curr = prev->GetNextNcNnl();
            }
         }
      }
      prev = pc;
   }
} // move_case_return


static Chunk *mod_case_brace_remove(Chunk *br_open)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LMCB, "%s(%d): line %zu",
           __func__, __LINE__, br_open->GetOrigLine());

   // Find the matching brace close
   Chunk *next     = br_open->GetNextNcNnl(E_Scope::PREPROC);
   Chunk *br_close = br_open->GetNextType(CT_BRACE_CLOSE, br_open->GetLevel(), E_Scope::PREPROC);

   if (br_close->IsNullChunk())
   {
      LOG_FMT(LMCB, "%s(%d):  - no close\n", __func__, __LINE__);
      return(next);
   }
   // Make sure 'break', 'return', 'goto', 'case' or '}' is after the close brace
   Chunk *pc = br_close->GetNextNcNnl(E_Scope::PREPROC);

   if (  pc->IsNullChunk()
      || (  pc->IsNot(CT_BREAK)
         && pc->IsNot(CT_RETURN)
         && pc->IsNot(CT_CASE)
         && pc->IsNot(CT_GOTO)
         && pc->IsNot(CT_BRACE_CLOSE)))
   {
      LOG_FMT(LMCB, "%s(%d):  - after '%s'\n",
              __func__, __LINE__, (pc->IsNullChunk()) ? "null chuck" : get_token_name(pc->GetType()));
      return(next);
   }

   // scan to make sure there are no definitions at brace level between braces
   for (Chunk *tmp_pc = br_open;
        tmp_pc != br_close;
        tmp_pc = tmp_pc->GetNextNcNnl(E_Scope::PREPROC))
   {
      if (  tmp_pc->GetLevel() == (br_open->GetLevel() + 1)
         && tmp_pc->TestFlags(PCF_VAR_DEF))
      {
         LOG_FMT(LMCB, "%s(%d):  - vardef on line %zu: '%s'\n",
                 __func__, __LINE__, tmp_pc->GetOrigLine(), pc->Text());
         return(next);
      }
   }

   LOG_FMT(LMCB, "%s(%d):  - removing braces on lines %zu and %zu\n",
           __func__, __LINE__, br_open->GetOrigLine(), br_close->GetOrigLine());

   for (Chunk *tmp_pc = br_open;
        tmp_pc != br_close;
        tmp_pc = tmp_pc->GetNextNcNnl(E_Scope::PREPROC))
   {
      if (tmp_pc->GetBraceLevel() == 0)
      {
         fprintf(stderr, "%s(%d): brace level is ZERO, cannot be decremented, at line %zu, column %zu\n",
                 __func__, __LINE__, tmp_pc->GetOrigLine(), tmp_pc->GetOrigCol());
         log_flush(true);
         exit(EX_SOFTWARE);
      }
      tmp_pc->SetBraceLevel(tmp_pc->GetBraceLevel() - 1);

      if (tmp_pc->GetLevel() == 0)
      {
         fprintf(stderr, "%s(%d): tmp_pc->GetLevel() is ZERO, cannot be decremented, at line %zu, column %zu\n",
                 __func__, __LINE__, tmp_pc->GetOrigLine(), tmp_pc->GetOrigCol());
         log_flush(true);
         exit(EX_SOFTWARE);
      }
      tmp_pc->SetLevel(tmp_pc->GetLevel() - 1);
   }

   next = br_open->GetPrev(E_Scope::PREPROC);

   Chunk::Delete(br_open);
   Chunk::Delete(br_close);

   return(next->GetNext(E_Scope::PREPROC));
} // mod_case_brace_remove


static Chunk *mod_case_brace_add(Chunk *cl_colon)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LMCB, "%s(%d): orig line %zu, orig col is %zu\n",
           __func__, __LINE__, cl_colon->GetOrigLine(), cl_colon->GetOrigCol());

   Chunk *pc   = cl_colon;
   Chunk *last = Chunk::NullChunkPtr;
   // look for the case token to the colon
   Chunk *cas_ = cl_colon->GetPrevType(CT_CASE, cl_colon->GetLevel());
   // look for the parent
   Chunk *swit = cas_->GetParent();
   // look for the opening brace of the switch
   Chunk *open = swit->GetNextType(CT_BRACE_OPEN, swit->GetLevel());
   // look for the closing brace of the switch
   Chunk *clos = open->GetClosingParen();

   // find the end of the case-block
   pc = pc->GetNextNcNnl(E_Scope::PREPROC);

   while (pc->IsNotNullChunk())
   {
      LOG_FMT(LMCB, "%s(%d): Text() is '%s', orig line %zu, orig col is %zu, pp level is %zu\n",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol(), pc->GetPpLevel());

      if (pc->GetLevel() == cl_colon->GetLevel())
      {
         if (pc->Is(CT_CASE))
         {
            LOG_FMT(LMCB, "%s(%d): Text() is '%s', orig line %zu, orig col is %zu\n",
                    __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
            last = calculate_closing_brace_position(cl_colon, pc);
            break;
         }
      }
      else if (pc->GetLevel() == cl_colon->GetLevel() - 1)
      {
         if (pc == clos)
         {
            LOG_FMT(LMCB, "%s(%d): Text() is '%s', orig line %zu, orig col is %zu\n",
                    __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
            // end of switch is reached
            last = calculate_closing_brace_position(cl_colon, pc);
            LOG_FMT(LMCB, "%s(%d): last->Text() is '%s', orig line %zu, orig col is %zu\n",
                    __func__, __LINE__, last->Text(), last->GetOrigLine(), last->GetOrigCol());
            break;
         }
      }
      pc = pc->GetNextNcNnl(E_Scope::PREPROC);
   }

   if (last->IsNullChunk())
   {
      LOG_FMT(LMCB, "%s(%d):  - last is null chunk\n", __func__, __LINE__);
      Chunk *next = cl_colon->GetNextNcNnl(E_Scope::PREPROC);
      return(next);
   }
   LOG_FMT(LMCB, "%s(%d): last->Text() is '%s', orig line %zu, orig col is %zu\n",
           __func__, __LINE__, last->Text(), last->GetOrigLine(), last->GetOrigCol());
   LOG_FMT(LMCB, "%s(%d): adding braces after '%s' on line %zu\n",
           __func__, __LINE__, cl_colon->Text(), cl_colon->GetOrigLine());

   Chunk chunk;

   chunk.SetType(CT_BRACE_OPEN);
   chunk.SetParentType(CT_CASE);
   chunk.SetOrigLine(cl_colon->GetOrigLine());
   chunk.SetOrigCol(cl_colon->GetOrigCol());
   chunk.SetLevel(cl_colon->GetLevel());
   chunk.SetPpLevel(cl_colon->GetPpLevel());
   chunk.SetBraceLevel(cl_colon->GetBraceLevel());
   chunk.SetFlags(pc->GetFlags() & PCF_COPY_FLAGS);
   chunk.Str() = "{";
   Chunk *br_open = chunk.CopyAndAddAfter(cl_colon);

   chunk.SetType(CT_BRACE_CLOSE);
   chunk.SetOrigLine(last->GetOrigLine());
   chunk.SetOrigCol(last->GetOrigCol());
   chunk.Str() = "}";
   Chunk *br_close = chunk.CopyAndAddAfter(last);

   for (pc = br_open->GetNext(E_Scope::PREPROC);
        pc != br_close;
        pc = pc->GetNext(E_Scope::PREPROC))
   {
      pc->SetLevel(pc->GetLevel() + 1);
      pc->SetBraceLevel(pc->GetBraceLevel() + 1);
   }

   return(br_open);
} // mod_case_brace_add


static void mod_case_brace()
{
   LOG_FUNC_ENTRY();

   Chunk *pc = Chunk::GetHead();

   // Make sure to start outside of a preprocessor line (see issue #3366)
   if (pc->IsPreproc())
   {
      pc = pc->GetNextNcNnlNpp();
   }

   while (pc->IsNotNullChunk())
   {
      Chunk *next = pc->GetNextNcNnl(E_Scope::PREPROC);

      if (next->IsNullChunk())
      {
         return;
      }
      log_rule_B("mod_case_brace");

      if (  options::mod_case_brace() == IARF_REMOVE
         && pc->Is(CT_BRACE_OPEN)
         && pc->GetParentType() == CT_CASE)
      {
         log_rule_B("mod_case_brace - add");
         pc = mod_case_brace_remove(pc);
      }
      else if (  (options::mod_case_brace() & IARF_ADD)
              && pc->Is(CT_CASE_COLON)
              && next->IsNot(CT_BRACE_OPEN)
              && next->IsNot(CT_BRACE_CLOSE)
              && next->IsNot(CT_CASE))
      {
         log_rule_B("mod_case_brace - remove");
         pc = mod_case_brace_add(pc);
      }
      else
      {
         pc = pc->GetNextNcNnl(E_Scope::PREPROC);
      }
   }
} // mod_case_brace


static void process_if_chain(Chunk *br_start)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LBRCH, "%s(%d): if starts on line %zu, orig col is %zu.\n",
           __func__, __LINE__, br_start->GetOrigLine(), br_start->GetOrigCol());

   vector<Chunk *> braces;

   braces.reserve(16);

   bool  must_have_braces   = false;
   bool  has_unbraced_block = false;

   Chunk *pc = br_start;

   while (pc->IsNotNullChunk())
   {
      LOG_CHUNK(LTOK, pc);

      if (pc->Is(CT_BRACE_OPEN))
      {
         const bool tmp = can_remove_braces(pc);
         LOG_FMT(LBRCH, "%s(%d): braces.size() is %zu, line is %zu, - can%s remove %s\n",
                 __func__, __LINE__, braces.size(), pc->GetOrigLine(), tmp ? "" : "not",
                 get_token_name(pc->GetType()));

         if (  !tmp
            || options::mod_full_brace_if_chain() == 2)
         {
            must_have_braces = true;
         }
      }
      else
      {
         const bool tmp = should_add_braces(pc);

         if (tmp)
         {
            must_have_braces = true;
         }
         LOG_FMT(LBRCH, "%s(%d): braces.size() is %zu, line is %zu, - %s %s\n",
                 __func__, __LINE__, braces.size(), pc->GetOrigLine(), tmp ? "should add" : "ignore",
                 get_token_name(pc->GetType()));

         has_unbraced_block = true;
      }

      if (  options::mod_full_brace_if_chain() == 3
         && !has_unbraced_block)
      {
         must_have_braces = true;
      }
      braces.push_back(pc);
      Chunk *br_close = pc->GetClosingParen(E_Scope::PREPROC);

      if (br_close->IsNullChunk())
      {
         break;
      }
      braces.push_back(br_close);

      pc = br_close->GetNextNcNnl(E_Scope::PREPROC);

      if (  pc->IsNullChunk()
         || pc->IsNot(CT_ELSE))
      {
         break;
      }
      log_rule_B("mod_full_brace_if_chain_only");

      if (options::mod_full_brace_if_chain_only())
      {
         // There is an 'else' - we want full braces.
         must_have_braces = true;
      }
      pc = pc->GetNextNcNnl(E_Scope::PREPROC);

      if (pc->Is(CT_ELSEIF))
      {
         while (  pc->IsNot(CT_VBRACE_OPEN)
               && pc->IsNot(CT_BRACE_OPEN))
         {
            pc = pc->GetNextNcNnl(E_Scope::PREPROC);
         }
      }

      if (pc->IsNullChunk())
      {
         break;
      }

      if (  pc->IsNot(CT_BRACE_OPEN)
         && pc->IsNot(CT_VBRACE_OPEN))
      {
         break;
      }
   }

   if (must_have_braces)
   {
      LOG_FMT(LBRCH, "%s(%d): add braces on lines[%zu]:",
              __func__, __LINE__, braces.size());

      const auto ite = braces.rend();

      for (auto itc = braces.rbegin(); itc != ite; ++itc)
      {
         const auto brace = *itc;

         brace->SetFlagBits(PCF_KEEP_BRACE);

         if (brace->IsVBrace())
         {
            LOG_FMT(LBRCH, "%s(%d):  %zu",
                    __func__, __LINE__, brace->GetOrigLine());
            convert_vbrace(brace);
         }
         else
         {
            LOG_FMT(LBRCH, "%s(%d):  {%zu}",
                    __func__, __LINE__, brace->GetOrigLine());
         }
      }

      LOG_FMT(LBRCH, "\n");
   }
   else if (options::mod_full_brace_if_chain())
   {
      log_rule_B("mod_full_brace_if_chain");
      LOG_FMT(LBRCH, "%s(%d): remove braces on lines[%zu]:\n",
              __func__, __LINE__, braces.size());

      /*
       * This might run because either
       * mod_full_brace_if_chain or mod_full_brace_if_chain_only
       * is used.
       * We only want to remove braces if the first one is active.
       */
      log_rule_B("mod_full_brace_nl_block_rem_mlcond");
      const auto multiline_block = options::mod_full_brace_nl_block_rem_mlcond();

      LOG_FMT(LBRCH, "%s(%d): remove braces on lines:\n", __func__, __LINE__);

      // Issue #2229
      const auto ite = braces.end();

      for (auto itc = braces.begin(); itc != ite; ++itc)
      {
         const auto brace = *itc;

         if (  (  brace->Is(CT_BRACE_OPEN)
               || brace->Is(CT_BRACE_CLOSE))
            && (brace->GetParentType() != CT_BRACED_INIT_LIST)
            && (multiline_block ? !paren_multiline_before_brace(brace) : true))
         {
            LOG_FMT(LBRCH, "%s(%d): brace orig line is %zu, orig col is %zu\n",
                    __func__, __LINE__, brace->GetOrigLine(), brace->GetOrigCol());
            convert_brace(brace);
         }
         else
         {
            LOG_FMT(LBRCH, "%s(%d): brace orig line is %zu, orig col is %zu\n",
                    __func__, __LINE__, brace->GetOrigLine(), brace->GetOrigCol());
         }
      }
   }
} // process_if_chain


static void mod_full_brace_if_chain()
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (  pc->IsBraceOpen()
         && pc->GetParentType() == CT_IF)
      {
         process_if_chain(pc);
      }
   }
}
