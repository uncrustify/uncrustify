/**
 * @file combine_tools.h
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#ifndef COMBINE_TOOLS_H_INCLUDED
#define COMBINE_TOOLS_H_INCLUDED

#include "chunk.h"
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
bool can_be_full_param(Chunk *start, Chunk *end);


//! Scan backwards to see if we might be on a type declaration
bool chunk_ends_type(Chunk *start);


bool chunkstack_match(ChunkStack &cs, Chunk *pc);


///**
// * Simply change any STAR to PTR_TYPE and WORD to TYPE
// *
// * @param start  points to the open paren
// */
void fix_fcn_def_params(Chunk *pc);


void flag_series(Chunk *start, Chunk *end, PcfFlags set_flags, PcfFlags clr_flags = {}, E_Scope nav = E_Scope::ALL);


/*
 * Checks whether or not a given chunk has a parent cpp template,
 * and if so returns the associated angle bracket nest level
 * with respect to the root parent template; returns 0 if
 * the chunk is not part of a template parameter list
 */
size_t get_cpp_template_angle_nest_level(Chunk *pc);


/**
 * Parse off the types in the D template args, adds to cs
 * returns the close_paren
 */
Chunk *get_d_template_types(ChunkStack &cs, Chunk *open_paren);


//! help function for mark_variable_definition...
bool go_on(Chunk *pc, Chunk *start);


bool is_ucase_str(const char *str, size_t len);


void make_type(Chunk *pc);

Chunk *set_paren_parent(Chunk *start, E_Token parent);


#endif /* COMBINE_TOOLS_H_INCLUDED */
