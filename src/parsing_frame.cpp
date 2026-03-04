/**
 * @file parsing_frame.cpp
 *
 * Holds data needed for indenting and brace parsing
 *
 * @author  Daniel Chumak
 * @license GPL v2+
 */

#include "parsing_frame.h"

#include "chunk.h"
#include "options.h"
#include "uncrustify.h"

#include <cstdio>
#include <stdexcept>            // to get std::logic_error
#include <string>
#include <vector>               // to get std::vector


using std::string;
using std::to_string;
using std::invalid_argument;


ParenStackEntry::ParenStackEntry()
{
   m_openToken       = E_Token::EOFILE;
   m_openChunk       = Chunk::NullChunkPtr;
   m_openLevel       = 0;
   m_openLine        = 0;
   m_openCol         = 0;
   m_braceIndent     = 0;
   m_indent          = 1;
   m_indentTmp       = 1;
   m_indentTab       = 1;
   m_nsCount         = 0;
   m_indentContinue  = false;
   m_inPreproc       = false;
   m_nonVardef       = false;
   m_parent          = E_Token::NONE;
   m_stage           = E_BraceStage::NONE;
   m_indentationData = { Chunk::NullChunkPtr, 0 };
   m_popChunk        = Chunk::NullChunkPtr;
}


ParsingFrame::ParsingFrame()
{
   //! amount of elements for which memory is going to be pre-initialized
   static constexpr int CONTAINER_INIT_SIZE = 16;

   m_parenStack = std::vector<ParenStackEntry>();
   m_parenStack.reserve(CONTAINER_INIT_SIZE);
   m_parenStack.push_back(ParenStackEntry());

   m_lastPopped  = ParenStackEntry();
   m_refNumber   = 0;
   m_parenLevel  = 0;
   m_braceLevel  = 0;
   m_ppLevel     = 0;
   m_sParenCount = 0;
   m_parenCount  = 0;
   m_ifdefType   = E_Token::NONE;
   m_stmtCount   = 0;
   m_exprCount   = 0;
}


ParenStackEntry &ParsingFrame::prev(size_t idx)
{
   LOG_FUNC_ENTRY();

   if (idx == 0)
   {
      throw invalid_argument(string(__FILE__) + ":" + to_string(__LINE__)
                             + " idx can't be zero");
   }

   if (idx >= m_parenStack.size())
   {
      LOG_FMT(LINDPSE, "%s(%d): idx is %zu, size is %zu\n",
              __func__, __LINE__, idx, m_parenStack.size());
      throw invalid_argument(string(__FILE__) + ":" + to_string(__LINE__)
                             + " idx can't be >= size()");
   }
   return(*std::prev(std::end(m_parenStack), idx + 1));
}


const ParenStackEntry &ParsingFrame::prev(size_t idx) const
{
   LOG_FUNC_ENTRY();
   // Reuse the code from non-const method
   return((const_cast<ParsingFrame *>(this))->prev(idx));
}


ParenStackEntry &ParsingFrame::top()
{
   // always at least one (dummy) element inside m_parenStack guaranteed
   return(*std::prev(std::end(m_parenStack)));
}


const ParenStackEntry &ParsingFrame::top() const
{
   // always at least one (dummy) element inside m_parenStack guaranteed
   return(*std::prev(std::end(m_parenStack)));
}


