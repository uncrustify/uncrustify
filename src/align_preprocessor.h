/**
 * @file align_preprocessor.h
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_PREPROCESSOR_H_INCLUDED
#define ALIGN_PREPROCESSOR_H_INCLUDED

#include "uncrustify_types.h"

//! Scans the whole file for #defines. Aligns all within X lines of each other
void align_preprocessor(void);

#endif /* ALIGN_PREPROCESSOR_H_INCLUDED */
