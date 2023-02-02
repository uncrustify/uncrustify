/**
 * @file parsing_frame_stack.h
 * mainly used to handle preprocessor stuff
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef PARSING_FRAME_STACK_H_INCLUDED
#define PARSING_FRAME_STACK_H_INCLUDED

#include "parsing_frame.h"

//! Class describing a parsing frame stack
class ParsingFrameStack
{
public:
   ParsingFrameStack();

   /**
    * Push a copy of a ParsingFrame onto the frame stack.
    */
   void push(ParsingFrame &frm);


   /**
    * Pop and return the top element of the frame stack.
    * TODO: return the frame rather than passing it as argument
    */
   void pop(ParsingFrame &pf);


   // TODO: this name is dumb:
   // - what is it checking?
   // - why does is much more than simple checks, it allters kinds of stuff
   //! Returns the pp_indent to use for this line
   int check(ParsingFrame &frm, int &pp_level, Chunk *pc);

private:
   std::vector<ParsingFrame> m_frames;
};

#endif /* PARSING_FRAME_STACK_H_INCLUDED */
