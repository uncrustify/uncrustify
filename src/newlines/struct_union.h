/**
 * @file struct_union.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_STRUCT_UNION_H_INCLUDED
#define NEWLINES_STRUCT_UNION_H_INCLUDED

#include "option.h"

class Chunk;

void newlines_struct_union(Chunk *start, uncrustify::iarf_e nl_opt, bool leave_trailing);

#endif /* NEWLINES_STRUCT_UNION_H_INCLUDED */
