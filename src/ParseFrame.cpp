/**
 * @file ParseFrame.cpp
 *
 * Container that holds data needed for indenting and brace parsing
 *
 * @author  Daniel Chumak
 * @license GPL v2+
 */

#include "ParseFrame.h"
#include "uncrustify.h"
#include "uncrustify_types.h"
#include <string>
#include <stdexcept>


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
   in_ifdef     = c_token_t::CT_NONE;
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

   if (idx == 0 || idx >= pse.size())
   {
      throw invalid_argument(string(__FILE__) + ":" + to_string(__LINE__)
                             + " idx can't be zero or >= size()");
   }
   return(*std::prev(std::end(pse), idx + 1));
}

const ContainerType &ParseFrame::prev(size_t idx) const
{
   LOG_FUNC_ENTRY();

   if (idx == 0 || idx >= pse.size())
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


void ParseFrame::push(chunk_t &pc, brace_stage_e stage)
{
   LOG_FUNC_ENTRY();

   ContainerType new_entry = {};
   new_entry.type      = pc.type;
   new_entry.level     = pc.level;
   new_entry.open_line = pc.orig_line;
   new_entry.pc        = &pc;

   new_entry.indent_tab  = top().indent_tab;
   new_entry.indent_cont = top().indent_cont;
   new_entry.stage       = stage;

   new_entry.in_preproc = (pc.flags & PCF_IN_PREPROC);
   new_entry.non_vardef = false;
   new_entry.ip         = top().ip;

   pse.push_back(new_entry);

   LOG_FMT(LINDPSE, "%s(%d): O%zu:%zu, type: %s, brace_level: %zu, "
           "level: %zu, pse_tos: %zu -> %zu\n",
           __func__, __LINE__, pc.orig_line, pc.orig_col,
           get_token_name(pc.type), pc.brace_level, pc.level, (pse.size() - 2),
           (pse.size() - 1));
}


void ParseFrame::pop()
{
   LOG_FUNC_ENTRY();

   // always at least one (dummy) element inside pse guaranteed
//   if (pse.empty())
//   {
//      throw logic_error(string(__FILE__) + ":" + to_string(__LINE__)
//                        + "the stack index is already zero");
//   }

   LOG_FMT(LINDPSE, "%s(%d): cpd.pp_level: %d, pse type: %s, "
           "pse open_line: %zu, pse level: %zu, pse_tos: %zu -> %zu\n",
           __func__, __LINE__, cpd.pp_level, get_token_name(pse.back().type),
           pse.back().open_line, pse.back().level, (pse.size() - 1),
           (pse.size() - 2));

   last_poped = *std::prev(std::end(pse));

   if (pse.size() == 1)
   {
      *std::begin(pse) = genDummy();
   }
   else
   {
      pse.pop_back();
   }
}


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
