/**
 * @file match_tools.cpp
 *
 * @author
 * @license GPL v2+
 */

#include "match_tools.h"

#include "chunk_list.h"
#include "chunk_tests.h"
#include "chunk_tools.h"
#include "combine_skip.h"
#include "keywords.h"

#include <map>


bool adj_chunks_match_compound_type_pattern(chunk_t *prev,
                                            chunk_t *next)
{
   LOG_FUNC_ENTRY();

   if (  prev != nullptr
      && next != nullptr)
   {
      auto get_token_type = [](chunk_t *pc)
      {
         static std::map<bool (*)(chunk_t *), c_token_t> chunk_reassignment_map =
         {
            {
               [](chunk_t *pc_arg)
               {
                  return(chunk_is_identifier(pc_arg, false));
               }, CT_TYPE
            },
            { chunk_is_paren_close_token, CT_PAREN_CLOSE },
            { chunk_is_paren_open_token, CT_PAREN_OPEN  }
         };

         for (auto &&chunk_reassignment_entry : chunk_reassignment_map)
         {
            auto &&test  = chunk_reassignment_entry.first;
            auto &&token = chunk_reassignment_entry.second;

            if (test(pc))
            {
               return(token);
            }
         }

         return(pc != nullptr ? pc->type : CT_NONE);
      };
      auto next_token_type = get_token_type(next);
      auto prev_token_type = get_token_type(prev);

      switch (prev_token_type)
      {
      case CT_ANGLE_CLOSE:
         /**
          * assuming the previous token is possibly the closing angle of a
          * templated type, the next token may be one of the following:
          * - a pointer symbol ('*', '^')
          * - a double colon ('::')
          * - a reference symbol ('&')
          * - a qualifier (const, etc.)
          * - an open square bracket
          * - a set of empty square brackets
          */
         return(  chunk_is_pointer_or_reference(next)
               || next_token_type == CT_DC_MEMBER
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_SQUARE_OPEN
               || next_token_type == CT_TSQUARE);

      case CT_ANGLE_OPEN:
         /**
          * assuming the previous token is possibly the opening angle of a
          * templated type, just check to see if there's a matching closing
          * angle
          */
         return(chunk_skip_to_match(prev, scope_e::PREPROC) != nullptr);

      case CT_CARET:
         /**
          * if the previous token is a managed C++/CLI pointer symbol ('^'),
          * the next token may be one of the following:
          * - a pointer symbol ('*', '^')
          * - a reference symbol ('&')
          * - a qualifier (const, etc.)
          * - an open square bracket
          * - a set of empty square brackets
          */
         return(  language_is_set(LANG_CPP)
               && (  chunk_is_pointer_or_reference(next)
                  || next_token_type == CT_QUALIFIER
                  || next_token_type == CT_SQUARE_OPEN
                  || next_token_type == CT_TSQUARE));

      case CT_DC_MEMBER:
         /**
          * if the previous token is a double colon ('::'), it is likely part
          * of a chain of scope-resolution qualifications preceding a word or
          * type
          */
         return(  next_token_type == CT_TYPE
               || next_token_type == CT_WORD);

      case CT_DECLTYPE:
         /**
          * if the previous token is the decltype keyword, then the next token
          * must be an open paren
          */
         return(next_token_type == CT_PAREN_OPEN);

      case CT_PAREN_CLOSE:
         /**
          * if the previous token is a closing paren, it may be part of a function
          * pointer signature, and so the next token may be an opening paren; on the
          * other hand, the closing paren may conclude the end of a decltype statement,
          * in which case the next token may be one of the following:
          * - a pointer symbol ('*', '^')
          * - a double colon ('::')
          * - a reference symbol ('&')
          * - a qualifier (const, etc.)
          * - an open square bracket
          * - a set of empty square brackets
          */
         return(  chunk_is_pointer_or_reference(next)
               || next_token_type == CT_DC_MEMBER
               || next_token_type == CT_PAREN_OPEN
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_SQUARE_OPEN
               || next_token_type == CT_TSQUARE);

      case CT_PAREN_OPEN:
         /**
          * if the previous token is an opening paren, it may be part of a function
          * pointer signature, and so the next token may be a pointer symbol; it also
          * may be associated with a decltype statement, so in that case simply skip
          * ahead to a matching closing paren
          */
         return(  chunk_is_ptr_operator(next)
               || chunk_skip_to_match(prev, scope_e::PREPROC) != nullptr);

      case CT_PTR_TYPE:
      case CT_STAR:

         /**
          * if the previous token is a pointer type, ('*'), it may be part of a function
          * pointer signature, and so the next token may be a closing paren; otherwise,
          * the next token may be one of the following:
          * - another pointer symbol ('*', '^')
          * - a reference symbol ('&')
          * - a qualifier (const, etc.)
          * - an open square bracket
          * - a set of empty square brackets
          */
         return(  chunk_is_pointer_or_reference(next)
               || next_token_type == CT_PAREN_CLOSE
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_SQUARE_OPEN
               || next_token_type == CT_TSQUARE);

      case CT_QUALIFIER:
         /**
          * if the previous token is a qualifier (const, etc.), the next token
          * may be one of the following:
          * - a pointer symbol ('*', '^')
          * - a reference symbol ('&')
          * - another qualifier
          * - an open square bracket
          * - a set of empty square brackets
          */
         return(  chunk_is_pointer_or_reference(next)
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_SQUARE_OPEN
               || next_token_type == CT_TSQUARE);

      case CT_SQUARE_CLOSE:
         /**
          * if the previous token is a close square bracket, the next token may be
          * another opening square bracket
          */
         return(next_token_type == CT_SQUARE_OPEN);

      case CT_SQUARE_OPEN:
         /**
          * if the previous token is an open square bracket, it may indicate an
          * array declaration - skip ahead to find a matching close square bracket
          */
         return(chunk_skip_to_match(prev, scope_e::PREPROC) != nullptr);

      case CT_TSQUARE:
         /**
          * if the previous token is a set of brackets, the next token may be
          * an open square bracket
          */
         return(next_token_type == CT_SQUARE_OPEN);

      case CT_TYPE:
         /**
          * if the previous token is marked as a type, the next token may be
          * one of the following:
          * - a pointer symbol ('*', '^')
          * - a reference symbol ('&')
          * - an opening angle, which may indicate a templated type
          * - a double colon ('::')
          * - an opening paren, which may indicate a function pointer
          * - a qualifier (const, etc.)
          * - an opening square bracket, which may indicate an array
          * - a set of empty square brackets, which may also indicate an array
          * - an identifier
          */
         return(  chunk_is_pointer_or_reference(next)
               || next_token_type == CT_ANGLE_OPEN
               || (  next_token_type == CT_DC_MEMBER
                  && !chunk_is_keyword(prev))
               || next_token_type == CT_PAREN_OPEN
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_SQUARE_OPEN
               || next_token_type == CT_TSQUARE
               || next_token_type == CT_WORD);

      case CT_TYPEDEF:
         /**
          * if the previous token is a typedef, the next token may be one of the
          * following:
          * - a double colon ('::')
          * - the decltype keyword
          * - a qualifier (const, etc.)
          * - an identifier
          * - the typename keyword
          */
         return(  next_token_type == CT_DC_MEMBER
               || next_token_type == CT_DECLTYPE
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_TYPE
               || next_token_type == CT_TYPENAME);

      case CT_TYPENAME:
         /**
          * if the previous token is a typename, the next token may be one of the
          * following:
          */
         return(  next_token_type == CT_DC_MEMBER
               || next_token_type == CT_DECLTYPE
               || next_token_type == CT_QUALIFIER
               || (  next_token_type == CT_TYPE
                  && !chunk_is_keyword(prev)));

      default:
         // do nothing
         break;
      } // switch
   }
   return(false);
} // adj_chunks_match_compound_type_pattern


