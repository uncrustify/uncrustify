/**
 * @file newlines_struct_union.h
 * prototype for newlines_struct_union.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINES_STRUCT_UNION_H_INCLUDED
#define NEWLINES_STRUCT_UNION_H_INCLUDED

#include "newlines_struct_union.h"

#include "chunk.h"


using namespace uncrustify;


void newlines_struct_union(Chunk *start, iarf_e nl_opt, bool leave_trailing);

#endif /* NEWLINES_STRUCT_UNION_H_INCLUDED */
