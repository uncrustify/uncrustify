/**
 * @file var_def_brace.h
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_VAR_DEF_BRACE_H_INCLUDED
#define ALIGN_VAR_DEF_BRACE_H_INCLUDED

#include <cstddef>

class Chunk;

/**
 * Scan everything at the current level until the close brace and find the
 * variable def align column.  Also aligns bit-colons, but that assumes that
 * bit-types are the same! But that should always be the case...
 */
Chunk *align_var_def_brace(Chunk *pc, size_t span, size_t *nl_count);

#endif /* ALIGN_VAR_DEF_BRACE_H_INCLUDED */
