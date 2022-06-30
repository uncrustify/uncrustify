/**
 * @file ParseFrame.cpp
 *
 * Container that holds data needed for indenting and brace parsing
 *
 * @author  Daniel Chumak
 * @license GPL v2+
 */

#include "ParseFrame.h"

#include "chunk.h"
#include "uncrustify.h"

#include <stdexcept>            // to get std::logic_error


using std::string;
using std::to_string;
using std::logic_error;
using std::invalid_argument;

using ContainerType = paren_stack_entry_t;
using Container     = std::vector<ContainerType>;


//! amount of elements for which memory is going to be pre-initialized
static constexpr const int CONTAINER_INIT_SIZE = 16;


static ContainerType genDummy()
{
   ContainerType tmp_dummy{};

   tmp_dummy.indent     = 1;
   tmp_dummy.indent_tmp = 1;
   tmp_dummy.indent_tab = 1;
   tmp_dummy.type       = CT_EOF;
   tmp_dummy.pc         = Chunk::NullChunkPtr;
   tmp_dummy.pop_pc     = Chunk::NullChunkPtr;

   return(tmp_dummy);
}


void ParseFrame::clear()
{
   last_poped = genDummy();

   pse = Container{};
   pse.reserve(CONTAINER_INIT_SIZE);
   pse.push_back(genDummy());

   ref_no       = 0;
   level        = 0;
   brace_level  = 0;
   pp_level     = 0;
   sparen_count = 0;
   paren_count  = 0;
   in_ifdef     = E_Token::CT_NONE;
   stmt_count   = 0;
   expr_count   = 0;
}


ParseFrame::ParseFrame()
{
   ParseFrame::clear();
}


bool ParseFrame::empty() const
{
   // always at least one (dummy) element inside pse guaranteed
   return(false);
//   return(pse.empty());
}


ContainerType &ParseFrame::at(size_t idx)
{
   return(pse.at(idx));
}


const ContainerType &ParseFrame::at(size_t idx) const
{
   return(pse.at(idx));
}


ContainerType &ParseFrame::prev(size_t idx)
{
   LOG_FUNC_ENTRY();

   if (idx == 0)
   {
      throw invalid_argument(string(__FILE__) + ":" + to_string(__LINE__)
                             + " idx can't be zero");
   }

   if (idx >= pse.size())
   {
      LOG_FMT(LINDPSE, "%s(%d): idx is %zu, size is %zu\n",
              __func__, __LINE__, idx, pse.size());
      throw invalid_argument(string(__FILE__) + ":" + to_string(__LINE__)
                             + " idx can't be >= size()");
   }
   return(*std::prev(std::end(pse), idx + 1));
}


const ContainerType &ParseFrame::prev(size_t idx) const
{
   LOG_FUNC_ENTRY();

   if (  idx == 0
      || idx >= pse.size())
   {
      throw invalid_argument(string(__FILE__) + ":" + to_string(__LINE__)
                             + " idx can't be zero or >= size()");
   }
   return(*std::prev(std::end(pse), idx + 1));
}


ContainerType &ParseFrame::top()
{
   // always at least one (dummy) element inside pse guaranteed
//   if (pse.empty())
//   {
//      throw logic_error(string(__FILE__) + ":" + to_string(__LINE__)
//                        + " called top on an empty stack");
//   }
   return(*std::prev(std::end(pse)));
}


const ContainerType &ParseFrame::top() const
{
   // always at least one (dummy) element inside pse guaranteed
//   if (pse.empty())
//   {
//      throw logic_error(string(__FILE__) + ":" + to_string(__LINE__)
//                        + " called top on an empty stack");
//   }
   return(*std::prev(std::end(pse)));
}


void ParseFrame::push(std::nullptr_t, brace_stage_e stage)
{
   static Chunk dummy;

   push(&dummy, __func__, __LINE__, stage);
   top().pc = Chunk::NullChunkPtr;
}


void ParseFrame::push(Chunk *pc, const char *func, int line, brace_stage_e stage)
{
   LOG_FUNC_ENTRY();

   ContainerType new_entry = {};

   new_entry.type      = pc->type;
   new_entry.level     = pc->level;
   new_entry.open_line = pc->orig_line;
   new_entry.open_colu = pc->orig_col;
   new_entry.pc        = pc;

   new_entry.indent_tab  = top().indent_tab;
   new_entry.indent_cont = top().indent_cont;
   new_entry.stage       = stage;

   new_entry.in_preproc = pc->flags.test(PCF_IN_PREPROC);
   new_entry.non_vardef = false;
   new_entry.ip         = top().ip;
   new_entry.pop_pc     = Chunk::NullChunkPtr;

   pse.push_back(new_entry);

// uncomment the line below to get the address of the pse
// #define DEBUG_PUSH_POP
#ifdef DEBUG_PUSH_POP
   LOG_FMT(LINDPSE, "ParseFrame::push(%s:%d) Add is %4zu: orig_line is %4zu, orig_col is %4zu, type is %12s, "
           "brace_level is %2zu, level is %2zu, pse_tos: %2zu -> %2zu\n",
           func, line, (size_t)this, pc->orig_line, pc->orig_col,
           get_token_name(pc->type), pc->brace_level, pc->level,
           (pse.size() - 2), (pse.size() - 1));
#else /* DEBUG_PUSH_POP */
   LOG_FMT(LINDPSE, "ParseFrame::push(%s:%d): orig_line is %4zu, orig_col is %4zu, type is %12s, "
           "brace_level is %2zu, level is %2zu, pse_tos: %2zu -> %2zu\n",
           func, line, pc->orig_line, pc->orig_col,
           get_token_name(pc->type), pc->brace_level, pc->level,
           (pse.size() - 2), (pse.size() - 1));
#endif /* DEBUG_PUSH_POP */
}


