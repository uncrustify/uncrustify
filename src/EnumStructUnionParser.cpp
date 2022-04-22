/**
 * @file EnumStructUnionParser.cpp
 *
 * @author
 * @license GPL v2+
 */

#include "EnumStructUnionParser.h"

#include "combine_fix_mark.h"
#include "combine_skip.h"
#include "combine_tools.h"
#include "flag_parens.h"
#include "lang_pawn.h"


/**
 * Extern declarations
 */
extern const char *get_token_name(E_Token);
extern void log_pcf_flags(log_sev_t, pcf_flags_t);


/**
 * Forward declarations
 */
static std::pair<Chunk *, Chunk *> match_variable_end(Chunk *, std::size_t);
static std::pair<Chunk *, Chunk *> match_variable_start(Chunk *, std::size_t);
static Chunk *skip_scope_resolution_and_nested_name_specifiers(Chunk *);
static Chunk *skip_scope_resolution_and_nested_name_specifiers_rev(Chunk *);


/**
 * Returns true if two adjacent chunks potentially match a pattern consistent
 * with that of a qualified identifier
 */
static bool adj_tokens_match_qualified_identifier_pattern(Chunk *prev, Chunk *next)
{
   LOG_FUNC_ENTRY();

   if (  prev != nullptr
      && next != nullptr)
   {
      auto prev_token_type = prev->type;
      auto next_token_type = next->type;

      switch (prev_token_type)
      {
      case CT_ANGLE_CLOSE:
         /**
          * assuming the previous token is possibly the closing angle of a
          * templated type, the next token may be a scope resolution operator ("::")
          */
         return(next_token_type == CT_DC_MEMBER);

      case CT_ANGLE_OPEN:
         /**
          * assuming the previous token is possibly the opening angle of a
          * templated type, just check to see if there's a matching closing
          * angle
          */
         return(chunk_skip_to_match(prev, E_Scope::PREPROC) != nullptr);

      case CT_DC_MEMBER:
         /**
          * if the previous token is a double colon ("::"), it is likely part
          * of a chain of scope-resolution qualifications preceding a word or
          * type
          */
         return(  next_token_type == CT_TYPE
               || next_token_type == CT_WORD);

      case CT_TYPE:
      case CT_WORD:
         /**
          * if the previous token is an identifier, the next token may be
          * one of the following:
          * - an opening angle, which may indicate a templated type as part of a
          *   scope resolution preceding the actual variable identifier
          * - a double colon ("::")
          */
         return(  next_token_type == CT_ANGLE_OPEN
               || next_token_type == CT_DC_MEMBER);

      default:
         // do nothing
         break;
      } // switch
   }
   return(false);
} // adj_tokens_match_qualified_identifier_pattern


/**
 * Returns true if two adjacent chunks potentially match a pattern consistent
 * with that of a variable definition
 */
static bool adj_tokens_match_var_def_pattern(Chunk *prev, Chunk *next)
{
   LOG_FUNC_ENTRY();

   if (  prev != nullptr
      && next != nullptr)
   {
      auto prev_token_type = prev->type;
      auto next_token_type = next->type;

      switch (prev_token_type)
      {
      case CT_ANGLE_CLOSE:
         /**
          * assuming the previous token is possibly the closing angle of a
          * templated type, the next token may be one of the following:
          * - a pointer symbol ('*', '^')
          * - a double colon ("::")
          * - a reference symbol ('&')
          * - a qualifier (const, etc.)
          * - an identifier
          */
         return(  chunk_is_pointer_or_reference(next)
               || next_token_type == CT_DC_MEMBER
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_WORD);


      case CT_ANGLE_OPEN:
         /**
          * assuming the previous token is possibly the opening angle of a
          * templated type, just check to see if there's a matching closing
          * angle
          */
         return(chunk_skip_to_match(prev, E_Scope::PREPROC) != nullptr);

      case CT_BRACE_CLOSE:
         /**
          * assuming the previous token is possibly the closing brace of a
          * class/enum/struct/union definition, one or more inline variable
          * definitions may follow; in that case, the next token may be one of
          * the following:
          * - a pointer symbol ('*', '^')
          * - a reference symbol ('&')
          * - a qualifier (const, etc.)
          * - an identifier
          */
         return(  chunk_is_pointer_or_reference(next)
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_WORD);

      case CT_BRACE_OPEN:
         /**
          * if the previous token is an opening brace, it may indicate the
          * start of a braced initializer list - skip ahead to find a matching
          * closing brace
          */
         return(chunk_skip_to_match(prev, E_Scope::PREPROC) != nullptr);

      case CT_BYREF:
         /**
          * if the previous token is a reference symbol ('&'), the next token
          * may be an identifier
          */
         return(next_token_type == CT_WORD);

      case CT_CARET:
         /**
          * if the previous token is a managed C++/CLI pointer symbol ('^'),
          * the next token may be one of the following:
          * - a pointer symbol ('*', '^')
          * - a reference symbol ('&')
          * - a qualifier (const, etc.)
          * - an identifier
          */
         return(  language_is_set(LANG_CPP)
               && (  chunk_is_pointer_or_reference(next)
                  || next_token_type == CT_QUALIFIER
                  || next_token_type == CT_WORD));

      case CT_COMMA:
         /**
          * if the previous token is a comma, this may indicate a variable
          * declaration trailing a prior declaration; in that case, the next
          * token may be one of the following:
          * - a pointer symbol ('*', '^')
          * - a reference symbol ('&')
          * - an identifier
          */
         return(  chunk_is_pointer_or_reference(next)
               || next_token_type == CT_WORD);

      case CT_DC_MEMBER:
         /**
          * if the previous token is a double colon ("::"), it is likely part
          * of a chain of scope-resolution qualifications preceding a word or
          * type
          */
         return(  next_token_type == CT_TYPE
               || next_token_type == CT_WORD);

      case CT_PAREN_OPEN:
         /**
          * if the previous token is an opening paren, it may indicate the
          * start of a constructor call parameter list - skip ahead to find a
          * matching closing paren
          */
         next = chunk_skip_to_match(prev, E_Scope::PREPROC);

         if (next != nullptr)
         {
            next_token_type = next->type;
         }
         return(next_token_type == CT_PAREN_CLOSE);

      case CT_PTR_TYPE:
         /**
          * if the previous token is a pointer type, ('*', '^'), the next token
          * may be one of the following:
          * - another pointer symbol ('*', '^')
          * - a reference symbol ('&')
          * - a qualifier (const, etc.)
          * - an identifier
          */
         return(  chunk_is_pointer_or_reference(next)
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_WORD);

      case CT_QUALIFIER:
         /**
          * if the previous token is a qualifier (const, etc.), the next token
          * may be one of the following:
          * - a pointer symbol ('*', '^')
          * - a reference symbol ('&')
          * - another qualifier
          * - an identifier
          */
         return(  chunk_is_pointer_or_reference(next)
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_WORD);

      case CT_SQUARE_CLOSE:
         /**
          * if the previous token is a closing bracket, the next token may be
          * an assignment following an array variable declaration
          */
         return(next_token_type == CT_ASSIGN);

      case CT_SQUARE_OPEN:
         /**
          * if the previous token is an opening bracket, it may indicate an
          * array declaration - skip ahead to find a matching closing bracket
          */
         return(chunk_skip_to_match(prev, E_Scope::PREPROC) != nullptr);

      case CT_STAR:
         /**
          * if the previous token is a pointer symbol, ('*'), the next token
          * may be one of the following:
          * - another pointer symbol ('*', '^')
          * - a reference symbol ('&')
          * - a qualifier (const, etc.)
          * - an identifier
          */
         return(  chunk_is_pointer_or_reference(next)
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_WORD);

      case CT_TSQUARE:
         /**
          * if the previous token is a set of brackets, the next token may be
          * an assignment following an array variable declaration
          */
         return(next_token_type == CT_ASSIGN);

      case CT_TYPE:
         /**
          * if the previous token is marked as a type, the next token may be
          * one of the following:
          * - a pointer symbol ('*', '^')
          * - a reference symbol ('&')
          * - an opening angle, which may indicate a templated type as part of a
          *   scope resolution preceding the actual variable identifier
          * - a double colon ("::")
          * - a qualifier (const, etc.)
          * - an identifier
          */
         return(  chunk_is_pointer_or_reference(next)
               || next_token_type == CT_ANGLE_OPEN
               || next_token_type == CT_DC_MEMBER
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_WORD);

      case CT_WORD:
         /**
          * if the previous token is an identifier, the next token may be one
          * of the following:
          * - an assignment symbol ('=')
          * - an opening angle, which may indicate a templated type as part of a
          *   scope resolution preceding the actual variable identifier
          * - an opening brace, which may indicate a braced-initializer list
          * - a double colon ("::")
          * - an opening paren, which may indicate a constructor call parameter
          *   list
          * - an opening square bracket, which may indicate an array variable
          * - an set of empty square brackets, which also may indicate an array
          *   variable
          */
         return(  next_token_type == CT_ANGLE_OPEN
               || next_token_type == CT_ASSIGN
               || next_token_type == CT_BRACE_OPEN
               || next_token_type == CT_DC_MEMBER
               || next_token_type == CT_PAREN_OPEN
               || next_token_type == CT_SQUARE_OPEN
               || next_token_type == CT_TSQUARE);

      default:
         // do nothing
         break;
      } // switch
   }
   return(false);
} // adj_tokens_match_var_def_pattern


/**
 * Returns true if the first chunk occurs AFTER the second chunk in the argument
 * list
 * @param pc         points to the first chunk
 * @param after      points to the second chunk
 * @param test_equal if true, returns true when both chunks refer to the same chunk
 */
static bool chunk_is_after(Chunk *pc, Chunk *after, bool test_equal = true)
{
   LOG_FUNC_ENTRY();

   if (  pc != nullptr
      && pc->IsNotNullChunk())
   {
      if (  test_equal
         && pc == after)
      {
         return(true);
      }
      else if (after != nullptr)
      {
         auto pc_column    = pc->orig_col;
         auto pc_line      = pc->orig_line;
         auto after_column = after->orig_col;
         auto after_line   = after->orig_line;

         return(  pc_line > after_line
               || (  pc_line == after_line
                  && pc_column > after_column));
      }
   }
   return(false);
} // chunk_is_after


/**
 * Returns true if the first chunk occurs BEFORE the second chunk in the argument
 * list
 * @param pc         points to the first chunk
 * @param before     points to the second chunk
 * @param test_equal if true, returns true when both chunks refer to the same chunk
 */
