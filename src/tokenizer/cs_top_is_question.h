/**
 * @file cs_top_is_question.h
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#ifndef TOKENIZER_CS_TOP_IS_QUESTION_H_INCLUDED
#define TOKENIZER_CS_TOP_IS_QUESTION_H_INCLUDED

#include <cstddef>

class ChunkStack;

bool cs_top_is_question(ChunkStack &cs, size_t level);

#endif /* TOKENIZER_CS_TOP_IS_QUESTION_H_INCLUDED */
