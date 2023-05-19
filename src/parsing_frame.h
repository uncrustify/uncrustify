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

   // --------- Access methods

   /**
    * @brief returns the token that opened the entry
    */
   E_Token GetOpenToken() const;

   /**
    * @brief Sets the token that opened the entry
    * @param token the token to set
    */
   void SetOpenToken(const E_Token token);

   /**
    * @brief returns the chunk that opened the entry
    */
   Chunk *GetOpenChunk() const;

   /**
    * @brief Sets the chunk that opened the entry
    * @param chunk the chunk to set
    */
   void SetOpenChunk(Chunk *chunk);

   /**
    * @brief returns the level that opened the entry
    */
   size_t GetOpenLevel() const;

   /**
    * @brief Sets the level that opened the entry
    * @param level the level to set
    */
   void SetOpenLevel(size_t level);

   /**
    * @brief returns the line that opened the entry
    */
   size_t GetOpenLine() const;

   /**
    * @brief Sets the line that opened the entry
    * @param line the line to set
    */
   void SetOpenLine(size_t line);

   /**
    * @brief returns the column that opened the entry
    */
   size_t GetOpenCol() const;

   /**
    * @brief Sets the column that opened the entry
    * @param column the column to set
    */
   void SetOpenCol(size_t column);

   /**
    * @brief returns the indent for braces
    */
   size_t GetBraceIndent() const;

   /**
    * @brief Sets the indent for braces
    * @param indent the indent for braces
    */
   void SetBraceIndent(size_t indent);

   /**
    * @brief returns the indent level
    */
   size_t GetIndent() const;

   /**
    * @brief Sets the indent level
    * @param level the indent level
    */
   void SetIndent(size_t level);

   /**
    * @brief returns the temporary indent level
    */
   size_t GetIndentTmp() const;

   /**
    * @brief Sets the temporary indent level
    * @param level the temporary indent level
    */
   void SetIndentTmp(size_t level);

   /**
    * @brief returns the tab indent level
    */
   size_t GetIndentTab() const;

   /**
    * @brief Sets the tab indent level
    * @param level the tab indent level
    */
   void SetIndentTab(size_t level);

   /**
    * @brief returns the consecutive namespace levels
    */
   size_t GetNsCount() const;

   /**
    * @brief Sets the consecutive namespace levels
    * @param count the consecutive namespace levels
    */
   void SetNsCount(size_t count);

   /**
    * @brief returns whether indent_continue was applied
    */
   bool GetIndentContinue() const;

   /**
    * @brief Sets whether indent_continue was applied
    * @param cont new value
    */
   void SetIndentContinue(bool cont);

   /**
    * @brief returns whether this was created in a preprocessor
    */
   bool GetInPreproc() const;

   /**
    * @brief Sets whether this was created in a preprocessor
    * @param preproc new value
    */
   void SetInPreproc(bool preproc);

   /**
    * @brief returns whether a non-vardef line was hit
    */
   bool GetNonVardef() const;

   /**
    * @brief Sets whether a non-vardef line was hit
    * @param vardef new value
    */
   void SetNonVardef(bool vardef);

   /**
    * @brief returns the parent token (if, for, function, etc)
    */
   E_Token GetParent() const;

   /**
    * @brief Sets the parent token (if, for, function, etc)
    * @param parent the token to set
    */
   void SetParent(const E_Token parent);

   /**
    * @brief returns the stage used to check progression of complex statements
    */
   E_BraceStage GetStage() const;

   /**
    * @brief Sets the stage used to check progression of complex statements
    * @param stage the new value
    */
   void SetStage(const E_BraceStage stage);

   /**
    * @brief Returns the associated indentation data as a const reference
    */
   const IndentationData &GetIndentData() const;

   /**
    * @brief Returns the associated indentation data as a modifiable reference
    */
   IndentationData &IndentData();

   /**
    * @brief returns the pop chunk
    */
   Chunk *GetPopChunk() const;

   /**
    * @brief Sets the pop chunk
    * @param chunk the new chunk
    */
   void SetPopChunk(Chunk *chunk);


