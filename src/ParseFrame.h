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
#include <vector>
#include <memory>


struct paren_stack_entry_t;
struct chunk_t;


//TODO: temp placement, move back to uncrustify_types.h
//! Brace stage enum used in brace_cleanup
enum class brace_stage_e : unsigned int
{
   NONE,
   PAREN1,      //! if/for/switch/while/synchronized
   OP_PAREN1,   //! optional paren: catch () {
   WOD_PAREN,   //! while of do parens
   WOD_SEMI,    //! semicolon after while of do
   BRACE_DO,    //! do
   BRACE2,      //! if/else/for/switch/while
   ELSE,        //! expecting 'else' after 'if'
   ELSEIF,      //! expecting 'if' after 'else'
   WHILE,       //! expecting 'while' after 'do'
   CATCH,       //! expecting 'catch' or 'finally' after 'try'
   CATCH_WHEN,  //! optional 'when' after 'catch'
};


class ParseFrame {
private:
   std::vector<paren_stack_entry_t>     pse;
   std::shared_ptr<paren_stack_entry_t> last_poped; // TODO simplify once paren_stack_entry_t is fully accessible

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

   size_t tos() const;

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