static bool chunk_is_before(Chunk *pc, Chunk *before, bool test_equal = true)
{
   LOG_FUNC_ENTRY();

   if (  pc != nullptr
      && pc->IsNotNullChunk())
   {
      if (  test_equal
         && pc == before)
      {
         return(true);
      }
      else if (before != nullptr)
      {
         auto pc_column     = pc->orig_col;
         auto pc_line       = pc->orig_line;
         auto before_column = before->orig_col;
         auto before_line   = before->orig_line;

         return(  pc_line < before_line
               || (  pc_line == before_line
                  && pc_column < before_column));
      }
   }
   return(false);
} // chunk_is_before


/**
 * Returns true if the first chunk occurs both AFTER and BEFORE
 * the second and third chunks, respectively, in the argument list
 * @param pc         points to the first chunk
 * @param after      points to the second chunk
 * @param before     points to the third chunk
 * @param test_equal if true, returns true when the first chunk tests equal to
 *                   either the second or third chunk
 */
static bool chunk_is_between(Chunk *pc, Chunk *after, Chunk *before, bool test_equal = true)
{
   LOG_FUNC_ENTRY();

   return(  chunk_is_before(pc, before, test_equal)
         && chunk_is_after(pc, after, test_equal));
} // chunk_is_between


/**
 * Returns true if the chunk under test is a reference to a macro defined elsewhere in
 * the source file currently being processed. Note that a macro may be defined in
 * another source or header file, for which this function does not currently account
 */
static bool chunk_is_macro_reference(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   Chunk *next = Chunk::GetHead();

   if (  (  language_is_set(LANG_CPP)
         || language_is_set(LANG_C))
      && chunk_is_token(pc, CT_WORD)
      && !pc->flags.test(PCF_IN_PREPROC))
   {
      while (  next != nullptr
            && next->IsNotNullChunk())
      {
         if (  next->flags.test(PCF_IN_PREPROC)
            && std::strcmp(pc->str.c_str(), next->str.c_str()) == 0)
         {
            return(true);
         }
         next = chunk_search_next_cat(next, CT_MACRO);
      }
   }
   return(false);
} // chunk_is_macro_reference


/**
 * Returns true if the input chunk is a pointer/reference operator or a
 * qualifier
 */
static bool chunk_is_pointer_reference_or_qualifier(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   return(  chunk_is_pointer_or_reference(pc)
         || (  chunk_is_token(pc, CT_QUALIFIER)
            && !chunk_is_cpp_inheritance_access_specifier(pc)));
} // chunk_is_pointer_reference_or_qualifier


/**
 * This function attempts to match the starting and ending chunks of a qualified
 * identifier, which consists of one or more scope resolution operator(s) and
 * zero or more nested name specifiers
 * specifiers
 * @param pc the starting chunk
 * @return   an std::pair, where the first chunk indicates the starting chunk of the
 *           match and second indicates the ending chunk. Upon finding a successful
 *           match, the starting chunk may consist of an identifier or a scope
 *           resolution operator, while the ending chunk may consist of identifier
 *           or the closing angle bracket of a template. If no match is found, a
 *           pair of null chunks is returned
 */
static std::pair<Chunk *, Chunk *> match_qualified_identifier(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   auto *end   = skip_scope_resolution_and_nested_name_specifiers(pc);
   auto *start = skip_scope_resolution_and_nested_name_specifiers_rev(pc);

   if (  end != nullptr
      && start != nullptr)
   {
      auto *double_colon = chunk_search_next_cat(start, CT_DC_MEMBER);

      if (  double_colon != nullptr
         && chunk_is_between(double_colon, start, end))
      {
         return(std::make_pair(start, end));
      }
   }
   return(std::make_pair(nullptr, nullptr));
} // match_qualified_identifier


/**
 * Starting from the input chunk, this function attempts to match a variable
 * declaration/definition in both the forward and reverse directions; each pair of
 * consecutive chunks is tested to determine if a potential match is satisfied.
 * @param pc    the starting chunk
 * @param level the brace level
 * @return      upon successful match, function returns an std::tuple, where the
 *              first chunk indicates the starting chunk, the second chunk indicates
 *              the identifier name, and the third chunk indicates the end associated
 *              with the variable declaration/definition
 */
static std::tuple<Chunk *, Chunk *, Chunk *> match_variable(Chunk *pc, std::size_t level)
{
   LOG_FUNC_ENTRY();

   auto identifier_end_pair   = match_variable_end(pc, level);
   auto start_identifier_pair = match_variable_start(pc, level);
   auto *end                  = identifier_end_pair.second;
   auto *identifier           = identifier_end_pair.first != nullptr ? identifier_end_pair.first : start_identifier_pair.second;
   auto *start                = start_identifier_pair.first;

   /**
    * a forward search starting at the chunk under test will fail if two consecutive chunks marked as CT_WORD
    * are encountered; in that case, it's likely that the preceding chunk indicates a type and the subsequent
    * chunk indicates a variable declaration/definition
    */

   if (  identifier->IsNotNullChunk()
      && start->IsNotNullChunk()
      && (  end != nullptr
         || chunk_is_token(identifier->GetPrevNcNnlNi(), CT_WORD)))
   {
      return(std::make_tuple(start, identifier, end));
   }
   return(std::make_tuple(nullptr, nullptr, nullptr));
} // match_variable


/**
 * Starting from the input chunk, this function attempts to match a variable in the
 * forward direction, and tests each pair of consecutive chunks to determine if a
 * potential variable declaration/definition match is satisfied. Secondly, the
 * function attempts to identify the end chunk associated with the candidate variable
 * match. For scalar variables (simply declared and not defined), both the end chunk
 * and identifier chunk should be one in the same
 * @param pc    the starting chunk
 * @param level the brace level
 * @return      an std::pair, where the first chunk indicates the identifier
 *              (if non-null) and the second chunk indicates the end associated with
 *              the variable declaration/definition; assuming a valid match, the first
 *              chunk may be null if the function is called with a starting chunk
 *              that occurs after the identifier
 */
static std::pair<Chunk *, Chunk *> match_variable_end(Chunk *pc, std::size_t level)
{
   LOG_FUNC_ENTRY();

   Chunk *identifier = nullptr;

   while (  pc != nullptr
         && pc->IsNotNullChunk())
   {
      /**
       * skip any right-hand side assignments
       */
      Chunk *rhs_exp_end = nullptr;

      if (chunk_is_token(pc, CT_ASSIGN))
      {
         /**
          * store a pointer to the end chunk of the rhs expression;
          * use it later to test against setting the identifier
          */
         rhs_exp_end = skip_to_expression_end(pc);
         pc          = rhs_exp_end;
      }

      /**
       * skip current and preceding chunks if at a higher brace level
       */
      while (  pc != nullptr
            && pc->IsNotNullChunk()
            && pc->level > level)
      {
         pc = pc->GetNextNcNnl();
      }

      /**
       * skip to any following match for angle brackets, braces, parens,
       * or square brackets
       */
      if (  chunk_is_token(pc, CT_ANGLE_OPEN)
         || chunk_is_token(pc, CT_BRACE_OPEN)
         || chunk_is_paren_open(pc)
         || chunk_is_token(pc, CT_SQUARE_OPEN))
      {
         pc = chunk_skip_to_match(pc, E_Scope::PREPROC);
      }
      /**
       * call a separate function to validate adjacent tokens as potentially
       * matching a variable declaration/definition
       */

      Chunk *next = pc->GetNextNcNnl();

      if (  chunk_is_not_token(next, CT_COMMA)
         && chunk_is_not_token(next, CT_FPAREN_CLOSE)
         && !chunk_is_semicolon(next)
         && !adj_tokens_match_var_def_pattern(pc, next))
      {
         /**
          * error, pattern is not consistent with a variable declaration/definition
          */

         break;
      }

      if (  chunk_is_token(pc, CT_WORD)
         && pc != rhs_exp_end)
      {
         /**
          * we've encountered a candidate for the variable name
          */

         identifier = pc;
      }

      /**
       * we're done searching if we've previously identified a variable name
       * and then encounter a comma or semicolon
       */
      if (  chunk_is_token(next, CT_COMMA)
         || chunk_is_token(next, CT_FPAREN_CLOSE)
         || chunk_is_semicolon(next))
      {
         return(std::make_pair(identifier, pc));
      }
      pc = next;
   }
   return(std::make_pair(nullptr, nullptr));
} // match_variable_end


/**
 * Starting from the input chunk, this function attempts to match a variable in the
 * reverse direction, and tests each pair of consecutive chunks to determine if a
 * potential variable declaration/definition match is satisfied. Secondly, the
 * function attempts to identify the starting chunk associated with the candidate
 * variable match. The start and identifier chunks may refer to each other in cases
 * where the identifier is not preceded by pointer or reference operators or qualifiers,
 * etc.
 * @param pc    the starting chunk
 * @param level the brace level
 * @return      an std::pair, where the first chunk indicates the starting chunk and
 *              the second chunk indicates the identifier associated with the variable
 *              match; assuming a valid match, the second chunk may be null if the
 *              function is called with a starting chunk that occurs before the
 *              identifier
 */
