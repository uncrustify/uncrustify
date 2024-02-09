/**
 * @file brace_cleanup.h
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef TOKENIZER_BRACE_CLEANUP_H_INCLUDED
#define TOKENIZER_BRACE_CLEANUP_H_INCLUDED

/**
 * Scans through the whole list and does stuff.
 * It has to do some tricks to parse preprocessors.
 */
void brace_cleanup();

#endif /* TOKENIZER_BRACE_CLEANUP_H_INCLUDED */
