/**
 * @file newlines_cuddle_uncuddle.h
 * prototype for newlines_cuddle_uncuddle.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINES_CUDDLE_UNCUDDLE_H_INCLUDED
#define NEWLINES_CUDDLE_UNCUDDLE_H_INCLUDED

#include "chunk.h"


using namespace uncrustify;


/**
 * Cuddles or un-cuddles a chunk with a previous close brace
 *
 * "} while" vs "} \n while"
 * "} else" vs "} \n else"
 *
 * @param start  The chunk - should be CT_ELSE or CT_WHILE_OF_DO
 */
void newlines_cuddle_uncuddle(Chunk *start, iarf_e nl_opt);

#endif /* NEWLINES_CUDDLE_UNCUDDLE_H_INCLUDED */
