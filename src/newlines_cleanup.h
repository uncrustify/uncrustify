/**
 * @file newlines_cleanup.h
 * prototype for newlines_cleanup.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINES_CLEANUP_H_INCLUDED
#define NEWLINES_CLEANUP_H_INCLUDED

//#include "chunk.h"

void newlines_cleanup_angles();

//! Step through all chunks.
void newlines_cleanup_braces(bool first);

void newlines_cleanup_dup();

#endif /* NEWLINES_CLEANUP_H_INCLUDED */
