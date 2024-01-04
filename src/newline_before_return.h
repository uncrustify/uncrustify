/**
 * @file newline_before_return.h
 * prototype for newline_before_return.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINE_BEFORE_RETURN_H_INCLUDED
#define NEWLINE_BEFORE_RETURN_H_INCLUDED

#include "newline_before_return.h"

#include "chunk.h"
#include "mark_change.h"

#define MARK_CHANGE()    mark_change(__func__, __LINE__)


//! Put a blank line before a return statement, unless it is after an open brace
void newline_before_return(Chunk *start);


#endif /* NEWLINE_BEFORE_RETURN_H_INCLUDED */
