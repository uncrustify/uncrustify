/**
 * @file flag_parens.h
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#ifndef TOKENIZER_FLAG_PARENS_H_INCLUDED
#define TOKENIZER_FLAG_PARENS_H_INCLUDED

#include "pcf_flags.h"
#include "token_enum.h"

class Chunk;

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
Chunk *flag_parens(Chunk *po, PcfFlags flags, E_Token opentype, E_Token parenttype, bool parent_all);

#endif /* TOKENIZER_FLAG_PARENS_H_INCLUDED */