bool adj_chunks_match_qualified_identifier_pattern(chunk_t *prev,
                                                   chunk_t *next)
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
          * templated type, the next token may be a scope resolution operator ('::')
          */
         return(next_token_type == CT_DC_MEMBER);

      case CT_ANGLE_OPEN:
         /**
          * assuming the previous token is possibly the opening angle of a
          * templated type, just check to see if there's a matching closing
          * angle
          */
         return(chunk_skip_to_match(prev, scope_e::PREPROC) != nullptr);

      case CT_DC_MEMBER:
         /**
          * if the previous token is a double colon ('::'), it is likely part
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
          * - a double colon ('::')
          */
         return(  next_token_type == CT_ANGLE_OPEN
               || next_token_type == CT_DC_MEMBER);

      default:
         // do nothing
         break;
      } // switch
   }
   return(false);
} // adj_chunks_match_qualified_identifier_pattern


bool adj_chunks_match_template_end_pattern(chunk_t *prev,
                                           chunk_t *next)
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
          * - class, struct or union:                template<... > class/struct/union
          * - a colon (':'):                                T<... > : public ... { }
          * - a pointer symbol ('*', '^'):               vector<T > *
          * - a reference symbol ('&'):                  vector<T > &
          * - a closing angle ('>'):            <Class<typename T > >
          * - an assignment symbol ('='):                         ? Not sure if this situation can arise in valid C++ syntax?
          * - an opening brace ('{'):                    vector<T > { t, ... }
          * - a comma (','):                    void foo(vector<T > , ...)
          * - a double colon ('::'):                     vector<T > ::iterator
          * - an ellipsis ('...'):                             <T > ...
          * - a closing paren (')'):            void foo(vector<T > )
          * - an opening paren ('('):                    vector<T > ()
          * - a qualifier (const, etc.):                 vector<T > const
          * - a semicolon (';'):               using A = vector<T > ;
          * - an opening square bracket ('['):           vector<T > []
          * - a template keyword:                       template< > template
          * - an identifier:                             vector<T > ClassType::function
          * - a using keyword:                       template<... > using
          */
         return(  chunk_is_class_struct_union(next)
               || chunk_is_colon_token(next)
               || chunk_is_pointer_or_reference(next)
               || next_token_type == CT_ANGLE_CLOSE
               || next_token_type == CT_ASSIGN // TODO: not sure about this one...
               || next_token_type == CT_BRACE_OPEN
               || next_token_type == CT_COMMA
               || next_token_type == CT_DC_MEMBER
               || next_token_type == CT_ELLIPSIS
               || next_token_type == CT_PAREN_CLOSE
               || next_token_type == CT_PAREN_OPEN
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_SEMICOLON
               || next_token_type == CT_SQUARE_OPEN
               || next_token_type == CT_TEMPLATE
               || next_token_type == CT_TYPE
               || next_token_type == CT_USING
               || next_token_type == CT_WORD
               || get_chunk_parent_type(next) == CT_CLASS
               || get_chunk_parent_type(next) == CT_ENUM
               || get_chunk_parent_type(next) == CT_ENUM_CLASS
               || get_chunk_parent_type(next) == CT_FUNC_CLASS_DEF
               || get_chunk_parent_type(next) == CT_FUNC_DEF
               || get_chunk_parent_type(next) == CT_FUNCTION
               || get_chunk_parent_type(next) == CT_STRUCT
               || get_chunk_parent_type(next) == CT_UNION);

      case CT_ANGLE_OPEN:
      case CT_BYREF:
      case CT_CLASS:
      case CT_ELLIPSIS:
      case CT_NUMBER:
      case CT_PAREN_CLOSE:
      case CT_PTR_TYPE:
      case CT_QUALIFIER:
      case CT_SQUARE_CLOSE:
      case CT_STAR:
      case CT_TYPE:
      case CT_TYPENAME:
      case CT_WORD:
         /**
          * assuming the next token may be a closing angle to a templated type,
          * the previous token may be one of the following:
          * - an opening angle ('<'):    template < >
          * - a reference symbol ('&'):        <T & >
          * - a class keyword:               <class >
          * - an ellipsis ('...'):    <typename ... >
          * - a number:                          <1 >
          * - a closing paren:                <T () >
          * - a pointer symbol ('*', '^'):     <T * >
          * - a qualifier (const, etc.):   <T const >
          * - a closing square bracket (']'): <T [] >
          * - an identifier:                     <T >
          * - typename keyword:           <typename >
          */
         return(next_token_type == CT_ANGLE_CLOSE);

      // TODO: verify that this actually works...
      case CT_STRING:
         /**
          * assuming the next token may be a closing angle to a templated type,
          * the previous token may also be a single quote ('''): T <'a' >
          */
         return(  prev->str.size() > 0
               && prev->str.back() == '\'');

      default:
         // do nothing
         break;
      } // switch
   }
   return(false);
} // adj_chunks_match_template_end_pattern