void ParseFrame::pop(const char *func, int line, Chunk *pc)
{
   LOG_FUNC_ENTRY();

   // always at least one (dummy) element inside pse guaranteed
//   if (pse.empty())
//   {
//      throw logic_error(string(__FILE__) + ":" + to_string(__LINE__)
//                        + "the stack index is already zero");
//   }

   if (  pc->type == CT_PAREN_CLOSE
      || pc->type == CT_BRACE_CLOSE
      || pc->type == CT_VBRACE_CLOSE
      || pc->type == CT_FPAREN_CLOSE
      || pc->type == CT_LPAREN_CLOSE
      || pc->type == CT_SPAREN_CLOSE
      || pc->type == CT_CLASS_COLON
      || pc->type == CT_ANGLE_CLOSE
      || pc->type == CT_SEMICOLON
      || pc->type == CT_SQUARE_CLOSE)
   {
      LOG_FMT(LINDPSE, "ParseFrame::pop (%s:%d): orig_line is %4zu, orig_col is %4zu, type is %12s, pushed with\n",
              func, line, pc->orig_line, pc->orig_col, get_token_name(pc->type));
   }
   else if (  pc->type == CT_ACCESS
           || pc->type == CT_ASSIGN
           || pc->type == CT_BRACE_OPEN
           || pc->type == CT_BOOL
           || pc->type == CT_CASE
           || pc->type == CT_COMMA
           || pc->type == CT_COMMENT
           || pc->type == CT_COMMENT_CPP
           || pc->type == CT_COMMENT_MULTI
           || pc->type == CT_COND_COLON
           || pc->type == CT_FPAREN_OPEN
           || pc->type == CT_PAREN_OPEN
           || pc->type == CT_TPAREN_OPEN
           || pc->type == CT_MACRO_CLOSE
           || pc->type == CT_MACRO_OPEN
           || pc->type == CT_NEWLINE
           || pc->type == CT_NONE
           || pc->type == CT_OC_END
           || pc->type == CT_OC_MSG_NAME
           || pc->type == CT_OC_SCOPE
           || pc->type == CT_PREPROC
           || pc->type == CT_SQUARE_OPEN
           || pc->type == CT_SQL_END
           || pc->type == CT_TYPEDEF
           || pc->type == CT_VSEMICOLON
           || pc->type == CT_WORD)
   {
      LOG_FMT(LINDPSE, "ParseFrame::pop (%s:%d): orig_line is %4zu, orig_col is %4zu, type is %12s\n",
              func, line, pc->orig_line, pc->orig_col, get_token_name(pc->type));
   }
   else
   {
      LOG_FMT(LINDPSE, "ParseFrame::pop (%s:%d): orig_line is %4zu, orig_col is %4zu, type is %12s,\n",
              func, line, pc->orig_line, pc->orig_col, get_token_name(pc->type));
      LOG_FMT(LINDPSE, "ParseFrame::pop (%s:%d): the type is %s, is not coded. Please make a call.\n",
              func, line, get_token_name(pc->type));
      log_flush(true);
      exit(EX_SOFTWARE);
   }
#ifdef DEBUG_PUSH_POP
   LOG_FMT(LINDPSE, "ParseFrame::pop (%s:%d) Add is %4zu: open_line is %4zu, clos_col is %4zu, type is %12s, "
           "cpd.level   is %2d, level is %2zu, pse_tos: %2zu -> %2zu\n",
           func, line, (size_t)this, pse.back().open_line, pse.back().open_colu,
           get_token_name(pse.back().type), cpd.pp_level, pse.back().level,
           (pse.size() - 1), (pse.size() - 2));
#else /* DEBUG_PUSH_POP */
   LOG_FMT(LINDPSE, "ParseFrame::pop (%s:%d): open_line is %4zu, clos_col is %4zu, type is %12s, "
           "cpd.level   is %2d, level is %2zu, pse_tos: %2zu -> %2zu\n",
           func, line, pse.back().open_line, pse.back().open_colu,
           get_token_name(pse.back().type), cpd.pp_level, pse.back().level,
           (pse.size() - 1), (pse.size() - 2));
#endif /* DEBUG_PUSH_POP */

   last_poped = *std::prev(std::end(pse));

   if (pse.size() == 1)
   {
      *std::begin(pse) = genDummy();
   }
   else
   {
      pse.pop_back();
   }
} // ParseFrame::pop


size_t ParseFrame::size() const
{
   // always at least one (dummy) element inside pse guaranteed
   return(pse.size());
}


const paren_stack_entry_t &ParseFrame::poped() const
{
   return(last_poped);
}


// TODO C++14: see abstract versions: std::rend, std::cend, std::crend ...
ParseFrame::iterator ParseFrame::begin()
{
   return(std::begin(pse));
}


ParseFrame::const_iterator ParseFrame::begin() const
{
   return(std::begin(pse));
}


ParseFrame::reverse_iterator ParseFrame::rbegin()
{
   return(pse.rbegin());
}


ParseFrame::const_reverse_iterator ParseFrame::rbegin() const
{
   return(pse.rbegin());
}


ParseFrame::iterator ParseFrame::end()
{
   return(std::end(pse));
}


ParseFrame::const_iterator ParseFrame::end() const
{
   return(std::end(pse));
}


ParseFrame::reverse_iterator ParseFrame::rend()
{
   return(pse.rend());
}


ParseFrame::const_reverse_iterator ParseFrame::rend() const
{
   return(pse.rend());
}
