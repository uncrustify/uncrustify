/**
 * @file combine_labels.h
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#ifndef TOKENIZER_COMBINE_LABELS_H_INCLUDED
#define TOKENIZER_COMBINE_LABELS_H_INCLUDED

/**
 * Examines the whole file and changes CT_COLON to
 * CT_Q_COLON, CT_LABEL_COLON, or CT_CASE_COLON.
 * It also changes the CT_WORD before CT_LABEL_COLON into CT_LABEL.
 */
void combine_labels();

#endif /* TOKENIZER_COMBINE_LABELS_H_INCLUDED */
