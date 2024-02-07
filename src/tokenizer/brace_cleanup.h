/**
 * @file brace_cleanup.h
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef TOKENIZER_BRACE_CLEANUP_H_INCLUDED
#define TOKENIZER_BRACE_CLEANUP_H_INCLUDED

#include "uncrustify_types.h"
// necessary to not sort
#include "parsing_frame.h"

/**
 * Scans through the whole list and does stuff.
 * It has to do some tricks to parse preprocessors.
 */
void brace_cleanup();


#endif /* TOKENIZER_BRACE_CLEANUP_H_INCLUDED */
