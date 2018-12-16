/**
 * @file align_struct_initializers.h
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_STRUCT_INITIALIZERS_H_INCLUDED
#define ALIGN_STRUCT_INITIALIZERS_H_INCLUDED

#include "uncrustify_types.h"

//! Aligns stuff inside a multi-line "= { ... }" sequence.
void align_struct_initializers(void);

#endif /* ALIGN_STRUCT_INITIALIZERS_H_INCLUDED */