void ParsingFrame::push(Chunk *pc, const char *func, int line, E_BraceStage stage)
{
   LOG_FUNC_ENTRY();

   ParenStackEntry new_entry;

   if (pc->IsNotNullChunk())
   {
      new_entry.SetOpenToken(pc->GetType());
      new_entry.SetOpenLevel(pc->GetLevel());
      new_entry.SetOpenLine(pc->GetOrigLine());
      new_entry.SetOpenCol(pc->GetOrigCol());
      new_entry.SetOpenChunk(pc);

      new_entry.SetIndentTab(top().GetIndentTab());
      new_entry.SetIndentContinue(top().GetIndentContinue());
      new_entry.SetStage(stage);

      new_entry.SetInPreproc(pc->TestFlags(PCF_IN_PREPROC));
      new_entry.SetNonVardef(false);
      new_entry.IndentData() = top().GetIndentData();
      new_entry.SetPopChunk(Chunk::NullChunkPtr);
   }
   m_parenStack.push_back(new_entry);

// uncomment the line below to get the address of the m_parenStack
// #define DEBUG_PUSH_POP
#ifdef DEBUG_PUSH_POP
   LOG_FMT(LINDPSE, "ParsingFrame::push(%s:%d) Add is %4zu: orig line is %4zu, orig col is %4zu, type is %12s, "
           "brace level is %2zu, level is %2zu, pse_tos: %2zu -> %2zu\n",
           func, line, (size_t)this, pc->GetOrigLine(), pc->GetOrigCol(),
           get_token_name(pc->GetType()), pc->GetBraceLevel(), pc->GetLevel(),
           (m_parenStack.size() - 2), (m_parenStack.size() - 1));
#else /* DEBUG_PUSH_POP */
   LOG_FMT(LINDPSE, "ParsingFrame::push(%s:%d): orig line is %4zu, orig col is %4zu, type is %12s, "
           "brace level is %2zu, level is %2zu, pse_tos: %2zu -> %2zu\n",
           func, line, pc->GetOrigLine(), pc->GetOrigCol(),
           get_token_name(pc->GetType()), pc->GetBraceLevel(), pc->GetLevel(),
           (m_parenStack.size() - 2), (m_parenStack.size() - 1));
#endif /* DEBUG_PUSH_POP */
}


void ParsingFrame::pop(const char *func, int line, Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (  pc->GetType() == E_Token::ACCESS
      || pc->GetType() == E_Token::ANGLE_CLOSE
      || pc->GetType() == E_Token::ANGLE_OPEN
      || pc->GetType() == E_Token::ARITH                    // Issue #3965
      || pc->GetType() == E_Token::ASSIGN
      || pc->GetType() == E_Token::ASSIGN_FUNC_PROTO        // Issue #4026
      || pc->GetType() == E_Token::BRACE_CLOSE
      || pc->GetType() == E_Token::BRACE_OPEN
      || pc->GetType() == E_Token::BOOL
      || pc->GetType() == E_Token::CARET                    // Issue #4593
      || pc->GetType() == E_Token::CASE
      || pc->GetType() == E_Token::CLASS_COLON
      || pc->GetType() == E_Token::COMMA
      || pc->GetType() == E_Token::COMMENT
      || pc->GetType() == E_Token::COMMENT_CPP
      || pc->GetType() == E_Token::COMMENT_MULTI
      || pc->GetType() == E_Token::COMPARE                  // Issue #3915
      || pc->GetType() == E_Token::COND_COLON
      || pc->GetType() == E_Token::DC_MEMBER                // Issue #4026
      || pc->GetType() == E_Token::DESTRUCTOR               // Issue #4593
      || pc->GetType() == E_Token::ELLIPSIS                 // Issue #4223
      || pc->GetType() == E_Token::FPAREN_CLOSE
      || pc->GetType() == E_Token::FPAREN_OPEN
      || pc->GetType() == E_Token::FUNC_CTOR_VAR            // Issue #4026
      || pc->GetType() == E_Token::INCDEC_AFTER             // Issue #4026
      || pc->GetType() == E_Token::LPAREN_CLOSE
      || pc->GetType() == E_Token::LPAREN_OPEN
      || pc->GetType() == E_Token::MACRO_CLOSE
      || pc->GetType() == E_Token::MACRO_FUNC_CALL          // Issue #4026
      || pc->GetType() == E_Token::MACRO_OPEN
      || pc->GetType() == E_Token::MEMBER                   // Issue #3996
      || pc->GetType() == E_Token::NEWLINE
      || pc->GetType() == E_Token::NONE
      || pc->GetType() == E_Token::OC_END
      || pc->GetType() == E_Token::OC_MSG_NAME
      || pc->GetType() == E_Token::OC_PROPERTY
      || pc->GetType() == E_Token::OC_SCOPE
      || pc->GetType() == E_Token::OPERATOR                 // Issue #4026
      || pc->GetType() == E_Token::PARAMETER_PACK           // Issue #4075
      || pc->GetType() == E_Token::PAREN_CLOSE
      || pc->GetType() == E_Token::PAREN_OPEN
      || pc->GetType() == E_Token::PREPROC
      || pc->GetType() == E_Token::QUESTION                 // Issue #4023
      || pc->GetType() == E_Token::RPAREN_CLOSE             // Issue #3914
      || pc->GetType() == E_Token::RPAREN_OPEN
      || pc->GetType() == E_Token::SBOOL                    // Issue #3965
      || pc->GetType() == E_Token::SEMICOLON
      || pc->GetType() == E_Token::SHIFT                    // Issue #3983
      || pc->GetType() == E_Token::SPAREN_CLOSE
      || pc->GetType() == E_Token::SPAREN_OPEN
      || pc->GetType() == E_Token::SQL_END
      || pc->GetType() == E_Token::SQUARE_CLOSE
      || pc->GetType() == E_Token::SQUARE_OPEN
      || pc->GetType() == E_Token::TEMPLATE                 // Issue #4220
      || pc->GetType() == E_Token::TPAREN_CLOSE
      || pc->GetType() == E_Token::TPAREN_OPEN
      || pc->GetType() == E_Token::TYPEDEF
      || pc->GetType() == E_Token::VBRACE_CLOSE
      || pc->GetType() == E_Token::VBRACE_OPEN
      || pc->GetType() == E_Token::VSEMICOLON
      || pc->GetType() == E_Token::WORD)
   {
      LOG_FMT(LINDPSE, "ParsingFrame::pop (%s:%d): orig line is %4zu, orig col is %4zu, type is %12s\n",
              func, line, pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));
   }
   else
   {
      fprintf(stderr, "ParsingFrame::pop (%s:%d): orig line is %4zu, orig col is %4zu, type is %12s,\n",
              func, line, pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));
      fprintf(stderr, "ParsingFrame::pop (%s:%d): the type is %s, is not coded. Please make a call.\n",
              func, line, get_token_name(pc->GetType()));
      log_flush(true);

      if (uncrustify::options::debug_use_the_exit_function_pop())  // Issue #4075
      {
         exit(EX_SOFTWARE);
      }
   }
