/**
 * @file combine_labels.h
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#ifndef TOKENIZER_COMBINE_LABELS_H_INCLUDED
#define TOKENIZER_COMBINE_LABELS_H_INCLUDED

/**
 * Examines the whole file and changes E_Token::COLON to
 * E_Token::Q_COLON, E_Token::LABEL_COLON, or E_Token::CASE_COLON.
 * It also changes the E_Token::WORD before E_Token::LABEL_COLON into E_Token::LABEL.
 */
void combine_labels();

#endif /* TOKENIZER_COMBINE_LABELS_H_INCLUDED */
