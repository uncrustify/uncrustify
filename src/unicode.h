/**
 * @file unicode.h
 * prototypes for unicode.cpp
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef UNICODE_H_INCLUDED
#define UNICODE_H_INCLUDED

#include "uncrustify_types.h"
#include <deque>
#include <vector>


void write_bom();


/**
 * @param ch the 31-bit char value
 */
void write_char(int ch);


void write_string(const UncText &text);


//! Figure out the encoding and convert to an int sequence
bool decode_unicode(const std::vector<UINT8> &in_data, std::deque<int> &out_data, char_encoding_e &enc, bool &has_bom);


void encode_utf8(int ch, std::vector<UINT8> &res);

#endif /* UNICODE_H_INCLUDED */
