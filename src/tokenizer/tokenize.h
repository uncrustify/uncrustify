/**
 * @file tokenize.h
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef TOKENIZER_TOKENIZE_H_INCLUDED
#define TOKENIZER_TOKENIZE_H_INCLUDED

#include <cstddef>
#include <deque>

class Chunk;
class UncText;

/**
 * Test the input string to see if it satisfies the criteria
 * specified by the disable_processing_cmt option
 * @param  text      the string to which a match will be attempted
 * @param  start_idx the starting index within the string from which the
 *                   search will be performed
 * @return           returns a non-negative position index that points to the beginning
 *                   of the line containing the marker, if found
 */
int find_disable_processing_comment_marker(const UncText &text, std::size_t start_idx = 0);


/**
 * Test the input string to see if it satisfies the criteria
 * specified by the enable_processing_cmt option
 * @param  text      the string to which a match will be attempted
 * @param  start_idx the starting index within the string from which the
 *                   search will be performed
 * @return           returns a non-negative position index that points to the end
 *                   of the line containing the marker, if found
 */
int find_enable_processing_comment_marker(const UncText &text, std::size_t start_idx = 0);


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
void tokenize(const std::deque<int> &data, Chunk *ref);


#endif /* TOKENIZER_TOKENIZE_H_INCLUDED */
