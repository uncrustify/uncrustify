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

#include <cstddef>
#include <memory>
#include <vector>


//! Class describing a parenthesis stack entry and its information
class ParenStackEntry
{
public:
   E_Token         type;         //! the type that opened the entry
   size_t          level;        //! level of opening type
   size_t          open_line;    //! line that open symbol is on, only for logging purposes
   size_t          open_colu;    //! column that open symbol is on, only for logging purposes
   Chunk           *pc;          //! chunk that opened the level, TODO: make const
   size_t          brace_indent; //! indent for braces - may not relate to indent
   size_t          indent;       //! indent level (depends on use)
   size_t          indent_tmp;   //! temporary indent level (depends on use)
   size_t          indent_tab;   //! the 'tab' indent (always <= real column)
   bool            indent_cont;  //! indent_continue was applied
   E_Token         parent;       //! if, for, function, etc
   E_BraceStage    stage;        //! used to check progression of complex statements.
   bool            in_preproc;   //! whether this was created in a preprocessor
   size_t          ns_cnt;       //! Number of consecutive namespace levels
   bool            non_vardef;   //! Hit a non-vardef line
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


   // Members
   size_t  pp_level;          //! level of preproc #if stuff
   size_t  sparen_count;
   size_t  paren_count;
   E_Token in_ifdef;
   size_t  stmt_count;
   size_t  expr_count;


protected:
   void clear();

   // Members
   std::vector<ParenStackEntry> m_parenStack;
   ParenStackEntry              m_lastPopped;

   size_t                       m_refNumber;  //! frame reference number
   size_t                       m_parenLevel; //! level of parens/square/angle/brace
   size_t                       m_braceLevel; //! level of brace/vbrace
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