bool adj_chunks_match_template_start_pattern(chunk_t *prev,
                                             chunk_t *next)
{
   LOG_FUNC_ENTRY();

   if (  prev != nullptr
      && next != nullptr)
   {
      auto prev_token_type = prev->type;
      auto next_token_type = next->type;

      switch (prev_token_type)
      {
      case CT_ANGLE_OPEN:

         /**
          * assuming the previous token is possibly the opening angle of a
          * templated type, the next token may be one of the following:
          * - class, enum, struct or union: template < class/enum/struct/union ...>
          * - an closing angle ('>'):       template < >
          * - a double colon ('::'):          vector < ::T>
          * - a decltype statement:           vector < decltype(T::foo)>
          * - bitwise not ('~'):                   T < ~0>
          * - unary minus ('-'):                   T < -1>
          * - a logical not operator ('!'):        T < !true>
          * - a number:                              < 1>
          * - an opening paren ('('):              T < (X > 3)>
          * - unary plus ('+'):                    T < +1>
          * - a qualifier (const, etc.):      vector < const T>
          * - sizeof operator:                     T < sizeof(int)>
          * - a single quote ('''):                T < 'a'>
          * - a template keyword:           template < template<class> T>
          * - an identifier:                  vector < T> ClassType::function
          * - typename keyword:             template < typename>
          */
         return(  chunk_is_class_struct_union(next)
               || next_token_type == CT_ANGLE_CLOSE
               || next_token_type == CT_DC_MEMBER
               || next_token_type == CT_DECLTYPE
               || next_token_type == CT_INV
               || next_token_type == CT_MINUS
               || next_token_type == CT_NOT
               || next_token_type == CT_NUMBER
               || next_token_type == CT_PAREN_OPEN
               || next_token_type == CT_PLUS
               || next_token_type == CT_QUALIFIER
               || next_token_type == CT_SIZEOF
               || (  next_token_type == CT_STRING // TODO: verify that this actually works...
                  && next->str.size() > 0
                  && next->str[0] == '\'')
               || next_token_type == CT_TEMPLATE
               || next_token_type == CT_TYPE
               || next_token_type == CT_TYPENAME
               || next_token_type == CT_WORD);

      case CT_TEMPLATE:
      case CT_TYPE:
      case CT_WORD:
         /**
          * assuming the next token may be an opening angle to a templated type,
          * the previous token may be one of the following:
          * - a template keyword: template < >
          * - an identifier:             T < ...>
          */
         return(next_token_type == CT_ANGLE_OPEN);

      default:
         // do nothing
         break;
      } // switch
   }
   return(false);
} // adj_chunks_match_template_start_pattern


