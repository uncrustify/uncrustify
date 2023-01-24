/**
 * @file ParsingFrameStack.cpp
 *
 * Holds data needed for indenting and brace parsing
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
using std::invalid_argument;


//! amount of elements for which memory is going to be pre-initialized
static constexpr const int CONTAINER_INIT_SIZE = 16;


static ParsingFrame genDummy()
{
   ParsingFrame tmp_dummy{};

   tmp_dummy.indent     = 1;
   tmp_dummy.indent_tmp = 1;
   tmp_dummy.indent_tab = 1;
   tmp_dummy.type       = CT_EOF;
   tmp_dummy.pc         = Chunk::NullChunkPtr;
   tmp_dummy.pop_pc     = Chunk::NullChunkPtr;

   return(tmp_dummy);
}


void ParsingFrameStack::clear()
{
   last_poped = genDummy();

   pse = std::vector<ParsingFrame>();
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


ParsingFrameStack::ParsingFrameStack()
{
   ParsingFrameStack::clear();
}


bool ParsingFrameStack::empty() const
{
   // always at least one (dummy) element inside pse guaranteed
   return(false);
//   return(pse.empty());
}


ParsingFrame &ParsingFrameStack::at(size_t idx)
{
   return(pse.at(idx));
}


const ParsingFrame &ParsingFrameStack::at(size_t idx) const
{
   return(pse.at(idx));
}


ParsingFrame &ParsingFrameStack::prev(size_t idx)
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


const ParsingFrame &ParsingFrameStack::prev(size_t idx) const
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


ParsingFrame &ParsingFrameStack::top()
{
   // always at least one (dummy) element inside pse guaranteed
//   if (pse.empty())
//   {
//      throw logic_error(string(__FILE__) + ":" + to_string(__LINE__)
//                        + " called top on an empty stack");
//   }
   return(*std::prev(std::end(pse)));
}


const ParsingFrame &ParsingFrameStack::top() const
{
   // always at least one (dummy) element inside pse guaranteed
//   if (pse.empty())
//   {
//      throw logic_error(string(__FILE__) + ":" + to_string(__LINE__)
//                        + " called top on an empty stack");
//   }
   return(*std::prev(std::end(pse)));
}


void ParsingFrameStack::push(std::nullptr_t, E_BraceStage stage)
{
   static Chunk dummy;

   push(&dummy, __func__, __LINE__, stage);
   top().pc = Chunk::NullChunkPtr;
}


void ParsingFrameStack::push(Chunk *pc, const char *func, int line, E_BraceStage stage)
{
   LOG_FUNC_ENTRY();

   ParsingFrame new_entry = {};

   new_entry.type      = pc->GetType();
   new_entry.level     = pc->GetLevel();
   new_entry.open_line = pc->GetOrigLine();
   new_entry.open_colu = pc->GetOrigCol();
   new_entry.pc        = pc;

   new_entry.indent_tab  = top().indent_tab;
   new_entry.indent_cont = top().indent_cont;
   new_entry.stage       = stage;

   new_entry.in_preproc = pc->TestFlags(PCF_IN_PREPROC);
   new_entry.non_vardef = false;
   new_entry.ip         = top().ip;
   new_entry.pop_pc     = Chunk::NullChunkPtr;

   pse.push_back(new_entry);

// uncomment the line below to get the address of the pse
// #define DEBUG_PUSH_POP
#ifdef DEBUG_PUSH_POP
   LOG_FMT(LINDPSE, "ParsingFrameStack::push(%s:%d) Add is %4zu: orig line is %4zu, orig col is %4zu, type is %12s, "
           "brace level is %2zu, level is %2zu, pse_tos: %2zu -> %2zu\n",
           func, line, (size_t)this, pc->GetOrigLine(), pc->GetOrigCol(),
           get_token_name(pc->GetType()), pc->GetBraceLevel(), pc->GetLevel(),
           (pse.size() - 2), (pse.size() - 1));
#else /* DEBUG_PUSH_POP */
   LOG_FMT(LINDPSE, "ParsingFrameStack::push(%s:%d): orig line is %4zu, orig col is %4zu, type is %12s, "
           "brace level is %2zu, level is %2zu, pse_tos: %2zu -> %2zu\n",
           func, line, pc->GetOrigLine(), pc->GetOrigCol(),
           get_token_name(pc->GetType()), pc->GetBraceLevel(), pc->GetLevel(),
           (pse.size() - 2), (pse.size() - 1));
#endif /* DEBUG_PUSH_POP */
}


