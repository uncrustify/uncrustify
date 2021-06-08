/**
 * @file chunk_tools.h
 *
 * @author
 * @license GPL v2+
 */

#ifndef CHUNK_TOOLS_H_INCLUDED
#define CHUNK_TOOLS_H_INCLUDED

#include "scope_enum.h"
#include "token_enum.h"

#include <vector>


/**
 * Return the next chunk that matches one of the specified strings at the given level
 * @param  pc      the starting chunk
 * @param  strings a vector of strings and size pairs for which the search will be performed
 * @param  level   the level of the match
 * @param  scope   code region to search
 * @return         the next chunk that matches one of the specified strings, or nullptr if no
 *                 match is found
 */
struct chunk_t *chunk_get_next_str(struct chunk_t *pc, const std::vector<std::pair<const char *, std::size_t> > &strings, int level, scope_e scope = scope_e::ALL);


/**
 * Return the previous chunk that matches one of the specified strings at the given level
 * @param  pc      the starting chunk
 * @param  strings a vector of strings and size pairs for which the search will be performed
 * @param  level   the level of the match
 * @param  scope   code region to search
 * @return         the previous chunk that matches one of the specified strings, or nullptr if no
 *                 match is found
 */
struct chunk_t *chunk_get_prev_str(struct chunk_t *pc, const std::vector<std::pair<const char *, std::size_t> > &strings, int level, scope_e scope = scope_e::ALL);


/**
 * Return the next chunk that matches one of the specified types at the given level
 * @param  pc    the starting chunk
 * @param  types a vector of token types for which the search will be performed
 * @param  level the level of the match
 * @param  scope code region to search
 * @return       the next chunk that matches one of the specified types, or nullptr if no
 *               match is found
 */
struct chunk_t *chunk_get_next_type(struct chunk_t *pc, const std::vector<c_token_t> &types, int level, scope_e scope = scope_e::ALL);


/**
 * Return the previous chunk that matches one of the specified types at the given level
 * @param  pc    the starting chunk
 * @param  types a vector of token types for which the search will be performed
 * @param  level the level of the match
 * @param  scope code region to search
 * @return       the previous chunk that matches one of the specified types, or nullptr if no
 *               match is found
 */
struct chunk_t *chunk_get_prev_type(struct chunk_t *pc, const std::vector<c_token_t> &types, int level, scope_e scope = scope_e::ALL);


