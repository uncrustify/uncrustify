/**
 * @file parsing_frame.h
 *
 * Holds data needed for indenting and brace parsing
 *
 * @author  Daniel Chumak
 * @license GPL v2+
 */

#ifndef PARSING_FRAME_H_INCLUDED
#define PARSING_FRAME_H_INCLUDED

#include "uncrustify_types.h"

#include <vector>


//! Class describing a parenthesis stack entry and its information
class ParenStackEntry
{
public:
   ParenStackEntry();

   E_Token         type;         //! the type that opened the entry
   Chunk           *pc;          //! chunk that opened the level, TODO: make const
   size_t          level;        //! level of opening type
   size_t          open_line;    //! line that open symbol is on, only for logging purposes
   size_t          open_colu;    //! column that open symbol is on, only for logging purposes
   size_t          brace_indent; //! indent for braces - may not relate to indent
   size_t          indent;       //! indent level (depends on use)
   size_t          indent_tmp;   //! temporary indent level (depends on use)
   size_t          indent_tab;   //! the 'tab' indent (always <= real column)
   size_t          ns_cnt;       //! Number of consecutive namespace levels
   bool            indent_cont;  //! indent_continue was applied
   bool            in_preproc;   //! whether this was created in a preprocessor
   bool            non_vardef;   //! Hit a non-vardef line
   E_Token         parent;       //! if, for, function, etc
   E_BraceStage    stage;        //! used to check progression of complex statements.
   IndentationData ip;
   Chunk           *pop_pc;
};


//! Class describing a parsing frame and its information
class ParsingFrame
{
public:
   ParsingFrame();
   virtual ~ParsingFrame() = default;

   /**
    * @brief Returns whether the frame paren stack is empty or not
    */
   bool empty() const;

   /**
    * @brief Returns the size of the frame paren stack
    */
   size_t size() const;

   /**
    * @brief Returns the last popped entry from the frame paren stack
    */
   const ParenStackEntry &lastPopped() const;

   /**
    * @brief Returns the frame reference number
    */
   size_t GetRefNumber() const;

   /**
    * @brief Set the frame reference number
    * @param the new reference number
    */
   void SetRefNumber(const size_t refNo);

   /**
    * @brief Returns the frame parenthesis level
    */
   size_t GetParenLevel() const;

   /**
    * @brief Set the frame parenthesis level
    * @param the new parenthesis level
    */
   void SetParenLevel(const size_t parenLevel);

   /**
    * @brief Returns the frame brace level
    */
   size_t GetBraceLevel() const;

   /**
    * @brief Set the frame brace level
    * @param the new brace level
    */
   void SetBraceLevel(const size_t braceLevel);

   /**
    * @brief Returns the frame preprocessor level
    */
   size_t GetPpLevel() const;

   /**
    * @brief Set the frame preprocessor level
    * @param the new preprocessor level
    */
   void SetPpLevel(const size_t ppLevel);

   /**
    * @brief Returns the count of special parenthesis
    */
   size_t GetSParenCount() const;

   /**
    * @brief Set the count of special parenthesis
    * @param the new special parenthesis count value
    */
   void SetSParenCount(const size_t sParenCount);

   /**
    * @brief Returns the count of parenthesis
    */
   size_t GetParenCount() const;

   /**
    * @brief Set the count of parenthesis
    * @param the new parenthesis count value
    */
   void SetParenCount(const size_t parenCount);

   /**
    * @brief Returns the count of statements
    */
   size_t GetStmtCount() const;

   /**
    * @brief Set the count of statements
    * @param the new statement count value
    */
   void SetStmtCount(const size_t stmtCount);

   /**
    * @brief Returns the count of statements
    */
   size_t GetExprCount() const;

   /**
    * @brief Set the count of statements
    * @param the new statement count value
    */
   void SetExprCount(const size_t exprCount);

   /**
    * @brief Returns the ifdef type
    */
   E_Token GetIfdefType() const;

   /**
    * @brief Set the ifdef type
    * @param the new type
    */
   void SetIfdefType(const E_Token inIfdef);

   ParenStackEntry &at(size_t idx);
   const ParenStackEntry &at(size_t idx) const;

   ParenStackEntry &prev(size_t idx = 1);
   const ParenStackEntry &prev(size_t idx = 1) const;

   ParenStackEntry &top();
   const ParenStackEntry &top() const;

   void push(Chunk *pc, const char *func, int line, E_BraceStage stage = E_BraceStage::NONE);
   void push(std::nullptr_t, E_BraceStage stage = E_BraceStage::NONE);
   void pop(const char *func, int line, Chunk *pc);

   using iterator = std::vector<ParenStackEntry>::iterator;
   iterator begin();
   iterator end();

