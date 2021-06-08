/**
 * @file chunk_tools.cpp
 *
 * @author
 * @license GPL v2+
 */

#include "chunk_tools.h"

#include "chunk_list.h"
#include "keywords.h"
#include "match_tools.h"


/**
 * use this enum to define in what direction or location an
 * operation shall be performed.
 */
enum class direction_e : unsigned int
{
   FORWARD,
   BACKWARD
};


/**
 * Typedef declarations
 */
typedef bool (*chunk_order_test_t)(chunk_t *, chunk_t *, bool);
typedef chunk_t * (*chunk_str_search_function_t)(chunk_t *, const char *, std::size_t, int, scope_e);
typedef chunk_t * (*chunk_type_search_function_t)(chunk_t *, c_token_t, int, scope_e);


/**
 * Forward declarations
 */
static chunk_t *chunk_get_str(chunk_t *pc, const std::vector<std::pair<const char *, std::size_t> > &strings, int level, scope_e scope, chunk_str_search_function_t search_function, chunk_order_test_t chunk_order_test);
static chunk_t *chunk_get_type(chunk_t *pc, const std::vector<c_token_t> &types, int level, scope_e scope, chunk_type_search_function_t search_function, chunk_order_test_t chunk_order_test);


chunk_t *chunk_get_next_str(chunk_t                                                  *pc,
                            const std::vector<std::pair<const char *, std::size_t> > &strings,
                            int                                                      level,
                            scope_e                                                  scope)
{
   return(chunk_get_str(pc,
                        strings,
                        level,
                        scope,
                        (chunk_str_search_function_t)&chunk_get_next_str,
                        chunk_is_before));
} // chunk_get_next_str


chunk_t *chunk_get_prev_str(chunk_t                                                  *pc,
                            const std::vector<std::pair<const char *, std::size_t> > &strings,
                            int                                                      level,
                            scope_e                                                  scope)
{
   return(chunk_get_str(pc,
                        strings,
                        level,
                        scope,
                        (chunk_str_search_function_t)&chunk_get_prev_str,
                        chunk_is_after));
} // chunk_get_prev_str


static chunk_t *chunk_get_str(chunk_t                                                  *pc,
                              const std::vector<std::pair<const char *, std::size_t> > &strings,
                              int                                                      level,
                              scope_e                                                  scope,
                              chunk_str_search_function_t                              search_function,
                              chunk_order_test_t                                       chunk_order_test)
{
   if (  pc != nullptr
      && search_function != nullptr)
   {
      chunk_t *start  = pc;
      chunk_t *result = nullptr;

      for (auto &&string_size_pair : strings)
      {
         auto &&string = string_size_pair.first;
         auto &&size   = string_size_pair.second;
         pc = (*search_function)(start,
                                 string,
                                 size,
                                 level,
                                 scope);

         if (  pc != nullptr
            && (  result == nullptr
               || (*chunk_order_test)(pc, result, false)))
         {
            result = pc;
         }
      }

      pc = result;
   }
   return(pc);
} // chunk_get_str


chunk_t *chunk_get_next_type(chunk_t                      *pc,
                             const std::vector<c_token_t> &types,
                             int                          level,
                             scope_e                      scope)
{
   return(chunk_get_type(pc,
                         types,
                         level,
                         scope,
                         (chunk_type_search_function_t)&chunk_get_next_type,
                         chunk_is_before));
} // chunk_get_next_type


chunk_t *chunk_get_prev_type(chunk_t                      *pc,
                             const std::vector<c_token_t> &types,
                             int                          level,
                             scope_e                      scope)
{
   return(chunk_get_type(pc,
                         types,
                         level,
                         scope,
                         (chunk_type_search_function_t)&chunk_get_prev_type,
                         chunk_is_after));
} // chunk_get_prev_type


