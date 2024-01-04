/**
 * @file newlines_enum_entries.h
 * prototype for newlines_enum_entries.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINES_ENUM_ENTRIES_H_INCLUDED
#define NEWLINES_ENUM_ENTRIES_H_INCLUDED

#include "newlines_enum_entries.h"

#include "chunk.h"

using namespace uncrustify;

//! If requested, make sure each entry in an enum is on its own line
void newlines_enum_entries(Chunk *open_brace, iarf_e av);

#endif /* NEWLINES_ENUM_ENTRIES_H_INCLUDED */
