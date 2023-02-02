/**
 * @file brace_cleanup.h
 * prototypes for brace_cleanup.cpp
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef BRACE_CLEANUP_H_INCLUDED
#define BRACE_CLEANUP_H_INCLUDED

#include "uncrustify_types.h"
// necessary to not sort
#include "parsing_frame.h"

/**
 * Scans through the whole list and does stuff.
 * It has to do some tricks to parse preprocessors.
 */
void brace_cleanup();


#endif /* BRACE_CLEANUP_H_INCLUDED */
