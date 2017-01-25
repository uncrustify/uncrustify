/**
 * @file brace_cleanup.h
 * prototypes for brace_cleanup.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef BRACE_CLEANUP_H_INCLUDED
#define BRACE_CLEANUP_H_INCLUDED

#include "uncrustify_types.h"


/**
 * Scans through the whole list and does stuff.
 * It has to do some tricks to parse preprocessors.
 *
 * TODO: This can be cleaned up and simplified - we can look both forward and backward!
 */
void brace_cleanup(void);

#endif /* BRACE_CLEANUP_H_INCLUDED */
