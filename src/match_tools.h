/**
 * @file match_tools.h
 *
 * @author
 * @license GPL v2+
 */

#ifndef MATCH_TOOLS_H_INCLUDED
#define MATCH_TOOLS_H_INCLUDED

#include "scope_enum.h"
#include "token_enum.h"

#include <tuple>
#include <vector>


/**
 * Returns true if two adjacent chunks potentially match a pattern consistent
 * with that of a compound type
 */
bool adj_chunks_match_compound_type_pattern(struct chunk_t *prev, struct chunk_t *next);


/**
 * Returns true if two adjacent chunks potentially match a pattern consistent
 * with that of a qualified identifier
 */
bool adj_chunks_match_qualified_identifier_pattern(struct chunk_t *prev, struct chunk_t *next);


/**
 * Returns true if two adjacent chunks potentially match a pattern consistent
 * with the end of a template definition
 */
bool adj_chunks_match_template_end_pattern(struct chunk_t *prev, struct chunk_t *next);


/**
 * Returns true if two adjacent chunks potentially match a pattern consistent
 * with the start of a template definition
 */
bool adj_chunks_match_template_start_pattern(struct chunk_t *prev, struct chunk_t *next);


/**
 * Returns true if two adjacent chunks potentially match a pattern consistent
 * with that of a variable definition
 */
bool adj_chunks_match_var_def_pattern(struct chunk_t *prev, struct chunk_t *next);


/**
 * Starting from the input chunk, this function attempts to match a type on the left-hand-side
 * of an assignment associated with a default template argument or type alias associated with
 * a using declaration
 * @param  pc the input chunk, which points to a chunk containing an assignment symbol '='
 * @return    upon successful match, function returns a non-null pointer to the identifier or
 *            auto keyword; if no match is not found, function returns null
 */
struct chunk_t *match_assigned_type(struct chunk_t *pc_assign);


/**
 * Searching in the forward direction, return the beginning chunk of a sequence that matches the specified
 * chain of strings at the given level, where level applies strictly to the start of the chain
 * @param  pc    the starting chunk
 * @param  chain a vector of strings and size pairs for which the search will be performed
 * @param  level the level of the match
 * @param  scope code region to search
 * @return       a pointer to the next chunk to match the chain specified by the search criteria,
 *               or nullptr if no match is found
 */
struct chunk_t *match_chain_next(struct chunk_t *pc, const std::vector<std::pair<const char *, std::size_t> > &chain, int level, scope_e scope = scope_e::ALL);


/**
 * Searching in the forward direction, return the beginning chunk of a sequence that matches the specified
 * chain of token types at the given level, where level applies strictly to the start of the chain
 * @param  pc    the starting chunk
 * @param  chain a vector of token types for which the search will be performed
 * @param  level the level of the match
 * @param  scope code region to search
 * @return       a pointer to the next chunk to match the sequence specified by the search criteria,
 *               or nullptr if no match is found
 */
struct chunk_t *match_chain_next(struct chunk_t *pc, const std::vector<c_token_t> &chain, int level, scope_e scope = scope_e::ALL);


/**
 * Searching in the forward direction, return the beginning chunk of the first-encountered sequence
 * that matches one of the specified chains of strings or token types at the given level, where level
 * applies strictly to the start of the chain
 * @param  pc    the starting chunk
 * @param  chain a vector of string or token type vector chains for which the search will be performed
 * @param  level the level of the match
 * @param  scope code region to search
 * @return       a pointer to the beginning chunk of the nearest sequence to match one of the specified chains,
 *               or nullptr if no match is found
 */
template<typename T = std::pair<const char *, std::size_t> >
auto match_chain_next(struct chunk_t *pc, const std::initializer_list<std::initializer_list<T> > &chains, int level, scope_e scope = scope_e::ALL) -> struct chunk_t *
{
   struct chunk_t *next = nullptr;

   for (auto &&chain : chains)
   {
      auto *tmp = match_chain_next(pc, chain, level, scope);

      if (  tmp != nullptr
         && (  next == nullptr
            || chunk_is_before(tmp, next)))
      {
         next = tmp;
      }
   }

   return(next);
} // match_chain_next


/**
 * Searching in the reverse direction, return the beginning chunk of a sequence that matches the specified
 * chain of strings at the given level, where level applies strictly to the start of the chain
 * @param  pc    the starting chunk
 * @param  chain a vector of strings and size pairs for which the search will be performed
 * @param  level the level of the match
 * @param  scope code region to search
 * @return       a pointer to the next chunk to match the sequence specified by the search criteria,
 *               or nullptr if no match is found
 */
struct chunk_t *match_chain_prev(struct chunk_t *pc, const std::vector<std::pair<const char *, std::size_t> > &chain, int level, scope_e scope = scope_e::ALL);


/**
 * Searching in the reverse direction, return the beginning chunk of a sequence that matches the specified
 * chain of token types at the given level, where level applies strictly to the start of the chain
 * @param  pc    the starting chunk
 * @param  chain a vector of token types for which the search will be performed
 * @param  level the level of the match
 * @param  scope code region to search
 * @return       a pointer to the next chunk to match the sequence specified by the search criteria,
 *               or nullptr if no match is found
 */