protected:
   E_Token         m_openToken;       // the type that opened the entry
   Chunk           *m_openChunk;      // chunk that opened the level
   size_t          m_openLevel;       // level of opening type
   size_t          m_openLine;        // line that open symbol is on, only for logging purposes
   size_t          m_openCol;         // column that open symbol is on, only for logging purposes
   size_t          m_braceIndent;     // indent for braces - may not relate to indent
   size_t          m_indent;          // indent level (depends on use)
   size_t          m_indentTmp;       // temporary indent level (depends on use)
   size_t          m_indentTab;       // the 'tab' indent (always <= real column)
   size_t          m_nsCount;         // Number of consecutive namespace levels
   bool            m_indentContinue;  // indent_continue was applied
   bool            m_inPreproc;       // whether this was created in a preprocessor
   bool            m_nonVardef;       // Hit a non-vardef line
   E_Token         m_parent;          // if, for, function, etc
   E_BraceStage    m_stage;           // used to check progression of complex statements.
   IndentationData m_indentationData; // Indentation data
   Chunk           *m_popChunk;       // Pop chunk
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


// ------------------------------
// ParenStackEntry inline methods
// ------------------------------
inline E_Token ParenStackEntry::GetOpenToken() const
{
   return(m_openToken);
}


inline void ParenStackEntry::SetOpenToken(const E_Token token)
{
   m_openToken = token;
}


inline Chunk *ParenStackEntry::GetOpenChunk() const
{
   return(m_openChunk);
}


inline void ParenStackEntry::SetOpenChunk(Chunk *chunk)
{
   m_openChunk = chunk;
}


inline size_t ParenStackEntry::GetOpenLevel() const
{
   return(m_openLevel);
}


inline void ParenStackEntry::SetOpenLevel(size_t level)
{
   m_openLevel = level;
}


inline size_t ParenStackEntry::GetOpenLine() const
{
   return(m_openLine);
}


inline void ParenStackEntry::SetOpenLine(size_t line)
{
   m_openLine = line;
}


inline size_t ParenStackEntry::GetOpenCol() const
{
   return(m_openCol);
}


inline void ParenStackEntry::SetOpenCol(size_t column)
{
   m_openCol = column;
}


inline size_t ParenStackEntry::GetBraceIndent() const
{
   return(m_braceIndent);
}


inline void ParenStackEntry::SetBraceIndent(size_t indent)
{
   m_braceIndent = indent;
}


inline size_t ParenStackEntry::GetIndent() const
{
   return(m_indent);
}


inline void ParenStackEntry::SetIndent(size_t level)
{
   m_indent = level;
}


inline size_t ParenStackEntry::GetIndentTmp() const
{
   return(m_indentTmp);
}


inline void ParenStackEntry::SetIndentTmp(size_t level)
{
   m_indentTmp = level;
}


inline size_t ParenStackEntry::GetIndentTab() const
{
   return(m_indentTab);
}


inline void ParenStackEntry::SetIndentTab(size_t level)
{
   m_indentTab = level;
}


inline size_t ParenStackEntry::GetNsCount() const
{
   return(m_nsCount);
}


inline void ParenStackEntry::SetNsCount(size_t count)
{
   m_nsCount = count;
}


inline bool ParenStackEntry::GetIndentContinue() const
{
   return(m_indentContinue);
}


inline void ParenStackEntry::SetIndentContinue(bool cont)
{
   m_indentContinue = cont;
}


inline bool ParenStackEntry::GetInPreproc() const
{
   return(m_inPreproc);
}


inline void ParenStackEntry::SetInPreproc(bool preproc)
{
   m_inPreproc = preproc;
}


inline bool ParenStackEntry::GetNonVardef() const
{
   return(m_nonVardef);
}


inline void ParenStackEntry::SetNonVardef(bool vardef)
{
   m_nonVardef = vardef;
}


inline E_Token ParenStackEntry::GetParent() const
{
   return(m_parent);
}


inline void ParenStackEntry::SetParent(const E_Token parent)
{
   m_parent = parent;
}


inline E_BraceStage ParenStackEntry::GetStage() const
{
   return(m_stage);
}


inline void ParenStackEntry::SetStage(const E_BraceStage stage)
{
   m_stage = stage;
}


inline const IndentationData &ParenStackEntry::GetIndentData() const
{
   return(m_indentationData);
}


inline IndentationData &ParenStackEntry::IndentData()
{
   return(m_indentationData);
}


inline Chunk *ParenStackEntry::GetPopChunk() const
{
   return(m_popChunk);
}


inline void ParenStackEntry::SetPopChunk(Chunk *chunk)
{
   m_popChunk = chunk;
}


// ------------------------------
// ParsingFrame inline methods
// ------------------------------
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
