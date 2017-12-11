/**
 * @file tokenize.h
 * prototypes for tokenize.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef TOKENIZE_H_INCLUDED
#define TOKENIZE_H_INCLUDED

#include "uncrustify_types.h"


/**
 * @brief Parse the text into chunks
 *
 * This function parses or tokenizes the whole buffer into a list.
 * It has to do some tricks to parse preprocessors.
 *
 * If output_text() were called immediately after, two things would happen:
 *  - trailing whitespace are removed.
 *  - leading space & tabs are converted to the appropriate format.
 *
 * All the tokens are inserted before ref. If ref is NULL, they are inserted
 * at the end of the list.  Line numbers are relative to the start of the data.
 */
void tokenize(const std::deque<int> &data, chunk_t *ref);


#endif /* TOKENIZE_H_INCLUDED */