static std::pair<Chunk *, Chunk *> match_variable_start(Chunk *pc, std::size_t level)
{
   LOG_FUNC_ENTRY();

   Chunk *identifier = Chunk::NullChunkPtr;

   if (pc == nullptr)
   {
      pc = Chunk::NullChunkPtr;
   }

   while (pc->IsNotNullChunk())
   {
      /**
       * skip any right-hand side assignments
       */
      Chunk *before_rhs_exp_start = skip_expression_rev(pc);
      Chunk *prev                 = Chunk::NullChunkPtr;
      Chunk *next                 = pc;

      while (  chunk_is_after(next, before_rhs_exp_start)
            && pc != prev)
      {
         next = prev;
         prev = next->GetPrevNcNnlNi();

         if (chunk_is_token(next, CT_ASSIGN))
         {
            pc = prev;
         }
      }
      /**
       * skip current and preceding chunks if at a higher brace level
       */

      while (  pc->IsNotNullChunk()
            && pc->level > level)
      {
         pc = pc->GetPrevNcNnlNi();
      }

      /**
       * skip to any preceding match for angle brackets, braces, parens,
       * or square brackets
       */
      if (  chunk_is_token(pc, CT_ANGLE_CLOSE)
         || chunk_is_token(pc, CT_BRACE_CLOSE)
         || chunk_is_paren_close(pc)
         || chunk_is_token(pc, CT_SQUARE_CLOSE))
      {
         pc = chunk_skip_to_match_rev(pc, E_Scope::PREPROC);
      }
      /**
       * call a separate function to validate adjacent tokens as potentially
       * matching a variable declaration/definition
       */

      prev = pc->GetPrevNcNnlNi();

      if (!adj_tokens_match_var_def_pattern(prev, pc))
      {
         /**
          * perhaps the previous chunk possibly indicates a type that yet to be
          * marked? if not, then break
          */
         if (  chunk_is_not_token(prev, CT_WORD)
            || (  !chunk_is_pointer_or_reference(pc)
               && chunk_is_not_token(pc, CT_WORD)))
         {
            /**
             * error, pattern is not consistent with a variable declaration/definition
             */

            break;
         }
      }

      if (  identifier->IsNullChunk()
         && chunk_is_token(pc, CT_WORD))
      {
         /**
          * we've encountered a candidate for the variable name
          */

         identifier = pc;
      }

      /**
       * we're done searching if we've previously identified a variable name
       * and then encounter another identifier, or we encounter a closing
       * brace (which would likely indicate an inline variable definition)
       */
      if (  chunk_is_token(prev, CT_ANGLE_CLOSE)
         || chunk_is_token(prev, CT_BRACE_CLOSE)
         || chunk_is_token(prev, CT_COMMA)
         || chunk_is_token(prev, CT_TYPE)
         || chunk_is_token(prev, CT_WORD))
      {
         return(std::make_pair(pc, identifier));
      }
      pc = prev;
   }
   return(std::make_pair(Chunk::NullChunkPtr, Chunk::NullChunkPtr));
} // match_variable_start


/**
 * Skip forward past any scope resolution operators and nested name specifiers and return
 * just the qualified identifier name; while similar to the existing skip_dc_member()
 * function, this function also takes into account templates that may comprise any
 * nested name specifiers
 */
static Chunk *skip_scope_resolution_and_nested_name_specifiers(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (  (  pc != nullptr
         && pc->flags.test(PCF_IN_TEMPLATE))
      || chunk_is_token(pc, CT_DC_MEMBER)
      || chunk_is_token(pc, CT_TYPE)
      || chunk_is_token(pc, CT_WORD))
   {
      while (  pc != nullptr
            && pc->IsNotNullChunk())
      {
         /**
          * skip to any following match for angle brackets
          */
         if (chunk_is_token(pc, CT_ANGLE_OPEN))
         {
            pc = chunk_skip_to_match(pc, E_Scope::PREPROC);
         }
         Chunk *next = pc->GetNextNcNnl();

         /**
          * call a separate function to validate adjacent tokens as potentially
          * matching a qualified identifier
          */
         if (!adj_tokens_match_qualified_identifier_pattern(pc, next))
         {
            break;
         }
         pc = next;
      }
   }
   return(pc);
} // skip_scope_resolution_and_nested_name_specifiers


/**
 * Skip in reverse to the beginning chunk of a qualified identifier; while similar to
 * the existing skip_dc_member_rev() function, this function also takes into account
 * templates that may comprise any nested name specifiers
 */
static Chunk *skip_scope_resolution_and_nested_name_specifiers_rev(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc == nullptr)
   {
      pc = Chunk::NullChunkPtr;
   }

   if (  (  pc->IsNotNullChunk()
         && pc->flags.test(PCF_IN_TEMPLATE))
      || chunk_is_token(pc, CT_DC_MEMBER)
      || chunk_is_token(pc, CT_TYPE)
      || chunk_is_token(pc, CT_WORD))
   {
      while (pc->IsNotNullChunk())
      {
         /**
          * skip to any preceding match for angle brackets
          */
         if (chunk_is_token(pc, CT_ANGLE_CLOSE))
         {
            pc = chunk_skip_to_match_rev(pc, E_Scope::PREPROC);
         }
         Chunk *prev = pc->GetPrevNcNnlNi();

         /**
          * call a separate function to validate adjacent tokens as potentially
          * matching a qualified identifier
          */
         if (!adj_tokens_match_qualified_identifier_pattern(prev, pc))
         {
            break;
         }
         pc = prev;
      }
   }
   return(pc);
} // skip_scope_resolution_and_nested_name_specifiers_rev


EnumStructUnionParser::EnumStructUnionParser()
   : m_end(nullptr)
   , m_parse_error(false)
   , m_start(nullptr)
   , m_type(nullptr)
{
} // EnumStructUnionParser::EnumStructUnionParser


EnumStructUnionParser::~EnumStructUnionParser()
{
} // EnumStructUnionParser::~EnumStructUnionParser


void EnumStructUnionParser::analyze_identifiers()
{
   LOG_FUNC_ENTRY();

   /**
    * the enum (and variable declarations thereof) could be of
    * the following forms:
    *
    * "enum type [: integral_type] { ... } [x, ...]"
    * "enum type : integral_type"
    * "enum type x, ..."
    * "enum class type [: integral_type] { ... } [x, ...]"
    * "enum class type [: integral_type]"
    * "enum [: integral_type] { ... } x, ..."
    */

   /**
    * the class/struct (and variable declarations thereof) could be of
    * the following forms:
    *
    * "template<...> class/struct[<...>] [macros/attributes ...] type [: bases ...] { }"
    * "template<...> class/struct[<...>] [macros/attributes ...] type"
    * "class/struct [macros/attributes ...] type [: bases ...] { } [x, ...]"
    * "class/struct [macros/attributes ...] type [x, ...]"
    * "class/struct [macros/attributes ...] [: bases] { } x, ..."
    */

   Chunk       *template_end      = get_template_end();
   auto        *body_end          = get_body_end();
   auto        *body_start        = get_body_start();
   pcf_flags_t flags              = PCF_VAR_1ST_DEF;
   auto        *inheritance_start = get_inheritance_start();
   Chunk       *pc                = body_end ? body_end : m_start;

   /**
    * first, try a simple approach to identify any associated type
    */
   if (try_pre_identify_type())
   {
      /**
       * a type was identified, meaning a pair of braces, angle brackets, or
       * a colon was found; if a colon was found, then there should be a
       * balanced set of braces that follow; therefore, start the search for
       * variable identifiers after the closing brace or close angle bracket
       */

      if (body_end != nullptr)
      {
         pc = body_end;
      }
      else if (template_end != nullptr)
      {
         pc = template_end;
      }
   }

   if (pc == nullptr)
   {
      pc = Chunk::NullChunkPtr;
   }

   if (pc->GetNextNcNnl() == m_end)
   {
      /**
       * we're likely at the end of a class/enum/struct/union body which lacks
       * any trailing inline definitions
       */

      pc = m_end->GetNextNcNnl();
   }

   if (  type_identified()
      || chunk_is_class_enum_struct_union(pc)
      || pc == m_end)
   {
      /**
       * in case we're pointing at the end chunk, advance the chunk pointer
       * by one more so that we don't perform a variable identifier search
       * below
       */
      pc = pc->GetNextNcNnl();
   }

   if (body_end != nullptr)
   {
      /**
       * a closing brace was found, so any identifiers trailing the closing
       * brace are probably inline variable declarations following a
       * class/enum/struct/union definition
       */
      flags |= PCF_VAR_INLINE;
   }
   else if (!type_identified())
   {
      /**
       * skip any chain of one or more function-like macro calls,
       * declspecs, and attributes
       */

      Chunk *tmp = pc;

      do
      {
         pc  = tmp;
         tmp = skip_attribute_next(tmp);
         tmp = skip_declspec_next(tmp);
      } while (tmp != pc);
   }
   /**
    * try to match some variable identifiers in the loop below
    */

   while (chunk_is_between(pc, m_start, m_end, false))
   {
      auto match       = match_variable(pc, m_start->level);
      auto *start      = std::get<0>(match);
      auto *identifier = std::get<1>(match);
      auto *end        = std::get<2>(match);

      if (  start != nullptr
         && identifier != nullptr)
      {
         if (end != nullptr)
         {
            mark_variable(identifier, flags);

            if (flags & PCF_VAR_1ST)
            {
               flags &= ~PCF_VAR_1ST; // clear the first flag for the next items
            }
         }
      }

      if (end != nullptr)
      {
         pc = end;
      }
      pc = pc->GetNextNcNnl();

      /**
       * skip any right-hand side assignments
       */
      if (chunk_is_token(pc, CT_ASSIGN))
      {
         pc = skip_to_expression_end(pc);
      }

      /**
       * if we're sitting at a comma or semicolon, skip it
       */
      if (  chunk_is_semicolon(pc)
         || (  chunk_is_token(pc, CT_COMMA)
            && !pc->flags.test_any(PCF_IN_FCN_DEF | PCF_IN_FCN_CALL | PCF_IN_TEMPLATE)
            && !chunk_is_between(pc, inheritance_start, body_start)))
      {
         pc = pc->GetNextNcNnl();
      }
   }
   /**
    * if we still haven't identified a type, try doing so now that the
    * variables, if any, have been marked
    */
   try_post_identify_type();

   /**
    * identify possible macros preceding the type name
    */
   try_post_identify_macro_calls();

   if (  chunk_is_class_or_struct(m_start)
      && (  chunk_is_not_token(m_start, CT_STRUCT)
         || !language_is_set(LANG_C)))
   {
      /**
       * if a type has been identified, mark any constructor matching constructor
       * declarations/definitions
       */
      mark_constructors();
   }

   if (type_identified())
   {
      if (~flags & PCF_VAR_1ST)
      {
         /**
          * PCF_VAR_1ST was cleared and a type was identified; therefore, set
          * PCF_VAR_TYPE for the identified type
          */
         chunk_flags_set(m_type, PCF_VAR_TYPE);
      }
      else if (~flags & PCF_VAR_INLINE)
      {
         /**
          * if a type was identified but no braced-enclosed body was found and no
          * identifiers were marked as variables, then we're likely we're likely
          * dealing with a forward declaration
          */
         flag_series(m_start, m_type, PCF_INCOMPLETE);
      }
   }
} // EnumStructUnionParser::analyze_identifiers


bool EnumStructUnionParser::body_detected() const
{
   LOG_FUNC_ENTRY();

   auto *body_end   = get_body_end();
   auto *body_start = get_body_start();

   return(  body_end != nullptr
         && body_start != nullptr);
} // EnumStructUnionParser::body_detected


