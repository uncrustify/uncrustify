/**
 * @file flag_parens.h
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#ifndef FLAG_PARENS_H_INCLUDED
#define FLAG_PARENS_H_INCLUDED

#include "chunk_list.h"


/**
 * Flags everything from the open paren to the close paren.
 *
 * @param po          Pointer to the open parenthesis
 * @param flags       flags to add
 * @param opentype
 * @param parenttype
 * @param parent_all
 *
 * @return The token after the close paren
 */
chunk_t *flag_parens(chunk_t *po, UINT64 flags, c_token_t opentype, c_token_t parenttype, bool parent_all);


#endif /* FLAG_PARENS_H_INCLUDED */
