/**
 * @file one_liner.h
 * prototype for one_liner.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef ONE_LINER_H_INCLUDED
#define ONE_LINER_H_INCLUDED

#include "chunk.h"


bool is_class_one_liner(Chunk *pc);


void nl_create_list_liner(Chunk *brace_open);


void nl_create_one_liner(Chunk *vbrace_open);


//! Find the next newline or nl_cont
void nl_handle_define(Chunk *pc);


/**
 * Checks to see if it is OK to add a newline around the chunk.
 * Don't want to break one-liners...
 * return value:
 *  true: a new line may be added
 * false: a new line may NOT be added
 */
bool one_liner_nl_ok(Chunk *pc);


/**
 * Clears the PCF_ONE_LINER flag on the current line.
 * Done right before inserting a newline.
 */
void undo_one_liner(Chunk *pc);

#endif /* ONE_LINER_H_INCLUDED */
