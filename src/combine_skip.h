/**
 * @file combine_skip.h
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#ifndef COMBINE_SKIP_H_INCLUDED
#define COMBINE_SKIP_H_INCLUDED

#include "chunk_list.h"


/**
 * Skips the D 'align()' statement and the colon, if present.
 *    align(2) int foo;  -- returns 'int'
 *    align(4):          -- returns 'int'
 *    int bar;
 */
chunk_t *skip_align(chunk_t *start);


/**
 * Skips chunks in the forward direction and attempts to find the
 * chunk associated with the end of the current expression; returns
 * the first chunk to satisfy one of the following:
 * 1) Chunk is a comma or semicolon at the level of the starting chunk
 * 2) Preceding chunk is at higher template nest level relative to the
 *    current chunk under test
 */
chunk_t *skip_expression(chunk_t *pc);


/**
 * Skips chunks in the reverse direction and attempts to find the
 * chunk associated with the start of the current expression; returns
 * the first chunk to satisfy one of the following:
 * 1) Chunk is a comma or semicolon at the level of the starting chunk
 * 2) Preceding chunk is at lower template nest level relative to the
 *    current chunk under test
 */
chunk_t *skip_expression_rev(chunk_t *pc);


/**
 * Skips chunks in the forward direction and attempts to find the
 * chunk associated with the end of the current expression; specifically,
 * the function returns that which immediately precedes a chunk
 * satisfying one of the following:
 * 1) Next chunk is a comma or semicolon at the level of the starting chunk
 * 2) Preceding chunk is at a higher template nest level relative to the
 *    subsequent chunk
 */
chunk_t *skip_to_expression_end(chunk_t *pc);


/**
 * Skips chunks in the reverse direction and attempts to find the chunk
 * associated with the start of the current expression; specifically,
 * the function returns that which immediately follows a chunk
 * satisfying one of the following:
 * 1) Prior chunk is a comma or semicolon at the level of the starting chunk
 * 2) Preceding chunk is at a lower template nest level relative to the
 *    subsequent chunk
 */
chunk_t *skip_to_expression_start(chunk_t *pc);


/**
 * Skips the list of class/struct parent types.
 */
chunk_t *skip_parent_types(chunk_t *colon);


/**
 * Skips over the rest of the template if ang_open is indeed a CT_ANGLE_OPEN.
 * Points to the chunk after the CT_ANGLE_CLOSE.
 * If the chunk isn't an CT_ANGLE_OPEN, then it is returned.
 */
chunk_t *skip_template_next(chunk_t *ang_open);


/**
 * Skips over the rest of the template if ang_close is indeed a CT_ANGLE_CLOSE.
 * Points to the chunk before the CT_ANGLE_OPEN
 * If the chunk isn't an CT_ANGLE_CLOSE, then it is returned.
 */
chunk_t *skip_template_prev(chunk_t *ang_close);


//! Skips to the start of the next statement.
chunk_t *skip_to_next_statement(chunk_t *pc);


/**
 * Skips the rest of the array definitions if ary_def is indeed a
 * CT_TSQUARE or CT_SQUARE_OPEN
 */
chunk_t *skip_tsquare_next(chunk_t *ary_def);


/**
 * If pc is CT_ATTRIBUTE, then skip it and everything preceding the closing
 * paren; return the chunk marked CT_FPAREN_CLOSE
 * If the chunk isn't a CT_ATTRIBUTE, then it is returned.
 */
chunk_t *skip_attribute(chunk_t *attr);


/**
 * If attr is CT_ATTRIBUTE, then skip it and the parens and return the chunk
 * after the CT_FPAREN_CLOSE.
 * If the chunk isn't an CT_ATTRIBUTE, then it is returned.
 */
chunk_t *skip_attribute_next(chunk_t *attr);


/**
 * If fp_close is a CT_FPAREN_CLOSE with a parent of CT_ATTRIBUTE, then skip it
 * and the '__attribute__' thingy and return the chunk before CT_ATTRIBUTE.
 * Otherwise return fp_close.
 */
chunk_t *skip_attribute_prev(chunk_t *fp_close);


/**
 * If pc is CT_DECLSPEC, then skip it and everything preceding the closing
 * paren; return the chunk marked CT_FPAREN_CLOSE
 * If the chunk isn't a CT_DECLSPEC, then it is returned.
 */
chunk_t *skip_declspec(chunk_t *pc);


/**
 * If pc is CT_DECLSPEC, then skip it and the parens and return the chunk
 * after the CT_FPAREN_CLOSE.
 * If the chunk isn't a CT_DECLSPEC, then it is returned.
 */
chunk_t *skip_declspec_next(chunk_t *pc);


/**
 * If pc is a CT_FPAREN_CLOSE with a parent of CT_DECLSPEC, then skip it
 * and the '__declspec' keyword and return the chunk before CT_DECLSPEC.
 * Otherwise return pc.
 */
chunk_t *skip_declspec_prev(chunk_t *pc);


/**
 * If pc is a CT_BRACE_OPEN, CT_PAREN_OPEN or CT_SQUARE_OPEN, then skip
 * forward to the next non-comment/non-newline chunk following the matching
 * CT_BRACE_CLOSE, CT_PAREN_CLOSE or CT_SQUARE_CLOSE; if pc is none of these
 * upon calling this function, then pc is returned.
 */
chunk_t *skip_matching_brace_bracket_paren_next(chunk_t *pc);


/**
 * If pc is a CT_BRACE_CLOSE, CT_PAREN_CLOSE or CT_SQUARE_CLOSE, then skip
 * in reverse to the first non-comment/non-newline chunk preceding the matching
 * CT_BRACE_OPEN, CT_PAREN_OPEN or CT_SQUARE_OPEN; if pc is none of these upon
 * calling this function, then pc is returned.
 */
chunk_t *skip_to_chunk_before_matching_brace_bracket_paren_rev(chunk_t *pc);


#endif /* COMBINE_SKIP_H_INCLUDED */
