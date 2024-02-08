/**
 * @file one_liner.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_ONE_LINER_H_INCLUDED
#define NEWLINES_ONE_LINER_H_INCLUDED

class Chunk;

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

#endif /* NEWLINES_ONE_LINER_H_INCLUDED */
