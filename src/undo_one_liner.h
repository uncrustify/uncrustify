/**
 * @file undo_one_liner.h
 * prototype for undo_one_liner.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.h
 * @license GPL v2+
 */
#ifndef UNDO_ONE_LINER_H_INCLUDED
#define UNDO_ONE_LINER_H_INCLUDED

#include "chunk.h"

/**
 * Clears the PCF_ONE_LINER flag on the current line.
 * Done right before inserting a newline.
 */
void undo_one_liner(Chunk *pc);


#endif /* UNDO_ONE_LINER_H_INCLUDED */
