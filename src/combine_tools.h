/**
 * @file combine_tools.h
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#ifndef COMBINE_TOOLS_H_INCLUDED
#define COMBINE_TOOLS_H_INCLUDED

#include "chunk_list.h"
#include "ChunkStack.h"

/**
 * Checks to see if a series of chunks could be a C++ parameter
 * FOO foo(5, &val);
 *
 * WORD means CT_WORD or CT_TYPE
 *
 * "WORD WORD"          ==> true
 * "QUALIFIER ??"       ==> true
 * "TYPE"               ==> true
 * "WORD"               ==> true
 * "WORD.WORD"          ==> true
 * "WORD::WORD"         ==> true
 * "WORD * WORD"        ==> true
 * "WORD & WORD"        ==> true
 * "NUMBER"             ==> false
 * "STRING"             ==> false
 * "OPEN PAREN"         ==> false
 *
 * @param start  the first chunk to look at
 * @param end    the chunk after the last one to look at
 */
bool can_be_full_param(chunk_t *start, chunk_t *end);


//! Scan backwards to see if we might be on a type declaration
bool chunk_ends_type(chunk_t *start);


bool chunkstack_match(ChunkStack &cs, chunk_t *pc);


///**
// * Simply change any STAR to PTR_TYPE and WORD to TYPE
// *
// * @param start  points to the open paren
// */
void fix_fcn_def_params(chunk_t *pc);


void flag_series(chunk_t *start, chunk_t *end, pcf_flags_t set_flags, pcf_flags_t clr_flags = {}, scope_e nav = scope_e::ALL);


chunk_t *get_d_template_types(ChunkStack &cs, chunk_t *open_paren);


/**
 * Parse off the types in the D template args, adds to cs
 * returns the close_paren
 */
chunk_t *get_d_template_types(ChunkStack &cs, chunk_t *open_paren);


//! help function for mark_variable_definition...
bool go_on(chunk_t *pc, chunk_t *start);


bool is_ucase_str(const char *str, size_t len);


void make_type(chunk_t *pc);

chunk_t *set_paren_parent(chunk_t *start, c_token_t parent);


#endif /* COMBINE_TOOLS_H_INCLUDED */
