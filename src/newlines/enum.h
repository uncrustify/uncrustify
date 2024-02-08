/**
 * @file enum.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_ENUM_H_INCLUDED
#define NEWLINES_ENUM_H_INCLUDED

#include "option.h"

class Chunk;

// enum {
// enum class angle_state_e : unsigned int {
// enum-key attr(optional) identifier(optional) enum-base(optional) { enumerator-list(optional) }
// enum-key attr(optional) nested-name-specifier(optional) identifier enum-base(optional) ; TODO
// enum-key         - one of enum, enum class or enum struct  TODO
// identifier       - the name of the enumeration that's being declared
// enum-base(C++11) - colon (:), followed by a type-specifier-seq
// enumerator-list  - comma-separated list of enumerator definitions
void newlines_enum(Chunk *start);

//! If requested, make sure each entry in an enum is on its own line
void newlines_enum_entries(Chunk *open_brace, uncrustify::iarf_e av);

#endif /* NEWLINES_ENUM_H_INCLUDED */