bool EnumStructUnionParser::comma_separated_values_detected() const
{
   LOG_FUNC_ENTRY();

   return(!get_top_level_commas().empty());
} // EnumStructUnionParser::comma_separated_values_detected


bool EnumStructUnionParser::enum_base_detected() const
{
   LOG_FUNC_ENTRY();

   return(m_chunk_map.find(CT_BIT_COLON) != m_chunk_map.cend());
} // EnumStructUnionParser::enum_base_detected


Chunk *EnumStructUnionParser::get_body_end() const
{
   LOG_FUNC_ENTRY();

   auto &&it_token_chunk_map_pair = m_chunk_map.find(CT_BRACE_CLOSE);

   if (it_token_chunk_map_pair != m_chunk_map.cend())
   {
      return(it_token_chunk_map_pair->second.at(0));
   }
   return(nullptr);
} // EnumStructUnionParser::get_body_end


Chunk *EnumStructUnionParser::get_body_start() const
{
   LOG_FUNC_ENTRY();

   auto &&it_token_chunk_map_pair = m_chunk_map.find(CT_BRACE_OPEN);

   if (it_token_chunk_map_pair != m_chunk_map.cend())
   {
      return(it_token_chunk_map_pair->second.at(0));
   }
   return(nullptr);
} // EnumStructUnionParser::get_body_start


Chunk *EnumStructUnionParser::get_enum_base_start() const
{
   LOG_FUNC_ENTRY();

   auto &&it_token_chunk_map_pair = m_chunk_map.find(CT_BIT_COLON);

   if (it_token_chunk_map_pair != m_chunk_map.cend())
   {
      return(it_token_chunk_map_pair->second.at(0));
   }
   return(nullptr);
} // EnumStructUnionParser::get_enum_base_start


Chunk *EnumStructUnionParser::get_first_top_level_comma() const
{
   LOG_FUNC_ENTRY();

   auto &&it_token_chunk_map_pair = m_chunk_map.find(CT_COMMA);

   if (it_token_chunk_map_pair != m_chunk_map.cend())
   {
      return(it_token_chunk_map_pair->second.at(0));
   }
   return(nullptr);
} // EnumStructUnionParser::get_first_top_level_comma


Chunk *EnumStructUnionParser::get_inheritance_end() const
{
   LOG_FUNC_ENTRY();

   Chunk *brace_open        = nullptr;
   auto  *inheritance_start = get_inheritance_start();

   if (inheritance_start != nullptr)
   {
      brace_open = get_body_start();

      if (brace_open == nullptr)
      {
         brace_open = inheritance_start->GetNextType(CT_BRACE_OPEN, m_start->level, E_Scope::ALL);
      }
   }
   return(brace_open);
} // EnumStructUnionParser::get_inheritance_end


Chunk *EnumStructUnionParser::get_inheritance_start() const
{
   LOG_FUNC_ENTRY();

   auto &&it_token_chunk_map_pair = m_chunk_map.find(CT_COLON);

   if (it_token_chunk_map_pair != m_chunk_map.cend())
   {
      return(it_token_chunk_map_pair->second.at(0));
   }
   return(nullptr);
} // EnumStructUnionParser::get_inheritance_start


std::map<std::size_t, Chunk *> EnumStructUnionParser::get_question_operators() const
{
   LOG_FUNC_ENTRY();

   auto &&it_token_chunk_map_pair = m_chunk_map.find(CT_QUESTION);

   if (it_token_chunk_map_pair != m_chunk_map.cend())
   {
      return(it_token_chunk_map_pair->second);
   }
   return(std::map<std::size_t, Chunk *>());
} // EnumStructUnionParser::get_question_operators


Chunk *EnumStructUnionParser::get_template_end() const
{
   LOG_FUNC_ENTRY();

   auto &&it_token_chunk_map_pair = m_chunk_map.find(CT_ANGLE_CLOSE);

   if (it_token_chunk_map_pair != m_chunk_map.cend())
   {
      return(it_token_chunk_map_pair->second.at(0));
   }
   return(nullptr);
} // EnumStructUnionParser::get_template_end


Chunk *EnumStructUnionParser::get_template_start() const
{
   LOG_FUNC_ENTRY();

   auto &&it_token_chunk_map_pair = m_chunk_map.find(CT_ANGLE_OPEN);

   if (it_token_chunk_map_pair != m_chunk_map.cend())
   {
      return(it_token_chunk_map_pair->second.at(0));
   }
   return(nullptr);
} // EnumStructUnionParser::get_template_start


std::map<std::size_t, Chunk *> EnumStructUnionParser::get_top_level_commas() const
{
   LOG_FUNC_ENTRY();

   auto &&it_token_chunk_map_pair = m_chunk_map.find(CT_COMMA);

   if (it_token_chunk_map_pair != m_chunk_map.cend())
   {
      return(it_token_chunk_map_pair->second);
   }
   return(std::map<std::size_t, Chunk *>());
} // EnumStructUnionParser::get_top_level_commas


Chunk *EnumStructUnionParser::get_where_end() const
{
   LOG_FUNC_ENTRY();

   Chunk *brace_open  = nullptr;
   auto  *where_start = get_where_start();

   if (where_start != nullptr)
   {
      brace_open = get_body_start();

      if (brace_open == nullptr)
      {
         brace_open = where_start->GetNextType(CT_BRACE_OPEN, m_start->level, E_Scope::ALL);
      }
   }
   return(brace_open);
} // EnumStructUnionParser::get_where_end


Chunk *EnumStructUnionParser::get_where_start() const
{
   LOG_FUNC_ENTRY();

   auto &&it_token_chunk_map_pair = m_chunk_map.find(CT_WHERE);

   if (it_token_chunk_map_pair != m_chunk_map.cend())
   {
      return(it_token_chunk_map_pair->second.at(0));
   }
   return(nullptr);
} // EnumStructUnionParser::get_where_start


bool EnumStructUnionParser::inheritance_detected() const
{
   LOG_FUNC_ENTRY();

   return(m_chunk_map.find(CT_COLON) != m_chunk_map.cend());
} // EnumStructUnionParser::inheritance_detected


void EnumStructUnionParser::initialize(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   parse_error_detected(false);
   m_chunk_map.clear();

   m_start = pc;
   m_type  = nullptr;
   pc      = try_find_end_chunk(pc);
   m_end   = refine_end_chunk(pc);
} // EnumStructUnionParser::initialize


bool EnumStructUnionParser::is_potential_end_chunk(Chunk *pc) const
{
   LOG_FUNC_ENTRY();

   /**
    * test for a semicolon or closing brace at the level of the starting chunk
    */
   if (  pc == nullptr
      || parse_error_detected()
      || (  (  chunk_is_semicolon(pc)
            || chunk_is_token(pc, CT_BRACE_CLOSE))
         && pc->level == m_start->level))
   {
      return(true);
   }
   /**
    * check for the following:
    * 1) did we encounter a closing paren, which may indicate the end of cast?
    * 2) did we cross a preprocessor boundary?
    * 3) did we cross the closing paren of a function signature?
    */

   auto const pc_in_funcdef    = pc->flags & PCF_IN_FCN_DEF;
   auto const pc_in_preproc    = pc->flags & PCF_IN_PREPROC;
   auto const start_in_funcdef = m_start->flags & PCF_IN_FCN_DEF;
   auto const start_in_preproc = m_start->flags & PCF_IN_PREPROC;

   /**
    * the following may identify cases where we've reached the
    * end of a cast terminated by a closing paren
    */
   if (  (  chunk_is_paren_close(pc) // Issue #3538
         && pc->level < m_start->level)
      || (start_in_funcdef ^ pc_in_funcdef).test_any()
      || (start_in_preproc ^ pc_in_preproc).test_any())
   {
      return(true);
   }
   /**
    * check whether the current chunk's nest level is less than that
    * of the starting chunk
    */

   std::size_t pc_template_nest    = get_cpp_template_angle_nest_level(pc);
   std::size_t start_template_nest = get_cpp_template_angle_nest_level(m_start);

   if (start_template_nest > pc_template_nest)
   {
      return(true);
   }
   /**
    * assuming the chunk is within a function call/definition, check the following:
    * 1) chunk is a closing function paren at a lower level than the starting chunk
    * 2) chunk is an assignment ('=') or comma at the level of the starting chunk
    */

   auto const pc_in_funccall    = pc->flags & PCF_IN_FCN_CALL;
   auto const start_in_funccall = m_start->flags & PCF_IN_FCN_CALL;

   if (  (  pc_in_funccall.test_any()
         && start_in_funccall.test_any()
         && chunk_is_token(pc, CT_COMMA)
         && pc->level == m_start->level)
      || (  pc_in_funcdef.test_any()
         && (  (  chunk_is_token(pc, CT_FPAREN_CLOSE)
               && pc->level < m_start->level)
            || (  (  chunk_is_token(pc, CT_ASSIGN)
                  || chunk_is_token(pc, CT_COMMA))
               && pc->level == m_start->level))))
   {
      return(true);
   }
   return(false);
} // EnumStructUnionParser::is_potential_end_chunk


bool EnumStructUnionParser::is_within_conditional(Chunk *pc) const
{
   LOG_FUNC_ENTRY();

   auto question_operators = get_question_operators();

   if (!question_operators.empty())
   {
      auto &&it_token_chunk_pair = question_operators.cbegin();

      while (it_token_chunk_pair != question_operators.cend())
      {
         auto *question = it_token_chunk_pair->second;
         auto *end      = skip_to_expression_end(question);
         auto *start    = skip_to_expression_start(question);

         if (chunk_is_between(pc, start, end))
         {
            return(true);
         }
         ++it_token_chunk_pair;
      }
   }
   return(false);
} // EnumStructUnionParser::is_within_conditional


bool EnumStructUnionParser::is_within_inheritance_list(Chunk *pc) const
{
   LOG_FUNC_ENTRY();

   if (  pc != nullptr
      && pc->flags.test(PCF_IN_CLASS_BASE))
   {
      return(true);
   }
   auto *inheritance_end   = get_inheritance_end();
   auto *inheritance_start = get_inheritance_start();

   if (  inheritance_end != nullptr
      && inheritance_start != nullptr)
   {
      return(chunk_is_between(pc, inheritance_start, inheritance_end));
   }
   return(false);
} // EnumStructUnionParser::is_within_inheritance_list