struct chunk_t *match_chain_prev(struct chunk_t *pc, const std::vector<c_token_t> &chain, int level, scope_e scope = scope_e::ALL);


/**
 * Searching in the reverse direction, return the beginning chunk of the first-encountered sequence
 * that matches one of the specified chains of strings or token types at the given level, where level
 * applies strictly to the start of the chain
 * @param  pc    the starting chunk
 * @param  chain a vector of string or token type vector chains for which the search will be performed
 * @param  level the level of the match
 * @param  scope code region to search
 * @return       a pointer to the beginning chunk of the nearest sequence to match one of the specified chains,
 *               or nullptr if no match is found
 */
template<typename T = std::pair<const char *, std::size_t> >
auto match_chain_prev(struct chunk_t *pc, const std::initializer_list<std::initializer_list<T> > &chains, int level, scope_e scope = scope_e::ALL) -> struct chunk_t *
{
   struct chunk_t *prev = nullptr;

   for (auto &&chain : chains)
   {
      auto *tmp = match_chain_prev(pc, chain, level, scope);

      if (  tmp != nullptr
         && (  prev == nullptr
            || chunk_is_after(tmp, prev)))
      {
         prev = tmp;
      }
   }

   return(prev);
} // match_chain_prev


/**
 * Attempt to match a potential compound type (including pointers, references,
 * qualifiers, etc.) starting at the input chunk
 * @param  pc    the input chunk
 * @param  level the level of the match
 * @return       upon success, function returns a pair of non-null starting and
 *               ending chunks if a potential type has been matched
 */
std::pair<struct chunk_t *,
          struct chunk_t *> match_compound_type(struct chunk_t *pc, std::size_t level);


/**
 * Attempt to match a potential compound type (including pointers, references,
 * qualifiers, etc.) in the forward direction starting at the input chunk
 * @param  pc    the input chunk
 * @param  level the level of the match
 * @return       upon success, function returns a non-null end chunk if
 *               a potential type has been matched
 */
struct chunk_t *match_compound_type_end(struct chunk_t *pc, std::size_t level);


/**
 * Attempt to match a potential compound type (including pointers, references,
 * qualifiers, etc.) in the reverse direction starting at the input chunk
 * @param  pc    the input chunk
 * @param  level the level of the match
 * @return       upon success, function returns a non-null starting chunk if
 *               a potential type has been matched
 */
struct chunk_t *match_compound_type_start(struct chunk_t *pc, std::size_t level);


/**
 * Attempt to match the beginning of a potential function header at the closing paren
 * associated with its parameter list
 * @param  pc the input chunk, which should point to a closing paren
 * @return    upon success, function returns a non-null starting chunk that
 *            points to the beginning of the function header, or nullptr if no
 *            match is found
 */
struct chunk_t *match_function_header_at_close_paren(struct chunk_t *pc);


/**
 * Starting from the input chunk, this function attempts to match a function pointer
 * type signature or variable declaration at an open/close paren
 * @param  pc_paren the input chunk, which should point to an open/close paren
 * @return          upon successful match, function returns a tuple of chunks, where the
 *                  first chunk indicates the beginning of the declaration or type signature,
 *                  the second chunk indicates the variable name (or null if a type signature
 *                  is detected), and the third chunk indicates the end associated with the
 *                  declaration/type signature; if no match is found, function returns a tuple
 *                  of null chunks
 */
std::tuple<struct chunk_t *,
           struct chunk_t *,
           struct chunk_t *> match_function_pointer_at_paren(struct chunk_t *pc_paren);


/**
 * Starting from the input chunk, this function attempts to match a function pointer
 * type signature or variable declaration at an open/close paren
 * @param[in]  pc_paren the input chunk, which should point to an open/close paren
 * @param[out] match    upon successful match, argument is populated with a tuple of chunks,
 *                      where the first chunk indicates the beginning of the declaration or type
 *                      signature, the second chunk indicates the variable name (or null if
 *                      a type signature is detected), and the third chunk indicates the end
 *                      associated with the declaration/type signature; if no match is found,
 *                      argument is unmodified
 * @return              true upon successful match
 */
bool match_function_pointer_at_paren(struct chunk_t *pc_paren, std::tuple<struct chunk_t *,
                                                                          struct chunk_t *,
                                                                          struct chunk_t *> &match);

/**
 * Starting from the input chunk, this function attempts to match a function pointer
 * typedef at the specified identifier
 * @param  pc_identifier the input chunk, which should point to an identifier
 * @return               upon successful match, function returns an std::tuple, where the
 *                       first chunk indicates the beginning of the typedef, the second
 *                       chunk indicates the variable name, and the third chunk indicates
 *                       the end associated with the function pointer typedef;
 *                       if no match is found, function returns a tuple of null chunks
 */
std::tuple<struct chunk_t *,
           struct chunk_t *,
           struct chunk_t *> match_function_pointer_typedef_at_identifier(struct chunk_t *pc_identifier);


