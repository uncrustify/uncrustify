/**
 * @file check_double_brace_init.h
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef TOKENIZER_CHECK_DOUBLE_BRACE_INIT_H_INCLUDED
#define TOKENIZER_CHECK_DOUBLE_BRACE_INIT_H_INCLUDED

class Chunk;

/**
 * Combines two tokens into {{ and }} if inside parens and nothing is between
 * either pair.
 */
void check_double_brace_init(Chunk *bo1);

#endif /* TOKENIZER_CHECK_DOUBLE_BRACE_INIT_H_INCLUDED */
