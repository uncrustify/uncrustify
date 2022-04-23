/**
 * @file braces.cpp
 * Adds or removes braces.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#include "braces.h"

#include "calculate_closing_brace_position.h"
#include "combine_tools.h"
#include "newlines.h"
#include "prototypes.h"

constexpr static auto LCURRENT = LBR;

using namespace uncrustify;

using std::vector;


//! Converts a single brace into a virtual brace
static void convert_brace(Chunk *br);


//! Converts a single virtual brace into a real brace
static void convert_vbrace(Chunk *br);


static void convert_vbrace_to_brace(void);


//! Go backwards to honor brace newline removal limits
static void examine_braces(void);


/**
 * Step forward and count the number of semi colons at the current level.
 * Abort if more than 1 or if we enter a preprocessor
 */
static void examine_brace(Chunk *bopen);


static void move_case_break(void);


static void move_case_return(void);


static void mod_case_brace(void);


static void mod_full_brace_if_chain(void);


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
static void append_tag_name(unc_text &txt, Chunk *pc);


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
 * @pre   the brace chunk cannot be a nullptr,
 *        it needs to be of type CT_BRACE_OPEN or CT_BRACE_CLOSE,
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
   if (  brace == nullptr
      || (  chunk_is_not_token(brace, CT_BRACE_OPEN)
         && chunk_is_not_token(brace, CT_BRACE_CLOSE))
      || (  get_chunk_parent_type(brace) != CT_IF
         && get_chunk_parent_type(brace) != CT_ELSEIF
         && get_chunk_parent_type(brace) != CT_FOR
         && get_chunk_parent_type(brace) != CT_USING_STMT
         && get_chunk_parent_type(brace) != CT_WHILE
         && get_chunk_parent_type(brace) != CT_FUNC_CLASS_DEF
         && get_chunk_parent_type(brace) != CT_FUNC_DEF))
   {
      return(false);
   }
   const auto paren_t = CT_SPAREN_CLOSE;

   // find parenthesis pair of the if/for/while/...
   auto paren_close = brace->GetPrevType(paren_t, brace->level, E_Scope::ALL);
   auto paren_open  = chunk_skip_to_match_rev(paren_close, E_Scope::ALL);

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


