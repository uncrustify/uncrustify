/**
 * @file tokenize_cleanup.h
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef TOKENIZER_TOKENIZE_CLEANUP_H_INCLUDED
#define TOKENIZER_TOKENIZE_CLEANUP_H_INCLUDED

class Chunk;

/**
 * @brief clean up tokens
 *
 * Change certain token types based on simple sequence.
 * Example: change '[' + ']' to '[]'
 * Note that level info is not yet available, so it is OK to do all
 * processing that doesn't need to know level info. (that's very little!)
 */
void tokenize_cleanup();

void tokenize_trailing_return_types();

void split_off_angle_close(Chunk *pc);

#endif /* TOKENIZER_TOKENIZE_CLEANUP_H_INCLUDED */