/**
 * Starting from the input chunk, this function attempts to match a function pointer
 * typedef at the specified identifier
 * @param[in]  pc_identifier the input chunk, which should point to an identifier
 * @param[out] match         upon successful match, argument is populated with a tuple of
 *                           chunks, where the first chunk indicates the beginning of the
 *                           declaration, the second chunk indicates the variable name, and
 *                           the third chunk indicates the end associated with the function
 *                           pointer typedef; if no match is found, argument is
 *                           unmodified
 * @return                   true upon successful match
 */
bool match_function_pointer_typedef_at_identifier(struct chunk_t *pc_identifier, std::tuple<struct chunk_t *,
                                                                                            struct chunk_t *,
                                                                                            struct chunk_t *> &match);


/**
 * Starting from the input chunk, this function attempts to match a function pointer
 * variable declaration at the specified identifier
 * @param  pc_identifier the input chunk, which should point to an identifier
 * @return               upon successful match, function returns an std::tuple, where the
 *                       first chunk indicates the beginning of the declaration, the second
 *                       chunk indicates the variable name, and the third chunk indicates
 *                       the end associated with the function pointer variable declaration;
 *                       if no match is found, function returns a tuple of null chunks
 */
std::tuple<struct chunk_t *,
           struct chunk_t *,
           struct chunk_t *> match_function_pointer_variable_at_identifier(struct chunk_t *pc_identifier);


/**
 * Starting from the input chunk, this function attempts to match a function pointer
 * variable declaration at the specified identifier
 * @param[in]  pc_identifier the input chunk, which should point to an identifier
 * @param[out] match         upon successful match, argument is populated with a tuple of
 *                           chunks, where the first chunk indicates the beginning of the
 *                           declaration, the second chunk indicates the variable name, and
 *                           the third chunk indicates the end associated with the function
 *                           pointer variable declaration; if no match is found, argument is
 *                           unmodified
 * @return                   true upon successful match
 */
bool match_function_pointer_variable_at_identifier(struct chunk_t *pc_identifier, std::tuple<struct chunk_t *,
                                                                                             struct chunk_t *,
                                                                                             struct chunk_t *> &match);


/**
 * This function attempts to match the starting and ending chunks of a qualified
 * identifier, which consists of one or more scope resolution operator(s) and
 * zero or more nested name specifiers
 * specifiers
 * @param  pc the starting chunk
 * @return    an std::pair, where the first chunk indicates the starting chunk of the
 *            match and second indicates the ending chunk. Upon finding a successful
 *            match, the starting chunk may consist of an identifier or a scope
 *            resolution operator, while the ending chunk may consist of identifier
 *            or the closing angle bracket of a template. If no match is found, a
 *            pair of null chunks is returned
 */
std::pair<struct chunk_t *,
          struct chunk_t *> match_qualified_identifier(struct chunk_t *pc);


/**
 * Starting from the input chunk, this function attempts to match a variable
 * declaration/definition in both the forward and reverse directions; each pair of
 * consecutive chunks is tested to determine if a potential match is satisfied.
 * @param  pc    the starting chunk
 * @param  level the level of the match
 * @return       upon successful match, function returns an std::tuple, where the
 *               first chunk indicates the starting chunk, the second chunk indicates
 *               the identifier name, and the third chunk indicates the end associated
 *               with the variable declaration/definition
 */
std::tuple<struct chunk_t *,
           struct chunk_t *,
           struct chunk_t *> match_variable(struct chunk_t *pc, std::size_t level);


/**
 * Starting from the input chunk, this function attempts to match a variable in the
 * forward direction, and tests each pair of consecutive chunks to determine if a
 * potential variable declaration/definition match is satisfied. Secondly, the
 * function attempts to identify the end chunk associated with the candidate variable
 * match. For scalar variables (simply declared and not defined), both the end chunk
 * and identifier chunk should be one in the same
 * @param  pc    the starting chunk
 * @param  level the level of the match
 * @return       an std::pair, where the first chunk indicates the identifier
 *               (if non-null) and the second chunk indicates the end associated with
 *               the variable declaration/definition; assuming a valid match, the first
 *               chunk may be null if the function is called with a starting chunk
 *               that occurs after the identifier
 */
std::pair<struct chunk_t *,
          struct chunk_t *> match_variable_end(struct chunk_t *pc, std::size_t level);


/**
 * Starting from the input chunk, this function attempts to match a variable in the
 * reverse direction, and tests each pair of consecutive chunks to determine if a
 * potential variable declaration/definition match is satisfied. Secondly, the
 * function attempts to identify the starting chunk associated with the candidate
 * variable match. The start and identifier chunks may refer to each other in cases
 * where the identifier is not preceded by pointer or reference operators or qualifiers,
 * etc.
 * @param  pc    the starting chunk
 * @param  level the level of the match
 * @return       an std::pair, where the first chunk indicates the starting chunk and
 *               the second chunk indicates the identifier associated with the variable
 *               match; assuming a valid match, the second chunk may be null if the
 *               function is called with a starting chunk that occurs before the
 *               identifier
 */
std::pair<struct chunk_t *,
          struct chunk_t *> match_variable_start(struct chunk_t *pc, std::size_t level);


#endif /* MATCH_TOOLS_H_INCLUDED */