#ifdef DEBUG_PUSH_POP
   LOG_FMT(LINDPSE, "ParsingFrame::pop (%s:%d) Add is %4zu: open_line is %4zu, clos_col is %4zu, type is %12s, "
           "cpd.level   is %2d, level is %2zu, pse_tos: %2zu -> %2zu\n",
           func, line, (size_t)this, m_parenStack.back().GetOpenLine(), m_parenStack.back().GetOpenCol(),
           get_token_name(m_parenStack.back().GetOpenToken()), cpd.pp_level, m_parenStack.back().GetOpenLevel(),
           (m_parenStack.size() - 1), (m_parenStack.size() - 2));
#else /* DEBUG_PUSH_POP */
   LOG_FMT(LINDPSE, "ParsingFrame::pop (%s:%d): open_line is %4zu, clos_col is %4zu, type is %12s, "
           "cpd.level   is %2d, level is %2zu, pse_tos: %2zu -> %2zu\n",
           func, line, m_parenStack.back().GetOpenLine(), m_parenStack.back().GetOpenCol(),
           get_token_name(m_parenStack.back().GetOpenToken()), cpd.pp_level, m_parenStack.back().GetOpenLevel(),
           (m_parenStack.size() - 1), (m_parenStack.size() - 2));
#endif /* DEBUG_PUSH_POP */

   m_lastPopped = *std::prev(std::end(m_parenStack));

   if (m_parenStack.size() == 1)
   {
      *std::begin(m_parenStack) = ParenStackEntry();
   }
   else
   {
      m_parenStack.pop_back();
   }
} // ParsingFrame::pop
