/**
 * @file combine.h
 * prototypes for combine.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef COMBINE_H_INCLUDED
#define COMBINE_H_INCLUDED

#include "uncrustify_types.h"
#include "chunk_list.h"


/**
 * Change CT_INCDEC_AFTER + WORD to CT_INCDEC_BEFORE
 * Change number/word + CT_ADDR to CT_ARITH
 * Change number/word + CT_STAR to CT_ARITH
 * Change number/word + CT_NEG to CT_ARITH
 * Change word + ( to a CT_FUNCTION
 * Change struct/union/enum + CT_WORD => CT_TYPE
 * Force parens on return.
 *
 * TODO: This could be done earlier.
 *
 * Patterns detected:
 *   STRUCT/ENUM/UNION + WORD :: WORD => TYPE
 *   WORD + '('               :: WORD => FUNCTION
 */
void fix_symbols(void);


/**
 * Examines the whole file and changes CT_COLON to
 * CT_Q_COLON, CT_LABEL_COLON, or CT_CASE_COLON.
 * It also changes the CT_WORD before CT_LABEL_COLON into CT_LABEL.
 */
void combine_labels(void);


/**
 * Sets the parent for comments.
 */
void mark_comments(void);


void make_type(chunk_t *pc);


void flag_series(chunk_t *start, chunk_t *end, UINT64 set_flags, UINT64 clr_flags = 0, scope_e nav = scope_e::ALL);


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


/**
 * Skips the rest of the array definitions if ary_def is indeed a
 * CT_TSQUARE or CT_SQUARE_OPEN
 */
chunk_t *skip_tsquare_next(chunk_t *ary_def);


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
 * Remove 'return;' that appears as the last statement in a function
 */
void remove_extra_returns(void);

#endif /* COMBINE_H_INCLUDED */
