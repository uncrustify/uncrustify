/**
 * @file indent.h
 * prototypes for indent.cpp
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef INDENT_H_INCLUDED
#define INDENT_H_INCLUDED

#include "uncrustify_types.h"


/**
 * Change the top-level indentation only by changing the column member in
 * the chunk structures.
 * The level indicator must already be set.
 */
void indent_text();


/**
 * Indent the preprocessor stuff from column 1.
 * FIXME: This is broken if there is a comment or escaped newline
 * between '#' and 'define'.
 */
void indent_preproc();

/**
 *
 * @param pc      chunk at the start of the line
 * @param column  desired column
 */
void indent_to_column(Chunk *pc, size_t column);


/**
 * Same as indent_to_column, except we can move both ways
 *
 * @param pc      chunk at the start of the line
 * @param column  desired column
 */
void align_to_column(Chunk *pc, size_t column);


#endif /* INDENT_H_INCLUDED */