bool EnumStructUnionParser::is_within_where_clause(Chunk *pc) const
{
   LOG_FUNC_ENTRY();

   if (  pc != nullptr
      && pc->flags.test(PCF_IN_WHERE_SPEC))
   {
      return(true);
   }
   auto *where_end   = get_where_end();
   auto *where_start = get_where_start();

   if (  where_end != nullptr
      && where_start != nullptr)
   {
      return(chunk_is_between(pc, where_start, where_end));
   }
   return(false);
} // EnumStructUnionParser::is_within_where_clause


void EnumStructUnionParser::mark_base_classes(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   pcf_flags_t flags = PCF_VAR_1ST_DEF;

   while (pc != nullptr)
   {
      chunk_flags_set(pc, PCF_IN_CLASS_BASE);
      /**
       * clear the PCF_VAR_TYPE flag for all chunks within the inheritance list
       * TODO: this may not be necessary in the future once code outside this
       *       class is improved such that PCF_VAR_TYPE is not set for these chunks
       */
      pc->flags &= ~PCF_VAR_TYPE;

      Chunk *next = pc->GetNextNcNnl(E_Scope::PREPROC);

      if (chunk_is_token(next, CT_DC_MEMBER))
      {
         /**
          * just in case it's a templated type
          */
         pc = skip_template_prev(pc);

         if (chunk_is_token(pc, CT_WORD))
         {
            /**
             * TODO:
             * To comply with conventions used elsewhere in the code, we're going
             * to change chunks marked CT_WORD to CT_TYPE if followed by a scope-
             * resolution operator; if a chunk marked CT_WORD is followed by a set
             * of angle brackets, then it's obviously a templated type. However,
             * in the absence of a pair trailing angle brackets, the chunk may be
             * a namespace rather than a type. Need to revisit this!
             */
            set_chunk_type(pc, CT_TYPE);
         }
      }
      else if (  (  chunk_is_token(next, CT_BRACE_OPEN)
                 || (  chunk_is_token(next, CT_COMMA)
                    && !is_within_where_clause(next)))
              && next->level == m_start->level)
      {
         /**
          * just in case it's a templated type
          */
         pc = skip_template_prev(pc);

         if (chunk_is_token(pc, CT_WORD))
         {
            chunk_flags_set(pc, flags);

            if (flags & PCF_VAR_1ST)
            {
               flags &= ~PCF_VAR_1ST; // clear the first flag for the next items
            }
         }

         if (chunk_is_token(next, CT_BRACE_OPEN))
         {
            break;
         }
      }
      pc = next;
   }
   chunk_flags_set(pc, PCF_IN_CLASS_BASE);
} // EnumStructUnionParser::mark_base_classes


void EnumStructUnionParser::mark_braces(Chunk *brace_open)
{
   LOG_FUNC_ENTRY();

   pcf_flags_t flags = PCF_NONE;

   if (chunk_is_token(m_start, CT_CLASS))
   {
      flags = PCF_IN_CLASS;
   }
   else if (chunk_is_enum(m_start))
   {
      flags = PCF_IN_ENUM;
   }
   else if (chunk_is_token(m_start, CT_STRUCT))
   {
      flags = PCF_IN_STRUCT;
   }
   /**
    * TODO: why does flag_parens() flag the closing paren,
    *       but it doesn't flag the opening paren?
    */

   flag_parens(brace_open,
               flags,
               CT_NONE,
               CT_NONE,
               false);

   if (chunk_is_class_struct_union(m_start))
   {
      mark_struct_union_body(brace_open);

      auto *inheritance_start = get_inheritance_start();

      if (inheritance_start != nullptr)
      {
         /**
          * the class/struct/union is a derived class; mark the base
          * classes between the colon/java "implements" keyword and the
          * opening brace
          */

         mark_base_classes(inheritance_start);
      }
   }
   set_chunk_parent(brace_open, m_start->type);

   auto *brace_close = chunk_skip_to_match(brace_open, E_Scope::PREPROC);

   if (brace_close != nullptr)
   {
      set_chunk_parent(brace_close, m_start->type);
   }
} // EnumStructUnionParser::mark_braces


void EnumStructUnionParser::mark_class_colon(Chunk *colon)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LFTOR,
           "%s(%d): Class colon detected: orig_line is %zu, orig_col is %zu\n",
           __unqualified_func__,
           __LINE__,
           colon->orig_line,
           colon->orig_col);

   set_chunk_type(colon, CT_CLASS_COLON);
   set_chunk_parent(colon, m_start->type);
} // EnumStructUnionParser::mark_class_colon


void EnumStructUnionParser::mark_conditional_colon(Chunk *colon)
{
   set_chunk_type(colon, CT_COND_COLON);
} // EnumStructUnionParser::mark_conditional_colon


void EnumStructUnionParser::mark_constructors()
{
   LOG_FUNC_ENTRY();

   /**
    * if a type was previously identified, then look for
    * class/struct constructors in the body
    */
   if (  body_detected()
      && type_identified()
      && chunk_is_class_or_struct(m_start))
   {
      LOG_FMT(LFTOR,
              "%s(%d): orig_line is %zu, orig_col is %zu, start is '%s', parent_type is %s\n",
              __unqualified_func__,
              __LINE__,
              m_start->orig_line,
              m_start->orig_col,
              m_start->Text(),
              get_token_name(get_chunk_parent_type(m_start)));

      log_pcf_flags(LFTOR, m_start->flags);

      /**
       * get the name of the type
       */
      auto *body_end   = get_body_end();
      auto *body_start = get_body_start();
      auto *name       = m_type->Text();

      LOG_FMT(LFTOR,
              "%s(%d): Name of type is '%s'\n",
              __unqualified_func__,
              __LINE__,
              name);
      log_pcf_flags(LFTOR, m_type->flags);

      Chunk       *next = Chunk::NullChunkPtr;
      std::size_t level = m_type->brace_level + 1;

      for (auto *prev = body_start; next != body_end; prev = next)
      {
         chunk_flags_set(prev, PCF_IN_CLASS);

         next = skip_template_next(prev->GetNextNcNnl(E_Scope::PREPROC));                         // Issue #3368

         /**
          * find a chunk within the class/struct body that
          */
         if (  prev->IsNotNullChunk()
            && std::strcmp(prev->Text(), name) == 0
            && prev->level == level
            && chunk_is_paren_open(next))
         {
            set_chunk_type(prev, CT_FUNC_CLASS_DEF);

            LOG_FMT(LFTOR,
                    "%s(%d): Constructor/destructor detected: '%s' at orig_line is %zu, orig_col is %zu, type is %s\n",
                    __unqualified_func__,
                    __LINE__,
                    name,
                    prev->orig_line,
                    prev->orig_col,
                    get_token_name(prev->type));

            mark_cpp_constructor(prev);
         }
      }

      chunk_flags_set(next, PCF_IN_CLASS);
   }
} // EnumStructUnionParser::mark_constructor


void EnumStructUnionParser::mark_enum_integral_type(Chunk *colon)
{
   LOG_FUNC_ENTRY();

   set_chunk_type(colon, CT_BIT_COLON);
   set_chunk_parent(colon, m_start->type);

   auto *body_start = get_body_start();
   auto *pc         = colon->GetNextNcNnl();

   /**
    * the chunk(s) between the colon and opening
    * brace (if present) should specify the enum's
    * integral type
    */

   while (  chunk_is_between(pc, m_start, m_end)
         && pc != body_start
         && chunk_is_not_token(pc, CT_BRACE_OPEN)
         && !chunk_is_semicolon(pc))
   {
      /**
       * clear the PCF_VAR_TYPE flag for all chunks within the enum integral base
       * TODO: this may not be necessary in the future once code outside this
       *       class is improved such that PCF_VAR_TYPE is not set for these chunks
       */
      if (chunk_is_not_token(pc, CT_DC_MEMBER))                             // Issue #3198
      {
         pc->flags &= ~PCF_VAR_TYPE;
         set_chunk_type(pc, CT_TYPE);
         set_chunk_parent(pc, colon->type);
      }
      pc = pc->GetNextNcNnl();
   }
} // EnumStructUnionParser::mark_enum_integral_type


void EnumStructUnionParser::mark_extracorporeal_lvalues()
{
   /**
    * clear the PCF_LVALUE flag for all chunks outside the body definition,
    * as this flag may have been set elsewhere by code outside this class
    * TODO: the mark_lvalue() function needs some improvement so that the
    *       following isn't necessary
    */
   Chunk *next = m_start;
   Chunk *prev = Chunk::NullChunkPtr;

   /**
    * if the class is a template, go the extra step and correct the
    * erroneously marked chunks - as previously mentioned, this likely
    * won't be necessary with improvements to the mark_lvalue() function
    */
   if (get_chunk_parent_type(next) == CT_TEMPLATE)
   {
      while (true)
      {
         prev = next->GetPrevNcNnlNi();

         if (  prev->IsNullChunk()
            || (  !prev->flags.test(PCF_IN_TEMPLATE)
               && chunk_is_not_token(prev, CT_TEMPLATE)))
         {
            break;
         }
         next = prev;
      }
   }
   Chunk *body_end   = get_body_end();
   Chunk *body_start = get_body_start();

   while (next != m_end)
   {
      if (  !chunk_is_between(next, body_start, body_end)
         && next->flags.test(PCF_LVALUE))
      {
         next->flags &= ~PCF_LVALUE;
      }
      else if (  (  chunk_is_token(next, CT_ASSIGN)
                 || chunk_is_token(next, CT_BRACE_OPEN))
              && chunk_is_token(prev, CT_WORD)
              && prev->flags.test_any(PCF_VAR_DEF | PCF_VAR_1ST | PCF_VAR_INLINE))
      {
         chunk_flags_set(prev, PCF_LVALUE);
      }
      prev = next;
      next = next->GetNextNcNnl();
   }
} // EnumStructUnionParser::mark_extracorporeal_lavlues


