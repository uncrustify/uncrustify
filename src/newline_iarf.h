/**
 * @file newline_iarf.h
 * prototypes for newline_iarf.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.h
 * @license GPL v2+
 */

#ifndef NEWLINE_IARF_H_INCLUDED
#define NEWLINE_IARF_H_INCLUDED

#include "chunk.h"

using namespace uncrustify;

/**
 * Does a simple Ignore, Add, Remove, or Force after the given chunk
 *
 * @param pc  The chunk
 * @param av  The IARF value
 */
void newline_iarf(Chunk *pc, iarf_e av);

#endif /* NEWLINE_IARF_H_INCLUDED */
