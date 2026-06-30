/**
 * @file for_section.h
 * prototype for for_section.cpp
 *
 * @author  Rodrigo Madera
 * @license GPL v2+
 */

#ifndef FOR_SECTION_H_INCLUDED
#define FOR_SECTION_H_INCLUDED

#include "uncrustify_types.h"


/**
 * For a chunk inside the SPAREN of a classic three-section 'for' statement,
 * returns which section the chunk lives in:
 *   0 = init       (before the first ';')
 *   1 = compare    (between the first and second ';')
 *   2 = increment  (after the second ';')
 *
 * Returns -1 if the chunk is not inside a for-SPAREN, or if it is inside a
 * range-based for (which has no semicolons in its SPAREN).
 *
 * Detection walks backward from the chunk to the enclosing
 * CT_SPAREN_OPEN(parent=CT_FOR), counting CT_SEMICOLON(parent=CT_FOR)
 * tokens encountered along the way.
 */
int get_for_section(const Chunk *pc);


#endif /* FOR_SECTION_H_INCLUDED */