static chunk_t *chunk_get_type(chunk_t                      *pc,
                               const std::vector<c_token_t> &types,
                               int                          level,
                               scope_e                      scope,
                               chunk_type_search_function_t search_function,
                               chunk_order_test_t           chunk_order_test)
{
   if (  pc != nullptr
      && search_function != nullptr)
   {
      chunk_t *start  = pc;
      chunk_t *result = nullptr;

      for (c_token_t type : types)
      {
         pc = (*search_function)(start,
                                 type,
                                 level,
                                 scope);

         if (  pc != nullptr
            && (  result == nullptr
               || (*chunk_order_test)(pc, result, false)))
         {
            result = pc;
         }
      }

      pc = result;
   }
   return(pc);
} // chunk_get_type


chunk_t *skip_member_initialization_list(chunk_t *pc, scope_e scope)
{
   if (chunk_is_colon_token(pc))
   {
      chunk_t *next = pc;
      chunk_t *prev = nullptr;

      do
      {
         next = chunk_get_next_ncnnl(next, scope);

         /**
          * Skip any scope resolution and nested name specifiers
          */
         next = skip_scope_resolution_and_nested_name_specifiers(next);

         /**
          * Test to see if an identifier precedes the open brace/paren
          */
         if (!chunk_is_identifier(next))
         {
            return(pc);
         }
         next = chunk_get_next_ncnnl(next, scope);

         if (  !chunk_is_brace_open_token(next)
            && !chunk_is_paren_open_token(next))
         {
            return(pc);
         }
         /**
          * Skip to the matching open brace/paren
          */
         prev = chunk_skip_to_match(next, scope);

         if (prev != nullptr)
         {
            next = chunk_get_next_ncnnl(prev, scope);
         }
      } while (chunk_is_comma_token(next));

      if (chunk_is_brace_open_token(next))
      {
         /**
          * Return the ending chunk
          */
         return(prev);
      }
   }
   return(pc);
} // skip_member_initialization_list_next


chunk_t *skip_member_initialization_list_rev(chunk_t *pc, scope_e scope)
{
   chunk_t *prev = pc;

   while (  chunk_is_brace_close_token(prev)
         || chunk_is_paren_close_token(prev))
   {
      /**
       * Skip to the matching open brace/paren
       */
      prev = chunk_skip_to_match_rev(prev, scope);

      if (prev != nullptr)
      {
         prev = chunk_get_prev_ncnnlni(prev, scope);
      }

      /**
       * Test to see if an identifier precedes the open brace/paren
       */
      if (!chunk_is_identifier(prev))
      {
         return(pc);
      }
      /**
       * Skip any scope resolution and nested name specifiers
       */
      prev = skip_scope_resolution_and_nested_name_specifiers_rev(prev,
                                                                  scope);

      if (chunk_is_comma_token(prev))
      {
         prev = chunk_get_prev_ncnnlni(prev, scope);
      }
   }

   if (chunk_is_colon_token(prev))
   {
      /**
       * Return the chunk preceding the start of the list
       */
      return(prev);
   }
   /**
    * Return the starting chunk
    */
   return(pc);
} // skip_member_initialization_list_rev


chunk_t *skip_operator_overload(chunk_t *pc, scope_e scope)
{
   if (chunk_is_operator_token(pc))
   {
      pc = chunk_get_prev_ncnnl(pc, scope);
   }
   return(pc);
} // skip_operator_overload


chunk_t *skip_operator_overload_next(chunk_t *pc, scope_e scope)
{
   pc = skip_operator_overload(pc);

   if (chunk_is_overloaded_token(pc))
   {
      pc = chunk_get_next_ncnnl(pc, scope);
   }
   return(pc);
} // skip_operator_overload_next


chunk_t *skip_operator_overload_prev(chunk_t *pc, scope_e scope)
{
   pc = skip_operator_overload_rev(pc);

   if (chunk_is_operator_token(pc))
   {
      pc = chunk_get_prev_ncnnlni(pc, scope);
   }
   return(pc);
} // skip_operator_overload_prev


