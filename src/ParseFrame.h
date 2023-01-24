/**
 * @file ParseFrame.h
 *
 * Holds data needed for indenting and brace parsing
 *
 * @author  Daniel Chumak
 * @license GPL v2+
 */

#ifndef SRC_PARSEFRAME_H_
#define SRC_PARSEFRAME_H_

#include "uncrustify_types.h"

#include <memory>


//! Structure describing a parsing frame and its information
class ParsingFrame
{
public:
   E_Token         type;         //! the type that opened the frame
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

class ParsingFrameStack
{
private:
   std::vector<ParsingFrame> pse;
   ParsingFrame              last_poped;

   void clear();

public:
   size_t  ref_no;
   size_t  level;             //! level of parens/square/angle/brace
   size_t  brace_level;       //! level of brace/vbrace
   size_t  pp_level;          //! level of preproc #if stuff
   size_t  sparen_count;
   size_t  paren_count;
   E_Token in_ifdef;
   size_t  stmt_count;
   size_t  expr_count;


   ParsingFrameStack();
   virtual ~ParsingFrameStack() = default;

   bool empty() const;

   ParsingFrame &at(size_t idx);
   const ParsingFrame &at(size_t idx) const;

   ParsingFrame &prev(size_t idx = 1);
   const ParsingFrame &prev(size_t idx = 1) const;

   ParsingFrame &top();
   const ParsingFrame &top() const;

   const ParsingFrame &poped() const;

   void push(Chunk *pc, const char *func, int line, E_BraceStage stage = E_BraceStage::NONE);
   void push(std::nullptr_t, E_BraceStage stage = E_BraceStage::NONE);
   void pop(const char *func, int line, Chunk *pc);

   size_t size() const;

   using iterator = std::vector<ParsingFrame>::iterator;
   iterator begin();
   iterator end();

   using const_iterator = std::vector<ParsingFrame>::const_iterator;
   const_iterator begin() const;
   const_iterator end() const;

   using reverse_iterator = std::vector<ParsingFrame>::reverse_iterator;
   reverse_iterator rbegin();
   reverse_iterator rend();

   using const_reverse_iterator = std::vector<ParsingFrame>::const_reverse_iterator;
   const_reverse_iterator rbegin() const;
   const_reverse_iterator rend() const;
};

#endif /* SRC_PARSEFRAME_H_ */
