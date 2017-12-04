/**
 * @file unicode.h
 * prototypes for unicode.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef UNICODE_H_INCLUDED
#define UNICODE_H_INCLUDED

#include "uncrustify_types.h"
#include "unc_text.h"


void write_bom(void);


/**
 * @param ch the 31-bit char value
 */
void write_char(int ch);


void write_string(const unc_text &text);


//! Figure out the encoding and convert to an int sequence
bool decode_unicode(const std::vector<UINT8> &in_data, std::deque<int> &out_data, char_encoding_e &enc, bool &has_bom);


void encode_utf8(int ch, std::vector<UINT8> &res);


#endif /* UNICODE_H_INCLUDED */
