/**
 * @file combine_labels.h
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#ifndef TOKENIZER_COMBINE_LABELS_H_INCLUDED
#define TOKENIZER_COMBINE_LABELS_H_INCLUDED

/**
 * Examines the whole file and changes E_Token::CT_COLON to
 * E_Token::CT_Q_COLON, E_Token::CT_LABEL_COLON, or E_Token::CT_CASE_COLON.
 * It also changes the E_Token::CT_WORD before E_Token::CT_LABEL_COLON into E_Token::CT_LABEL.
 */
void combine_labels();

#endif /* TOKENIZER_COMBINE_LABELS_H_INCLUDED */
