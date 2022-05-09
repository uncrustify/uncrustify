/**
 * @file combine_skip.h
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#ifndef COMBINE_SKIP_H_INCLUDED
#define COMBINE_SKIP_H_INCLUDED

#include "chunk.h"


/**
 * Skips the D 'align()' statement and the colon, if present.
 *    align(2) int foo;  -- returns 'int'
 *    align(4):          -- returns 'int'
 *    int bar;
 */
Chunk *skip_align(Chunk *start);


/**
 * Skips chunks in the forward direction and attempts to find the
 * chunk associated with the end of the current expression; returns
 * the first chunk to satisfy one of the following:
 * 1) Chunk is a comma or semicolon at the level of the starting chunk
 * 2) Preceding chunk is at higher template nest level relative to the
 *    current chunk under test
 */
Chunk *skip_expression(Chunk *pc);


/**
 * Skips chunks in the reverse direction and attempts to find the
 * chunk associated with the start of the current expression; returns
 * the first chunk to satisfy one of the following:
 * 1) Chunk is a comma or semicolon at the level of the starting chunk
 * 2) Preceding chunk is at lower template nest level relative to the
 *    current chunk under test
 */
Chunk *skip_expression_rev(Chunk *pc);


/**
 * Skips chunks in the forward direction and attempts to find the
 * chunk associated with the end of the current expression; specifically,
 * the function returns that which immediately precedes a chunk
 * satisfying one of the following:
 * 1) Next chunk is a comma or semicolon at the level of the starting chunk
 * 2) Preceding chunk is at a higher template nest level relative to the
 *    subsequent chunk
 */
Chunk *skip_to_expression_end(Chunk *pc);


/**
 * Skips chunks in the reverse direction and attempts to find the chunk
 * associated with the start of the current expression; specifically,
 * the function returns that which immediately follows a chunk
 * satisfying one of the following:
 * 1) Prior chunk is a comma or semicolon at the level of the starting chunk
 * 2) Preceding chunk is at a lower template nest level relative to the
 *    subsequent chunk
 */
Chunk *skip_to_expression_start(Chunk *pc);


/**
 * Skips over the rest of the template if ang_open is indeed a CT_ANGLE_OPEN.
 * Points to the chunk after the CT_ANGLE_CLOSE.
 * If the chunk isn't an CT_ANGLE_OPEN, then it is returned.
 */
Chunk *skip_template_next(Chunk *ang_open);


/**
 * Skips over the rest of the template if ang_close is indeed a CT_ANGLE_CLOSE.
 * Points to the chunk before the CT_ANGLE_OPEN
 * If the chunk isn't an CT_ANGLE_CLOSE, then it is returned.
 */
Chunk *skip_template_prev(Chunk *ang_close);


//! Skips to the start of the next statement.
Chunk *skip_to_next_statement(Chunk *pc);


/**
 * Skips the rest of the array definitions if ary_def is indeed a
 * CT_TSQUARE or CT_SQUARE_OPEN
 */
Chunk *skip_tsquare_next(Chunk *ary_def);


/**
 * If pc is CT_ATTRIBUTE, then skip it and everything preceding the closing
 * paren; return the chunk marked CT_FPAREN_CLOSE
 * If the chunk isn't a CT_ATTRIBUTE, then it is returned.
 */
Chunk *skip_attribute(Chunk *attr);


/**
 * If attr is CT_ATTRIBUTE, then skip it and the parens and return the chunk
 * after the CT_FPAREN_CLOSE.
 * If the chunk isn't an CT_ATTRIBUTE, then it is returned.
 */
Chunk *skip_attribute_next(Chunk *attr);


/**
 * If fp_close is a CT_FPAREN_CLOSE with a parent of CT_ATTRIBUTE, then skip it
 * and the '__attribute__' thingy and return the chunk before CT_ATTRIBUTE.
 * Otherwise return fp_close.
 */
Chunk *skip_attribute_prev(Chunk *fp_close);


/**
 * If pc is CT_DECLSPEC, then skip it and everything preceding the closing
 * paren; return the chunk marked CT_FPAREN_CLOSE
 * If the chunk isn't a CT_DECLSPEC, then it is returned.
 */
Chunk *skip_declspec(Chunk *pc);


/**
 * If pc is CT_DECLSPEC, then skip it and the parens and return the chunk
 * after the CT_FPAREN_CLOSE.
 * If the chunk isn't a CT_DECLSPEC, then it is returned.
 */
Chunk *skip_declspec_next(Chunk *pc);


#endif /* COMBINE_SKIP_H_INCLUDED */