void EnumStructUnionParser::mark_nested_name_specifiers(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   auto start_end_pair = match_qualified_identifier(pc);
   auto start          = start_end_pair.first;
   auto end            = start_end_pair.second;

   for (pc = start; chunk_is_between(pc, start, end); pc = pc->GetNextNcNnl())
   {
      if (chunk_is_token(pc, CT_WORD))
      {
         /**
          * if the next token is an opening angle, then we can safely
          * mark the current identifier as a type
          */
         auto *next = pc->GetNextNcNnl();

         if (chunk_is_token(next, CT_ANGLE_OPEN))
         {
            /**
             * the template may have already been previously marked elsewhere...
             */
            auto *angle_open  = next;
            auto *angle_close = chunk_skip_to_match(angle_open, E_Scope::PREPROC);

            if (angle_close == nullptr)
            {
               // parse error
               parse_error_detected(true);

               // TODO: should this be just a warning or an error (with exit condition?)
               LOG_FMT(LWARN,
                       "%s(%d): Unmatched '<' at orig_line is %zu, orig_col is %zu\n",
                       __unqualified_func__,
                       __LINE__,
                       angle_open->orig_line,
                       angle_open->orig_col);

               break;
            }
            set_chunk_type(pc, CT_TYPE);
            mark_template(next);
            pc = angle_close;
         }
         else if (  is_within_inheritance_list(pc)
                 && (  chunk_is_token(next, CT_COMMA)
                    || chunk_is_token(next, CT_BRACE_OPEN)))
         {
            set_chunk_type(pc, CT_TYPE);
         }
      }
   }
} // EnumStructUnionParser::mark_nested_name_specifiers


void EnumStructUnionParser::mark_pointer_types(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (chunk_is_token(pc, CT_WORD))
   {
      do
      {
         // TODO: should there be a CT_BYREF_TYPE?
         pc = pc->GetPrevNcNnlNi();

         if (chunk_is_ptr_operator(pc))
         {
            set_chunk_parent(pc, m_start->type);
            set_chunk_type(pc, CT_PTR_TYPE);
         }
      } while (chunk_is_pointer_reference_or_qualifier(pc));
   }
} // EnumStructUnionParser::mark_pointer_types


void EnumStructUnionParser::mark_template(Chunk *start) const
{
   LOG_FUNC_ENTRY();

   if (start != nullptr)
   {
      LOG_FMT(LTEMPL,
              "%s(%d): Template detected: '%s' at orig_line %zu, orig_col %zu\n",
              __unqualified_func__,
              __LINE__,
              start->Text(),
              start->orig_line,
              start->orig_col);
   }
   set_chunk_parent(start, CT_TEMPLATE);

   auto *end = chunk_skip_to_match(start, E_Scope::PREPROC);

   if (end != nullptr)
   {
      set_chunk_parent(end, CT_TEMPLATE);

      mark_template_args(start, end);
   }
} // EnumStructUnionParser::mark_template


void EnumStructUnionParser::mark_template_args(Chunk *start, Chunk *end) const
{
   LOG_FUNC_ENTRY();

   if (  end != nullptr
      && start != nullptr)
   {
      LOG_FMT(LTEMPL,
              "%s(%d): Start of template detected: '%s' at orig_line %zu, orig_col %zu\n",
              __unqualified_func__,
              __LINE__,
              start->Text(),
              start->orig_line,
              start->orig_col);

      pcf_flags_t flags = PCF_IN_TEMPLATE;
      Chunk       *next = start;

      /**
       * TODO: for now, just mark the chunks within the template as PCF_IN_TEMPLATE;
       *       we probably need to create a TemplateParser class to handle all
       *       things template-related
       */

      while (true)
      {
         next = next->GetNextNcNnl();

         if (next == end)
         {
            break;
         }
         chunk_flags_set(next, flags);
      }
      LOG_FMT(LTEMPL,
              "%s(%d): End of template detected: '%s' at orig_line %zu, orig_col %zu\n",
              __unqualified_func__,
              __LINE__,
              end->Text(),
              end->orig_line,
              end->orig_col);
   }
} // EnumStructUnionParser::mark_template_args


void EnumStructUnionParser::mark_type(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc != nullptr)
   {
      m_type = pc;

      do
      {
         make_type(pc);
         set_chunk_parent(pc, m_start->type);
         pc = pc->GetNextNcNnl(E_Scope::PREPROC);
      } while (chunk_is_pointer_or_reference(pc));
   }
} // EnumStructUnionParser::mark_type


void EnumStructUnionParser::mark_variable(Chunk *variable, pcf_flags_t flags)
{
   LOG_FUNC_ENTRY();

   if (variable != nullptr)
   {
      LOG_FMT(LVARDEF,
              "%s(%d): Variable definition detected: '%s' at orig_line is %zu, orig_col is %zu, set %s\n",
              __unqualified_func__,
              __LINE__,
              variable->Text(),
              variable->orig_line,
              variable->orig_col,
              flags & PCF_VAR_1ST_DEF ? "PCF_VAR_1ST_DEF" : "PCF_VAR_1ST");

      chunk_flags_set(variable, flags);
      set_chunk_type(variable, CT_WORD);
      mark_pointer_types(variable);
   }
} // EnumStructUnionParser::mark_variable


void EnumStructUnionParser::mark_where_clause(Chunk *where)
{
   LOG_FUNC_ENTRY();

   if (where != nullptr)
   {
      LOG_FMT(LFTOR,
              "%s(%d): Where clause detected: orig_line is %zu, orig_col is %zu\n",
              __unqualified_func__,
              __LINE__,
              where->orig_line,
              where->orig_col);
   }
   set_where_start(where);

   auto *where_end   = get_where_end();
   auto *where_start = get_where_start();

   set_where_end(where_end);

   pcf_flags_t flags;

   for (auto *pc = where_start; pc != where_end; pc = pc->GetNextNcNnl())
   {
      flags = mark_where_chunk(pc, m_start->type, flags);
   }
} // EnumStructUnionParser::mark_where_clause


void EnumStructUnionParser::mark_where_colon(Chunk *colon)
{
   LOG_FUNC_ENTRY();

   if (colon != nullptr)
   {
      LOG_FMT(LFTOR,
              "%s(%d): Where colon detected: orig_line is %zu, orig_col is %zu\n",
              __unqualified_func__,
              __LINE__,
              colon->orig_line,
              colon->orig_col);
   }
   set_chunk_type(colon, CT_WHERE_COLON);
   set_chunk_parent(colon, m_start->type);
} // EnumStructUnionParser::mark_where_colon


void EnumStructUnionParser::parse(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   initialize(pc);

   /**
    * make sure this wasn't a cast, and also make sure we're
    * actually dealing with a class/enum/struct/union type
    */
   if (  get_chunk_parent_type(m_start) == CT_C_CAST
      || !chunk_is_class_enum_struct_union(m_start))
   {
      return;
   }
   Chunk *prev = m_start;
   Chunk *next = prev->GetNextNcNnl();

   /**
    * the enum-key might be enum, enum class or enum struct
    */
   if (chunk_is_enum(next))
   {
      prev = next;
      next = prev->GetNextNcNnl();
   }
   else if (chunk_is_enum(prev))
   {
      auto *prev_prev = prev->GetPrevNcNnlNi();

      if (  chunk_is_enum(prev_prev)
         && chunk_is_enum(prev))
      {
         m_start = prev_prev;
      }
   }
   /**
    * pre-process all chunks between the starting and ending chunks identified
    * in the initial pass
    */

   while (chunk_is_between(next, m_start, m_end))
   {
      /**
       * skip attributes
       */
      next = skip_attribute(next);

      /**
       * skip declspec
       */
      next = skip_declspec(next);

      /**
       * skip any right-hand side assignments
       */
      if (chunk_is_token(next, CT_ASSIGN))
      {
         next = skip_to_expression_end(next);
      }

      if (  chunk_is_token(next, CT_ANGLE_OPEN)
         && !template_detected())
      {
         next = parse_angles(next);
      }
      else if (  chunk_is_token(next, CT_BRACE_OPEN)
              && !body_detected())
      {
         next = parse_braces(next);
      }
      else if (chunk_is_colon(next))
      {
         parse_colon(next);
      }
      else if (chunk_is_token(next, CT_COMMA))
      {
         record_top_level_comma(next);
      }
      else if (chunk_is_token(next, CT_DC_MEMBER))
      {
         next = parse_double_colon(next);
      }
      else if (  chunk_is_paren_open(next)
              && (  language_is_set(LANG_D)
                 || (  language_is_set(LANG_PAWN)
                    && chunk_is_enum(m_start))))
      {
         set_paren_parent(next, m_start->type);

         if (  chunk_is_token(prev, CT_WORD)
            && language_is_set(LANG_D))
         {
            mark_template(next);
         }
         next = chunk_skip_to_match(next, E_Scope::PREPROC);
      }
      else if (  chunk_is_token(next, CT_QUALIFIER)
              && language_is_set(LANG_JAVA)
              && std::strncmp(next->str.c_str(), "implements", 10) == 0)
      {
         mark_base_classes(next);
      }
      else if (chunk_is_token(next, CT_QUESTION))
      {
         record_question_operator(next);
      }
      else if (  chunk_is_token(next, CT_WHERE)
              && !where_clause_detected())
      {
         mark_where_clause(next);
      }
      prev = next;

      do
      {
         next = next->GetNextNcNnl();
      } while (  next->IsNotNullChunk()
              && next->level > m_start->level);
   }
   /**
    * identify the type and/or variable(s)
    */
   analyze_identifiers();

   /**
    * identify and mark lvalues occurring outside the body definition
    */
   mark_extracorporeal_lvalues();

   if (  prev != nullptr
      && chunk_is_semicolon(prev)
      && prev->level == m_start->level
      && !prev->flags.test(PCF_IN_FOR))
   {
      set_chunk_parent(prev, m_start->type);
   }
} // EnumStructUnionParser::parse


Chunk *EnumStructUnionParser::parse_angles(Chunk *angle_open)
{
   LOG_FUNC_ENTRY();

   /**
    * first check to see if the open angle occurs within an inheritance list
    */
   auto *pc = angle_open;

   if (!is_within_inheritance_list(pc))
   {
      /**
       * check to see if there's a matching closing angle bracket
       */
      auto *angle_close = chunk_skip_to_match(angle_open, E_Scope::PREPROC);

      if (angle_close == nullptr)
      {
         // parse error
         parse_error_detected(true);

         // TODO: should this be just a warning or an error (with exit condition?)
         LOG_FMT(LWARN,
                 "%s(%d): Unmatched '<' at orig_line is %zu, orig_col is %zu\n",
                 __unqualified_func__,
                 __LINE__,
                 angle_open->orig_line,
                 angle_open->orig_col);
      }
      else
      {
         /**
          * check to make sure that the template is the final chunk in a list
          * of scope-resolution qualifications
          */
         auto *next = angle_close->GetNextNcNnl();

         if (chunk_is_not_token(next, CT_DC_MEMBER))
         {
            set_template_start(angle_open);

            /**
             * we could be dealing with a template type; if so, the opening angle
             * bracket should be preceded by a CT_WORD token and we should have
             * found a closing angle bracket
             */
            auto *prev = angle_open->GetPrevNcNnlNi();

            if (chunk_is_not_token(prev, CT_WORD))
            {
               // parse error
               parse_error_detected(true);

               // TODO: should this be just a warning or an error (with exit condition?)
               LOG_FMT(LWARN,
                       "%s(%d): Identifier missing before '<' at orig_line is %zu, orig_col is %zu\n",
                       __unqualified_func__,
                       __LINE__,
                       angle_open->orig_line,
                       angle_open->orig_col);
            }
            else
            {
               set_template_end(angle_close);
               mark_template(angle_open);
            }
         }
         /**
          * update input argument to point to the closing angle bracket
          */
         pc = angle_close;
      }
   }
   return(pc);
} // EnumStructUnionParser::parse_angles