   using const_iterator = std::vector<ParenStackEntry>::const_iterator;
   const_iterator begin() const;
   const_iterator end() const;

   using reverse_iterator = std::vector<ParenStackEntry>::reverse_iterator;
   reverse_iterator rbegin();
   reverse_iterator rend();

   using const_reverse_iterator = std::vector<ParenStackEntry>::const_reverse_iterator;
   const_reverse_iterator rbegin() const;
   const_reverse_iterator rend() const;


protected:
   // Members
   std::vector<ParenStackEntry> m_parenStack;  //! The parenthesis stack
   ParenStackEntry              m_lastPopped;  //! last popped frame or nullptr

   size_t                       m_refNumber;   //! frame reference number
   size_t                       m_parenLevel;  //! level of parens/square/angle/brace
   size_t                       m_braceLevel;  //! level of brace/vbrace
   size_t                       m_ppLevel;     //! level of preproc #if stuff
   size_t                       m_sParenCount; //! count of special parenthesis
   size_t                       m_parenCount;  //! count of parenthesis
   size_t                       m_stmtCount;   //! count of statements
   size_t                       m_exprCount;   //! count of expressions
   E_Token                      m_ifdefType;   //! the ifdef type
};


inline bool ParsingFrame::empty() const
{
   return(m_parenStack.empty());
}


inline size_t ParsingFrame::size() const
{
   return(m_parenStack.size());
}


inline const ParenStackEntry &ParsingFrame::lastPopped() const
{
   return(m_lastPopped);
}


inline size_t ParsingFrame::GetRefNumber() const
{
   return(m_refNumber);
}


inline void ParsingFrame::SetRefNumber(const size_t refNo)
{
   m_refNumber = refNo;
}


inline size_t ParsingFrame::GetParenLevel() const
{
   return(m_parenLevel);
}


inline void ParsingFrame::SetParenLevel(const size_t parenLevel)
{
   m_parenLevel = parenLevel;
}


inline size_t ParsingFrame::GetBraceLevel() const
{
   return(m_braceLevel);
}


inline void ParsingFrame::SetBraceLevel(const size_t braceLevel)
{
   m_braceLevel = braceLevel;
}


inline size_t ParsingFrame::GetPpLevel() const
{
   return(m_ppLevel);
}


inline void ParsingFrame::SetPpLevel(const size_t ppLevel)
{
   m_ppLevel = ppLevel;
}


inline size_t ParsingFrame::GetSParenCount() const
{
   return(m_sParenCount);
}


inline void ParsingFrame::SetSParenCount(const size_t sParenCount)
{
   m_sParenCount = sParenCount;
}


inline size_t ParsingFrame::GetParenCount() const
{
   return(m_parenCount);
}


inline void ParsingFrame::SetParenCount(const size_t parenCount)
{
   m_parenCount = parenCount;
}


inline size_t ParsingFrame::GetStmtCount() const
{
   return(m_stmtCount);
}


inline void ParsingFrame::SetStmtCount(const size_t stmtCount)
{
   m_stmtCount = stmtCount;
}


inline size_t ParsingFrame::GetExprCount() const
{
   return(m_exprCount);
}


inline void ParsingFrame::SetExprCount(const size_t exprCount)
{
   m_exprCount = exprCount;
}


inline E_Token ParsingFrame::GetIfdefType() const
{
   return(m_ifdefType);
}


inline void ParsingFrame::SetIfdefType(const E_Token inIfdef)
{
   m_ifdefType = inIfdef;
}


inline ParenStackEntry &ParsingFrame::at(size_t idx)
{
   return(m_parenStack.at(idx));
}


inline const ParenStackEntry &ParsingFrame::at(size_t idx) const
{
   return(m_parenStack.at(idx));
}


inline ParsingFrame::iterator ParsingFrame::begin()
{
   return(std::begin(m_parenStack));
}


inline ParsingFrame::const_iterator ParsingFrame::begin() const
{
   return(std::begin(m_parenStack));
}


inline ParsingFrame::reverse_iterator ParsingFrame::rbegin()
{
   return(m_parenStack.rbegin());
}


inline ParsingFrame::const_reverse_iterator ParsingFrame::rbegin() const
{
   return(m_parenStack.rbegin());
}


inline ParsingFrame::iterator ParsingFrame::end()
{
   return(std::end(m_parenStack));
}


inline ParsingFrame::const_iterator ParsingFrame::end() const
{
   return(std::end(m_parenStack));
}


inline ParsingFrame::reverse_iterator ParsingFrame::rend()
{
   return(m_parenStack.rend());
}


inline ParsingFrame::const_reverse_iterator ParsingFrame::rend() const
{
   return(m_parenStack.rend());
}


#endif /* PARSING_FRAME_H_INCLUDED */
