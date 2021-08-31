/**
 * @file using_type_alias.h
 * Look after a Type alias
 * https://en.cppreference.com/w/cpp/language/type_alias
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#ifndef USING_TYPE_ALIAS_H_INCLUDED
#define USING_TYPE_ALIAS_H_INCLUDED

#include "uncrustify_types.h"


static bool    using_found     = false;
static bool    candidate_found = false;
static bool    assign_found    = false;
static bool    type_found      = false;
static chunk_t *candidate      = nullptr;

void using_type_alias(chunk_t chunk);

#endif /* USING_TYPE_ALIAS_H_INCLUDED */
