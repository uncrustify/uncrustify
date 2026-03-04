/**
 * @file combine.h
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef TOKENIZER_COMBINE_H_INCLUDED
#define TOKENIZER_COMBINE_H_INCLUDED

#include "token_enum.h"

class Chunk;


/**
 * Change E_Token::CT_INCDEC_AFTER + WORD to E_Token::CT_INCDEC_BEFORE
 * Change number/word + E_Token::CT_ADDR to E_Token::CT_ARITH
 * Change number/word + E_Token::CT_STAR to E_Token::CT_ARITH
 * Change number/word + E_Token::CT_NEG to E_Token::CT_ARITH
 * Change word + ( to a E_Token::CT_FUNCTION
 * Change struct/union/enum + E_Token::CT_WORD => E_Token::CT_TYPE
 * Force parens on return.
 *
 * TODO: This could be done earlier.
 *
 * Patterns detected:
 *   STRUCT/ENUM/UNION + WORD :: WORD => TYPE
 *   WORD + '('               :: WORD => FUNCTION
 */
void fix_symbols();


/**
 * Examines the whole file and changes E_Token::CT_COLON to
 * E_Token::CT_Q_COLON, E_Token::CT_LABEL_COLON, or E_Token::CT_CASE_COLON.
 * It also changes the E_Token::CT_WORD before E_Token::CT_LABEL_COLON into E_Token::CT_LABEL.
 */
void combine_labels();


//! help function for mark_variable_definition...
bool go_on(Chunk *pc, Chunk *start);


//! Sets the parent for comments.
void mark_comments();


void make_type(Chunk *pc);


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
Chunk *set_paren_parent(Chunk *start, E_Token parent);


/**
 * This is called on every chunk.
 * First on all non-preprocessor chunks and then on each preprocessor chunk.
 * It does all the detection and classifying.
 * This is only called by fix_symbols.
 */
void do_symbol_check(Chunk *prev, Chunk *pc, Chunk *next);


#endif /* TOKENIZER_COMBINE_H_INCLUDED */