bool adj_chunks_match_var_def_pattern(chunk_t *prev,
                                      chunk_t *next)
{
   LOG_FUNC_ENTRY();

   if (  prev != nullptr
      && next != nullptr)
   {
      auto get_token_type = [](chunk_t *pc)
      {
         if (chunk_is_paren_close_token(pc))
         {
            return(CT_PAREN_CLOSE);
         }
         else if (chunk_is_paren_open_token(pc))
         {
            return(CT_PAREN_OPEN);
         }
         return(pc != nullptr ? pc->type : CT_NONE);
      };
      auto next_token_type = get_token_type(next);
      auto prev_token_type = get_token_type(prev);

      switch (prev_token_type)
      {
      case CT_ANGLE_CLOSE:
         /**
          * assuming the previous token is possibly the closing angle of a
          * templated type, the next token may be one of the following:
          * - a pointer symbol ('*', '^')
          * - a double colon ('::')
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
         return(chunk_skip_to_match(prev, scope_e::PREPROC) != nullptr);

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
         return(chunk_skip_to_match(prev, scope_e::PREPROC) != nullptr);

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
          * if the previous token is a double colon ('::'), it is likely part
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
         return(chunk_skip_to_match(prev, scope_e::PREPROC) != nullptr);

      case CT_PTR_TYPE:
      case CT_STAR:

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
          * if the previous token is a close square bracket, the next token may be
          * another open square bracket or an assignment following an array variable
          * declaration
          */
         return(  next_token_type == CT_ASSIGN
               || next_token_type == CT_SQUARE_OPEN);

      case CT_SQUARE_OPEN:
         /**
          * if the previous token is an open square bracket, it may indicate an
          * array declaration - skip ahead to find a matching close square bracket
          */
         return(chunk_skip_to_match(prev, scope_e::PREPROC) != nullptr);

      case CT_TSQUARE:
         /**
          * if the previous token is a set of brackets, the next token may be
          * another open square bracket or an assignment following an array variable
          * declaration
          */
         return(  next_token_type == CT_ASSIGN
               || next_token_type == CT_SQUARE_OPEN);

      case CT_TYPE:
         /**
          * if the previous token is marked as a type, the next token may be
          * one of the following:
          * - a pointer symbol ('*', '^')
          * - a reference symbol ('&')
          * - an opening angle, which may indicate a templated type as part of a
          *   scope resolution preceding the actual variable identifier
          * - a double colon ('::')
          * - a qualifier (const, etc.)
          * - an identifier
          */
         return(  chunk_is_pointer_or_reference(next)
               || next_token_type == CT_ANGLE_OPEN
               || (  next_token_type == CT_DC_MEMBER
                  && !chunk_is_keyword(prev))
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
          * - a double colon ('::')
          * - an opening paren, which may indicate a constructor call parameter
          *   list
          * - an opening square bracket, which may indicate an array variable
          * - a set of empty square brackets, which also may indicate an array
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
} // adj_chunks_match_var_def_pattern


chunk_t *match_assigned_type(chunk_t *pc_assign)
{
   if (chunk_is_assign_token(pc_assign))
   {
      auto *prev = chunk_get_prev_ncnnlni(pc_assign, scope_e::PREPROC);

      /**
       * skip any preceding pointers, references, or qualifiers
       */
      prev = skip_pointers_references_and_qualifiers(prev, scope_e::PREPROC);

      if (chunk_is_identifier(prev))
      {
         auto *next = prev;
         prev = chunk_get_prev_ncnnlni(prev);

         if (  chunk_is_typename_token(prev)
            || chunk_is_token(prev, CT_USING))
         {
            return(next);
         }
      }
      else if (chunk_is_auto_token(prev))
      {
         return(prev);
      }
   }
   return(nullptr);
} // match_assigned_type


chunk_t *match_chain_next(chunk_t                                                  *pc,
                          const std::vector<std::pair<const char *, std::size_t> > &chain,
                          int                                                      level,
                          scope_e                                                  scope)
{
   chunk_t *next = nullptr;

   do
   {
      auto &&itChain = chain.cbegin();

      while (itChain != chain.cend())
      {
         auto &&string = itChain->first;
         auto &&size   = itChain->second;

         if (itChain == chain.cbegin())
         {
            if (!chunk_is_str(pc, string, size))
            {
               pc = chunk_get_next_str(pc,
                                       string,
                                       size,
                                       level,
                                       scope);
            }
            next = pc;
         }
         else
         {
            next = chunk_get_next(next, scope);

            if (  next == nullptr
               || !chunk_is_str(next, string, size))
            {
               break;
            }
         }
         ++itChain;
      }

      if (itChain == chain.cend())
      {
         return(pc);
      }
   } while (next != nullptr);

   return(nullptr);
} // match_chain_next


chunk_t *match_chain_next(chunk_t                      *pc,
                          const std::vector<c_token_t> &chain,
                          int                          level,
                          scope_e                      scope)
{
   chunk_t *next = nullptr;

   do
   {
      auto &&itChain = chain.cbegin();

      while (itChain != chain.cend())
      {
         auto &&type = *itChain;

         if (itChain == chain.cbegin())
         {
            if (chunk_is_not_token(pc, type))
            {
               pc = chunk_get_next_type(pc,
                                        type,
                                        level,
                                        scope);
            }
            next = pc;
         }
         else
         {
            next = chunk_get_next(next, scope);

            if (  next == nullptr
               || chunk_is_not_token(next, type))
            {
               break;
            }
         }
         ++itChain;
      }

      if (itChain == chain.cend())
      {
         return(pc);
      }
   } while (next != nullptr);

   return(nullptr);
} // match_chain_next


chunk_t *match_chain_prev(chunk_t                                                  *pc,
                          const std::vector<std::pair<const char *, std::size_t> > &chain,
                          int                                                      level,
                          scope_e                                                  scope)
{
   chunk_t *prev = nullptr;

   do
   {
      auto &&itChain = chain.crbegin();

      while (itChain != chain.crend())
      {
         auto &&string = itChain->first;
         auto &&size   = itChain->second;

         if (itChain == chain.crbegin())
         {
            if (!chunk_is_str(pc, string, size))
            {
               pc = chunk_get_prev_str(pc,
                                       string,
                                       size,
                                       -1,
                                       scope);
            }
            prev = pc;
         }
         else
         {
            prev = chunk_get_prev(prev, scope);

            if (  prev == nullptr
               || !chunk_is_str(prev, string, size))
            {
               break;
            }
         }
         ++itChain;
      }

      if (  itChain == chain.crend()
         && int(pc->level) == level)
      {
         return(pc);
      }
   } while (prev != nullptr);

   return(nullptr);
} // match_chain_prev


chunk_t *match_chain_prev(chunk_t                      *pc,
                          const std::vector<c_token_t> &chain,
                          int                          level,
                          scope_e                      scope)
{
   chunk_t *prev = nullptr;

   do
   {
      auto &&itChain = chain.crbegin();

      while (itChain != chain.crend())
      {
         auto &&type = *itChain;

         if (itChain == chain.crbegin())
         {
            if (chunk_is_not_token(pc, type))
            {
               pc = chunk_get_prev_type(pc,
                                        type,
                                        -1,
                                        scope);
            }
            prev = pc;
         }
         else
         {
            prev = chunk_get_prev(prev, scope);

            if (  prev == nullptr
               || chunk_is_not_token(prev, type))
            {
               break;
            }
         }
         ++itChain;
      }

      if (  itChain == chain.crend()
         && int(pc->level) == level)
      {
         return(pc);
      }
   } while (prev != nullptr);

   return(nullptr);
} // match_chain_prev


std::pair<chunk_t *,
          chunk_t *> match_compound_type(chunk_t *pc, std::size_t level)
{
   chunk_t *start = match_compound_type_start(pc, level);
   chunk_t *end   = match_compound_type_end(pc, level);

   if (  start != nullptr
      && end != nullptr)
   {
      return(std::make_pair(start, end));
   }
   return(std::make_pair(nullptr, nullptr));
} // match_compound_type


chunk_t *match_compound_type_end(chunk_t *pc, std::size_t level)
{
   LOG_FUNC_ENTRY();

   /**
    * if the chunk under test is a closing paren, back up to the matching open paren
    */
   if (chunk_is_paren_close_token(pc))
   {
      pc = chunk_skip_to_match_rev(pc, scope_e::PREPROC);
   }

   /**
    * if the chunk under test is an open paren, back up to the preceding chunk
    */
   if (chunk_is_paren_open_token(pc))
   {
      pc = chunk_get_prev_ncnnlni(pc, scope_e::PREPROC);
   }

   while (pc != nullptr)
   {
      /**
       * skip current and subsequent chunks if at a higher level
       */
      while (  pc != nullptr
            && pc->level > level)
      {
         pc = chunk_get_next_ncnnl(pc, scope_e::PREPROC);
      }

      /**
       * skip to any subsequent match for angle brackets or square brackets
       */
      if (  chunk_is_angle_open_token(pc)
         || chunk_is_square_open_token(pc))
      {
         pc = chunk_skip_to_match(pc, scope_e::PREPROC);

         if (pc == nullptr)
         {
            return(nullptr);
         }
         else if (chunk_is_angle_close_token(pc))
         {
            // TODO: Should we descend into sequence enclosed by the angle brackets
            //       and perform tests to determine if a valid template is represented?
         }
      }
      /**
       * get the next chunk
       */
      auto *next = chunk_get_next_ncnnl(pc, scope_e::PREPROC);

      if (  chunk_is_intrinsic_type(pc)
         && chunk_is_angle_open_token(next))
      {
         /**
          * This shouldn't happen if the code is valid...
          */
         return(nullptr);
      }

      /**
       * skip decltype statements
       */
      if (  chunk_is_decltype_token(pc)
         && chunk_is_token(next, CT_PAREN_OPEN))
      {
         pc   = chunk_skip_to_match(next, scope_e::PREPROC);
         next = chunk_get_next_ncnnl(pc, scope_e::PREPROC);
      }

      /**
       * test for type assignment, which may be embedded within
       * template argument lists or type alias declarations
       */
      if (  chunk_is_identifier(pc)
         && chunk_is_assign_token(next)
         && match_assigned_type(next) == pc)
      {
         return(pc);
      }

      /**
       * we're done searching under the following conditions:
       * - next chunk is null
       * - the level decreases relative to the starting chunk
       * - a comma is encountered at brace level
       * - a semicolon is encountered
       * TODO: what are some additional constraints that should be tested?
       */
      if (  next == nullptr
         || next->level < level
         || (  chunk_is_comma_token(next)
            && next->level == level)
         || chunk_is_semicolon_token(next))
      {
         return(pc);
      }

      /**
       * if the chunk is an opening paren, test to see if it belongs
       * to a function pointer signature
       */
      if (chunk_is_paren_open_token(next))
      {
         /**
          * check to see the paren is part of a function pointer signature
          */
         std::tuple<chunk_t *, chunk_t *, chunk_t *> match(nullptr, nullptr, nullptr);

         if (match_function_pointer_at_paren(next, match))
         {
            /**
             * it matches a function pointer, return the ending chunk
             */
            return(std::get<2>(match));
         }
         /**
          * TODO: is there another case in which an opening paren is valid as part of a
          * compound type declaration?
          */
         return(nullptr);
      }

      /**
       * call a separate function to validate adjacent tokens as potentially
       * matching a variable declaration/definition
       */
      if (!adj_chunks_match_compound_type_pattern(pc, next))
      {
         /**
          * before abandoning, test for two adjacent identifiers -
          * it's possible that one may be a reference to a macro
          */
         if (  !chunk_is_macro_reference(pc)
            || !chunk_is_identifier(next))
         {
            return(nullptr);
         }
      }
      pc = next;
   }
   return(nullptr);
} // match_compound_type_end


chunk_t *match_compound_type_start(chunk_t *pc, std::size_t level)
{
   LOG_FUNC_ENTRY();

   while (pc != nullptr)
   {
      /**
       * skip current and preceding chunks if at a higher level
       */
      while (  pc != nullptr
            && pc->level > level)
      {
         pc = chunk_get_prev_ncnnlni(pc, scope_e::PREPROC);
      }

      /**
       * skip to any preceding match for angle brackets or square brackets
       */
      if (  chunk_is_angle_close_token(pc)
         || chunk_is_square_close_token(pc))
      {
         pc = chunk_skip_to_match_rev(pc, scope_e::PREPROC);

         if (pc == nullptr)
         {
            return(nullptr);
         }
         else if (chunk_is_angle_open_token(pc))
         {
            // TODO: Should we descend into sequence enclosed by the angle brackets
            //       and perform tests to determine if a valid template is represented?
         }
      }
      /**
       * get the previous chunk
       */
      auto *prev = chunk_get_prev_ncnnlni(pc, scope_e::PREPROC);

      if (  chunk_is_intrinsic_type(prev)
         && chunk_is_angle_open_token(pc))
      {
         /**
          * This shouldn't happen if the code is valid...
          */
         return(nullptr);
      }

      /**
       * test for type assignment, which may be embedded within
       * template argument lists or type alias declarations
       */
      if (  chunk_is_identifier(prev)
         && chunk_is_assign_token(pc)
         && match_assigned_type(pc) == prev)
      {
         return(prev);
      }

      /**
       * we're done searching under the following conditions:
       * - previous chunk is null
       * - the level decreases relative to the starting chunk
       * - a comma is encountered at brace level
       * - a semicolon is encountered
       * - typedef keyword is encountered
       * - typename keyword is encountered
       *
       * TODO: what are some additional constraints that should be tested?
       */
      if (  prev == nullptr
         || prev->level < level
         || (  chunk_is_comma_token(prev)
            && prev->level == level)
         || chunk_is_semicolon_token(prev)
         || chunk_is_token(prev, CT_TYPEDEF)
         || chunk_is_typename_token(prev))
      {
         return(pc);
      }

      /**
       * if the chunk is a closing paren, skip back to the matching
       * open paren
       */
      if (chunk_is_paren_close_token(pc))
      {
         pc   = chunk_skip_to_match_rev(pc, scope_e::PREPROC);
         prev = chunk_get_prev_ncnnlni(pc, scope_e::PREPROC);
      }

      /**
       * if the chunk is an opening paren, test to see if it belongs
       * to a function pointer signature or decltype statement
       */
      if (  chunk_is_paren_open_token(pc)
         && chunk_is_not_token(prev, CT_DECLTYPE))
      {
         /**
          * check to see the paren is part of a function pointer signature
          */
         std::tuple<chunk_t *, chunk_t *, chunk_t *> match(nullptr, nullptr, nullptr);

         if (match_function_pointer_at_paren(pc, match))
         {
            /**
             * it matches a function pointer, return the starting chunk
             */
            return(std::get<0>(match));
         }
         /**
          * TODO: is there another case in which a paren is valid as part of a
          * compound type declaration?
          */
         return(nullptr);
      }

      /**
       * call a separate function to validate adjacent tokens as potentially
       * matching a variable declaration/definition
       */
      if (!adj_chunks_match_compound_type_pattern(prev, pc))
      {
         /**
          * before abandoning, test for two adjacent identifiers -
          * it's possible that one may be a reference to a macro
          */
         if (  !chunk_is_macro_reference(prev)
            || !chunk_is_identifier(pc))
         {
            return(nullptr);
         }
      }
      pc = prev;
   }
   return(nullptr);
} // match_compound_type_start


chunk_t *match_function_header_at_close_paren(chunk_t *pc)
{
   // TODO: Need to account for virtual and override keywords!!!!

   if (chunk_is_paren_close_token(pc))
   {
      /**
       * Skip to the matching open paren
       */
      auto *paren_close = pc;
      auto *paren_open  = chunk_skip_to_match_rev(paren_close, scope_e::PREPROC);

      if (paren_open != nullptr)
      {
         chunk_t *identifier = nullptr;

         /**
          * Test to see if an identifier precedes the open paren
          */
         pc = chunk_get_prev_ncnnlni(paren_open, scope_e::PREPROC);

         if (chunk_is_identifier(pc))
         {
            /**
             * Skip any scope resolution and nested name specifiers
             */
            identifier = skip_scope_resolution_and_nested_name_specifiers_rev(pc,
                                                                              scope_e::PREPROC);
         }
         else if (chunk_is_overloaded_token(pc))
         {
            pc = skip_operator_overload_prev(pc, scope_e::PREPROC);

            /**
             * It's an operator overload; if a double colon precedes the operator keyword,
             * it's a member operator overload
             */
            if (chunk_is_double_colon_token(pc))
            {
               /**
                * Skip any scope resolution and nested name specifiers
                */
               identifier = skip_scope_resolution_and_nested_name_specifiers_rev(pc,
                                                                                 scope_e::PREPROC);
            }
         }
         else
         {
            /**
             * If neither an identifier nor an operator overload precede the open paren,
             * we're likely not dealing with a function header
             */
            return(nullptr);
         }

         if (identifier != nullptr)
         {
            /**
             * If we're dealing with something other than a non-member operator overload,
             * then get the chunk preceding the (qualified) identifier
             */
            pc = chunk_get_prev_ncnnlni(identifier, scope_e::PREPROC);
         }
         std::size_t level = pc->level;
         pc = chunk_get_prev_ncnnlni(pc, scope_e::PREPROC);
         auto        *return_type_start = match_compound_type_start(pc, level);

         if (  return_type_start != nullptr
            && (  chunk_is_identifier(return_type_start)
               || chunk_is_intrinsic_type(return_type_start)))
         {
            /**
             * We've matched a chain of chunks consisting of the form:
             *
             * - return_type function(...) [const/volatile/&/&&] { ... pc ... }
             */
            return(return_type_start);
         }
         /**
          * If no return type, test to see if it is a constructor...
          */
         else if (identifier != nullptr)
         {
            /**
             * We've matched a chain of chunks consisting of the form:
             *
             * - function(...) [const/volatile/&/&&] { ... pc ... }
             */
            chunk_t *next = chunk_get_next_ncnnl(paren_close);

            if (  level > 0
                  /* constructors CANNOT have trailing qualifiers... */
               && (  chunk_is_ampersand_token(next)
                  || chunk_is_cv_qualifier_token(next)
                  || chunk_is_double_ampersand_token(next)
                  || chunk_is_noexcept_token(next)))
            {
               chunk_t *brace_open = chunk_get_prev_type(identifier,
                                                         CT_BRACE_OPEN,
                                                         level - 1,
                                                         scope_e::PREPROC);
               chunk_t *class_type = nullptr;

               if (brace_open != nullptr)
               {
                  class_type = chunk_get_prev_str(brace_open,
                                                  identifier->str.c_str(),
                                                  identifier->str.size(),
                                                  level - 1,
                                                  scope_e::PREPROC);
               }
               chunk_t *keyword = nullptr;

               if (class_type != nullptr)
               {
                  keyword = chunk_get_prev_type(class_type,
                                                { CT_CLASS, CT_STRUCT },
                                                level - 1,
                                                scope_e::PREPROC);
               }

               if (  class_type != nullptr
                  && keyword != nullptr)
               {
                  /**
                   * We've matched a class constructor definition of the form:
                   *
                   * - class_type(...) { ... pc ... }
                   */

                  return(identifier);
               }
            }
         }
      }
   }
   return(nullptr);
} // match_function_header_at_close_paren


std::tuple<chunk_t *,
           chunk_t *,
           chunk_t *> match_function_pointer_at_paren(chunk_t *pc_paren)
{
   LOG_FUNC_ENTRY();

   std::tuple<chunk_t *, chunk_t *, chunk_t *> match(nullptr, nullptr, nullptr);

   match_function_pointer_at_paren(pc_paren,
                                   match);

   return(match);
} // match_function_pointer_at_paren


bool match_function_pointer_at_paren(chunk_t               *pc_paren,
                                     std::tuple<chunk_t *,
                                                chunk_t *,
                                                chunk_t *> &match)
{
   LOG_FUNC_ENTRY();

   if (chunk_is_paren_close_token(pc_paren))
   {
      pc_paren = chunk_skip_to_match_rev(pc_paren);
   }

   if (chunk_is_paren_open_token(pc_paren))
   {
      /**
       * The form of a function pointer will look similar to the following:
       * - [return_type/void] (*ptr)(...)
       * - [return_type/void] (Class::*ptr)(...) [const]
       */

      chunk_t *param_list_paren_close = nullptr;
      chunk_t *param_list_paren_open  = nullptr;
      chunk_t *pc_paren_open          = pc_paren;
      chunk_t *pc_paren_close         = chunk_skip_to_match(pc_paren_open, scope_e::PREPROC);

      chunk_t *prev = chunk_get_prev_ncnnlni(pc_paren_open, scope_e::PREPROC);

      if (chunk_is_paren_close_token(prev))
      {
         param_list_paren_close = pc_paren_close;
         param_list_paren_open  = pc_paren_open;
         pc_paren_close         = prev;
         pc_paren_open          = chunk_skip_to_match_rev(pc_paren_close, scope_e::PREPROC);
      }
      else
      {
         chunk_t *next = chunk_get_next_ncnnl(pc_paren_close, scope_e::PREPROC);

         if (chunk_is_paren_open_token(next))
         {
            param_list_paren_open  = next;
            param_list_paren_close = chunk_skip_to_match(param_list_paren_open, scope_e::PREPROC);
         }
      }

      if (  param_list_paren_close != nullptr
         && param_list_paren_open != nullptr
         && pc_paren_close != nullptr
         && pc_paren_open != nullptr)
      {
         /**
          * test the tokens between the first set of parenthesis
          */
         chunk_t *next = chunk_get_next_ncnnl(pc_paren_open, scope_e::PREPROC);

         /**
          * skip any scope resolution qualifiers
          */
         next = chunk_skip_dc_member(next, scope_e::PREPROC);

         /**
          * check to see if the next token is a star
          */
         if (chunk_is_star_token(next))
         {
            /**
             * get the next chunk - if it's a function pointer variable or typedef,
             * the next chunk will be an identifier
             */
            next = chunk_get_next_ncnnl(next, scope_e::PREPROC);

            chunk_t *identifier = nullptr;

            if (chunk_is_identifier(next))
            {
               identifier = next;

               next = chunk_get_next_ncnnl(identifier, scope_e::PREPROC);
            }

            /**
             * the next chunk should be the closing paren identified earlier
             */
            if (next == param_list_paren_close)
            {
               std::size_t level  = prev->level;
               auto        *start = match_compound_type_start(prev, level);

               if (start != nullptr)
               {
                  /**
                   * check the chunk after the parameter list closing paren - it could
                   * be a member function qualifier such as 'const' or a ref qualifier '& or &&'
                   */
                  auto *end = param_list_paren_close;
                  next = chunk_get_next_ncnnl(param_list_paren_close,
                                              scope_e::PREPROC);

                  if (chunk_is_token(next, CT_QUALIFIER))
                  {
                     end = next;
                  }
                  next = chunk_get_next_ncnnl(next, scope_e::PREPROC);

                  if (  chunk_is_double_ampersand_str(next)
                     || chunk_is_ampersand_str(next))
                  {
                     end = next;
                  }
                  std::get<0>(match) = start;
                  std::get<1>(match) = identifier;
                  std::get<2>(match) = end;

                  return(true);
               }
            }
         }
      }
   }
   return(false);
} // match_function_pointer_at_paren


std::tuple<chunk_t *,
           chunk_t *,
           chunk_t *> match_function_pointer_typedef_at_identifier(chunk_t *pc_identifier)
{
   LOG_FUNC_ENTRY();

   std::tuple<chunk_t *, chunk_t *, chunk_t *> match(nullptr, nullptr, nullptr);

   match_function_pointer_at_paren(pc_identifier,
                                   match);

   return(match);
} // match_function_pointer_typedef_at_identifier


bool match_function_pointer_typedef_at_identifier(chunk_t *pc_identifier, std::tuple<chunk_t *,
                                                                                     chunk_t *,
                                                                                     chunk_t *> &match)
{
   LOG_FUNC_ENTRY();

   if (match_function_pointer_variable_at_identifier(pc_identifier,
                                                     match))
   {
      auto * &start = std::get<0>(match);
      auto   *prev  = chunk_get_prev_ncnnlni(start, scope_e::PREPROC);

      if (chunk_is_token(prev, CT_TYPEDEF))
      {
         start = prev;

         return(true);
      }
      match = { nullptr, nullptr, nullptr };
   }
   return(false);
} // match_function_pointer_typedef_at_identifier


std::tuple<chunk_t *,
           chunk_t *,
           chunk_t *> match_function_pointer_variable_at_identifier(chunk_t *pc_identifier)
{
   LOG_FUNC_ENTRY();

   std::tuple<chunk_t *, chunk_t *, chunk_t *> match(nullptr, nullptr, nullptr);

   match_function_pointer_variable_at_identifier(pc_identifier,
                                                 match);

   return(match);
} // match_function_pointer_variable_at_identifier


bool match_function_pointer_variable_at_identifier(chunk_t               *pc_identifier,
                                                   std::tuple<chunk_t *,
                                                              chunk_t *,
                                                              chunk_t *> &match)
{
   LOG_FUNC_ENTRY();

   chunk_t *next = pc_identifier;

   if (chunk_is_identifier(next))
   {
      /**
       * skip any scope resolution qualifiers
       */
      next = chunk_skip_dc_member(next, scope_e::PREPROC);

      /**
       * get the next chunk - it must be a closing paren if we're hoping to match a function pointer
       */
      next = chunk_get_next_ncnnl(next, scope_e::PREPROC);

      if (chunk_is_paren_close_token(next))
      {
         /**
          * get the next chunk - it must be an opening paren if we're hoping to match a function pointer
          */
         next = chunk_get_next_ncnnl(next, scope_e::PREPROC);

         if (chunk_is_paren_open_token(next))
         {
            return(match_function_pointer_at_paren(next, match));
         }
      }
   }
   return(false);
} // match_function_pointer_variable_at_identifier


std::pair<chunk_t *, chunk_t *> match_qualified_identifier(chunk_t *pc)
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


std::tuple<chunk_t *, chunk_t *, chunk_t *> match_variable(chunk_t *pc, std::size_t level)
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

   if (  identifier != nullptr
      && start != nullptr
      && (  end != nullptr
         || chunk_is_token(chunk_get_prev_ncnnlni(identifier), CT_WORD)))
   {
      return(std::make_tuple(start, identifier, end));
   }
   return(std::make_tuple(nullptr, nullptr, nullptr));
} // match_variable


std::pair<chunk_t *, chunk_t *> match_variable_end(chunk_t *pc, std::size_t level)
{
   LOG_FUNC_ENTRY();

   chunk_t *identifier = nullptr;

   while (pc != nullptr)
   {
      /**
       * skip any right-hand side assignments
       */
      chunk_t *rhs_exp_end = nullptr;

      if (chunk_is_assign_token(pc))
      {
         /**
          * store a pointer to the end chunk of the rhs expression;
          * use it later to test against setting the identifier
          */
         rhs_exp_end = skip_to_expression_end(pc);
         pc          = rhs_exp_end;
      }

      /**
       * skip current and subsequent chunks if at a higher level
       */
      while (  pc != nullptr
            && pc->level > level)
      {
         pc = chunk_get_next_ncnnl(pc);
      }

      /**
       * skip to any following match for angle brackets, braces, parens,
       * or square brackets
       */
      if (  chunk_is_angle_open_token(pc)
         || chunk_is_brace_open_token(pc)
         || chunk_is_paren_open_token(pc)
         || chunk_is_square_open_token(pc))
      {
         pc = chunk_skip_to_match(pc, scope_e::PREPROC);
      }
      /**
       * call a separate function to validate adjacent tokens as potentially
       * matching a variable declaration/definition
       */

      chunk_t *next = chunk_get_next_ncnnl(pc);

      if (  chunk_is_not_token(next, CT_COMMA)
         && chunk_is_not_token(next, CT_FPAREN_CLOSE)
         && !chunk_is_semicolon_token(next)
         && !adj_chunks_match_var_def_pattern(pc, next))
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
      if (  chunk_is_comma_token(next)
         || chunk_is_token(next, CT_FPAREN_CLOSE)
         || chunk_is_semicolon_token(next))
      {
         return(std::make_pair(identifier, pc));
      }
      pc = next;
   }
   return(std::make_pair(nullptr, nullptr));
} // match_variable_end


std::pair<chunk_t *, chunk_t *> match_variable_start(chunk_t *pc, std::size_t level)
{
   LOG_FUNC_ENTRY();

   chunk_t *identifier = nullptr;

   while (pc != nullptr)
   {
      /**
       * skip any right-hand side assignments
       */
      chunk_t *before_rhs_exp_start = skip_expression_rev(pc);
      chunk_t *prev                 = pc;
      chunk_t *next                 = nullptr;

      while (  chunk_is_after(prev, before_rhs_exp_start)
            && pc != prev)
      {
         next = prev;
         prev = chunk_get_prev_ncnnlni(next, scope_e::PREPROC);

         if (chunk_is_assign_token(next))
         {
            pc = prev;
         }
      }

      /**
       * skip current and preceding chunks if at a higher level
       */
      while (  pc != nullptr
            && pc->level > level)
      {
         pc = chunk_get_prev_ncnnlni(pc, scope_e::PREPROC);
      }

      /**
       * skip to any preceding match for angle brackets, braces, parens,
       * or square brackets
       */
      if (  chunk_is_angle_close_token(pc)
         || chunk_is_brace_close_token(pc)
         || chunk_is_paren_close_token(pc)
         || chunk_is_square_close_token(pc))
      {
         pc = chunk_skip_to_match_rev(pc, scope_e::PREPROC);
      }
      /**
       * call a separate function to validate adjacent tokens as potentially
       * matching a variable declaration/definition
       */

      prev = chunk_get_prev_ncnnlni(pc, scope_e::PREPROC);

      if (!adj_chunks_match_var_def_pattern(prev, pc))
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

      if (  identifier == nullptr
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
      if (  chunk_is_angle_close_token(prev)
         || chunk_is_brace_close_token(prev)
         || chunk_is_comma_token(prev)
         || chunk_is_token(prev, CT_TYPE)
         || chunk_is_token(prev, CT_WORD))
      {
         return(std::make_pair(pc, identifier));
      }
      pc = prev;
   }
   return(std::make_pair(nullptr, nullptr));
} // match_variable_start
