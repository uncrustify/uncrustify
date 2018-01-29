/**
 * @file ParseFrame.cpp
 *
 * Container that holds data needed for indenting and brace parsing
 *
 * @author  Daniel Chumak
 * @license GPL v2+
 */

#ifndef SRC_PARSEFRAME_H_
#define SRC_PARSEFRAME_H_

#include "token_enum.h"
#include "uncrustify_types.h"
#include <vector>
#include <memory>


//! Structure for counting nested level
struct paren_stack_entry_t
{
   c_token_t     type;         //! the type that opened the entry
   size_t        level;        //! Level of opening type
   size_t        open_line;    //! line that open symbol is on, only for logging purposes
   chunk_t       *pc;          //! Chunk that opened the level, TODO: make const
   size_t        brace_indent; //! indent for braces - may not relate to indent
   size_t        indent;       //! indent level (depends on use)
   size_t        indent_tmp;   //! temporary indent level (depends on use)
   size_t        indent_tab;   //! the 'tab' indent (always <= real column)
   bool          indent_cont;  //! indent_continue was applied
   c_token_t     parent;       //! if, for, function, etc
   brace_stage_e stage;        //! used to check progression of complex statements.
   bool          in_preproc;   //! whether this was created in a preprocessor
   size_t        ns_cnt;       //! Number of consecutive namespace levels
   bool          non_vardef;   //! Hit a non-vardef line
   indent_ptr_t  ip;
   chunk_t       *pop_pc;
};

class ParseFrame {
private:
   std::vector<paren_stack_entry_t> pse;
   paren_stack_entry_t              last_poped;

   void clear();

public:
   size_t    ref_no;
   size_t    level;           //! level of parens/square/angle/brace
   size_t    brace_level;     //! level of brace/vbrace
   size_t    pp_level;        //! level of preproc #if stuff
   size_t    sparen_count;
   size_t    paren_count;
   c_token_t in_ifdef;
   size_t    stmt_count;
   size_t    expr_count;


   ParseFrame();
   virtual ~ParseFrame() = default;

   bool empty() const;

   paren_stack_entry_t &      at(size_t idx);
   const paren_stack_entry_t &at(size_t idx) const;

   paren_stack_entry_t &      prev(size_t idx = 1);
   const paren_stack_entry_t &prev(size_t idx = 1) const;

   paren_stack_entry_t &      top();
   const paren_stack_entry_t &top() const;

   const paren_stack_entry_t &poped() const;

   void push(chunk_t &pc, brace_stage_e stage = brace_stage_e::NONE);
   void pop();

   size_t size() const;

   using iterator = std::vector<paren_stack_entry_t>::iterator;
   iterator begin();
   iterator end();

   using const_iterator = std::vector<paren_stack_entry_t>::const_iterator;
   const_iterator begin() const;
   const_iterator end() const;

   using reverse_iterator = std::vector<paren_stack_entry_t>::reverse_iterator;
   reverse_iterator rbegin();
   reverse_iterator rend();

   using const_reverse_iterator = std::vector<paren_stack_entry_t>::const_reverse_iterator;
   const_reverse_iterator rbegin() const;
   const_reverse_iterator rend() const;
};

#endif /* SRC_PARSEFRAME_H_ */
