/**
 * @file combine_mark_.h
 * prototypes for combine_mark_.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract fom combine.h
 */

#ifndef COMBINE_MARK__H_INCLUDED
#define COMBINE_MARK__H_INCLUDED

#include "chunk_list.h"
#include "ChunkStack.h"


/**
 * We're on a 'class' or 'struct'.
 * Scan for CT_FUNCTION with a string that matches pclass->str
 */
void mark_class_ctor(chunk_t *pclass);


void mark_cpp_constructor(chunk_t *pc);


//! Sets the parent for comments.
void mark_comments(void);


/**
 * Marks statement starts in a macro body.
 * REVISIT: this may already be done
 */
void mark_define_expressions(void);


/**
 * Just mark every CT_WORD until a semicolon as CT_SQL_WORD.
 * Adjust the levels if pc is CT_SQL_BEGIN
 */
void mark_exec_sql(chunk_t *pc);


/**
 * We are on a function word. we need to:
 *  - find out if this is a call or prototype or implementation
 *  - mark return type
 *  - mark parameter types
 *  - mark brace pair
 *
 * REVISIT:
 * This whole function is a mess.
 * It needs to be reworked to eliminate duplicate logic and determine the
 * function type more directly.
 *  1. Skip to the close paren and see what is after.
 *     a. semicolon - function call or function proto
 *     b. open brace - function call (ie, list_for_each) or function def
 *     c. open paren - function type or chained function call
 *     d. qualifier - function def or proto, continue to semicolon or open brace
 *  2. Examine the 'parameters' to see if it can be a proto/def
 *  3. Examine what is before the function name to see if it is a proto or call
 * Constructor/destructor detection should have already been done when the
 * 'class' token was encountered (see mark_class_ctor).
 */
void mark_function(chunk_t *pc);


/**
 * Changes the return type to type and set the parent.
 *
 * @param pc           the last chunk of the return type
 * @param parent_type  CT_NONE (no change) or the new parent type
 */
void mark_function_return_type(chunk_t *fname, chunk_t *pc, c_token_t parent_type);


/**
 * Process a function type that is not in a typedef.
 * pc points to the first close paren.
 *
 * void (*func)(params);
 * const char * (*func)(params);
 * const char * (^func)(params);   -- Objective C
 *
 * @param pc  Points to the first closing paren
 *
 * @return whether a function type was processed
 */
bool mark_function_type(chunk_t *pc);


/**
 *  Just hit an assign. Go backwards until we hit an open brace/paren/square or
 * semicolon (TODO: other limiter?) and mark as a LValue.
 */
void mark_lvalue(chunk_t *pc);


/**
 * Examines the stuff between braces { }.
 * There should only be variable definitions and methods.
 * Skip the methods, as they will get handled elsewhere.
 */
void mark_struct_union_body(chunk_t *start);


/**
 * We are on a word followed by a angle open which is part of a template.
 * If the angle close is followed by a open paren, then we are on a template
 * function def or a template function call:
 *   Vector2<float>(...) [: ...[, ...]] { ... }
 * Or we could be on a variable def if it's followed by a word:
 *   Renderer<rgb32> rend;
 */
void mark_template_func(chunk_t *pc, chunk_t *pc_next);


/**
 * We are on the first word of a variable definition.
 * Mark all the variable names with PCF_VAR_1ST and PCF_VAR_DEF as appropriate.
 * Also mark any '*' encountered as a CT_PTR_TYPE.
 * Skip over []. Go until a ';' is hit.
 *
 * Example input:
 * int   a = 3, b, c = 2;              ## called with 'a'
 * foo_t f = {1, 2, 3}, g = {5, 6, 7}; ## called with 'f'
 * struct {...} *a, *b;                ## called with 'a' or '*'
 * myclass a(4);
 */
chunk_t *mark_variable_definition(chunk_t *start);


void mark_variable_stack(ChunkStack &cs, log_sev_t sev);


/**
 * TODO: add doc cmt
 *
 */
pcf_flags_t mark_where_chunk(chunk_t *pc, c_token_t parent_type, pcf_flags_t flags);


#endif /* COMBINE_MARK__H_INCLUDED */
