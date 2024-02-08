/**
 * @file do_it_newlines_func_pre_blank_lines.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef DO_IT_NEWLINES_FUNC_PRE_BLANK_LINES_H_INCLUDED
#define DO_IT_NEWLINES_FUNC_PRE_BLANK_LINES_H_INCLUDED

#include "token_enum.h"

class Chunk;

bool do_it_newlines_func_pre_blank_lines(Chunk *last_nl, E_Token start_type);

#endif /* DO_IT_NEWLINES_FUNC_PRE_BLANK_LINES_H_INCLUDED */
