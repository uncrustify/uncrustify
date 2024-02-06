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

#include <stdexcept>            // to get std::logic_error


using std::string;
using std::to_string;
using std::invalid_argument;


ParenStackEntry::ParenStackEntry()
{
   m_openToken       = CT_EOF;
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
   m_parent          = CT_NONE;
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
   m_ifdefType   = E_Token::CT_NONE;
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

   if (  pc->GetType() == CT_ACCESS
      || pc->GetType() == CT_ANGLE_CLOSE
      || pc->GetType() == CT_ANGLE_OPEN
      || pc->GetType() == CT_ARITH                    // Issue #3965
      || pc->GetType() == CT_ASSIGN
      || pc->GetType() == CT_ASSIGN_FUNC_PROTO        // Issue #4026
      || pc->GetType() == CT_BRACE_CLOSE
      || pc->GetType() == CT_BRACE_OPEN
      || pc->GetType() == CT_BOOL
      || pc->GetType() == CT_CASE
      || pc->GetType() == CT_CLASS_COLON
      || pc->GetType() == CT_COMMA
      || pc->GetType() == CT_COMMENT
      || pc->GetType() == CT_COMMENT_CPP
      || pc->GetType() == CT_COMMENT_MULTI
      || pc->GetType() == CT_COMPARE                  // Issue #3915
      || pc->GetType() == CT_COND_COLON
      || pc->GetType() == CT_DC_MEMBER                // Issue #4026
      || pc->GetType() == CT_ELLIPSIS                 // Issue #4223
      || pc->GetType() == CT_FPAREN_CLOSE
      || pc->GetType() == CT_FPAREN_OPEN
      || pc->GetType() == CT_FUNC_CTOR_VAR            // Issue #4026
      || pc->GetType() == CT_INCDEC_AFTER             // Issue #4026
      || pc->GetType() == CT_LPAREN_CLOSE
      || pc->GetType() == CT_LPAREN_OPEN
      || pc->GetType() == CT_MACRO_CLOSE
      || pc->GetType() == CT_MACRO_FUNC_CALL          // Issue #4026
      || pc->GetType() == CT_MACRO_OPEN
      || pc->GetType() == CT_MEMBER                   // Issue #3996
      || pc->GetType() == CT_NEWLINE
      || pc->GetType() == CT_NONE
      || pc->GetType() == CT_OC_END
      || pc->GetType() == CT_OC_MSG_NAME
      || pc->GetType() == CT_OC_PROPERTY
      || pc->GetType() == CT_OC_SCOPE
      || pc->GetType() == CT_OPERATOR                 // Issue #4026
      || pc->GetType() == CT_PARAMETER_PACK           // Issue #4075
      || pc->GetType() == CT_PAREN_CLOSE
      || pc->GetType() == CT_PAREN_OPEN
      || pc->GetType() == CT_PREPROC
      || pc->GetType() == CT_QUESTION                 // Issue #4023
      || pc->GetType() == CT_RPAREN_CLOSE             // Issue #3914
      || pc->GetType() == CT_RPAREN_OPEN
      || pc->GetType() == CT_SBOOL                    // Issue #3965
      || pc->GetType() == CT_SEMICOLON
      || pc->GetType() == CT_SHIFT                    // Issue #3983
      || pc->GetType() == CT_SPAREN_CLOSE
      || pc->GetType() == CT_SPAREN_OPEN
      || pc->GetType() == CT_SQL_END
      || pc->GetType() == CT_SQUARE_CLOSE
      || pc->GetType() == CT_SQUARE_OPEN
      || pc->GetType() == CT_TEMPLATE                 // Issue #4220
      || pc->GetType() == CT_TPAREN_CLOSE
      || pc->GetType() == CT_TPAREN_OPEN
      || pc->GetType() == CT_TYPEDEF
      || pc->GetType() == CT_VBRACE_CLOSE
      || pc->GetType() == CT_VBRACE_OPEN
      || pc->GetType() == CT_VSEMICOLON
      || pc->GetType() == CT_WORD)
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
           func, line, (size_t)this, m_parenStack.back().open_line, m_parenStack.back().open_colu,
           get_token_name(m_parenStack.back().type), cpd.pp_level, m_parenStack.back().level,
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