Chunk *EnumStructUnionParser::parse_braces(Chunk *brace_open)
{
   LOG_FUNC_ENTRY();

   /**
    * check to see if there's a matching closing brace
    */

   auto *pc          = brace_open;
   auto *brace_close = chunk_skip_to_match(pc, E_Scope::PREPROC);

   if (brace_close != nullptr)
   {
      /**
       * we could be dealing with a variable definition preceded by
       * the class/struct keyword. It's possible that the variable is
       * assigned via direct-list initialization, hence the open brace
       * is NOT part of a class/struct type definition.
       */
      auto *first_comma = get_first_top_level_comma();

      if (chunk_is_after(pc, first_comma))
      {
         /**
          * the open brace occurs after a top-level comma was encountered, which
          * likely implies a direct-initialization or braced initializer list in
          * the midst of a list of variable definitions
          */

         return(pc);
      }
      set_body_end(brace_close);
      set_body_start(brace_open);

      auto *enum_base_start   = get_enum_base_start();
      auto *inheritance_start = get_inheritance_start();
      auto *prev              = pc->GetPrevNcNnlNi();

      /**
       * check to see if the open brace was preceded by a closing paren;
       * it could possibly be a function-like macro call preceding the
       * open brace, but it's more likely that we're dealing with a
       * signature associated with a function definition
       */
      bool is_potential_function_definition = false;

      if (  (  language_is_set(LANG_C)
            || language_is_set(LANG_CPP))
         && chunk_is_paren_close(prev))
      {
         /**
          * we may be dealing with a c/cpp function definition, where the 'struct'
          * or 'class' keywords appear as the return type preceding a pair of braces
          * and therefore may be associated with a function definition body
          */
         auto *paren_close = prev;

         // skip in reverse to the matching open paren
         auto *paren_open = chunk_skip_to_match_rev(paren_close);

         if (paren_open != nullptr)
         {
            /**
             * determine if there's an identifier preceding the open paren;
             * if so, the identifier is very likely to be associated with
             * a function definition
             */
            auto *type       = m_start->GetNextNcNnl();
            auto *identifier = paren_open->GetPrevNcNnlNi(E_Scope::PREPROC);
            is_potential_function_definition = (  (  chunk_is_token(identifier, CT_FUNCTION)
                                                  || chunk_is_token(identifier, CT_FUNC_DEF)
                                                  || chunk_is_token(identifier, CT_WORD))
                                               && type != identifier);
         }
      }

      if (  language_is_set(LANG_D)
         || language_is_set(LANG_PAWN)
         || !chunk_is_paren_close(prev)
         || is_potential_function_definition
         || chunk_is_between(prev, enum_base_start, brace_open)
         || chunk_is_between(prev, inheritance_start, brace_open))
      {
         mark_braces(brace_open);

         /**
          * D does not require a semicolon after an enum, but we add one to make
          * other code happy.
          */
         if (  language_is_set(LANG_D)
            && chunk_is_enum(m_start))
         {
            pawn_add_vsemi_after(brace_close); // Issue #2279
         }
         pc = brace_close;
      }
      else
      {
         // TODO: should this be just a warning or an error (with exit condition?)
         LOG_FMT(LWARN,
                 "%s(%d): Parsing error precedes start of body '{' at orig_line is %zu, orig_col is %zu\n",
                 __unqualified_func__,
                 __LINE__,
                 brace_open->orig_line,
                 brace_open->orig_col);

         // parse error
         parse_error_detected(true);
      }
   }
   return(pc);
} // EnumStructUnionParser::parse_braces


void EnumStructUnionParser::parse_colon(Chunk *colon)
{
   LOG_FUNC_ENTRY();

   if (chunk_is_token(m_start, CT_UNION))
   {
      /**
       * unions do not implement inheritance
       */

      // TODO: should this be just a warning or an error (with exit condition?)
      LOG_FMT(LWARN,
              "%s(%d): Colon follows union declaration at orig_line is %zu, orig_col is %zu\n",
              __unqualified_func__,
              __LINE__,
              colon->orig_line,
              colon->orig_col);

      // parse error
      parse_error_detected(true);
   }
   else if (is_within_conditional(colon))
   {
      mark_conditional_colon(colon);
   }
   else if (is_within_where_clause(colon))
   {
      mark_where_colon(colon);
   }
   else if (!inheritance_detected())
   {
      if (chunk_is_class_or_struct(m_start))
      {
         /**
          * the colon likely specifies an inheritance list for a struct
          * or class type
          */

         set_inheritance_start(colon);
         mark_class_colon(colon);
      }
      else if (chunk_is_enum(m_start))
      {
         set_enum_base_start(colon);
         mark_enum_integral_type(colon);
      }
   }
} // EnumStructUnionParser::parse_colon


Chunk *EnumStructUnionParser::parse_double_colon(Chunk *double_colon)
{
   LOG_FUNC_ENTRY();

   auto *pc = double_colon;

   if (  language_is_set(LANG_CPP)
      && chunk_is_token(pc, CT_DC_MEMBER))
   {
      mark_nested_name_specifiers(pc);
      pc = skip_scope_resolution_and_nested_name_specifiers(pc);
   }
   return(pc);
} // EnumStructUnionParser::parse_double_colon


bool EnumStructUnionParser::parse_error_detected() const
{
   LOG_FUNC_ENTRY();

   return(m_parse_error);
} // EnumStructUnionParser::parse_error_detected


void EnumStructUnionParser::parse_error_detected(bool status)
{
   LOG_FUNC_ENTRY();

   m_parse_error = status;
} // EnumStructUnionParser::parse_error_detected


void EnumStructUnionParser::record_question_operator(Chunk *question)
{
   LOG_FUNC_ENTRY();

   if (chunk_is_token(question, CT_QUESTION))
   {
      std::size_t index = m_chunk_map[CT_QUESTION].size();

      m_chunk_map[CT_QUESTION][index] = question;
   }
} // EnumStructUnionParser::record_question_operator


void EnumStructUnionParser::record_top_level_comma(Chunk *comma)
{
   if (  comma != nullptr
      && comma->level == m_start->level
      && !is_within_conditional(comma)
      && !is_within_inheritance_list(comma))
   {
      std::size_t index = m_chunk_map[CT_COMMA].size();

      m_chunk_map[CT_COMMA][index] = comma;
   }
} // EnumStructUnionParser::record_top_level_comma


Chunk *EnumStructUnionParser::refine_end_chunk(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (  (  language_is_set(LANG_C)
         || language_is_set(LANG_CPP))
      && chunk_is_token(pc, CT_BRACE_CLOSE))
   {
      /**
       * if dealing with C/C++, one or more trailing variable definitions may
       * follow the closing brace; a semi-colon should've been good enough to
       * indicate the terminating condition, however some of the classes defined
       * in the input tests cases for Continuous Integration DO NOT correctly
       * terminate classes/struct with a semicolon (which is compilation error).
       * As a consequence, more checks must be performed to determine where
       * the terminating chunk is located. For instance, see operator.cpp and
       * enum_comma.h for examples of offenders
       */
      auto *next = pc->GetNextNcNnl();

      while (true)
      {
         if (chunk_is_semicolon(next))
         {
            pc = next;

            break;
         }
         else
         {
            /**
             * if we're sitting at a comma, skip it
             */
            if (chunk_is_token(next, CT_COMMA))
            {
               next = next->GetNextNcNnl();
            }
            auto match       = match_variable(next, m_start->level);
            auto *start      = std::get<0>(match);
            auto *identifier = std::get<1>(match);
            auto *end        = std::get<2>(match);

            if (  end == nullptr
               || identifier == nullptr
               || start == nullptr)
            {
               break;
            }
            else
            {
               pc = end->GetNextNcNnl();

               /**
                * skip any right-hand side assignments
                */
               if (chunk_is_token(pc, CT_ASSIGN))
               {
                  pc = skip_to_expression_end(pc);
               }
               next = pc;
            }
         }
      }
   }
   return(pc);
} // EnumStructUnionParser::refine_end_chunk


void EnumStructUnionParser::set_body_end(Chunk *body_end)
{
   LOG_FUNC_ENTRY();

   if (chunk_is_token(body_end, CT_BRACE_CLOSE))
   {
      m_chunk_map[CT_BRACE_CLOSE][0] = body_end;
   }
} // EnumStructUnionParser::set_body_end


void EnumStructUnionParser::set_body_start(Chunk *body_start)
{
   LOG_FUNC_ENTRY();

   if (chunk_is_token(body_start, CT_BRACE_OPEN))
   {
      m_chunk_map[CT_BRACE_OPEN][0] = body_start;
   }
} // EnumStructUnionParser::set_body_start


void EnumStructUnionParser::set_enum_base_start(Chunk *enum_base_start)
{
   LOG_FUNC_ENTRY();

   if (chunk_is_colon(enum_base_start))
   {
      m_chunk_map[CT_BIT_COLON][0] = enum_base_start;
   }
} // EnumStructUnionParser::set_enum_base_start


void EnumStructUnionParser::set_inheritance_start(Chunk *inheritance_start)
{
   LOG_FUNC_ENTRY();

   if (chunk_is_colon(inheritance_start))
   {
      m_chunk_map[CT_COLON][0] = inheritance_start;
   }
} // EnumStructUnionParser::set_inheritance_start


void EnumStructUnionParser::set_template_end(Chunk *template_end)
{
   LOG_FUNC_ENTRY();

   if (chunk_is_token(template_end, CT_ANGLE_CLOSE))
   {
      m_chunk_map[CT_ANGLE_CLOSE][0] = template_end;
   }
} // EnumStructUnionParser::set_template_end