void ParsingFrameStack::pop(const char *func, int line, Chunk *pc)
{
   LOG_FUNC_ENTRY();

   // always at least one (dummy) element inside pse guaranteed
//   if (pse.empty())
//   {
//      throw logic_error(string(__FILE__) + ":" + to_string(__LINE__)
//                        + "the stack index is already zero");
//   }

   if (  pc->GetType() == CT_PAREN_CLOSE
      || pc->GetType() == CT_BRACE_CLOSE
      || pc->GetType() == CT_VBRACE_CLOSE
      || pc->GetType() == CT_FPAREN_CLOSE
      || pc->GetType() == CT_LPAREN_CLOSE
      || pc->GetType() == CT_SPAREN_CLOSE
      || pc->GetType() == CT_TPAREN_CLOSE
      || pc->GetType() == CT_CLASS_COLON
      || pc->GetType() == CT_ANGLE_CLOSE
      || pc->GetType() == CT_SEMICOLON
      || pc->GetType() == CT_SQUARE_CLOSE)
   {
      LOG_FMT(LINDPSE, "ParsingFrameStack::pop (%s:%d): orig line is %4zu, orig col is %4zu, type is %12s, pushed with\n",
              func, line, pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));
   }
   else if (  pc->GetType() == CT_ACCESS
           || pc->GetType() == CT_ASSIGN
           || pc->GetType() == CT_BRACE_OPEN
           || pc->GetType() == CT_BOOL
           || pc->GetType() == CT_CASE
           || pc->GetType() == CT_COMMA
           || pc->GetType() == CT_COMMENT
           || pc->GetType() == CT_COMMENT_CPP
           || pc->GetType() == CT_COMMENT_MULTI
           || pc->GetType() == CT_COND_COLON
           || pc->GetType() == CT_FPAREN_OPEN
           || pc->GetType() == CT_PAREN_OPEN
           || pc->GetType() == CT_TPAREN_OPEN
           || pc->GetType() == CT_MACRO_CLOSE
           || pc->GetType() == CT_MACRO_OPEN
           || pc->GetType() == CT_NEWLINE
           || pc->GetType() == CT_NONE
           || pc->GetType() == CT_OC_END
           || pc->GetType() == CT_OC_MSG_NAME
           || pc->GetType() == CT_OC_SCOPE
           || pc->GetType() == CT_OC_PROPERTY
           || pc->GetType() == CT_PREPROC
           || pc->GetType() == CT_SQUARE_OPEN
           || pc->GetType() == CT_SQL_END
           || pc->GetType() == CT_TYPEDEF
           || pc->GetType() == CT_VSEMICOLON
           || pc->GetType() == CT_WORD)
   {
      LOG_FMT(LINDPSE, "ParsingFrameStack::pop (%s:%d): orig line is %4zu, orig col is %4zu, type is %12s\n",
              func, line, pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));
   }
   else
   {
      LOG_FMT(LINDPSE, "ParsingFrameStack::pop (%s:%d): orig line is %4zu, orig col is %4zu, type is %12s,\n",
              func, line, pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));
      LOG_FMT(LINDPSE, "ParsingFrameStack::pop (%s:%d): the type is %s, is not coded. Please make a call.\n",
              func, line, get_token_name(pc->GetType()));
      log_flush(true);
      exit(EX_SOFTWARE);
   }
#ifdef DEBUG_PUSH_POP
   LOG_FMT(LINDPSE, "ParsingFrameStack::pop (%s:%d) Add is %4zu: open_line is %4zu, clos_col is %4zu, type is %12s, "
           "cpd.level   is %2d, level is %2zu, pse_tos: %2zu -> %2zu\n",
           func, line, (size_t)this, pse.back().open_line, pse.back().open_colu,
           get_token_name(pse.back().type), cpd.pp_level, pse.back().level,
           (pse.size() - 1), (pse.size() - 2));
#else /* DEBUG_PUSH_POP */
   LOG_FMT(LINDPSE, "ParsingFrameStack::pop (%s:%d): open_line is %4zu, clos_col is %4zu, type is %12s, "
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
} // ParsingFrameStack::pop


size_t ParsingFrameStack::size() const
{
   // always at least one (dummy) element inside pse guaranteed
   return(pse.size());
}


const ParsingFrame &ParsingFrameStack::poped() const
{
   return(last_poped);
}


// TODO C++14: see abstract versions: std::rend, std::cend, std::crend ...
ParsingFrameStack::iterator ParsingFrameStack::begin()
{
   return(std::begin(pse));
}


ParsingFrameStack::const_iterator ParsingFrameStack::begin() const
{
   return(std::begin(pse));
}


ParsingFrameStack::reverse_iterator ParsingFrameStack::rbegin()
{
   return(pse.rbegin());
}


ParsingFrameStack::const_reverse_iterator ParsingFrameStack::rbegin() const
{
   return(pse.rbegin());
}


ParsingFrameStack::iterator ParsingFrameStack::end()
{
   return(std::end(pse));
}


ParsingFrameStack::const_iterator ParsingFrameStack::end() const
{
   return(std::end(pse));
}


ParsingFrameStack::reverse_iterator ParsingFrameStack::rend()
{
   return(pse.rend());
}


ParsingFrameStack::const_reverse_iterator ParsingFrameStack::rend() const
{
   return(pse.rend());
}
