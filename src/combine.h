/**
 * @file combine.h
 * prototypes for combine.cpp
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef COMBINE_H_INCLUDED
#define COMBINE_H_INCLUDED

#include "chunk_list.h"
#include "uncrustify_types.h"


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


//! help function for mark_variable_definition...
bool go_on(chunk_t *pc, chunk_t *start);


//! Sets the parent for comments.
void mark_comments(void);


void make_type(chunk_t *pc);


/**
 * Sets the parent of the open paren/brace/square/angle and the closing.
 * Note - it is assumed that pc really does point to an open item and the
 * close must be open + 1.
 *
 * @param start   The open paren
 * @param parent  The type to assign as the parent
 *
 * @return The chunk after the close paren
 */
chunk_t *set_paren_parent(chunk_t *start, c_token_t parent);


/**
 * This is called on every chunk.
 * First on all non-preprocessor chunks and then on each preprocessor chunk.
 * It does all the detection and classifying.
 * This is only called by fix_symbols.
 * The three parameters never get the value nullptr.
 * it is not necessary to test.
 */
void do_symbol_check(chunk_t *prev, chunk_t *pc, chunk_t *next);


#endif /* COMBINE_H_INCLUDED */