void EnumStructUnionParser::set_template_start(Chunk *template_start)
{
   LOG_FUNC_ENTRY();

   if (chunk_is_token(template_start, CT_ANGLE_OPEN))
   {
      m_chunk_map[CT_ANGLE_OPEN][0] = template_start;
   }
} // EnumStructUnionParser::set_template_start


void EnumStructUnionParser::set_where_end(Chunk *where_end)
{
   LOG_FUNC_ENTRY();

   if (chunk_is_token(where_end, CT_BRACE_OPEN))
   {
      m_chunk_map[CT_WHERE][0] = where_end;
   }
} // EnumStructUnionParser::set_where_end


void EnumStructUnionParser::set_where_start(Chunk *where_start)
{
   LOG_FUNC_ENTRY();

   if (chunk_is_token(where_start, CT_WHERE))
   {
      m_chunk_map[CT_WHERE][0] = where_start;
   }
} // EnumStructUnionParser::set_where_start


bool EnumStructUnionParser::template_detected() const
{
   LOG_FUNC_ENTRY();

   auto *template_end   = get_template_end();
   auto *template_start = get_template_start();

   return(  template_end != nullptr
         && template_start != nullptr);
} // EnumStructUnionParser::template_detected


Chunk *EnumStructUnionParser::try_find_end_chunk(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   do
   {
      /**
       * clear some previously marked token types, some of which have likely
       * been erroneously marked up to this point; a good example of this
       * arises when macro variables and/or macro function calls follow the
       * class/enum/struct/union keyword and precede the actual type name
       */
      if (  chunk_is_token(pc, CT_TYPE)
         || chunk_is_token(pc, CT_WORD))
      {
         set_chunk_type(pc, CT_WORD);
         set_chunk_parent(pc, CT_NONE);
      }

      do
      {
         pc = pc->GetNextNcNnl(E_Scope::PREPROC);
      } while (  pc->IsNotNullChunk()
              && pc->level > m_start->level);
   } while (!is_potential_end_chunk(pc));

   /**
    * perform a second pass for c++ that
    */
   pc = refine_end_chunk(pc);

   return(pc);
} // EnumStructUnionParser::try_find_end_chunk


void EnumStructUnionParser::try_post_identify_macro_calls()
{
   LOG_FUNC_ENTRY();

   if (  language_is_set(LANG_CPP)
      && type_identified())
   {
      /**
       * for all chunks at class/enum/struct/union level, identify function-like
       * macro calls and mark them as CT_MACRO_FUNC_CALL. The reason for doing
       * so is to avoid mis-interpretation by code executed at a later time
       */

      auto  *body_start        = get_body_start();
      auto  *inheritance_start = get_inheritance_start();
      Chunk *pc                = m_start;
      Chunk *prev              = nullptr;

      do
      {
         if (  !chunk_is_between(prev, inheritance_start, body_start)
            && (  chunk_is_token(prev, CT_WORD)
               || chunk_is_token(prev, CT_FUNCTION)
               || chunk_is_token(prev, CT_FUNC_DEF))
            && !prev->flags.test_any(PCF_VAR_DEF | PCF_VAR_1ST | PCF_VAR_INLINE)
            && prev->level == m_start->level)
         {
            if (chunk_is_paren_open(pc))
            {
               auto *paren_open  = pc;
               auto *paren_close = chunk_skip_to_match(paren_open, E_Scope::PREPROC);

               if (paren_close != nullptr)
               {
                  set_chunk_type(paren_open, CT_FPAREN_OPEN);
                  set_chunk_parent(paren_open, CT_MACRO_FUNC_CALL);
                  set_chunk_type(paren_close, CT_FPAREN_CLOSE);
                  set_chunk_parent(paren_close, CT_MACRO_FUNC_CALL);
                  set_chunk_type(prev, CT_MACRO_FUNC_CALL);
               }
            }
         }
         prev = pc;
         pc   = prev->GetNextNcNnl();
      } while (chunk_is_between(pc, m_start, m_end));
   }
} // EnumStructUnionParser::try_post_identify_macro_calls


void EnumStructUnionParser::try_post_identify_type()
{
   LOG_FUNC_ENTRY();

   Chunk *body_end = get_body_end();

   if (  !type_identified()
      && body_end == nullptr)
   {
      /**
       * a type wasn't identified and no closing brace is present; we're
       * likely not dealing with an anonymous enum/class/struct
       */

      /**
       * a type has yet to be identified, so search for the last word
       * that hasn't been marked as a variable
       */
      Chunk *type = nullptr;
      Chunk *pc   = m_start;

      do
      {
         /**
          * in case it's a qualified identifier, skip scope-resolution and
          * nested name specifiers and return just the qualified identifier name
          */
         pc = skip_scope_resolution_and_nested_name_specifiers(pc);

         if (pc->flags.test_any(PCF_VAR_DEF | PCF_VAR_1ST | PCF_VAR_INLINE))
         {
            break;
         }
         else if (  chunk_is_token(pc, CT_WORD)
                 || chunk_is_token(pc, CT_ANGLE_CLOSE))
         {
            type = skip_template_prev(pc);
         }
         pc = pc->GetNextNcNnl();
      } while (chunk_is_between(pc, m_start, m_end));

      if (type != nullptr)
      {
         mark_type(type);
      }
   }
} // EnumStructUnionParser::try_post_identify_type


bool EnumStructUnionParser::try_pre_identify_type()
{
   LOG_FUNC_ENTRY();

   Chunk *pc = get_body_start();

   if (  language_is_set(LANG_PAWN)
      && chunk_is_enum(m_start))
   {
      set_paren_parent(pc, m_start->type);
   }
   else if (template_detected())
   {
      pc = get_template_start();
   }
   else if (enum_base_detected())
   {
      pc = get_enum_base_start();
   }
   else if (inheritance_detected())
   {
      pc = get_inheritance_start();

      if (chunk_is_token(m_start, CT_UNION))
      {
         /**
          * unions do not implement inheritance
          */

         // TODO: should this be just a warning or an error (with exit condition?)
         LOG_FMT(LWARN,
                 "%s(%d): Bad union declaration detected at orig_line is %zu, orig_col is %zu\n",
                 __unqualified_func__,
                 __LINE__,
                 m_start->orig_line,
                 m_start->orig_col);

         parse_error_detected(true);

         return(false);
      }
   }

   if (pc == nullptr)
   {
      Chunk *next = m_start->GetNextNcNnl();

      /**
       * in case it's a qualified identifier, skip scope-resolution and
       * nested name specifiers and return just the qualified identifier name
       */
      next = skip_scope_resolution_and_nested_name_specifiers(next);

      Chunk *next_next = next->GetNextNcNnl();

      /**
       * in case it's a qualified identifier, skip scope-resolution and
       * nested name specifiers and return just the qualified identifier name
       */
      next_next = skip_scope_resolution_and_nested_name_specifiers(next_next);

      /**
       * if there is one word between the start and end chunks, then we've likely
       * identified the type; if there are two words, then the first is likely a
       * type and the second is an instantiation thereof; however, it is possible
       * that the first word is actually a reference to a macro definition, in which
       * the second word would be the type
       */
      if (next_next == m_end)
      {
         pc = next_next;
      }
      else if (  next->IsNotNullChunk()
              && chunk_is_token(next, CT_WORD)
              && chunk_is_token(next_next, CT_WORD)
              && m_end->GetPrevNcNnlNi() == next_next)
      {
         /**
          * check to see if we've got a macro reference preceding the last word chunk;
          * this won't work in all cases, because a macro may be defined in another header
          * file, but this is an attempt to increase the chances of identifying the correct
          * chunk as the type
          */
         if (  chunk_is_macro_reference(next)
            || get_chunk_parent_type(m_start) == CT_TEMPLATE)
         {
            pc = m_end;
         }
         else
         {
            pc = next_next;
         }
      }
      else
      {
         /**
          * search for some common patterns that may indicate a type
          */
         Chunk *prev = m_start;

         while (  chunk_is_between(next, m_start, m_end)
               && (  (  chunk_is_not_token(next, CT_ASSIGN)
                     && chunk_is_not_token(next, CT_COMMA))
                  || next->level != m_start->level)
               && !chunk_is_semicolon(next))
         {
            prev = next;
            next = next->GetNextNcNnl();

            /**
             * in case it's a qualified identifier, skip scope-resolution and
             * nested name specifiers and return just the qualified identifier name
             */
            next = skip_scope_resolution_and_nested_name_specifiers(next);

            /**
             * skip array brackets, as the type cannot be located within;
             * also skip a set of parens - there may be a type embedded within,
             * but it's not the type with which we're concerned
             */
            if (  next->IsSquareBracket()                       // Issue #3601
               || chunk_is_paren_open(next))
            {
               prev = chunk_skip_to_match(next, E_Scope::PREPROC);
               next = prev->GetNextNcNnl(E_Scope::PREPROC);
            }

            if (  chunk_is_token(prev, CT_WORD)
               && chunk_is_pointer_or_reference(next))
            {
               pc = next;

               break;
            }
         }
      }
   }

   if (  pc != nullptr
      && pc->IsNotNullChunk())
   {
      /**
       * the chunk preceding the previously selected chunk should indicate the type
       */

      pc = pc->GetPrevNcNnlNi(E_Scope::PREPROC);

      if (  chunk_is_token(pc, CT_QUALIFIER)
         && std::strncmp(pc->str.c_str(), "final", 5) == 0)
      {
         pc = pc->GetPrevNcNnlNi(E_Scope::PREPROC);
      }

      if (  language_is_set(LANG_D)
         && chunk_is_paren_close(pc))
      {
         pc = chunk_skip_to_match_rev(pc);
         pc = pc->GetPrevNcNnlNi();
      }

      if (chunk_is_token(pc, CT_WORD))
      {
         mark_type(pc);

         return(true);
      }
   }
   return(false);
} // EnumStructUnionParser::try_pre_identify_type


bool EnumStructUnionParser::type_identified() const
{
   LOG_FUNC_ENTRY();

   return(m_type != nullptr);
} // EnumStructUnionParser::type_identified


/**
 * Returns true if a where clause was detected during parsing
 */
bool EnumStructUnionParser::where_clause_detected() const
{
   LOG_FUNC_ENTRY();

   auto *where_end   = get_where_end();
   auto *where_start = get_where_start();

   return(  where_end != nullptr
         && where_start != nullptr);
} // EnumStructUnionParser::where_clause_detected