void do_braces(void)
{
   LOG_FUNC_ENTRY();
   // Mark one-liners
   // Issue #2232 put this at the beginning
   Chunk *pc = Chunk::GetHead()->GetNextNcNnl();

   while (pc->IsNotNullChunk())
   {
      if (  chunk_is_not_token(pc, CT_BRACE_OPEN)
         && chunk_is_not_token(pc, CT_VBRACE_OPEN))
      {
         pc = pc->GetNextNcNnl();
         continue;
      }
      Chunk         *br_open = pc;
      const E_Token brc_type = E_Token(pc->type + 1); // corresponds to closing type
      // Detect empty bodies
      Chunk         *tmp = pc->GetNextNcNnl();

      if (chunk_is_token(tmp, brc_type))
      {
         chunk_flags_set(br_open, PCF_EMPTY_BODY);
         chunk_flags_set(tmp, PCF_EMPTY_BODY);
      }
      // Scan for the brace close or a newline
      tmp = br_open->GetNextNc();

      while (tmp->IsNotNullChunk())
      {
         if (chunk_is_newline(tmp))
         {
            break;
         }

         if (  chunk_is_token(tmp, brc_type)
            && br_open->level == tmp->level)
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


static void examine_braces(void)
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
      Chunk *prev = pc->GetPrevType(CT_BRACE_OPEN, -1);

      if (  chunk_is_token(pc, CT_BRACE_OPEN)
         && !pc->flags.test(PCF_IN_PREPROC)
         && (  (  (  get_chunk_parent_type(pc) == CT_IF
                  || get_chunk_parent_type(pc) == CT_ELSE
                  || get_chunk_parent_type(pc) == CT_ELSEIF)
               && options::mod_full_brace_if() == IARF_REMOVE)
            || (  get_chunk_parent_type(pc) == CT_DO
               && options::mod_full_brace_do() == IARF_REMOVE)
            || (  get_chunk_parent_type(pc) == CT_FOR
               && options::mod_full_brace_for() == IARF_REMOVE)
            || (  get_chunk_parent_type(pc) == CT_USING_STMT
               && options::mod_full_brace_using() == IARF_REMOVE)
            || (  get_chunk_parent_type(pc) == CT_WHILE
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
           __func__, __LINE__, vbopen->orig_line);

   size_t nl_count = 0;

   Chunk  *pc = Chunk::NullChunkPtr;

   for (pc = vbopen->GetNextNc(E_Scope::PREPROC);
        (pc->IsNotNullChunk() && pc->level > vbopen->level);
        pc = pc->GetNextNc(E_Scope::PREPROC))
   {
      if (chunk_is_newline(pc))
      {
         nl_count += pc->nl_count;
      }
   }

   if (  pc->IsNotNullChunk()
      && nl_count > nl_max
      && vbopen->pp_level == pc->pp_level)
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
           __func__, __LINE__, bopen->orig_line);

   // Cannot remove braces inside a preprocessor
   if (bopen->flags.test(PCF_IN_PREPROC))
   {
      return(false);
   }
   Chunk *pc = bopen->GetNextNcNnl(E_Scope::PREPROC);

   if (chunk_is_token(pc, CT_BRACE_CLOSE))
   {
      // Can't remove empty statement
      return(false);
   }
   const size_t level = bopen->level + 1;

   log_rule_B("mod_full_brace_nl");
   const size_t nl_max = options::mod_full_brace_nl();
   Chunk        *prev  = Chunk::NullChunkPtr;

   size_t       semi_count = 0;
   bool         hit_semi   = false;
   size_t       nl_count   = 0;
   size_t       if_count   = 0;
   int          br_count   = 0;

   pc = bopen->GetNextNc(E_Scope::ALL);
   LOG_FMT(LBRDEL, "%s(%d):  - begin with token '%s', orig_line is %zu, orig_col is %zu\n",
           __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);

   while (  pc->IsNotNullChunk()
         && pc->level >= level)
   {
      LOG_FMT(LBRDEL, "%s(%d): test token '%s', orig_line is %zu, orig_col is %zu\n",
              __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);

      if (pc->flags.test(PCF_IN_PREPROC))
      {
         // Cannot remove braces that contain a preprocessor
         return(false);
      }

      if (chunk_is_newline(pc))
      {
         nl_count += pc->nl_count;

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
         if (chunk_is_token(pc, CT_BRACE_OPEN))
         {
            br_count++;
         }
         else if (chunk_is_token(pc, CT_BRACE_CLOSE))
         {
            if (br_count == 0)
            {
               fprintf(stderr, "%s(%d): br_count is ZERO, cannot be decremented, at line %zu, column %zu\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col);
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            br_count--;

            if (pc->level == level)
            {
               // mean a statement in a braces { stmt; }
               // as a statement with a semicolon { stmt; };
               ++semi_count;
               hit_semi = true;
            }
         }
         else if (  (  chunk_is_token(pc, CT_IF)
                    || chunk_is_token(pc, CT_ELSEIF))
                 && br_count == 0)
         {
            if_count++;
         }

         if (pc->level == level)
         {
            if (  semi_count > 0
               && hit_semi)
            {
               // should have bailed due to close brace level drop
               LOG_FMT(LBRDEL, "%s(%d):  no close brace\n", __func__, __LINE__);
               return(false);
            }
            LOG_FMT(LBRDEL, "%s(%d): Text() '%s', orig_line is %zu, semi_count is %zu\n",
                    __func__, __LINE__, pc->Text(), pc->orig_line, semi_count);

            if (chunk_is_token(pc, CT_ELSE))
            {
               LOG_FMT(LBRDEL, "%s(%d):  bailed on '%s' on line %zu\n",
                       __func__, __LINE__, pc->Text(), pc->orig_line);
               return(false);
            }

            if (  chunk_is_semicolon(pc)
               || chunk_is_token(pc, CT_IF)
               || chunk_is_token(pc, CT_ELSEIF)
               || chunk_is_token(pc, CT_FOR)
               || chunk_is_token(pc, CT_DO)
               || chunk_is_token(pc, CT_WHILE)
               || chunk_is_token(pc, CT_USING_STMT)
               || (  chunk_is_token(pc, CT_BRACE_OPEN)
                  && chunk_is_token(prev, CT_FPAREN_CLOSE)))
            {
               hit_semi |= chunk_is_semicolon(pc);

               if (++semi_count > 1)
               {
                  LOG_FMT(LBRDEL, "%s(%d):  bailed on %zu because of '%s' on line %zu\n",
                          __func__, __LINE__, bopen->orig_line, pc->Text(), pc->orig_line);
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

   if (  chunk_is_token(pc, CT_BRACE_CLOSE)
      && get_chunk_parent_type(pc) == CT_IF)
   {
      Chunk *next     = pc->GetNextNcNnl(E_Scope::PREPROC);
      Chunk *tmp_prev = pc->GetPrevNcNnl(E_Scope::PREPROC);

      if (  chunk_is_token(next, CT_ELSE)
         && (  chunk_is_token(tmp_prev, CT_BRACE_CLOSE)
            || chunk_is_token(tmp_prev, CT_VBRACE_CLOSE))
         && get_chunk_parent_type(tmp_prev) == CT_IF)
      {
         LOG_FMT(LBRDEL, "%s(%d):  - bailed on '%s'[%s] on line %zu due to 'if' and 'else' sequence\n",
                 __func__, __LINE__, get_token_name(pc->type), get_token_name(get_chunk_parent_type(pc)),
                 pc->orig_line);
         return(false);
      }
   }
   LOG_FMT(LBRDEL, "%s(%d):  - end on '%s' on line %zu. if_count is %zu semi_count is %zu\n",
           __func__, __LINE__, get_token_name(pc->type), pc->orig_line, if_count, semi_count);

   return(  chunk_is_token(pc, CT_BRACE_CLOSE)
         && pc->pp_level == bopen->pp_level);
} // can_remove_braces


static void examine_brace(Chunk *bopen)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LBRDEL, "%s(%d): start on orig_line %zu, bopen->level is %zu\n",
           __func__, __LINE__, bopen->orig_line, bopen->level);

   const size_t level = bopen->level + 1;

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
         && pc->level >= level)
   {
      if (chunk_is_token(pc, CT_NEWLINE))
      {
         LOG_FMT(LBRDEL, "%s(%d): orig_line is %zu, orig_col is %zu, <Newline>\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col);
      }
      else
      {
         LOG_FMT(LBRDEL, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
      }

      if (pc->flags.test(PCF_IN_PREPROC))
      {
         // Cannot remove braces that contain a preprocessor
         LOG_FMT(LBRDEL, "%s(%d):  PREPROC\n", __func__, __LINE__);
         return;
      }

      if (chunk_is_newline(pc))
      {
         nl_count += pc->nl_count;

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
         LOG_FMT(LBRDEL, "%s(%d): for pc->Text() '%s', pc->level is %zu,  bopen->level is %zu\n",
                 __func__, __LINE__, pc->Text(), pc->level, bopen->level);

         if (  chunk_is_token(pc, CT_BRACE_OPEN)
            && pc->level == bopen->level)
         {
            br_count++;
            LOG_FMT(LBRDEL, "%s(%d): br_count is now %d, pc->level is %zu,  bopen->level is %zu\n",
                    __func__, __LINE__, br_count, pc->level, bopen->level);
         }
         else if (  chunk_is_token(pc, CT_BRACE_CLOSE)
                 && pc->level == bopen->level)
         {
            if (br_count == 0)
            {
               fprintf(stderr, "%s(%d): br_count is ZERO, cannot be decremented, at line %zu, column %zu\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col);
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            br_count--;
            LOG_FMT(LBRDEL, "%s(%d): br_count is now %d, pc->level is %zu,  bopen->level is %zu\n",
                    __func__, __LINE__, br_count, pc->level, bopen->level);

            if (br_count == 0)
            {
               Chunk *next = pc->GetNextNcNnl(E_Scope::PREPROC);

               if (  next->IsNullChunk()
                  || chunk_is_not_token(next, CT_BRACE_CLOSE))
               {
                  LOG_FMT(LBRDEL, "%s(%d):  junk after close brace\n", __func__, __LINE__);
                  return;
               }
            }
         }
         else if (  (  chunk_is_token(pc, CT_IF)
                    || chunk_is_token(pc, CT_ELSEIF))
                 && br_count == 0)
         {
            if_count++;
         }
         LOG_FMT(LBRDEL, "%s(%d): pc->level is %zu, level is %zu\n",
                 __func__, __LINE__, pc->level, level);

         if (pc->level == level)
         {
            if (  semi_count > 0
               && hit_semi)
            {
               // should have bailed due to close brace level drop
               LOG_FMT(LBRDEL, "%s(%d):  no close brace\n", __func__, __LINE__);
               return;
            }
            LOG_FMT(LBRDEL, "%s(%d): Text() '%s', orig_line is %zu, semi_count is %zu\n",
                    __func__, __LINE__, pc->Text(), pc->orig_line, semi_count);

            if (chunk_is_token(pc, CT_ELSE))
            {
               LOG_FMT(LBRDEL, "%s(%d):  bailed on '%s' on line %zu\n",
                       __func__, __LINE__, pc->Text(), pc->orig_line);
               return;
            }

            if (prev->IsNotNullChunk())
            {
               LOG_FMT(LBRDEL, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s', prev->Text '%s', prev->type %s\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), prev->Text(), get_token_name(prev->type));
            }
            else
            {
               LOG_FMT(LBRDEL, "%s(%d): orig_line is %zu, orig_col is %zu, Text() '%s', prev is a null chunk\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text());
            }
            LOG_FMT(LBRDEL, "%s(%d): for pc->Text() '%s', pc->level is %zu,  bopen->level is %zu\n",
                    __func__, __LINE__, pc->Text(), pc->level, bopen->level);

            if (  chunk_is_semicolon(pc)
               || chunk_is_token(pc, CT_IF)
               || chunk_is_token(pc, CT_ELSEIF)
               || chunk_is_token(pc, CT_FOR)
               || chunk_is_token(pc, CT_DO)
               || chunk_is_token(pc, CT_WHILE)
               || chunk_is_token(pc, CT_SWITCH)
               || chunk_is_token(pc, CT_USING_STMT)
               || (  chunk_is_token(pc, CT_BRACE_OPEN)
                  && pc->level == bopen->level)) // Issue #1758
            {
               LOG_FMT(LBRDEL, "%s(%d): pc->Text() '%s', orig_line is %zu, orig_col is %zu, level is %zu\n",
                       __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col, pc->level);
               hit_semi |= chunk_is_semicolon(pc);
               semi_count++;
               LOG_FMT(LBRDEL, "%s(%d): semi_count is %zu\n",
                       __func__, __LINE__, semi_count);

               if (semi_count > 1)
               {
                  LOG_FMT(LBRDEL, "%s(%d):  bailed on %zu because of '%s' on line %zu\n",
                          __func__, __LINE__, bopen->orig_line, pc->Text(), pc->orig_line);
                  return;
               }
            }
         }
      }
      prev = pc;
      pc   = pc->GetNext();                  // Issue #1907
   }

   if (pc == nullptr)
   {
      LOG_FMT(LBRDEL, "%s(%d): pc is nullptr\n", __func__, __LINE__);
      return;
   }
   LOG_FMT(LBRDEL, "%s(%d):  - end on '%s' on line %zu. if_count is %zu, semi_count is %zu\n",
           __func__, __LINE__, get_token_name(pc->type), pc->orig_line, if_count, semi_count);

   if (chunk_is_token(pc, CT_BRACE_CLOSE))
   {
      Chunk *next = pc->GetNextNcNnl();

      if (next->IsNotNullChunk())
      {
         while (chunk_is_token(next, CT_VBRACE_CLOSE))
         {
            next = next->GetNextNcNnl();
         }

         if (next->IsNotNullChunk())
         {
            LOG_FMT(LBRDEL, "%s(%d): orig_line is %zu, orig_col is %zu, next is '%s'\n",
                    __func__, __LINE__, next->orig_line, next->orig_col, get_token_name(next->type));
         }

         if (  if_count > 0
            && (  chunk_is_token(next, CT_ELSE)
               || chunk_is_token(next, CT_ELSEIF)))
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
         LOG_FMT(LBRDEL, "%s(%d): bopen->parent_type is %s\n",
                 __func__, __LINE__, get_token_name(get_chunk_parent_type(bopen)));

         if (get_chunk_parent_type(bopen) == CT_ELSE)
         {
            Chunk *tmp_next = bopen->GetNextNcNnl();

            if (chunk_is_token(tmp_next, CT_IF))
            {
               Chunk *tmp_prev = bopen->GetPrevNcNnl();
               LOG_FMT(LBRDEL, "%s(%d):  else-if removing braces on line %zu and %zu\n",
                       __func__, __LINE__, bopen->orig_line, pc->orig_line);

               chunk_del(bopen);
               chunk_del(pc);
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
                 __func__, __LINE__, bopen->orig_line, pc->orig_line);
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

   if (  br == nullptr
      || br->flags.test(PCF_KEEP_BRACE))
   {
      return;
   }
   Chunk *tmp;

   if (chunk_is_token(br, CT_BRACE_OPEN))
   {
      set_chunk_type(br, CT_VBRACE_OPEN);
      br->str.clear();
      tmp = br->GetPrev();

      if (tmp->IsNullChunk())
      {
         return;
      }
   }
   else if (chunk_is_token(br, CT_BRACE_CLOSE))
   {
      set_chunk_type(br, CT_VBRACE_CLOSE);
      br->str.clear();
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

   if (chunk_is_newline(tmp))
   {
      if (tmp->nl_count > 1)
      {
         if (!br->flags.test(PCF_ONE_LINER)) // Issue #2232
         {
            tmp->nl_count--;
            LOG_FMT(LBRDEL, "%s(%d): tmp->nl_count is %zu\n",
                    __func__, __LINE__, tmp->nl_count);
         }
      }
      else
      {
         // Issue #2219
         // look for opening brace
         Chunk *brace = Chunk::NullChunkPtr;

         if (chunk_is_token(br, CT_VBRACE_OPEN))
         {
            brace = tmp;
         }
         else if (chunk_is_token(br, CT_VBRACE_CLOSE))
         {
            brace = chunk_skip_to_match_rev(br);

            if (brace->IsNullChunk())
            {
               brace = br->GetPrevType(CT_BRACE_OPEN, br->level);
            }
         }

         if (  chunk_is_token(br, CT_VBRACE_OPEN)
            || (  chunk_is_token(br, CT_VBRACE_CLOSE)
               && brace->orig_line < tmp->orig_line))
         {
            if (chunk_safe_to_del_nl(tmp))
            {
               chunk_del(tmp);
            }
         }
      }
   }
} // convert_brace


static void convert_vbrace(Chunk *vbr)
{
   LOG_FUNC_ENTRY();

   if (vbr == nullptr)
   {
      return;
   }

   if (chunk_is_token(vbr, CT_VBRACE_OPEN))
   {
      set_chunk_type(vbr, CT_BRACE_OPEN);
      vbr->str = "{";

      /*
       * If the next chunk is a preprocessor, then move the open brace after the
       * preprocessor.
       */
      Chunk *tmp = vbr->GetNext();

      if (chunk_is_token(tmp, CT_PREPROC))
      {
         tmp = vbr->GetNext(E_Scope::PREPROC);
         chunk_move_after(vbr, tmp);
         newline_add_after(vbr);
      }
   }
   else if (chunk_is_token(vbr, CT_VBRACE_CLOSE))
   {
      set_chunk_type(vbr, CT_BRACE_CLOSE);
      vbr->str = "}";

      /*
       * If the next chunk is a comment, followed by a newline, then
       * move the brace after the newline and add another newline after
       * the close brace.
       */
      Chunk *tmp = vbr->GetNext();

      if (tmp->IsComment())
      {
         tmp = tmp->GetNext();

         if (chunk_is_newline(tmp))
         {
            chunk_move_after(vbr, tmp);
            newline_add_after(vbr);
         }
      }
   }
} // convert_vbrace


static void convert_vbrace_to_brace(void)
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

   for (Chunk *pc = Chunk::GetHead(); pc != nullptr && pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (chunk_is_not_token(pc, CT_VBRACE_OPEN))
      {
         continue;
      }
      auto const in_preproc = pc->flags.test(PCF_IN_PREPROC);

      if (  (  (  get_chunk_parent_type(pc) == CT_IF
               || get_chunk_parent_type(pc) == CT_ELSE
               || get_chunk_parent_type(pc) == CT_ELSEIF)
            && (options::mod_full_brace_if() & IARF_ADD)
            && !options::mod_full_brace_if_chain())
         || (  get_chunk_parent_type(pc) == CT_FOR
            && (options::mod_full_brace_for() & IARF_ADD))
         || (  get_chunk_parent_type(pc) == CT_DO
            && (options::mod_full_brace_do() & IARF_ADD))
         || (  get_chunk_parent_type(pc) == CT_WHILE
            && (options::mod_full_brace_while() & IARF_ADD))
         || (  get_chunk_parent_type(pc) == CT_USING_STMT
            && (options::mod_full_brace_using() & IARF_ADD))
         || (  get_chunk_parent_type(pc) == CT_FUNC_DEF
            && (options::mod_full_brace_function() & IARF_ADD)))
      {
         // Find the matching vbrace close
         Chunk *vbc = Chunk::NullChunkPtr;
         Chunk *tmp = pc->GetNext();

         while (tmp->IsNotNullChunk())
         {
            if (  in_preproc
               && !tmp->flags.test(PCF_IN_PREPROC))
            {
               // Can't leave a preprocessor
               break;
            }

            if (  pc->brace_level == tmp->brace_level
               && chunk_is_token(tmp, CT_VBRACE_CLOSE)
               && get_chunk_parent_type(pc) == get_chunk_parent_type(tmp)
               && ((tmp->flags & PCF_IN_PREPROC) == (pc->flags & PCF_IN_PREPROC)))
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
                            const unc_text &cmt_text)
{
   LOG_FUNC_ENTRY();

   Chunk new_cmt = *ref;

   new_cmt.prev  = nullptr;
   new_cmt.next  = nullptr;
   new_cmt.flags = (ref->flags & PCF_COPY_FLAGS);
   set_chunk_type(&new_cmt, cmt_type);
   new_cmt.str.clear();

   if (cmt_type == CT_COMMENT_CPP)
   {
      new_cmt.str.append("// ");
      new_cmt.str.append(cmt_text);
   }
   else
   {
      if (chunk_is_token(ref, CT_PP_ELSE))
      {  // make test c/ 02501 stable
         new_cmt.str.append(" ");
      }
      new_cmt.str.append("/* ");
      new_cmt.str.append(cmt_text);
      new_cmt.str.append(" */");
   }
   // TODO: expand comment type to cover other comment styles?

   new_cmt.column   = ref->column + ref->Len() + 1;
   new_cmt.orig_col = new_cmt.column;

   return(chunk_add_after(&new_cmt, ref));
}


static void append_tag_name(unc_text &txt, Chunk *pc)
{
   LOG_FUNC_ENTRY();
   Chunk *tmp = pc->GetPrevNcNnl();

   LOG_FMT(LMCB, "%s(%d): txt is '%s'\n",
           __func__, __LINE__, txt.c_str());

   // step backwards over all a::b stuff
   while (tmp->IsNotNullChunk())
   {
      if (  chunk_is_not_token(tmp, CT_DC_MEMBER)
         && chunk_is_not_token(tmp, CT_MEMBER))
      {
         break;
      }
      tmp = tmp->GetPrevNcNnl();
      pc  = tmp;

      if (!chunk_is_word(tmp))
      {
         break;
      }
   }
   txt += pc->str;
   LOG_FMT(LMCB, "%s(%d): txt is '%s'\n",
           __func__, __LINE__, txt.c_str());

   pc = pc->GetNextNcNnl();

   while (pc->IsNotNullChunk())
   {
      if (  chunk_is_not_token(pc, CT_DC_MEMBER)
         && chunk_is_not_token(pc, CT_MEMBER))
      {
         break;
      }
      txt += pc->str;
      LOG_FMT(LMCB, "%s(%d): txt is '%s'\n",
              __func__, __LINE__, txt.c_str());
      pc = pc->GetNextNcNnl();

      if (pc->IsNotNullChunk())
      {
         txt += pc->str;
         LOG_FMT(LMCB, "%s(%d): txt is '%s'\n",
                 __func__, __LINE__, txt.c_str());
      }
      pc = pc->GetNextNcNnl();
   }
} // append_tag_name


void add_long_closebrace_comment(void)
{
   LOG_FUNC_ENTRY();
   Chunk *fcn_pc = Chunk::NullChunkPtr;
   Chunk *sw_pc  = Chunk::NullChunkPtr;
   Chunk *ns_pc  = Chunk::NullChunkPtr;
   Chunk *cl_pc  = Chunk::NullChunkPtr;

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (  chunk_is_token(pc, CT_FUNC_DEF)
         || chunk_is_token(pc, CT_OC_MSG_DECL))
      {
         fcn_pc = pc;
      }
      else if (chunk_is_token(pc, CT_SWITCH))
      {
         // pointless, since it always has the text "switch"
         sw_pc = pc;
      }
      else if (chunk_is_token(pc, CT_NAMESPACE))
      {
         ns_pc = pc;
      }
      else if (chunk_is_token(pc, CT_CLASS))
      {
         cl_pc = pc;
      }

      if (  chunk_is_not_token(pc, CT_BRACE_OPEN)
         || pc->flags.test(PCF_IN_PREPROC))
      {
         continue;
      }
      Chunk  *br_open = pc;
      size_t nl_count = 0;

      Chunk  *tmp = pc;

      while ((tmp = tmp->GetNext(E_Scope::PREPROC))->IsNotNullChunk())
      {
         if (chunk_is_newline(tmp))
         {
            nl_count += tmp->nl_count;
            continue;
         }

         // handle only matching closing braces, skip other chunks
         if (  tmp->level != br_open->level
            || chunk_is_not_token(tmp, CT_BRACE_CLOSE))
         {
            continue;
         }
         Chunk *br_close = tmp;

         tmp = tmp->GetNext();

         // check for a possible end semicolon
         if (chunk_is_token(tmp, CT_SEMICOLON))
         {
            // set br_close to the semi token,
            // as br_close is used to add the coment after it
            br_close = tmp;
            tmp      = tmp->GetNext();
         }

         // make sure a newline follows in order to not overwrite an already
         // existring comment
         if (  tmp->IsNotNullChunk()
            && !chunk_is_newline(tmp))
         {
            break;
         }
         size_t   nl_min  = 0;
         Chunk    *tag_pc = Chunk::NullChunkPtr;
         unc_text xstr;

         if (  get_chunk_parent_type(br_open) == CT_FUNC_DEF
            || get_chunk_parent_type(br_open) == CT_OC_MSG_DECL)
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
         else if (  get_chunk_parent_type(br_open) == CT_SWITCH
                 && sw_pc != nullptr)
         {
            log_rule_B("mod_add_long_switch_closebrace_comment");
            nl_min = options::mod_add_long_switch_closebrace_comment();
            tag_pc = sw_pc;
            xstr   = sw_pc->str;
            LOG_FMT(LMCB, "%s(%d): xstr is '%s'\n",
                    __func__, __LINE__, xstr.c_str());
         }
         else if (  get_chunk_parent_type(br_open) == CT_NAMESPACE
                 && ns_pc != nullptr)
         {
            log_rule_B("mod_add_long_namespace_closebrace_comment");
            nl_min = options::mod_add_long_namespace_closebrace_comment();
            tag_pc = ns_pc;
            xstr   = tag_pc->str;    // add 'namespace' to the string
            LOG_FMT(LMCB, "%s(%d): xstr is '%s'\n",
                    __func__, __LINE__, xstr.c_str());

            // next chunk, normally is going to be the namespace name
            // append it with a space to generate "namespace xyz"
            Chunk *tmp_next = tag_pc->GetNextNcNnl();

            if (chunk_is_not_token(tmp_next, CT_BRACE_OPEN)) // anonymous namespace -> ignore
            {
               xstr.append(" ");
               LOG_FMT(LMCB, "%s(%d): xstr is '%s'\n",
                       __func__, __LINE__, xstr.c_str());
               append_tag_name(xstr, tmp_next);
               LOG_FMT(LMCB, "%s(%d): xstr is '%s'\n",
                       __func__, __LINE__, xstr.c_str());
            }
         }
         else if (  get_chunk_parent_type(br_open) == CT_CLASS
                 && cl_pc->IsNotNullChunk()
                 && (  !language_is_set(LANG_CPP)               // proceed if not C++
                    || chunk_is_token(br_close, CT_SEMICOLON))) // else a C++ class needs to end with a semicolon
         {
            log_rule_B("mod_add_long_class_closebrace_comment");
            nl_min = options::mod_add_long_class_closebrace_comment();
            tag_pc = cl_pc;
            xstr   = tag_pc->str;
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
            // use the comment style that fits to the selected language
            const E_Token style = language_is_set(LANG_CPP | LANG_CS)
                                    ? CT_COMMENT_CPP : CT_COMMENT;

            // Add a comment after the close brace
            LOG_FMT(LMCB, "%s(%d): xstr is '%s'\n",
                    __func__, __LINE__, xstr.c_str());
            insert_comment_after(br_close, style, xstr);
         }
         break;
      }
   }
} // add_long_closebrace_comment


static void move_case_break(void)
{
   LOG_FUNC_ENTRY();
   Chunk *prev = Chunk::NullChunkPtr;

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (  chunk_is_token(pc, CT_BREAK)
         && chunk_is_token(prev, CT_BRACE_CLOSE)
         && get_chunk_parent_type(prev) == CT_CASE
         && chunk_is_newline(pc->GetPrev())
         && chunk_is_newline(prev->GetPrev()))
      {
         chunk_swap_lines(prev, pc);
      }
      prev = pc;
   }
}


static void move_case_return(void)
{
   LOG_FUNC_ENTRY();
   Chunk *prev = Chunk::NullChunkPtr;

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (  chunk_is_token(pc, CT_RETURN)
         && chunk_is_token(prev, CT_BRACE_CLOSE)
         && get_chunk_parent_type(prev) == CT_CASE
         && chunk_is_newline(pc->GetPrev())
         && chunk_is_newline(prev->GetPrev()))
      {
         // Find the end of the return statement
         while (chunk_is_not_token(pc, CT_SEMICOLON))
         {
            if (  chunk_is_token(pc, CT_CASE)
               || chunk_is_token(pc, CT_BRACE_CLOSE))
            {
               // This may indicate a semicolon was missing in the code to format.
               // Avoid moving the return statement to prevent potential unwanted erros.
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
                    __func__, __LINE__, prev->orig_line, pc->orig_line);
            Chunk *curr = prev->GetNextNcNnl();

            while (curr != pc)
            {
               chunk_swap_lines(prev, curr);
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
           __func__, __LINE__, br_open->orig_line);

   // Find the matching brace close
   Chunk *next     = br_open->GetNextNcNnl(E_Scope::PREPROC);
   Chunk *br_close = br_open->GetNextType(CT_BRACE_CLOSE, br_open->level, E_Scope::PREPROC);

   if (br_close->IsNullChunk())
   {
      LOG_FMT(LMCB, "%s(%d):  - no close\n", __func__, __LINE__);
      return(next);
   }
   // Make sure 'break', 'return', 'goto', 'case' or '}' is after the close brace
   Chunk *pc = br_close->GetNextNcNnl(E_Scope::PREPROC);

   if (  pc->IsNullChunk()
      || (  chunk_is_not_token(pc, CT_BREAK)
         && chunk_is_not_token(pc, CT_RETURN)
         && chunk_is_not_token(pc, CT_CASE)
         && chunk_is_not_token(pc, CT_GOTO)
         && chunk_is_not_token(pc, CT_BRACE_CLOSE)))
   {
      LOG_FMT(LMCB, "%s(%d):  - after '%s'\n",
              __func__, __LINE__, (pc == nullptr) ? "<null>" : get_token_name(pc->type));
      return(next);
   }

   // scan to make sure there are no definitions at brace level between braces
   for (Chunk *tmp_pc = br_open;
        tmp_pc != br_close;
        tmp_pc = tmp_pc->GetNextNcNnl(E_Scope::PREPROC))
   {
      if (  tmp_pc->level == (br_open->level + 1)
         && tmp_pc->flags.test(PCF_VAR_DEF))
      {
         LOG_FMT(LMCB, "%s(%d):  - vardef on line %zu: '%s'\n",
                 __func__, __LINE__, tmp_pc->orig_line, pc->Text());
         return(next);
      }
   }

   LOG_FMT(LMCB, "%s(%d):  - removing braces on lines %zu and %zu\n",
           __func__, __LINE__, br_open->orig_line, br_close->orig_line);

   for (Chunk *tmp_pc = br_open;
        tmp_pc != br_close;
        tmp_pc = tmp_pc->GetNextNcNnl(E_Scope::PREPROC))
   {
      if (tmp_pc->brace_level == 0)
      {
         fprintf(stderr, "%s(%d): tmp_pc->brace_level is ZERO, cannot be decremented, at line %zu, column %zu\n",
                 __func__, __LINE__, tmp_pc->orig_line, tmp_pc->orig_col);
         log_flush(true);
         exit(EX_SOFTWARE);
      }
      tmp_pc->brace_level--;

      if (tmp_pc->level == 0)
      {
         fprintf(stderr, "%s(%d): tmp_pc->level is ZERO, cannot be decremented, at line %zu, column %zu\n",
                 __func__, __LINE__, tmp_pc->orig_line, tmp_pc->orig_col);
         log_flush(true);
         exit(EX_SOFTWARE);
      }
      tmp_pc->level--;
   }

   next = br_open->GetPrev(E_Scope::PREPROC);

   chunk_del(br_open);
   chunk_del(br_close);

   return(next->GetNext(E_Scope::PREPROC));
} // mod_case_brace_remove


static Chunk *mod_case_brace_add(Chunk *cl_colon)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LMCB, "%s(%d): orig_line %zu, orig_col is %zu\n",
           __func__, __LINE__, cl_colon->orig_line, cl_colon->orig_col);

   Chunk *pc   = cl_colon;
   Chunk *last = Chunk::NullChunkPtr;
   // look for the case token to the colon
   Chunk *cas_ = cl_colon->GetPrevType(CT_CASE, cl_colon->level);
   // look for the parent
   Chunk *swit = cas_->parent;
   // look for the opening brace of the switch
   Chunk *open = swit->GetNextType(CT_BRACE_OPEN, swit->level);
   // look for the closing brace of the switch
   Chunk *clos = chunk_skip_to_match(open);

   // find the end of the case-block
   pc = pc->GetNextNcNnl(E_Scope::PREPROC);

   while (pc->IsNotNullChunk())
   {
      LOG_FMT(LMCB, "%s(%d): Text() is '%s', orig_line %zu, orig_col is %zu, pp_level is %zu\n",
              __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col, pc->pp_level);

      if (pc->level == cl_colon->level)
      {
         if (chunk_is_token(pc, CT_CASE))
         {
            LOG_FMT(LMCB, "%s(%d): Text() is '%s', orig_line %zu, orig_col is %zu\n",
                    __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);
            last = calculate_closing_brace_position(cl_colon, pc);
            break;
         }
      }
      else if (pc->level == cl_colon->level - 1)
      {
         if (pc == clos)
         {
            LOG_FMT(LMCB, "%s(%d): Text() is '%s', orig_line %zu, orig_col is %zu\n",
                    __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);
            // end of switch is reached
            last = calculate_closing_brace_position(cl_colon, pc);
            LOG_FMT(LMCB, "%s(%d): last->Text() is '%s', orig_line %zu, orig_col is %zu\n",
                    __func__, __LINE__, last->Text(), last->orig_line, last->orig_col);
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
   LOG_FMT(LMCB, "%s(%d): last->Text() is '%s', orig_line %zu, orig_col is %zu\n",
           __func__, __LINE__, last->Text(), last->orig_line, last->orig_col);
   LOG_FMT(LMCB, "%s(%d): adding braces after '%s' on line %zu\n",
           __func__, __LINE__, cl_colon->Text(), cl_colon->orig_line);

   Chunk chunk;

   set_chunk_type(&chunk, CT_BRACE_OPEN);
   set_chunk_parent(&chunk, CT_CASE);
   chunk.orig_line   = cl_colon->orig_line;
   chunk.orig_col    = cl_colon->orig_col;
   chunk.level       = cl_colon->level;
   chunk.pp_level    = cl_colon->pp_level;
   chunk.brace_level = cl_colon->brace_level;
   chunk.flags       = pc->flags & PCF_COPY_FLAGS;
   chunk.str         = "{";
   Chunk *br_open = chunk_add_after(&chunk, cl_colon);

   set_chunk_type(&chunk, CT_BRACE_CLOSE);
   chunk.orig_line = last->orig_line;
   chunk.orig_col  = last->orig_col;
   chunk.str       = "}";
   Chunk *br_close = chunk_add_after(&chunk, last);

   for (pc = br_open->GetNext(E_Scope::PREPROC);
        pc != br_close;
        pc = pc->GetNext(E_Scope::PREPROC))
   {
      pc->level++;
      pc->brace_level++;
   }

   return(br_open);
} // mod_case_brace_add


static void mod_case_brace(void)
{
   LOG_FUNC_ENTRY();

   Chunk *pc = Chunk::GetHead();

   // Make sure to start outside of a preprocessor line (see issue #3366)
   if (pc->IsPreproc())
   {
      pc = pc->GetNextNcNnlNpp();
   }

   while (  pc != nullptr
         && pc->IsNotNullChunk())
   {
      Chunk *next = pc->GetNextNcNnl(E_Scope::PREPROC);

      if (next->IsNullChunk())
      {
         return;
      }
      log_rule_B("mod_case_brace");

      if (  options::mod_case_brace() == IARF_REMOVE
         && chunk_is_token(pc, CT_BRACE_OPEN)
         && get_chunk_parent_type(pc) == CT_CASE)
      {
         log_rule_B("mod_case_brace - add");
         pc = mod_case_brace_remove(pc);
      }
      else if (  (options::mod_case_brace() & IARF_ADD)
              && chunk_is_token(pc, CT_CASE_COLON)
              && chunk_is_not_token(next, CT_BRACE_OPEN)
              && chunk_is_not_token(next, CT_BRACE_CLOSE)
              && chunk_is_not_token(next, CT_CASE))
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
   LOG_FMT(LBRCH, "%s(%d): if starts on line %zu, orig_col is %zu.\n",
           __func__, __LINE__, br_start->orig_line, br_start->orig_col);

   vector<Chunk *> braces;

   braces.reserve(16);

   bool  must_have_braces   = false;
   bool  has_unbraced_block = false;

   Chunk *pc = br_start;

   while (pc != nullptr)
   {
      LOG_FMT(LBRCH, "%s(%d): pc->Text() is '%s', orig_line is %zu, orig_col is %zu.\n",
              __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);

      if (chunk_is_token(pc, CT_BRACE_OPEN))
      {
         const bool tmp = can_remove_braces(pc);
         LOG_FMT(LBRCH, "%s(%d): braces.size() is %zu, line is %zu, - can%s remove %s\n",
                 __func__, __LINE__, braces.size(), pc->orig_line, tmp ? "" : "not",
                 get_token_name(pc->type));

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
                 __func__, __LINE__, braces.size(), pc->orig_line, tmp ? "should add" : "ignore",
                 get_token_name(pc->type));

         has_unbraced_block = true;
      }

      if (  options::mod_full_brace_if_chain() == 3
         && !has_unbraced_block)
      {
         must_have_braces = true;
      }
      braces.push_back(pc);
      Chunk *br_close = chunk_skip_to_match(pc, E_Scope::PREPROC);

      if (br_close == nullptr)
      {
         break;
      }
      braces.push_back(br_close);

      pc = br_close->GetNextNcNnl(E_Scope::PREPROC);

      if (  pc->IsNullChunk()
         || chunk_is_not_token(pc, CT_ELSE))
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

      if (chunk_is_token(pc, CT_ELSEIF))
      {
         while (  chunk_is_not_token(pc, CT_VBRACE_OPEN)
               && chunk_is_not_token(pc, CT_BRACE_OPEN))
         {
            pc = pc->GetNextNcNnl(E_Scope::PREPROC);
         }
      }

      if (pc->IsNullChunk())
      {
         break;
      }

      if (  chunk_is_not_token(pc, CT_BRACE_OPEN)
         && chunk_is_not_token(pc, CT_VBRACE_OPEN))
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

         chunk_flags_set(brace, PCF_KEEP_BRACE);

         if (  chunk_is_token(brace, CT_VBRACE_OPEN)
            || chunk_is_token(brace, CT_VBRACE_CLOSE))
         {
            LOG_FMT(LBRCH, "%s(%d):  %zu",
                    __func__, __LINE__, brace->orig_line);
            convert_vbrace(brace);
         }
         else
         {
            LOG_FMT(LBRCH, "%s(%d):  {%zu}",
                    __func__, __LINE__, brace->orig_line);
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

         if (  (  chunk_is_token(brace, CT_BRACE_OPEN)
               || chunk_is_token(brace, CT_BRACE_CLOSE))
            && (get_chunk_parent_type(brace) != CT_BRACED_INIT_LIST)
            && (multiline_block ? !paren_multiline_before_brace(brace) : true))
         {
            LOG_FMT(LBRCH, "%s(%d): brace->orig_line is %zu, brace->orig_col is %zu\n",
                    __func__, __LINE__, brace->orig_line, brace->orig_col);
            convert_brace(brace);
         }
         else
         {
            LOG_FMT(LBRCH, "%s(%d): brace->orig_line is %zu, brace->orig_col is %zu\n",
                    __func__, __LINE__, brace->orig_line, brace->orig_col);
         }
      }
   }
} // process_if_chain


static void mod_full_brace_if_chain(void)
{
   LOG_FUNC_ENTRY();

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (  (  chunk_is_token(pc, CT_BRACE_OPEN)
            || chunk_is_token(pc, CT_VBRACE_OPEN))
         && get_chunk_parent_type(pc) == CT_IF)
      {
         process_if_chain(pc);
      }
   }
}