chunk_t *skip_operator_overload_rev(chunk_t *pc, scope_e scope)
{
   if (chunk_is_overloaded_token(pc))
   {
      pc = chunk_get_prev_ncnnl(pc, scope);
   }
   return(pc);
} // skip_operator_overload_rev


chunk_t *skip_pointers_references_and_qualifiers(chunk_t *pc, scope_e scope)
{
   chunk_t *next = pc;

   do
   {
      pc   = next;
      next = chunk_get_next_ncnnl(pc, scope);
   } while (chunk_is_pointer_reference_or_cv_qualifier(next));

   return(pc);
} // skip_pointers_references_and_qualifiers


chunk_t *skip_pointers_references_and_qualifiers_next(chunk_t *pc, scope_e scope)
{
   auto *next = skip_pointers_references_and_qualifiers(pc);

   if (next != pc)
   {
      return(chunk_get_next_ncnnl(next, scope));
   }
   return(pc);
} // skip_pointers_references_and_qualifiers_next


chunk_t *skip_pointers_references_and_qualifiers_prev(chunk_t *pc, scope_e scope)
{
   auto *prev = skip_pointers_references_and_qualifiers_rev(pc);

   if (prev != pc)
   {
      return(chunk_get_prev_ncnnlni(prev, scope));
   }
   return(pc);
} // skip_pointers_references_and_qualifiers_prev


chunk_t *skip_pointers_references_and_qualifiers_rev(chunk_t *pc, scope_e scope)
{
   chunk_t *prev = pc;

   do
   {
      pc   = prev;
      prev = chunk_get_prev_ncnnlni(pc, scope);
   } while (chunk_is_pointer_reference_or_cv_qualifier(prev));

   return(pc);
} // skip_pointers_references_and_qualifiers_rev


chunk_t *skip_scope_resolution_and_nested_name_specifiers(chunk_t *pc,
                                                          scope_e scope)
{
   if (  (  pc != nullptr
         && pc->flags.test(PCF_IN_TEMPLATE))
      || chunk_is_double_colon_token(pc)
      || chunk_is_token(pc, CT_TYPE)
      || chunk_is_token(pc, CT_WORD))
   {
      std::size_t level = pc->level;

      while (  pc != nullptr
            && pc->level >= level
            && !chunk_is_intrinsic_type(pc))
      {
         /**
          * skip to any following match for angle brackets
          */
         if (chunk_is_angle_open_token(pc))
         {
            pc = chunk_skip_to_match(pc, scope);
         }
         auto *next = chunk_get_next_ncnnl(pc, scope);

         /**
          * call a separate function to validate adjacent tokens as potentially
          * matching a qualified identifier
          */
         if (!adj_chunks_match_qualified_identifier_pattern(pc, next))
         {
            break;
         }
         pc = next;
      }
   }
   return(pc);
} // skip_scope_resolution_and_nested_name_specifiers


chunk_t *skip_scope_resolution_and_nested_name_specifiers_next(chunk_t *pc, scope_e scope)
{
   auto *next = skip_scope_resolution_and_nested_name_specifiers(pc);

   if (next != pc)
   {
      return(chunk_get_next_ncnnl(next, scope));
   }
   return(pc);
} // skip_scope_resolution_and_nested_name_specifiers_next


chunk_t *skip_scope_resolution_and_nested_name_specifiers_prev(chunk_t *pc, scope_e scope)
{
   auto *prev = skip_scope_resolution_and_nested_name_specifiers_rev(pc);

   if (prev != pc)
   {
      return(chunk_get_prev_ncnnlni(prev, scope));
   }
   return(pc);
} // skip_scope_resolution_and_nested_name_specifiers_prev


