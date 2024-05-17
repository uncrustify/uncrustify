/**
 * @file EnumStructUnionParser.cpp
 *
 * @author
 * @license GPL v2+
 */
#include "tokenizer/EnumStructUnionParser.h"

#include "lang_pawn.h"
#include "tokenizer/combine_fix_mark.h"
#include "tokenizer/combine_skip.h"
#include "tokenizer/combine_tools.h"
#include "tokenizer/flag_parens.h"


/**
 * Extern declarations
 */
extern const char *get_token_name(E_Token);


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

   if (  prev->IsNotNullChunk()
      && next->IsNotNullChunk())
   {
      auto prev_token_type = prev->GetType();
      auto next_token_type = next->GetType();

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
         return(prev->GetClosingParen(E_Scope::PREPROC)->IsNotNullChunk());

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

   if (  prev->IsNotNullChunk()
      && next->IsNotNullChunk())
   {
      auto prev_token_type = prev->GetType();
      auto next_token_type = next->GetType();

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
         return(  next->IsPointerOrReference()
               || next_token_type == CT_DC_MEMBER
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_WORD);


      case CT_ANGLE_OPEN:
         /**
          * assuming the previous token is possibly the opening angle of a
          * templated type, just check to see if there's a matching closing
          * angle
          */
         return(prev->GetClosingParen(E_Scope::PREPROC)->IsNotNullChunk());

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
         return(  next->IsPointerOrReference()
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_WORD);

      case CT_BRACE_OPEN:
         /**
          * if the previous token is an opening brace, it may indicate the
          * start of a braced initializer list - skip ahead to find a matching
          * closing brace
          */
         return(prev->GetClosingParen(E_Scope::PREPROC)->IsNotNullChunk());

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
         return(  language_is_set(lang_flag_e::LANG_CPP)
               && (  next->IsPointerOrReference()
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
         return(  next->IsPointerOrReference()
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
         next = prev->GetClosingParen(E_Scope::PREPROC);

         if (next->IsNotNullChunk())
         {
            next_token_type = next->GetType();
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
         return(  next->IsPointerOrReference()
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
         return(  next->IsPointerOrReference()
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
         return(prev->GetClosingParen(E_Scope::PREPROC)->IsNotNullChunk());

      case CT_STAR:
         /**
          * if the previous token is a pointer symbol, ('*'), the next token
          * may be one of the following:
          * - another pointer symbol ('*', '^')
          * - a reference symbol ('&')
          * - a qualifier (const, etc.)
          * - an identifier
          */
         return(  next->IsPointerOrReference()
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
         return(  next->IsPointerOrReference()
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

   if (pc->IsNotNullChunk())
   {
      if (  test_equal
         && pc == after)
      {
         return(true);
      }
      else if (after->IsNotNullChunk())
      {
         auto pc_column    = pc->GetOrigCol();
         auto pc_line      = pc->GetOrigLine();
         auto after_column = after->GetOrigCol();
         auto after_line   = after->GetOrigLine();

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

   if (pc->IsNotNullChunk())
   {
      if (  test_equal
         && pc == before)
      {
         return(true);
      }
      else if (before->IsNotNullChunk())
      {
         auto pc_column     = pc->GetOrigCol();
         auto pc_line       = pc->GetOrigLine();
         auto before_column = before->GetOrigCol();
         auto before_line   = before->GetOrigLine();

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

   if (  (  language_is_set(lang_flag_e::LANG_CPP)
         || language_is_set(lang_flag_e::LANG_C))
      && pc->Is(CT_WORD)
      && !pc->TestFlags(PCF_IN_PREPROC))
   {
      while (next->IsNotNullChunk())
      {
         if (  next->TestFlags(PCF_IN_PREPROC)
            && std::strcmp(pc->GetStr().c_str(), next->GetStr().c_str()) == 0)
         {
            return(true);
         }
         next = next->GetNextType(CT_MACRO);
      }
   }
   return(false);
} // chunk_is_macro_reference


bool Chunk::IsPointerReferenceOrQualifier() const
{
   LOG_FUNC_ENTRY();

   return(  IsPointerOrReference()
         || (  Is(CT_QUALIFIER)
            && !IsCppInheritanceAccessSpecifier()));
}


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

   if (  end->IsNotNullChunk()
      && start->IsNotNullChunk())
   {
      auto *double_colon = start->GetNextType(CT_DC_MEMBER);

      if (  double_colon->IsNotNullChunk()
         && chunk_is_between(double_colon, start, end))
      {
         return(std::make_pair(start, end));
      }
   }
   return(std::make_pair(Chunk::NullChunkPtr, Chunk::NullChunkPtr));
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
   auto *identifier           = identifier_end_pair.first->IsNotNullChunk() ? identifier_end_pair.first : start_identifier_pair.second;
   auto *start                = start_identifier_pair.first;

   /**
    * a forward search starting at the chunk under test will fail if two consecutive chunks marked as CT_WORD
    * are encountered; in that case, it's likely that the preceding chunk indicates a type and the subsequent
    * chunk indicates a variable declaration/definition
    */

   if (  identifier->IsNotNullChunk()
      && start->IsNotNullChunk()
      && (  end->IsNotNullChunk()
         || identifier->GetPrevNcNnlNi()->Is(CT_WORD)))
   {
      return(std::make_tuple(start, identifier, end));
   }
   return(std::make_tuple(Chunk::NullChunkPtr, Chunk::NullChunkPtr, Chunk::NullChunkPtr));
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

   Chunk *identifier = Chunk::NullChunkPtr;

   while (pc->IsNotNullChunk())
   {
      /**
       * skip any right-hand side assignments
       */
      Chunk *rhs_exp_end = Chunk::NullChunkPtr;

      if (pc->Is(CT_ASSIGN))
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
      while (  pc->IsNotNullChunk()
            && pc->GetLevel() > level)
      {
         pc = pc->GetNextNcNnl();
      }

      /**
       * skip to any following match for angle brackets, braces, parens,
       * or square brackets
       */
      if (  pc->Is(CT_ANGLE_OPEN)
         || pc->Is(CT_BRACE_OPEN)
         || pc->IsParenOpen()
         || pc->Is(CT_SQUARE_OPEN))
      {
         pc = pc->GetClosingParen(E_Scope::PREPROC);
      }
      /**
       * call a separate function to validate adjacent tokens as potentially
       * matching a variable declaration/definition
       */

      Chunk *next = pc->GetNextNcNnl();

      if (  next->IsNot(CT_COMMA)
         && next->IsNot(CT_FPAREN_CLOSE)
         && !next->IsSemicolon()
         && !adj_tokens_match_var_def_pattern(pc, next))
      {
         /**
          * error, pattern is not consistent with a variable declaration/definition
          */
         break;
      }

      if (  pc->Is(CT_WORD)
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
      if (  next->Is(CT_COMMA)
         || next->Is(CT_FPAREN_CLOSE)
         || next->IsSemicolon())
      {
         return(std::make_pair(identifier, pc));
      }
      pc = next;
   }
   return(std::make_pair(Chunk::NullChunkPtr, Chunk::NullChunkPtr));
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

         if (next->Is(CT_ASSIGN))
         {
            pc = prev;
         }
      }
      /**
       * skip current and preceding chunks if at a higher brace level
       */

      while (  pc->IsNotNullChunk()
            && pc->GetLevel() > level)
      {
         pc = pc->GetPrevNcNnlNi();
      }

      /**
       * skip to any preceding match for angle brackets, braces, parens,
       * or square brackets
       */
      if (  pc->Is(CT_ANGLE_CLOSE)
         || pc->Is(CT_BRACE_CLOSE)
         || pc->IsParenClose()
         || pc->Is(CT_SQUARE_CLOSE))
      {
         pc = pc->GetOpeningParen(E_Scope::PREPROC);
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
         if (  prev->IsNot(CT_WORD)
            || (  !pc->IsPointerOrReference()
               && pc->IsNot(CT_WORD)))
         {
            /**
             * error, pattern is not consistent with a variable declaration/definition
             */

            break;
         }
      }

      if (  identifier->IsNullChunk()
         && pc->Is(CT_WORD))
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
      if (  prev->Is(CT_ANGLE_CLOSE)
         || prev->Is(CT_BRACE_CLOSE)
         || prev->Is(CT_COMMA)
         || prev->Is(CT_TYPE)
         || prev->Is(CT_WORD))
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

   if (  pc->TestFlags(PCF_IN_TEMPLATE)
      || pc->Is(CT_DC_MEMBER)
      || pc->Is(CT_TYPE)
      || pc->Is(CT_WORD))
   {
      while (pc->IsNotNullChunk())
      {
         /**
          * skip to any following match for angle brackets
          */
         if (pc->Is(CT_ANGLE_OPEN))
         {
            pc = pc->GetClosingParen(E_Scope::PREPROC);
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

   if (  pc->TestFlags(PCF_IN_TEMPLATE)
      || pc->Is(CT_DC_MEMBER)
      || pc->Is(CT_TYPE)
      || pc->Is(CT_WORD))
   {
      while (pc->IsNotNullChunk())
      {
         /**
          * skip to any preceding match for angle brackets
          */
         if (pc->Is(CT_ANGLE_CLOSE))
         {
            pc = pc->GetOpeningParen(E_Scope::PREPROC);
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
   : m_end(Chunk::NullChunkPtr)
   , m_parse_error(false)
   , m_start(Chunk::NullChunkPtr)
   , m_type(Chunk::NullChunkPtr)
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

   Chunk    *template_end      = get_template_end();
   auto     *body_end          = get_body_end();
   auto     *body_start        = get_body_start();
   PcfFlags flags              = PCF_VAR_1ST_DEF;
   auto     *inheritance_start = get_inheritance_start();
   Chunk    *pc                = body_end->IsNotNullChunk() ? body_end : m_start;

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

      if (body_end->IsNotNullChunk())
      {
         pc = body_end;
      }
      else if (template_end->IsNotNullChunk())
      {
         pc = template_end;
      }
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
      || pc->IsClassEnumStructOrUnion()
      || pc == m_end)
   {
      /**
       * in case we're pointing at the end chunk, advance the chunk pointer
       * by one more so that we don't perform a variable identifier search
       * below
       */
      pc = pc->GetNextNcNnl();
   }

   if (body_end->IsNotNullChunk())
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
      auto match       = match_variable(pc, m_start->GetLevel());
      auto *start      = std::get<0>(match);
      auto *identifier = std::get<1>(match);
      auto *end        = std::get<2>(match);

      if (  start->IsNotNullChunk()
         && identifier->IsNotNullChunk())
      {
         if (end->IsNotNullChunk())
         {
            mark_variable(identifier, flags);

            if (flags & PCF_VAR_1ST)
            {
               flags &= ~PCF_VAR_1ST; // clear the first flag for the next items
            }
         }
      }

      if (end->IsNotNullChunk())
      {
         pc = end;
      }
      pc = pc->GetNextNcNnl();

      /**
       * skip any right-hand side assignments
       */
      if (pc->Is(CT_ASSIGN))
      {
         pc = skip_to_expression_end(pc);
      }

      /**
       * if we're sitting at a comma or semicolon, skip it
       */
      if (  pc->IsSemicolon()
         || (  pc->Is(CT_COMMA)
            && !pc->GetFlags().test_any(PCF_IN_FCN_DEF | PCF_IN_FCN_CALL | PCF_IN_TEMPLATE)
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

   if (  m_start->IsClassOrStruct()
      && (  m_start->IsNot(CT_STRUCT)
         || !language_is_set(lang_flag_e::LANG_C)))
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
         m_type->SetFlagBits(PCF_VAR_TYPE);
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

   return(  body_end->IsNotNullChunk()
         && body_start->IsNotNullChunk());
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
   return(Chunk::NullChunkPtr);
} // EnumStructUnionParser::get_body_end


Chunk *EnumStructUnionParser::get_body_start() const
{
   LOG_FUNC_ENTRY();

   auto &&it_token_chunk_map_pair = m_chunk_map.find(CT_BRACE_OPEN);

   if (it_token_chunk_map_pair != m_chunk_map.cend())
   {
      return(it_token_chunk_map_pair->second.at(0));
   }
   return(Chunk::NullChunkPtr);
} // EnumStructUnionParser::get_body_start


Chunk *EnumStructUnionParser::get_enum_base_start() const
{
   LOG_FUNC_ENTRY();

   auto &&it_token_chunk_map_pair = m_chunk_map.find(CT_BIT_COLON);

   if (it_token_chunk_map_pair != m_chunk_map.cend())
   {
      return(it_token_chunk_map_pair->second.at(0));
   }
   return(Chunk::NullChunkPtr);
} // EnumStructUnionParser::get_enum_base_start


Chunk *EnumStructUnionParser::get_first_top_level_comma() const
{
   LOG_FUNC_ENTRY();

   auto &&it_token_chunk_map_pair = m_chunk_map.find(CT_COMMA);

   if (it_token_chunk_map_pair != m_chunk_map.cend())
   {
      return(it_token_chunk_map_pair->second.at(0));
   }
   return(Chunk::NullChunkPtr);
} // EnumStructUnionParser::get_first_top_level_comma


Chunk *EnumStructUnionParser::get_inheritance_end() const
{
   LOG_FUNC_ENTRY();

   Chunk *brace_open        = Chunk::NullChunkPtr;
   auto  *inheritance_start = get_inheritance_start();

   if (inheritance_start->IsNotNullChunk())
   {
      brace_open = get_body_start();

      if (brace_open->IsNullChunk())
      {
         brace_open = inheritance_start->GetNextType(CT_BRACE_OPEN, m_start->GetLevel(), E_Scope::ALL);
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
   return(Chunk::NullChunkPtr);
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
   return(Chunk::NullChunkPtr);
} // EnumStructUnionParser::get_template_end


Chunk *EnumStructUnionParser::get_template_start() const
{
   LOG_FUNC_ENTRY();

   auto &&it_token_chunk_map_pair = m_chunk_map.find(CT_ANGLE_OPEN);

   if (it_token_chunk_map_pair != m_chunk_map.cend())
   {
      return(it_token_chunk_map_pair->second.at(0));
   }
   return(Chunk::NullChunkPtr);
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

   Chunk *brace_open  = Chunk::NullChunkPtr;
   auto  *where_start = get_where_start();

   if (where_start->IsNotNullChunk())
   {
      brace_open = get_body_start();

      if (brace_open->IsNullChunk())
      {
         brace_open = where_start->GetNextType(CT_BRACE_OPEN, m_start->GetLevel(), E_Scope::ALL);
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
   return(Chunk::NullChunkPtr);
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
   m_type  = Chunk::NullChunkPtr;
   pc      = try_find_end_chunk(pc);

   if (parse_error_detected())
   {
      return;
   }
   m_end = refine_end_chunk(pc);
} // EnumStructUnionParser::initialize


bool EnumStructUnionParser::is_potential_end_chunk(Chunk *pc) const
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LFTOR, "%s(%d): orig line is %zu, orig col is %zu, type is %s\n",
           __unqualified_func__, __LINE__,
           pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));

   /**
    * test for a semicolon or closing brace at the level of the starting chunk
    */
   if (  pc->IsNullChunk()
      || parse_error_detected()
      || (  (  pc->IsSemicolon()
            || pc->Is(CT_BRACE_CLOSE))
         && pc->GetLevel() == m_start->GetLevel()))
   {
      LOG_FMT(LFTOR, "%s(%d): orig line is %zu, orig col is %zu, type is %s\n",
              __unqualified_func__, __LINE__,
              pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));
      return(true);
   }
   /**
    * check for the following:
    * 1) did we encounter a closing paren, which may indicate the end of cast?
    * 2) did we cross a preprocessor boundary?
    * 3) did we cross the closing paren of a function signature?
    */

   auto const pc_in_funcdef    = pc->GetFlags() & PCF_IN_FCN_DEF;
   auto const pc_in_preproc    = pc->GetFlags() & PCF_IN_PREPROC;
   auto const start_in_funcdef = m_start->GetFlags() & PCF_IN_FCN_DEF;
   auto const start_in_preproc = m_start->GetFlags() & PCF_IN_PREPROC;

   /**
    * the following may identify cases where we've reached the
    * end of a cast terminated by a closing paren
    */
   if (  (  pc->IsParenClose() // Issue #3538
         && pc->GetLevel() < m_start->GetLevel())
      || (start_in_funcdef ^ pc_in_funcdef).test_any()
      || (start_in_preproc ^ pc_in_preproc).test_any())
   {
      LOG_FMT(LFTOR, "%s(%d): orig line is %zu, orig col is %zu, type is %s\n",
              __unqualified_func__, __LINE__,
              pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));
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
      LOG_FMT(LFTOR, "%s(%d): orig line is %zu, orig col is %zu, type is %s\n",
              __unqualified_func__, __LINE__,
              pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));
      return(true);
   }
   /**
    * assuming the chunk is within a function call/definition, check the following:
    * 1) chunk is a closing function paren at a lower level than the starting chunk
    * 2) chunk is an assignment ('=') or comma at the level of the starting chunk
    */

   auto const pc_in_funccall    = pc->GetFlags() & PCF_IN_FCN_CALL;
   auto const start_in_funccall = m_start->GetFlags() & PCF_IN_FCN_CALL;

   if (  (  pc_in_funccall.test_any()
         && start_in_funccall.test_any()
         && pc->Is(CT_COMMA)
         && pc->GetLevel() == m_start->GetLevel())
      || (  pc_in_funcdef.test_any()
         && (  (  pc->Is(CT_FPAREN_CLOSE)
               && pc->GetLevel() < m_start->GetLevel())
            || (  (  pc->Is(CT_ASSIGN)
                  || pc->Is(CT_COMMA))
               && pc->GetLevel() == m_start->GetLevel()))))
   {
      LOG_FMT(LFTOR, "%s(%d): orig line is %zu, orig col is %zu, type is %s\n",
              __unqualified_func__, __LINE__,
              pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));
      return(true);
   }
   LOG_FMT(LFTOR, "%s(%d): orig line is %zu, orig col is %zu, type is %s\n",
           __unqualified_func__, __LINE__,
           pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));
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

   if (pc->TestFlags(PCF_IN_CLASS_BASE))
   {
      return(true);
   }
   auto *inheritance_end   = get_inheritance_end();
   auto *inheritance_start = get_inheritance_start();

   if (  inheritance_end->IsNotNullChunk()
      && inheritance_start->IsNotNullChunk())
   {
      return(chunk_is_between(pc, inheritance_start, inheritance_end));
   }
   return(false);
} // EnumStructUnionParser::is_within_inheritance_list


bool EnumStructUnionParser::is_within_where_clause(Chunk *pc) const
{
   LOG_FUNC_ENTRY();

   if (pc->TestFlags(PCF_IN_WHERE_SPEC))
   {
      return(true);
   }
   auto *where_end   = get_where_end();
   auto *where_start = get_where_start();

   if (  where_end->IsNotNullChunk()
      && where_start->IsNotNullChunk())
   {
      return(chunk_is_between(pc, where_start, where_end));
   }
   return(false);
} // EnumStructUnionParser::is_within_where_clause


void EnumStructUnionParser::mark_base_classes(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   PcfFlags flags = PCF_VAR_1ST_DEF;

   while (pc->IsNotNullChunk())
   {
      pc->SetFlagBits(PCF_IN_CLASS_BASE);
      /**
       * clear the PCF_VAR_TYPE flag for all chunks within the inheritance list
       * TODO: this may not be necessary in the future once code outside this
       *       class is improved such that PCF_VAR_TYPE is not set for these chunks
       */
      pc->ResetFlagBits(PCF_VAR_TYPE);

      Chunk *next = pc->GetNextNcNnl(E_Scope::PREPROC);

      if (next->Is(CT_DC_MEMBER))
      {
         /**
          * just in case it's a templated type
          */
         pc = skip_template_prev(pc);

         if (pc->Is(CT_WORD))
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
            pc->SetType(CT_TYPE);
         }
      }
      else if (  (  next->Is(CT_BRACE_OPEN)
                 || (  next->Is(CT_COMMA)
                    && !is_within_where_clause(next)))
              && next->GetLevel() == m_start->GetLevel())
      {
         /**
          * just in case it's a templated type
          */
         pc = skip_template_prev(pc);

         if (pc->Is(CT_WORD))
         {
            pc->SetFlagBits(flags);

            if (flags & PCF_VAR_1ST)
            {
               flags &= ~PCF_VAR_1ST; // clear the first flag for the next items
            }
         }

         if (next->Is(CT_BRACE_OPEN))
         {
            break;
         }
      }
      pc = next;
   }
   pc->SetFlagBits(PCF_IN_CLASS_BASE);
} // EnumStructUnionParser::mark_base_classes


void EnumStructUnionParser::mark_braces(Chunk *brace_open)
{
   LOG_FUNC_ENTRY();

   PcfFlags flags = PCF_NONE;

   if (m_start->Is(CT_CLASS))
   {
      flags = PCF_IN_CLASS;
   }
   else if (m_start->IsEnum())
   {
      flags = PCF_IN_ENUM;
   }
   else if (m_start->Is(CT_STRUCT))
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

   if (m_start->IsClassStructOrUnion())
   {
      mark_struct_union_body(brace_open);

      auto *inheritance_start = get_inheritance_start();

      if (inheritance_start->IsNotNullChunk())
      {
         /**
          * the class/struct/union is a derived class; mark the base
          * classes between the colon/java "implements" keyword and the
          * opening brace
          */

         mark_base_classes(inheritance_start);
      }
   }
   brace_open->SetParentType(m_start->GetType());

   auto *brace_close = brace_open->GetClosingParen(E_Scope::PREPROC);

   if (brace_close->IsNotNullChunk())
   {
      brace_close->SetParentType(m_start->GetType());
   }
} // EnumStructUnionParser::mark_braces


void EnumStructUnionParser::mark_class_colon(Chunk *colon)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LFTOR,
           "%s(%d): Class colon detected: orig line is %zu, orig col is %zu\n",
           __unqualified_func__,
           __LINE__,
           colon->GetOrigLine(),
           colon->GetOrigCol());

   colon->SetType(CT_CLASS_COLON);
   colon->SetParentType(m_start->GetType());
} // EnumStructUnionParser::mark_class_colon


void EnumStructUnionParser::mark_conditional_colon(Chunk *colon)
{
   colon->SetType(CT_COND_COLON);
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
      && m_start->IsClassOrStruct())
   {
      LOG_FMT(LFTOR,
              "%s(%d): orig line is %zu, orig col is %zu, start is '%s', parent type is %s\n",
              __unqualified_func__,
              __LINE__,
              m_start->GetOrigLine(),
              m_start->GetOrigCol(),
              m_start->Text(),
              get_token_name(m_start->GetParentType()));

      log_pcf_flags(LFTOR, m_start->GetFlags());

      /**
       * get the name of the type
       */
      auto *body_end   = get_body_end();
      auto *body_start = get_body_start();
      auto *name       = m_type->Text();

      LOG_FMT(LFTOR, "%s(%d): Name of type is '%s'\n",
              __unqualified_func__, __LINE__, name);
      log_pcf_flags(LFTOR, m_type->GetFlags());

      Chunk       *next = Chunk::NullChunkPtr;
      std::size_t level = m_type->GetBraceLevel() + 1;

      for (auto *prev = body_start; next != body_end; prev = next)
      {
         prev->SetFlagBits(PCF_IN_CLASS);

         next = skip_template_next(prev->GetNextNcNnl(E_Scope::PREPROC));                         // Issue #3368

         /**
          * find a chunk within the class/struct body that
          */
         if (prev->IsNullChunk())
         {
            break;                            // Issue #4250
         }

         if (  std::strcmp(prev->Text(), name) == 0
            && prev->GetLevel() == level
            && next->IsParenOpen())
         {
            prev->SetType(CT_FUNC_CLASS_DEF);

            LOG_FMT(LFTOR,
                    "%s(%d): Constructor/destructor detected: '%s' at orig line is %zu, orig col is %zu, type is %s\n",
                    __unqualified_func__,
                    __LINE__,
                    name,
                    prev->GetOrigLine(),
                    prev->GetOrigCol(),
                    get_token_name(prev->GetType()));

            mark_cpp_constructor(prev);
         }
      }

      next->SetFlagBits(PCF_IN_CLASS);
   }
} // EnumStructUnionParser::mark_constructor


void EnumStructUnionParser::mark_enum_integral_type(Chunk *colon)
{
   LOG_FUNC_ENTRY();

   // Issue #4040
   LOG_FMT(LFTOR,
           "%s(%d): orig line is %zu, orig col is %zu\n",
           __unqualified_func__, __LINE__,
           colon->GetOrigLine(), colon->GetOrigCol());
   colon->SetType(CT_ENUM_COLON);
   colon->SetParentType(m_start->GetType());

   auto *body_start = get_body_start();
   auto *pc         = colon->GetNextNcNnl();

   /**
    * the chunk(s) between the colon and opening
    * brace (if present) should specify the enum's
    * integral type
    */

   while (  chunk_is_between(pc, m_start, m_end)
         && pc != body_start
         && pc->IsNot(CT_BRACE_OPEN)
         && !pc->IsSemicolon())
   {
      /**
       * clear the PCF_VAR_TYPE flag for all chunks within the enum integral base
       * TODO: this may not be necessary in the future once code outside this
       *       class is improved such that PCF_VAR_TYPE is not set for these chunks
       */
      if (pc->IsNot(CT_DC_MEMBER))                             // Issue #3198
      {
         pc->ResetFlagBits(PCF_VAR_TYPE);
         pc->SetType(CT_TYPE);
         pc->SetParentType(colon->GetType());
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
   if (next->GetParentType() == CT_TEMPLATE)
   {
      while (true)
      {
         prev = next->GetPrevNcNnlNi();

         if (  prev->IsNullChunk()
            || (  !prev->TestFlags(PCF_IN_TEMPLATE)
               && prev->IsNot(CT_TEMPLATE)))
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
         && next->TestFlags(PCF_LVALUE))
      {
         next->ResetFlagBits(PCF_LVALUE);
      }
      else if (  (  next->Is(CT_ASSIGN)
                 || next->Is(CT_BRACE_OPEN))
              && prev->Is(CT_WORD)
              && prev->GetFlags().test_any(PCF_VAR_DEF | PCF_VAR_1ST | PCF_VAR_INLINE))
      {
         prev->SetFlagBits(PCF_LVALUE);
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
      if (pc->Is(CT_WORD))
      {
         /**
          * if the next token is an opening angle, then we can safely
          * mark the current identifier as a type
          */
         auto *next = pc->GetNextNcNnl();

         if (next->Is(CT_ANGLE_OPEN))
         {
            /**
             * the template may have already been previously marked elsewhere...
             */
            auto *angle_open  = next;
            auto *angle_close = angle_open->GetClosingParen(E_Scope::PREPROC);

            if (angle_close->IsNullChunk())
            {
               // parse error
               parse_error_detected(true);

               // TODO: should this be just a warning or an error (with exit condition?)
               LOG_FMT(LWARN,
                       "%s(%d): Unmatched '<' at orig line is %zu, orig col is %zu\n",
                       __unqualified_func__,
                       __LINE__,
                       angle_open->GetOrigLine(),
                       angle_open->GetOrigCol());

               break;
            }
            pc->SetType(CT_TYPE);
            mark_template(next);
            pc = angle_close;
         }
         else if (  is_within_inheritance_list(pc)
                 && (  next->Is(CT_COMMA)
                    || next->Is(CT_BRACE_OPEN)))
         {
            pc->SetType(CT_TYPE);
         }
      }
   }
} // EnumStructUnionParser::mark_nested_name_specifiers


void EnumStructUnionParser::mark_pointer_types(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc->Is(CT_WORD))
   {
      do
      {
         // TODO: should there be a CT_BYREF_TYPE?
         pc = pc->GetPrevNcNnlNi();

         if (pc->IsPointerOperator())
         {
            pc->SetParentType(m_start->GetType());
            pc->SetType(CT_PTR_TYPE);
         }
      } while (pc->IsPointerReferenceOrQualifier());
   }
} // EnumStructUnionParser::mark_pointer_types


void EnumStructUnionParser::mark_template(Chunk *start) const
{
   LOG_FUNC_ENTRY();

   if (start->IsNotNullChunk())
   {
      LOG_FMT(LTEMPL,
              "%s(%d): Template detected: '%s' at orig line %zu, orig col %zu\n",
              __unqualified_func__,
              __LINE__,
              start->Text(),
              start->GetOrigLine(),
              start->GetOrigCol());
   }
   start->SetParentType(CT_TEMPLATE);

   auto *end = start->GetClosingParen(E_Scope::PREPROC);

   if (end->IsNotNullChunk())
   {
      end->SetParentType(CT_TEMPLATE);

      mark_template_args(start, end);
   }
} // EnumStructUnionParser::mark_template


void EnumStructUnionParser::mark_template_args(Chunk *start, Chunk *end) const
{
   LOG_FUNC_ENTRY();

   if (  end->IsNotNullChunk()
      && start->IsNotNullChunk())
   {
      LOG_FMT(LTEMPL,
              "%s(%d): Start of template detected: '%s' at orig line %zu, orig col %zu\n",
              __unqualified_func__,
              __LINE__,
              start->Text(),
              start->GetOrigLine(),
              start->GetOrigCol());

      PcfFlags flags = PCF_IN_TEMPLATE;
      Chunk    *next = start;

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
         next->SetFlagBits(flags);
      }
      LOG_FMT(LTEMPL,
              "%s(%d): End of template detected: '%s' at orig line %zu, orig col %zu\n",
              __unqualified_func__,
              __LINE__,
              end->Text(),
              end->GetOrigLine(),
              end->GetOrigCol());
   }
} // EnumStructUnionParser::mark_template_args


void EnumStructUnionParser::mark_type(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (pc->IsNotNullChunk())
   {
      m_type = pc;

      do
      {
         make_type(pc);
         pc->SetParentType(m_start->GetType());
         pc = pc->GetNextNcNnl(E_Scope::PREPROC);
      } while (pc->IsPointerOrReference());
   }
} // EnumStructUnionParser::mark_type


void EnumStructUnionParser::mark_variable(Chunk *variable, PcfFlags flags)
{
   LOG_FUNC_ENTRY();

   if (variable->IsNotNullChunk())
   {
      LOG_FMT(LVARDEF,
              "%s(%d): Variable definition detected: '%s' at orig line is %zu, orig col is %zu, set %s\n",
              __unqualified_func__,
              __LINE__,
              variable->Text(),
              variable->GetOrigLine(),
              variable->GetOrigCol(),
              flags & PCF_VAR_1ST_DEF ? "PCF_VAR_1ST_DEF" : "PCF_VAR_1ST");

      variable->SetFlagBits(flags);
      variable->SetType(CT_WORD);
      mark_pointer_types(variable);
   }
} // EnumStructUnionParser::mark_variable


void EnumStructUnionParser::mark_where_clause(Chunk *where)
{
   LOG_FUNC_ENTRY();

   if (where->IsNotNullChunk())
   {
      LOG_FMT(LFTOR,
              "%s(%d): Where clause detected: orig line is %zu, orig col is %zu\n",
              __unqualified_func__,
              __LINE__,
              where->GetOrigLine(),
              where->GetOrigCol());
   }
   set_where_start(where);

   auto *where_end   = get_where_end();
   auto *where_start = get_where_start();

   set_where_end(where_end);

   PcfFlags flags;

   for (auto *pc = where_start; pc != where_end; pc = pc->GetNextNcNnl())
   {
      flags = mark_where_chunk(pc, m_start->GetType(), flags);
   }
} // EnumStructUnionParser::mark_where_clause


void EnumStructUnionParser::mark_where_colon(Chunk *colon)
{
   LOG_FUNC_ENTRY();

   if (colon->IsNotNullChunk())
   {
      LOG_FMT(LFTOR,
              "%s(%d): Where colon detected: orig line is %zu, orig col is %zu\n",
              __unqualified_func__,
              __LINE__,
              colon->GetOrigLine(),
              colon->GetOrigCol());
   }
   colon->SetType(CT_WHERE_COLON);
   colon->SetParentType(m_start->GetType());
} // EnumStructUnionParser::mark_where_colon


void EnumStructUnionParser::parse(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   initialize(pc);

   if (parse_error_detected())
   {
      return;
   }

   /**
    * make sure this wasn't a cast, and also make sure we're
    * actually dealing with a class/enum/struct/union type
    */
   if (  m_start->GetParentType() == CT_C_CAST
      || !m_start->IsClassEnumStructOrUnion())
   {
      return;
   }
   Chunk *prev = m_start;
   Chunk *next = prev->GetNextNcNnl();

   /**
    * the enum-key might be enum, enum class or enum struct
    */
   if (next->IsEnum())
   {
      prev = next;
      next = prev->GetNextNcNnl();
   }
   else if (prev->IsEnum())
   {
      Chunk *prev_prev = prev->GetPrevNcNnlNi();

      if (  prev_prev->IsEnum()
         && prev->IsEnum())
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
      if (next->Is(CT_ASSIGN))
      {
         next = skip_to_expression_end(next);
      }

      if (  next->Is(CT_ANGLE_OPEN)
         && !template_detected())
      {
         next = parse_angles(next);
      }
      else if (  next->Is(CT_BRACE_OPEN)
              && !body_detected())
      {
         next = parse_braces(next);
      }
      else if (next->IsColon())
      {
         parse_colon(next);
      }
      else if (next->Is(CT_COMMA))
      {
         record_top_level_comma(next);
      }
      else if (next->Is(CT_DC_MEMBER))
      {
         next = parse_double_colon(next);
      }
      else if (  next->IsParenOpen()
              && (  language_is_set(lang_flag_e::LANG_D)
                 || (  language_is_set(lang_flag_e::LANG_PAWN)
                    && m_start->IsEnum())))
      {
         set_paren_parent(next, m_start->GetType());

         if (  prev->Is(CT_WORD)
            && language_is_set(lang_flag_e::LANG_D))
         {
            mark_template(next);
         }
         next = next->GetClosingParen(E_Scope::PREPROC);
      }
      else if (  next->Is(CT_QUALIFIER)
              && language_is_set(lang_flag_e::LANG_JAVA)
              && std::strncmp(next->GetStr().c_str(), "implements", 10) == 0)
      {
         mark_base_classes(next);
      }
      else if (next->Is(CT_QUESTION))
      {
         record_question_operator(next);
      }
      else if (  next->Is(CT_WHERE)
              && !where_clause_detected())
      {
         mark_where_clause(next);
      }
      prev = next;

      do
      {
         next = next->GetNextNcNnl();
      } while (  next->IsNotNullChunk()
              && next->GetLevel() > m_start->GetLevel());
   }
   /**
    * identify the type and/or variable(s)
    */
   analyze_identifiers();

   /**
    * identify and mark lvalues occurring outside the body definition
    */
   mark_extracorporeal_lvalues();

   if (  prev->IsNotNullChunk()
      && prev->IsSemicolon()
      && prev->GetLevel() == m_start->GetLevel()
      && !prev->TestFlags(PCF_IN_FOR))
   {
      prev->SetParentType(m_start->GetType());
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
      auto *angle_close = angle_open->GetClosingParen(E_Scope::PREPROC);

      if (angle_close->IsNullChunk())
      {
         // parse error
         parse_error_detected(true);

         // TODO: should this be just a warning or an error (with exit condition?)
         LOG_FMT(LWARN,
                 "%s(%d): Unmatched '<' at orig line is %zu, orig col is %zu\n",
                 __unqualified_func__,
                 __LINE__,
                 angle_open->GetOrigLine(),
                 angle_open->GetOrigCol());
      }
      else
      {
         /**
          * check to make sure that the template is the final chunk in a list
          * of scope-resolution qualifications
          */
         auto *next = angle_close->GetNextNcNnl();

         if (next->IsNot(CT_DC_MEMBER))
         {
            set_template_start(angle_open);

            /**
             * we could be dealing with a template type; if so, the opening angle
             * bracket should be preceded by a CT_WORD token and we should have
             * found a closing angle bracket
             */
            auto *prev = angle_open->GetPrevNcNnlNi();

            if (prev->IsNot(CT_WORD))
            {
               // parse error
               parse_error_detected(true);

               // TODO: should this be just a warning or an error (with exit condition?)
               LOG_FMT(LWARN,
                       "%s(%d): Identifier missing before '<' at orig line is %zu, orig col is %zu\n",
                       __unqualified_func__,
                       __LINE__,
                       angle_open->GetOrigLine(),
                       angle_open->GetOrigCol());
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
   auto *brace_close = pc->GetClosingParen(E_Scope::PREPROC);

   if (brace_close->IsNotNullChunk())
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

      if (  (  language_is_set(lang_flag_e::LANG_C)
            || language_is_set(lang_flag_e::LANG_CPP))
         && prev->IsParenClose())
      {
         /**
          * we may be dealing with a c/cpp function definition, where the 'struct'
          * or 'class' keywords appear as the return type preceding a pair of braces
          * and therefore may be associated with a function definition body
          */
         auto *paren_close = prev;

         // skip in reverse to the matching open paren
         auto *paren_open = paren_close->GetOpeningParen();

         if (paren_open->IsNotNullChunk())
         {
            /**
             * determine if there's an identifier preceding the open paren;
             * if so, the identifier is very likely to be associated with
             * a function definition
             */
            auto *type       = m_start->GetNextNcNnl();
            auto *identifier = paren_open->GetPrevNcNnlNi(E_Scope::PREPROC);
            is_potential_function_definition = (  (  identifier->Is(CT_FUNCTION)
                                                  || identifier->Is(CT_FUNC_DEF)
                                                  || identifier->Is(CT_WORD))
                                               && type != identifier);
         }
      }

      if (  language_is_set(lang_flag_e::LANG_D)
         || language_is_set(lang_flag_e::LANG_PAWN)
         || !prev->IsParenClose()
         || is_potential_function_definition
         || chunk_is_between(prev, enum_base_start, brace_open)
         || chunk_is_between(prev, inheritance_start, brace_open))
      {
         mark_braces(brace_open);

         /**
          * D does not require a semicolon after an enum, but we add one to make
          * other code happy.
          */
         if (  language_is_set(lang_flag_e::LANG_D)
            && m_start->IsEnum())
         {
            pawn_add_vsemi_after(brace_close); // Issue #2279
         }
         pc = brace_close;
      }
      else
      {
         // TODO: should this be just a warning or an error (with exit condition?)
         LOG_FMT(LWARN,
                 "%s(%d): Parsing error precedes start of body '{' at orig line is %zu, orig col is %zu\n",
                 __unqualified_func__,
                 __LINE__,
                 brace_open->GetOrigLine(),
                 brace_open->GetOrigCol());

         // parse error
         parse_error_detected(true);
      }
   }
   return(pc);
} // EnumStructUnionParser::parse_braces


void EnumStructUnionParser::parse_colon(Chunk *colon)
{
   LOG_FUNC_ENTRY();

   if (m_start->Is(CT_UNION))
   {
      /**
       * unions do not implement inheritance
       */

      // TODO: should this be just a warning or an error (with exit condition?)
      LOG_FMT(LWARN,
              "%s(%d): Colon follows union declaration at orig line is %zu, orig col is %zu\n",
              __unqualified_func__,
              __LINE__,
              colon->GetOrigLine(),
              colon->GetOrigCol());

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
      if (m_start->IsClassOrStruct())
      {
         /**
          * the colon likely specifies an inheritance list for a struct
          * or class type
          */

         set_inheritance_start(colon);
         mark_class_colon(colon);
      }
      else if (m_start->IsEnum())
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

   if (  language_is_set(lang_flag_e::LANG_CPP)
      && pc->Is(CT_DC_MEMBER))
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

   if (question->Is(CT_QUESTION))
   {
      std::size_t index = m_chunk_map[CT_QUESTION].size();

      m_chunk_map[CT_QUESTION][index] = question;
   }
} // EnumStructUnionParser::record_question_operator


void EnumStructUnionParser::record_top_level_comma(Chunk *comma)
{
   if (  comma->IsNotNullChunk()
      && comma->GetLevel() == m_start->GetLevel()
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

   if (  (  language_is_set(lang_flag_e::LANG_C)
         || language_is_set(lang_flag_e::LANG_CPP))
      && pc->Is(CT_BRACE_CLOSE))
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
         if (next->IsSemicolon())
         {
            pc = next;

            break;
         }
         else
         {
            /**
             * if we're sitting at a comma, skip it
             */
            if (next->Is(CT_COMMA))
            {
               next = next->GetNextNcNnl();
            }
            auto match       = match_variable(next, m_start->GetLevel());
            auto *start      = std::get<0>(match);
            auto *identifier = std::get<1>(match);
            auto *end        = std::get<2>(match);

            if (  end->IsNullChunk()
               || identifier->IsNullChunk()
               || start->IsNullChunk())
            {
               break;
            }
            else
            {
               pc = end->GetNextNcNnl();

               /**
                * skip any right-hand side assignments
                */
               if (pc->Is(CT_ASSIGN))
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

   if (body_end->Is(CT_BRACE_CLOSE))
   {
      m_chunk_map[CT_BRACE_CLOSE][0] = body_end;
   }
} // EnumStructUnionParser::set_body_end


void EnumStructUnionParser::set_body_start(Chunk *body_start)
{
   LOG_FUNC_ENTRY();

   if (body_start->Is(CT_BRACE_OPEN))
   {
      m_chunk_map[CT_BRACE_OPEN][0] = body_start;
   }
} // EnumStructUnionParser::set_body_start


void EnumStructUnionParser::set_enum_base_start(Chunk *enum_base_start)
{
   LOG_FUNC_ENTRY();

   if (enum_base_start->IsColon())
   {
      m_chunk_map[CT_BIT_COLON][0] = enum_base_start;
   }
} // EnumStructUnionParser::set_enum_base_start


void EnumStructUnionParser::set_inheritance_start(Chunk *inheritance_start)
{
   LOG_FUNC_ENTRY();

   if (inheritance_start->IsColon())
   {
      m_chunk_map[CT_COLON][0] = inheritance_start;
   }
} // EnumStructUnionParser::set_inheritance_start


void EnumStructUnionParser::set_template_end(Chunk *template_end)
{
   LOG_FUNC_ENTRY();

   if (template_end->Is(CT_ANGLE_CLOSE))
   {
      m_chunk_map[CT_ANGLE_CLOSE][0] = template_end;
   }
} // EnumStructUnionParser::set_template_end


void EnumStructUnionParser::set_template_start(Chunk *template_start)
{
   LOG_FUNC_ENTRY();

   if (template_start->Is(CT_ANGLE_OPEN))
   {
      m_chunk_map[CT_ANGLE_OPEN][0] = template_start;
   }
} // EnumStructUnionParser::set_template_start


void EnumStructUnionParser::set_where_end(Chunk *where_end)
{
   LOG_FUNC_ENTRY();

   if (where_end->Is(CT_BRACE_OPEN))
   {
      m_chunk_map[CT_WHERE][0] = where_end;
   }
} // EnumStructUnionParser::set_where_end


void EnumStructUnionParser::set_where_start(Chunk *where_start)
{
   LOG_FUNC_ENTRY();

   if (where_start->Is(CT_WHERE))
   {
      m_chunk_map[CT_WHERE][0] = where_start;
   }
} // EnumStructUnionParser::set_where_start


bool EnumStructUnionParser::template_detected() const
{
   LOG_FUNC_ENTRY();

   auto *template_end   = get_template_end();
   auto *template_start = get_template_start();

   return(  template_end->IsNotNullChunk()
         && template_start->IsNotNullChunk());
} // EnumStructUnionParser::template_detected


Chunk *EnumStructUnionParser::try_find_end_chunk(Chunk *pc)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LFTOR, "%s(%d): orig line is %zu, orig col is %zu, type is %s\n",
           __unqualified_func__, __LINE__,
           pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));

   do
   {
      LOG_FMT(LFTOR, "%s(%d): orig line is %zu, orig col is %zu, type is %s\n",
              __unqualified_func__, __LINE__,
              pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));

      /**
       * clear some previously marked token types, some of which have likely
       * been erroneously marked up to this point; a good example of this
       * arises when macro variables and/or macro function calls follow the
       * class/enum/struct/union keyword and precede the actual type name
       */
      if (  pc->Is(CT_TYPE)
         || pc->Is(CT_WORD))
      {
         pc->SetType(CT_WORD);
         pc->SetParentType(CT_NONE);
      }
      LOG_FMT(LFTOR, "%s(%d): orig line is %zu, orig col is %zu, type is %s\n",
              __unqualified_func__, __LINE__,
              pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));

      do
      {
         pc = pc->GetNextNcNnl(E_Scope::PREPROC);
         LOG_FMT(LFTOR, "%s(%d): orig line is %zu, orig col is %zu, type is %s\n",
                 __unqualified_func__, __LINE__,
                 pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));
      } while (  pc->IsNotNullChunk()
              && pc->GetLevel() > m_start->GetLevel());

      if (pc->IsNullChunk())
      {
         LOG_FMT(LFTOR, "%s(%d): IsNullChunk\n",
                 __unqualified_func__, __LINE__);
         // parse error
         parse_error_detected(true);
         return(Chunk::NullChunkPtr);
      }
      else
      {
         LOG_FMT(LFTOR, "%s(%d): orig line is %zu, orig col is %zu, type is %s\n",
                 __unqualified_func__, __LINE__,
                 pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()));
      }
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

   if (  language_is_set(lang_flag_e::LANG_CPP)
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
      Chunk *prev              = Chunk::NullChunkPtr;

      do
      {
         if (  !chunk_is_between(prev, inheritance_start, body_start)
            && (  prev->Is(CT_WORD)
               || prev->Is(CT_FUNCTION)
               || prev->Is(CT_FUNC_DEF))
            && !prev->GetFlags().test_any(PCF_VAR_DEF | PCF_VAR_1ST | PCF_VAR_INLINE)
            && prev->GetLevel() == m_start->GetLevel())
         {
            if (pc->IsParenOpen())
            {
               auto *paren_open  = pc;
               auto *paren_close = paren_open->GetClosingParen(E_Scope::PREPROC);

               if (paren_close->IsNotNullChunk())
               {
                  paren_open->SetType(CT_FPAREN_OPEN);
                  paren_open->SetParentType(CT_MACRO_FUNC_CALL);
                  paren_close->SetType(CT_FPAREN_CLOSE);
                  paren_close->SetParentType(CT_MACRO_FUNC_CALL);
                  prev->SetType(CT_MACRO_FUNC_CALL);
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
      && body_end->IsNullChunk())
   {
      /**
       * a type wasn't identified and no closing brace is present; we're
       * likely not dealing with an anonymous enum/class/struct
       */

      /**
       * a type has yet to be identified, so search for the last word
       * that hasn't been marked as a variable
       */
      Chunk *type = Chunk::NullChunkPtr;
      Chunk *pc   = m_start;

      do
      {
         /**
          * in case it's a qualified identifier, skip scope-resolution and
          * nested name specifiers and return just the qualified identifier name
          */
         pc = skip_scope_resolution_and_nested_name_specifiers(pc);

         if (pc->GetFlags().test_any(PCF_VAR_DEF | PCF_VAR_1ST | PCF_VAR_INLINE))
         {
            break;
         }
         else if (  pc->Is(CT_WORD)
                 || pc->Is(CT_ANGLE_CLOSE))
         {
            type = skip_template_prev(pc);
         }
         pc = pc->GetNextNcNnl();
      } while (chunk_is_between(pc, m_start, m_end));

      if (type->IsNotNullChunk())
      {
         mark_type(type);
      }
   }
} // EnumStructUnionParser::try_post_identify_type


bool EnumStructUnionParser::try_pre_identify_type()
{
   LOG_FUNC_ENTRY();

   Chunk *pc = get_body_start();

   if (  language_is_set(lang_flag_e::LANG_PAWN)
      && m_start->IsEnum())
   {
      set_paren_parent(pc, m_start->GetType());
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

      if (m_start->Is(CT_UNION))
      {
         /**
          * unions do not implement inheritance
          */

         // TODO: should this be just a warning or an error (with exit condition?)
         LOG_FMT(LWARN,
                 "%s(%d): Bad union declaration detected at orig line is %zu, orig col is %zu\n",
                 __unqualified_func__,
                 __LINE__,
                 m_start->GetOrigLine(),
                 m_start->GetOrigCol());

         parse_error_detected(true);

         return(false);
      }
   }

   if (pc->IsNullChunk())
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
              && next->Is(CT_WORD)
              && next_next->Is(CT_WORD)
              && m_end->GetPrevNcNnlNi() == next_next)
      {
         /**
          * check to see if we've got a macro reference preceding the last word chunk;
          * this won't work in all cases, because a macro may be defined in another header
          * file, but this is an attempt to increase the chances of identifying the correct
          * chunk as the type
          */
         if (  chunk_is_macro_reference(next)
            || m_start->GetParentType() == CT_TEMPLATE)
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
               && (  (  next->IsNot(CT_ASSIGN)
                     && next->IsNot(CT_COMMA))
                  || next->GetLevel() != m_start->GetLevel())
               && !next->IsSemicolon())
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
               || next->IsParenOpen())
            {
               prev = next->GetClosingParen(E_Scope::PREPROC);
               next = prev->GetNextNcNnl(E_Scope::PREPROC);
            }

            if (  prev->Is(CT_WORD)
               && next->IsPointerOrReference())
            {
               pc = next;

               break;
            }
         }
      }
   }

   if (pc->IsNotNullChunk())
   {
      /**
       * the chunk preceding the previously selected chunk should indicate the type
       */

      pc = pc->GetPrevNcNnlNi(E_Scope::PREPROC);

      if (  pc->Is(CT_QUALIFIER)
         && std::strncmp(pc->GetStr().c_str(), "final", 5) == 0)
      {
         pc = pc->GetPrevNcNnlNi(E_Scope::PREPROC);
      }

      if (  language_is_set(lang_flag_e::LANG_D)
         && pc->IsParenClose())
      {
         pc = pc->GetOpeningParen();
         pc = pc->GetPrevNcNnlNi();
      }

      if (pc->Is(CT_WORD))
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

   return(m_type->IsNotNullChunk());
} // EnumStructUnionParser::type_identified


/**
 * Returns true if a where clause was detected during parsing
 */
bool EnumStructUnionParser::where_clause_detected() const
{
   LOG_FUNC_ENTRY();

   auto *where_end   = get_where_end();
   auto *where_start = get_where_start();

   return(  where_end->IsNotNullChunk()
         && where_start->IsNotNullChunk());
} // EnumStructUnionParser::where_clause_detected
