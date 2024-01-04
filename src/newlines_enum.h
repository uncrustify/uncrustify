/**
 * @file newlines_enum.h
 * prototype for newlines_enum.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINES_ENUM_H_INCLUDED
#define NEWLINES_ENUM_H_INCLUDED

#include "chunk.h"

using namespace uncrustify;

// enum {
// enum class angle_state_e : unsigned int {
// enum-key attr(optional) identifier(optional) enum-base(optional) { enumerator-list(optional) }
// enum-key attr(optional) nested-name-specifier(optional) identifier enum-base(optional) ; TODO
// enum-key         - one of enum, enum class or enum struct  TODO
// identifier       - the name of the enumeration that's being declared
// enum-base(C++11) - colon (:), followed by a type-specifier-seq
// enumerator-list  - comma-separated list of enumerator definitions
void newlines_enum(Chunk *start);

#endif /* NEWLINES_ENUM_H_INCLUDED */