chunk_t *skip_scope_resolution_and_nested_name_specifiers_rev(chunk_t *pc,
                                                              scope_e scope)
{
   if (  (  pc != nullptr
         && pc->flags.test(PCF_IN_TEMPLATE))
      || chunk_is_double_colon_token(pc)
      || chunk_is_token(pc, CT_TYPE)
      || chunk_is_token(pc, CT_WORD))
   {
      std::size_t level = pc->level;

      while (  pc != nullptr
            && pc->level >= level
            && !chunk_is_intrinsic_type(pc))
      {
         /**
          * skip to any preceding match for angle brackets
          */
         if (chunk_is_angle_close_token(pc))
         {
            pc = chunk_skip_to_match_rev(pc, scope);
         }
         auto *prev = chunk_get_prev_ncnnlni(pc, scope);

         /**
          * call a separate function to validate adjacent tokens as potentially
          * matching a qualified identifier
          */
         if (!adj_chunks_match_qualified_identifier_pattern(prev, pc))
         {
            break;
         }
         pc = prev;
      }
   }
   return(pc);
} // skip_scope_resolution_and_nested_name_specifiers_rev


static chunk_t *skip_trailing_function_qualifiers(chunk_t     *pc,
                                                  scope_e     scope,
                                                  direction_e direction)
{
   typedef std::size_t (*index_t)(std::size_t);
   typedef chunk_t * (*search_t)(chunk_t *, scope_e);
   typedef bool (*test_t)(chunk_t *);

   auto backward_index_function = [](std::size_t index)
   {
      return(0x03 & ~index);
   };

   auto forward_index_function = [](std::size_t index)
   {
      return(index);
   };

   index_t  index_function  = nullptr;
   search_t search_function = nullptr;

   if (direction == direction_e::FORWARD)
   {
      index_function  = forward_index_function;
      search_function = chunk_get_next_ncnnl;
   }
   else
   {
      index_function  = backward_index_function;
      search_function = chunk_get_prev_ncnnlni;
   }
   auto chunk_is_ref_qualifier_token = [](chunk_t *pc_ref)
   {
      return(  chunk_is_ampersand_token(pc_ref)
            || chunk_is_double_ampersand_token(pc_ref));
   };

   test_t tests[] =
   {
      chunk_is_const_token,         // skips the 'const' keyword
      chunk_is_volatile_token,      // skip the 'volatile' keyword
      chunk_is_ref_qualifier_token, // skips ref-qaulifiers
      chunk_is_noexcept_token       // skips the 'noexcept' keyword
   };

   chunk_t *next = pc;

   for (std::size_t i = 0; i < 4; ++i)
   {
      auto index = (*index_function)(i);

      if ((*tests[index])(next))
      {
         /**
          * Skip macro references...
          */
         do
         {
            pc   = next;
            next = (*search_function)(pc,
                                      scope);
         } while (chunk_is_macro_reference(next));
      }
   }

   return(pc);
} // skip_trailing_function_qualifiers


chunk_t *skip_trailing_function_qualifiers(chunk_t *pc,
                                           scope_e scope)
{
   return(skip_trailing_function_qualifiers(pc, scope, direction_e::FORWARD));
} // skip_trailing_function_qualifiers_next


chunk_t *skip_trailing_function_qualifiers_next(chunk_t *pc, scope_e scope)
{
   auto *next = skip_trailing_function_qualifiers(pc);

   if (next != pc)
   {
      return(chunk_get_next_ncnnl(next, scope));
   }
   return(pc);
} // skip_trailing_function_qualifiers_next


chunk_t *skip_trailing_function_qualifiers_prev(chunk_t *pc, scope_e scope)
{
   auto *prev = skip_trailing_function_qualifiers_rev(pc);

   if (prev != pc)
   {
      return(chunk_get_prev_ncnnlni(prev, scope));
   }
   return(pc);
} // skip_trailing_function_qualifiers_prev


chunk_t *skip_trailing_function_qualifiers_rev(chunk_t *pc,
                                               scope_e scope)
{
   return(skip_trailing_function_qualifiers(pc, scope, direction_e::BACKWARD));
} // skip_trailing_function_qualifiers_rev
