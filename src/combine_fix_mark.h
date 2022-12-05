/**
 * @file combine_fix_mark.h
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.cpp
 */

#ifndef COMBINE_FIX_MARK_H_INCLUDED
#define COMBINE_FIX_MARK_H_INCLUDED

#include "ChunkStack.h"


/**
 * Checks to see if the current paren is part of a cast.
 * We already verified that this doesn't follow function, TYPE, IF, FOR,
 * SWITCH, or WHILE and is followed by WORD, TYPE, STRUCT, ENUM, or UNION.
 *
 * @param start  Pointer to the open paren
 */
void fix_casts(Chunk *start);


/**
 * Simply change any STAR to PTR_TYPE and WORD to TYPE
 *
 * @param start  points to the open paren
 */
void fix_fcn_def_params(Chunk *start);


/**
 * CT_TYPE_CAST follows this pattern:
 * dynamic_cast<...>(...)
 *
 * Mark everything between the <> as a type and set the paren parent
 */
void fix_type_cast(Chunk *start);


/**
 * We are on a typedef.
 * If the next word is not enum/union/struct, then the last word before the
 * next ',' or ';' or '__attribute__' is a type.
 *
 * typedef [type...] [*] type [, [*]type] ;
 * typedef <return type>([*]func)();
 * typedef <return type>([*]func)(params);
 * typedef <return type>(__stdcall *func)(); Bug # 633    MS-specific extension
 *                                           include the config-file "test/config/MS-calling_conventions.cfg"
 * typedef <return type>func(params);
 * typedef <enum/struct/union> [type] [*] type [, [*]type] ;
 * typedef <enum/struct/union> [type] { ... } [*] type [, [*]type] ;
 */
void fix_typedef(Chunk *start);


/**
 * We are on the start of a sequence that could be a variable definition
 *  - FPAREN_OPEN (parent == CT_FOR)
 *  - BRACE_OPEN
 *  - SEMICOLON
 */
Chunk *fix_variable_definition(Chunk *start);


void mark_cpp_constructor(Chunk *pc);


/**
 * Mark all chunks belonging to a C++ lambda expression
 *
 * @param square_open points to a chunk of type CT_SQUARE_OPEN, and the parent type is
 *                    assumed to have already been assigned the value CT_CPP_LAMBDA;
 *                    under this assumption, the function marks all chunks as PCF_IN_LAMBDA
 *                    until the corresponding closing CT_BRACE_CLOSE (with parent type
 *                    CT_CPP_LAMBDA) is encountered
 */
void mark_cpp_lambda(Chunk *square_open);


/**
 * Marks statement starts in a macro body.
 * REVISIT: this may already be done
 */
void mark_define_expressions();


/**
 * Just mark every CT_WORD until a semicolon as CT_SQL_WORD.
 * Adjust the levels if pc is CT_SQL_BEGIN
 */
void mark_exec_sql(Chunk *pc);


/**
 * Changes the return type to type and set the parent.
 *
 * @param pc           the last chunk of the return type
 * @param parent_type  CT_NONE (no change) or the new parent type
 */
void mark_function_return_type(Chunk *fname, Chunk *start, E_Token parent_type);


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
void mark_function(Chunk *pc);


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
bool mark_function_type(Chunk *pc);


/**
 *  Just hit an assign. Go backwards until we hit an open brace/paren/square or
 * semicolon (TODO: other limiter?) and mark as a LValue.
 */
void mark_lvalue(Chunk *pc);


/**
 * Examines the stuff between braces { }.
 * There should only be variable definitions and methods.
 * Skip the methods, as they will get handled elsewhere.
 */
void mark_struct_union_body(Chunk *start);


/**
 * We are on a word followed by a angle open which is part of a template.
 * If the angle close is followed by a open paren, then we are on a template
 * function def or a template function call:
 *   Vector2<float>(...) [: ...[, ...]] { ... }
 * Or we could be on a variable def if it's followed by a word:
 *   Renderer<rgb32> rend;
 */
void mark_template_func(Chunk *pc, Chunk *pc_next);


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
Chunk *mark_variable_definition(Chunk *start);


void mark_variable_stack(ChunkStack &cs, log_sev_t sev);


/**
 * TODO: add doc cmt
 *
 */
PcfFlags mark_where_chunk(Chunk *pc, E_Token parent_type, PcfFlags flags);


#endif /* COMBINE_FIX_MARK_H_INCLUDED */