/**
 * Skip forward to the ending chunk in a member initialization list
 * @param  pc    the starting chunk, which should point to a colon
 * @param  scope code region to search
 * @return       the ending chunk of the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_member_initialization_list(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip forward to that which follows the ending chunk in a member initialization list
 * @param  pc    the starting chunk, which should point to a colon
 * @param  scope code region to search
 * @return       the chunk following the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_member_initialization_list_next(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip in reverse to that which precedes the beginning chunk in a member initialization list
 * @param  pc    the starting chunk, which should point to a closing paren or closing brace
 * @param  scope code region to search
 * @return       the chunk preceding the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_member_initialization_list_prev(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip in reverse to the beginning chunk in a member initialization list
 * @param  pc    the starting chunk, which should point to a closing paren or closing brace
 * @param  scope code region to search
 * @return       the beginning chunk of the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_member_initialization_list_rev(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip forward to the ending chunk of an operator overload sequence
 * @param  pc    the starting chunk, which should point to the operator keyword
 * @param  scope code region to search
 * @return       the ending chunk of the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_operator_overload(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip forward to the chunk following an operator overload sequence
 * @param  pc    the starting chunk, which should point to the operator keyword
 * @param  scope code region to search
 * @return       the chunk following the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_operator_overload_next(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip in reverse to the chunk preceding an operator overload sequence
 * @param  pc    the starting chunk, which should point to an overloaded symbol
 * @param  scope code region to search
 * @return       the chunk preceding the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_operator_overload_prev(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip in reverse to the beginning chunk of an operator overload sequence
 * @param  pc    the starting chunk, which should point to an overloaded symbol
 * @param  scope code region to search
 * @return       the beginning chunk of the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_operator_overload_rev(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip forward to the ending chunk in a sequence of pointers, references, and/or qualifiers
 * @param  pc    the starting chunk, which should point to the operator keyword
 * @param  scope code region to search
 * @return       the ending chunk of the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_pointers_references_and_qualifiers(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip forward to the chunk following a sequence of pointers, references, and/or qualifiers
 * @param  pc    the starting chunk, which should point to the operator keyword
 * @param  scope code region to search
 * @return       the chunk following the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_pointers_references_and_qualifiers_next(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip in reverse to the chunk preceding a sequence of pointers, references, and/or qualifiers
 * @param  pc    the starting chunk, which should point to an overloaded symbol
 * @param  scope code region to search
 * @return       the chunk preceding the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_pointers_references_and_qualifiers_prev(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip in reverse to the beginning chunk in a sequence of pointers, references, and/or qualifiers
 * @param  pc    the starting chunk, which should point to an overloaded symbol
 * @param  scope code region to search
 * @return       the beginning chunk of the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_pointers_references_and_qualifiers_rev(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip forward past any scope resolution operators and nested name specifiers and return
 * just the qualified identifier name; while similar to the existing skip_dc_member()
 * function, this function also takes into account templates that may comprise any
 * nested name specifiers
 * @param  pc    the starting chunk
 * @param  scope code region to search
 * @return       the ending chunk of the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_scope_resolution_and_nested_name_specifiers(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip forward to the chunk following the ending chunk of a qualified identifier
 * @param  pc    the starting chunk
 * @param  scope code region to search
 * @return       the chunk following the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_scope_resolution_and_nested_name_specifiers_next(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip in reverse to the chunk preceding the beginning chunk of a qualified identifier
 * @param  pc    the starting chunk
 * @param  scope code region to search
 * @return       the chunk preceding the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_scope_resolution_and_nested_name_specifiers_prev(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip in reverse to the beginning chunk of a qualified identifier; while similar to
 * the existing skip_dc_member_rev() function, this function also takes into account
 * templates that may comprise any nested name specifiers
 * @param  pc    the starting chunk
 * @param  scope code region to search
 * @return       the beginning chunk of the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_scope_resolution_and_nested_name_specifiers_rev(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip forward to the ending chunk in a sequence of trailing function qualifiers
 * parameter signature list
 * @param  pc    the starting chunk, which is assumed to point to a qualifier
 *               following the closing paren of a function parameter list
 * @param  scope code region to search
 * @return       the ending chunk of the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_trailing_function_qualifiers(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip forward to the chunk following a sequence of trailing function qualifiers
 * @param  pc    the starting chunk, which is assumed to point to a qualifier
 *               following the closing paren of a function parameter list
 * @param  scope code region to search
 * @return       the chunk following the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_trailing_function_qualifiers_next(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip in reverse to the chunk preceding a sequence of trailing function qualifiers
 * @param  pc    the starting chunk, which is assumed to point to a qualifier
 *               following the closing paren of a function parameter list
 * @param  scope code region to search
 * @return       the chunk preceding the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_trailing_function_qualifiers_prev(struct chunk_t *pc, scope_e scope = scope_e::ALL);


/**
 * Skip in reverse to the beginning chunk in a sequence of trailing function qualifiers
 * @param  pc    the starting chunk, which is assumed to point to a qualifier
 *               following the closing paren of a function parameter list
 * @param  scope code region to search
 * @return       the beginning chunk of the sequence or the input chunk if no skipping occurred
 */
struct chunk_t *skip_trailing_function_qualifiers_rev(struct chunk_t *pc, scope_e scope = scope_e::ALL);

#endif /* CHUNK_TOOLS_H_INCLUDED */
