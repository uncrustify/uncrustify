/**
 * @file tokenize_cleanup.cpp
 * Looks at simple sequences to refine the chunk types.
 * Examples:
 *  - change '[' + ']' into '[]'/
 *  - detect "version = 10;" vs "version (xxx) {"
 *
 * @author  Ben Gardner
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */
#include "tokenize_cleanup.h"
#include "uncrustify_types.h"
#include "prototypes.h"
#include "chunk_list.h"
#include "char_table.h"
#include "unc_ctype.h"
#include "uncrustify.h"
#include "space.h"
#include "combine.h"
#include "keywords.h"
#include <cstring>


/**
 * If there is nothing but CT_WORD and CT_MEMBER, then it's probably a
 * template thingy.  Otherwise, it's likely a comparison.
 *
 * @param start  chunk to start check at
 */
static void check_template(chunk_t *start);


/**
 * Convert '>' + '>' into '>>'
 * If we only have a single '>', then change it to CT_COMPARE.
 *
 * @param pc  chunk to start at
 */
static chunk_t *handle_double_angle_close(chunk_t *pc);


/**
 * Marks ObjC specific chunks in propery declaration, by setting
 * parent types and chunk types.
 */
static void cleanup_objc_property(chunk_t *start);


/**
 * Marks ObjC specific chunks in propery declaration (getter/setter attribute)
 * Will mark 'test4Setter'and ':' in '@property (setter=test4Setter:, strong) int test4;' as CT_OC_SEL_NAME
 */
static void mark_selectors_in_property_with_open_paren(chunk_t *open_paren);


/**
 * Marks ObjC specific chunks in propery declaration ( attributes)
 * Changes all the CT_WORD and CT_TYPE to CT_OC_PROPERTY_ATTR
 */
static void mark_attributes_in_property_with_open_paren(chunk_t *open_paren);


static chunk_t *handle_double_angle_close(chunk_t *pc)
{
   chunk_t *next = chunk_get_next(pc);

   if (next)
   {
      if (  pc->type == CT_ANGLE_CLOSE
         && next->type == CT_ANGLE_CLOSE
         && pc->parent_type == CT_NONE
         && (pc->orig_col_end + 1) == next->orig_col
         && next->parent_type == CT_NONE)
      {
         pc->str.append('>');
         set_chunk_type(pc, CT_ARITH);
         pc->orig_col_end = next->orig_col_end;

         chunk_t *tmp = chunk_get_next_ncnl(next);
         chunk_del(next);
         next = tmp;
      }
      else
      {
         // bug #663
         set_chunk_type(pc, CT_COMPARE);
      }
   }
   return(next);
}


void split_off_angle_close(chunk_t *pc)
{
   const chunk_tag_t *ct = find_punctuator(pc->text() + 1, cpd.lang_flags);

   if (ct == nullptr)
   {
      return;
   }

   chunk_t nc = *pc;
   pc->str.resize(1);
   pc->orig_col_end = pc->orig_col + 1;
   set_chunk_type(pc, CT_ANGLE_CLOSE);

   nc.type = ct->type;
   nc.str.pop_front();
   nc.orig_col++;
   nc.column++;
   chunk_add_after(&nc, pc);
}


void tokenize_cleanup(void)
{
   LOG_FUNC_ENTRY();

   chunk_t *prev = nullptr;
   chunk_t *next;
   bool    in_type_cast = false;

   cpd.unc_stage = unc_stage_e::TOKENIZE_CLEANUP;

   /*
    * Since [] is expected to be TSQUARE for the 'operator', we need to make
    * this change in the first pass.
    */
   chunk_t *pc;
   for (pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->type == CT_SQUARE_OPEN)
      {
         next = chunk_get_next_ncnl(pc);
         if (chunk_is_token(next, CT_SQUARE_CLOSE))
         {
            // Change '[' + ']' into '[]'
            set_chunk_type(pc, CT_TSQUARE);
            pc->str = "[]";
            /*
             * bug #664: The original orig_col_end of CT_SQUARE_CLOSE is
             * stored at orig_col_end of CT_TSQUARE.
             * pc->orig_col_end += 1;
             */
            pc->orig_col_end = next->orig_col_end;
            chunk_del(next);
         }
      }
      if (  pc->type == CT_SEMICOLON
         && (pc->flags & PCF_IN_PREPROC)
         && !chunk_get_next_ncnl(pc, scope_e::PREPROC))
      {
         LOG_FMT(LNOTE, "%s(%d): %s:%zu Detected a macro that ends with a semicolon. Possible failures if used.\n",
                 __func__, __LINE__, cpd.filename.c_str(), pc->orig_line);
      }
   }

   // We can handle everything else in the second pass
   pc   = chunk_get_head();
   next = chunk_get_next_ncnl(pc);
   while (pc != nullptr && next != nullptr)
   {
      if (pc->type == CT_DOT && (cpd.lang_flags & LANG_ALLC))
      {
         set_chunk_type(pc, CT_MEMBER);
      }

      if (pc->type == CT_NULLCOND && (cpd.lang_flags & LANG_CS))
      {
         set_chunk_type(pc, CT_MEMBER);
      }

      // Determine the version stuff (D only)
      if (pc->type == CT_D_VERSION)
      {
         if (next->type == CT_PAREN_OPEN)
         {
            set_chunk_type(pc, CT_D_VERSION_IF);
         }
         else
         {
            if (next->type != CT_ASSIGN)
            {
               LOG_FMT(LERR, "%s(%d): %s:%zu: version: Unexpected token %s\n",
                       __func__, __LINE__, cpd.filename.c_str(), pc->orig_line, get_token_name(next->type));
               cpd.error_count++;
            }
            set_chunk_type(pc, CT_WORD);
         }
      }

      // Determine the scope stuff (D only)
      if (pc->type == CT_D_SCOPE)
      {
         if (next->type == CT_PAREN_OPEN)
         {
            set_chunk_type(pc, CT_D_SCOPE_IF);
         }
         else
         {
            set_chunk_type(pc, CT_TYPE);
         }
      }

      /*
       * Change CT_BASE before CT_PAREN_OPEN to CT_WORD.
       * public myclass() : base() {}
       * -or-
       * var x = (T)base.y;
       */
      if (pc->type == CT_BASE && (next->type == CT_PAREN_OPEN || next->type == CT_DOT))
      {
         set_chunk_type(pc, CT_WORD);
      }

      if (pc->type == CT_ENUM && next->type == CT_CLASS)
      {
         set_chunk_type(next, CT_ENUM_CLASS);
      }

      /*
       * Change CT_WORD after CT_ENUM, CT_UNION, or CT_STRUCT to CT_TYPE
       * Change CT_WORD before CT_WORD to CT_TYPE
       */
      if (next->type == CT_WORD)
      {
         if (  pc->type == CT_ENUM
            || pc->type == CT_ENUM_CLASS
            || pc->type == CT_UNION
            || pc->type == CT_STRUCT)
         {
            set_chunk_type(next, CT_TYPE);
         }
         if (pc->type == CT_WORD)
         {
            set_chunk_type(pc, CT_TYPE);
         }
      }

      /*
       * change extern to qualifier if extern isn't followed by a string or
       * an open parenthesis
       */
      if (pc->type == CT_EXTERN)
      {
         if (next->type == CT_STRING)
         {
            // Probably 'extern "C"'
         }
         else if (next->type == CT_PAREN_OPEN)
         {
            // Probably 'extern (C)'
         }
         else
         {
            // Something else followed by a open brace
            chunk_t *tmp = chunk_get_next_ncnl(next);
            if (tmp == nullptr || tmp->type != CT_BRACE_OPEN)
            {
               set_chunk_type(pc, CT_QUALIFIER);
            }
         }
      }

      /*
       * Change CT_STAR to CT_PTR_TYPE if preceded by
       *     CT_TYPE, CT_QUALIFIER, or CT_PTR_TYPE
       * or by a
       *     CT_WORD which is preceded by CT_DC_MEMBER: '::aaa *b'
       */
      if (  (next->type == CT_STAR)
         || ((cpd.lang_flags & LANG_CPP) && (next->type == CT_CARET))
         || ((cpd.lang_flags & LANG_CS) && (next->type == CT_QUESTION) && (strcmp(pc->text(), "null") != 0)))
      {
         if (  pc->type == CT_TYPE
            || pc->type == CT_QUALIFIER
            || pc->type == CT_PTR_TYPE)
         {
            set_chunk_type(next, CT_PTR_TYPE);
         }
         else if (  pc->type == CT_WORD
                 && prev != nullptr
                 && prev->type == CT_DC_MEMBER
                 && (cpd.lang_flags & LANG_CPP) != 0)
         {
            set_chunk_type(pc, CT_TYPE);
            set_chunk_type(next, CT_PTR_TYPE);
         }
      }

      if (pc->type == CT_TYPE_CAST && next->type == CT_ANGLE_OPEN)
      {
         set_chunk_parent(next, CT_TYPE_CAST);
         in_type_cast = true;
      }

      // Change angle open/close to CT_COMPARE, if not a template thingy
      if (pc->type == CT_ANGLE_OPEN && pc->parent_type != CT_TYPE_CAST)
      {
         /*
          * pretty much all languages except C use <> for something other than
          * comparisons.  "#include<xxx>" is handled elsewhere.
          */
         if (cpd.lang_flags & (LANG_CPP | LANG_CS | LANG_JAVA | LANG_VALA | LANG_OC))
         {
            // bug #663
            check_template(pc);
         }
         else
         {
            // convert CT_ANGLE_OPEN to CT_COMPARE
            set_chunk_type(pc, CT_COMPARE);
         }
      }
      if (pc->type == CT_ANGLE_CLOSE && pc->parent_type != CT_TEMPLATE)
      {
         if (in_type_cast)
         {
            in_type_cast = false;
            set_chunk_parent(pc, CT_TYPE_CAST);
         }
         else
         {
            next = handle_double_angle_close(pc);
         }
      }

      if (cpd.lang_flags & LANG_D)
      {
         // Check for the D string concat symbol '~'
         if (  pc->type == CT_INV
            && (  prev->type == CT_STRING
               || prev->type == CT_WORD
               || next->type == CT_STRING))
         {
            set_chunk_type(pc, CT_CONCAT);
         }

         // Check for the D template symbol '!' (word + '!' + word or '(')
         if (  pc->type == CT_NOT
            && prev->type == CT_WORD
            && (  next->type == CT_PAREN_OPEN
               || next->type == CT_WORD
               || next->type == CT_TYPE))
         {
            set_chunk_type(pc, CT_D_TEMPLATE);
         }

         // handle "version(unittest) { }" vs "unittest { }"
         if (  prev
            && pc->type == CT_UNITTEST
            && prev->type == CT_PAREN_OPEN)
         {
            set_chunk_type(pc, CT_WORD);
         }

         // handle 'static if' and merge the tokens
         if (  prev
            && pc->type == CT_IF
            && chunk_is_str(prev, "static", 6))
         {
            // delete PREV and merge with IF
            pc->str.insert(0, ' ');
            pc->str.insert(0, prev->str);
            pc->orig_col  = prev->orig_col;
            pc->orig_line = prev->orig_line;
            chunk_t *to_be_deleted = prev;
            prev = chunk_get_prev_ncnl(prev);
            chunk_del(to_be_deleted);
         }
      }

      if (cpd.lang_flags & LANG_CPP)
      {
         // Change Word before '::' into a type
         if (pc->type == CT_WORD && next->type == CT_DC_MEMBER)
         {
            set_chunk_type(pc, CT_TYPE);
         }
      }

      // Change get/set to CT_WORD if not followed by a brace open
      if (pc->type == CT_GETSET && next->type != CT_BRACE_OPEN)
      {
         if (  next->type == CT_SEMICOLON
            && (  prev->type == CT_BRACE_CLOSE
               || prev->type == CT_BRACE_OPEN
               || prev->type == CT_SEMICOLON))
         {
            set_chunk_type(pc, CT_GETSET_EMPTY);
            set_chunk_parent(next, CT_GETSET);
         }
         else
         {
            set_chunk_type(pc, CT_WORD);
         }
      }

      /*
       * Interface is only a keyword in MS land if followed by 'class' or 'struct'
       * likewise, 'class' may be a member name in Java.
       */
      if (  pc->type == CT_CLASS
         && !CharTable::IsKw1(next->str[0])
         && pc->next->type != CT_DC_MEMBER)
      {
         set_chunk_type(pc, CT_WORD);
      }

      /*
       * Change item after operator (>=, ==, etc) to a CT_OPERATOR_VAL
       * Usually the next item is part of the operator.
       * In a few cases the next few tokens are part of it:
       *  operator +       - common case
       *  operator >>      - need to combine '>' and '>'
       *  operator ()
       *  operator []      - already converted to TSQUARE
       *  operator new []
       *  operator delete []
       *  operator const char *
       *  operator const B&
       *  operator std::allocator<U>
       *
       * In all cases except the last, this will put the entire operator value
       * in one chunk.
       */
      if (pc->type == CT_OPERATOR)
      {
         chunk_t *tmp2 = chunk_get_next(next);
         // Handle special case of () operator -- [] already handled
         if (next->type == CT_PAREN_OPEN)
         {
            chunk_t *tmp = chunk_get_next(next);
            if (tmp != nullptr && tmp->type == CT_PAREN_CLOSE)
            {
               next->str = "()";
               set_chunk_type(next, CT_OPERATOR_VAL);
               chunk_del(tmp);
               next->orig_col_end += 1;
            }
         }
         else if (  next->type == CT_ANGLE_CLOSE
                 && tmp2
                 && tmp2->type == CT_ANGLE_CLOSE
                 && tmp2->orig_col == next->orig_col_end)
         {
            next->str.append('>');
            next->orig_col_end++;
            set_chunk_type(next, CT_OPERATOR_VAL);
            chunk_del(tmp2);
         }
         else if (next->flags & PCF_PUNCTUATOR)
         {
            set_chunk_type(next, CT_OPERATOR_VAL);
         }
         else
         {
            set_chunk_type(next, CT_TYPE);

            /*
             * Replace next with a collection of all tokens that are part of
             * the type.
             */
            tmp2 = next;
            chunk_t *tmp;
            while ((tmp = chunk_get_next(tmp2)) != nullptr)
            {
               if (  tmp->type != CT_WORD
                  && tmp->type != CT_TYPE
                  && tmp->type != CT_QUALIFIER
                  && tmp->type != CT_STAR
                  && tmp->type != CT_CARET
                  && tmp->type != CT_AMP
                  && tmp->type != CT_TSQUARE)
               {
                  break;
               }
               // Change tmp into a type so that space_needed() works right
               make_type(tmp);
               size_t num_sp = space_needed(tmp2, tmp);
               while (num_sp-- > 0)
               {
                  next->str.append(" ");
               }
               next->str.append(tmp->str);
               tmp2 = tmp;
            }

            while ((tmp2 = chunk_get_next(next)) != tmp)
            {
               chunk_del(tmp2);
            }

            set_chunk_type(next, CT_OPERATOR_VAL);

            next->orig_col_end = next->orig_col + next->len();
         }
         set_chunk_parent(next, CT_OPERATOR);

         LOG_FMT(LOPERATOR, "%s(%d): %zu:%zu operator '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, next->text());
      }

      // Change private, public, protected into either a qualifier or label
      if (pc->type == CT_PRIVATE)
      {
         // Handle Qt slots - maybe should just check for a CT_WORD?
         if (chunk_is_str(next, "slots", 5) || chunk_is_str(next, "Q_SLOTS", 7))
         {
            chunk_t *tmp = chunk_get_next(next);
            if (tmp != nullptr && tmp->type == CT_COLON)
            {
               next = tmp;
            }
         }
         if (next->type == CT_COLON)
         {
            set_chunk_type(next, CT_PRIVATE_COLON);
            chunk_t *tmp;
            if ((tmp = chunk_get_next_ncnl(next)) != nullptr)
            {
               chunk_flags_set(tmp, PCF_STMT_START | PCF_EXPR_START);
            }
         }
         else
         {
            set_chunk_type(pc, (  chunk_is_str(pc, "signals", 7)
                               || chunk_is_str(pc, "Q_SIGNALS", 9))
                           ? CT_WORD : CT_QUALIFIER);
         }
      }

      // Look for <newline> 'EXEC' 'SQL'
      if (  (chunk_is_str_case(pc, "EXEC", 4) && chunk_is_str_case(next, "SQL", 3))
         || (  (*pc->str.c_str() == '$') && pc->type != CT_SQL_WORD
               /* but avoid breaking tokenization for C# 6 interpolated strings. */
            && (  (cpd.lang_flags & LANG_CS) == 0
               || ((pc->type == CT_STRING) && (!pc->str.startswith("$\"")) && (!pc->str.startswith("$@\""))))))
      {
         chunk_t *tmp = chunk_get_prev(pc);
         if (chunk_is_newline(tmp))
         {
            if (*pc->str.c_str() == '$')
            {
               set_chunk_type(pc, CT_SQL_EXEC);
               if (pc->len() > 1)
               {
                  // SPLIT OFF '$'
                  chunk_t nc;

                  nc = *pc;
                  pc->str.resize(1);
                  pc->orig_col_end = pc->orig_col + 1;

                  nc.type = CT_SQL_WORD;
                  nc.str.pop_front();
                  nc.orig_col++;
                  nc.column++;
                  chunk_add_after(&nc, pc);

                  next = chunk_get_next(pc);
               }
            }
            tmp = chunk_get_next(next);
            if (chunk_is_str_case(tmp, "BEGIN", 5))
            {
               set_chunk_type(pc, CT_SQL_BEGIN);
            }
            else if (chunk_is_str_case(tmp, "END", 3))
            {
               set_chunk_type(pc, CT_SQL_END);
            }
            else
            {
               set_chunk_type(pc, CT_SQL_EXEC);
            }

            // Change words into CT_SQL_WORD until CT_SEMICOLON
            while (tmp != nullptr)
            {
               if (tmp->type == CT_SEMICOLON)
               {
                  break;
               }
               if (  (tmp->len() > 0)
                  && (  unc_isalpha(*tmp->str.c_str())
                     || (*tmp->str.c_str() == '$')))
               {
                  set_chunk_type(tmp, CT_SQL_WORD);
               }
               tmp = chunk_get_next_ncnl(tmp);
            }
         }
      }

      // handle MS abomination 'for each'
      if (  pc->type == CT_FOR
         && chunk_is_str(next, "each", 4)
         && (next == chunk_get_next(pc)))
      {
         // merge the two with a space between
         pc->str.append(' ');
         pc->str         += next->str;
         pc->orig_col_end = next->orig_col_end;
         chunk_del(next);
         next = chunk_get_next_ncnl(pc);
         // label the 'in'
         if (next && next->type == CT_PAREN_OPEN)
         {
            chunk_t *tmp = chunk_get_next_ncnl(next);
            while (tmp && tmp->type != CT_PAREN_CLOSE)
            {
               if (chunk_is_str(tmp, "in", 2))
               {
                  set_chunk_type(tmp, CT_IN);
                  break;
               }
               tmp = chunk_get_next_ncnl(tmp);
            }
         }
      }

      /*
       * ObjectiveC allows keywords to be used as identifiers in some situations
       * This is a dirty hack to allow some of the more common situations.
       */
      if (cpd.lang_flags & LANG_OC)
      {
         if (  (  pc->type == CT_IF
               || pc->type == CT_FOR
               || pc->type == CT_WHILE)
            && !chunk_is_token(next, CT_PAREN_OPEN))
         {
            set_chunk_type(pc, CT_WORD);
         }
         if (  pc->type == CT_DO
            && (  chunk_is_token(prev, CT_MINUS)
               || chunk_is_token(next, CT_SQUARE_CLOSE)))
         {
            set_chunk_type(pc, CT_WORD);
         }

         // Fix self keyword back to word when mixing c++/objective-c
         if (pc->type == CT_THIS && !strcmp(pc->text(), "self") && (next->type == CT_COMMA || next->type == CT_PAREN_CLOSE))
         {
            set_chunk_type(pc, CT_WORD);
         }
      }

      // Another hack to clean up more keyword abuse
      if (  pc->type == CT_CLASS
         && (chunk_is_token(prev, CT_DOT) || chunk_is_token(next, CT_DOT)))
      {
         set_chunk_type(pc, CT_WORD);
      }

      // Detect Objective C class name
      if (  pc->type == CT_OC_IMPL
         || pc->type == CT_OC_INTF
         || pc->type == CT_OC_PROTOCOL)
      {
         if (next->type != CT_PAREN_OPEN)
         {
            set_chunk_type(next, CT_OC_CLASS);
         }
         set_chunk_parent(next, pc->type);

         chunk_t *tmp = chunk_get_next_ncnl(next);
         if (tmp != nullptr)
         {
            chunk_flags_set(tmp, PCF_STMT_START | PCF_EXPR_START);
         }

         tmp = chunk_get_next_type(pc, CT_OC_END, pc->level);
         if (tmp != nullptr)
         {
            set_chunk_parent(tmp, pc->type);
         }
      }

      if (pc->type == CT_OC_INTF)
      {
         chunk_t *tmp = chunk_get_next_ncnl(pc, scope_e::PREPROC);
         while (tmp != nullptr && tmp->type != CT_OC_END)
         {
            if (get_token_pattern_class(tmp->type) != pattern_class_e::NONE)
            {
               LOG_FMT(LOBJCWORD, "%s(%d): @interface %zu:%zu change '%s' (%s) to CT_WORD\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, tmp->text(),
                       get_token_name(tmp->type));
               set_chunk_type(tmp, CT_WORD);
            }
            tmp = chunk_get_next_ncnl(tmp, scope_e::PREPROC);
         }
      }

      /*
       * Detect Objective-C categories and class extensions:
       *   @interface ClassName (CategoryName)
       *   @implementation ClassName (CategoryName)
       *   @interface ClassName ()
       *   @implementation ClassName ()
       */
      if (  (  pc->parent_type == CT_OC_IMPL
            || pc->parent_type == CT_OC_INTF
            || pc->type == CT_OC_CLASS)
         && next->type == CT_PAREN_OPEN)
      {
         set_chunk_parent(next, pc->parent_type);

         chunk_t *tmp = chunk_get_next(next);
         if (tmp != nullptr && tmp->next != nullptr)
         {
            if (tmp->type == CT_PAREN_CLOSE)
            {
               //set_chunk_type(tmp, CT_OC_CLASS_EXT);
               set_chunk_parent(tmp, pc->parent_type);
            }
            else
            {
               set_chunk_type(tmp, CT_OC_CATEGORY);
               set_chunk_parent(tmp, pc->parent_type);
            }
         }

         tmp = chunk_get_next_type(pc, CT_PAREN_CLOSE, pc->level);
         if (tmp != nullptr)
         {
            set_chunk_parent(tmp, pc->parent_type);
         }
      }

      /*
       * Detect Objective C @property:
       *   @property NSString *stringProperty;
       *   @property(nonatomic, retain) NSMutableDictionary *shareWith;
       */
      if (pc->type == CT_OC_PROPERTY)
      {
         if (next->type != CT_PAREN_OPEN)
         {
            chunk_flags_set(next, PCF_STMT_START | PCF_EXPR_START);
         }
         else
         {
            cleanup_objc_property(pc);
         }
      }

      /*
       * Detect Objective C @selector:
       *   @selector(msgNameWithNoArg)
       *   @selector(msgNameWith1Arg:)
       *   @selector(msgNameWith2Args:arg2Name:)
       */
      if (pc->type == CT_OC_SEL && next->type == CT_PAREN_OPEN)
      {
         set_chunk_parent(next, pc->type);

         chunk_t *tmp = chunk_get_next(next);
         if (tmp != nullptr)
         {
            set_chunk_type(tmp, CT_OC_SEL_NAME);
            set_chunk_parent(tmp, pc->type);

            while ((tmp = chunk_get_next_ncnl(tmp)) != nullptr)
            {
               if (tmp->type == CT_PAREN_CLOSE)
               {
                  set_chunk_parent(tmp, CT_OC_SEL);
                  break;
               }
               set_chunk_type(tmp, CT_OC_SEL_NAME);
               set_chunk_parent(tmp, pc->type);
            }
         }
      }

      // Handle special preprocessor junk
      if (pc->type == CT_PREPROC)
      {
         set_chunk_parent(pc, next->type);
      }

      // Detect "pragma region" and "pragma endregion"
      if (pc->type == CT_PP_PRAGMA && next->type == CT_PREPROC_BODY)
      {
         if (  (strncmp(next->str.c_str(), "region", 6) == 0)
            || (strncmp(next->str.c_str(), "endregion", 9) == 0))
         // TODO: probably better use strncmp
         {
            set_chunk_type(pc, (*next->str.c_str() == 'r') ? CT_PP_REGION : CT_PP_ENDREGION);

            set_chunk_parent(prev, pc->type);
         }
      }


      // Change 'default(' into a sizeof-like statement
      if (  (cpd.lang_flags & LANG_CS)
         && pc->type == CT_DEFAULT
         && next->type == CT_PAREN_OPEN)
      {
         set_chunk_type(pc, CT_SIZEOF);
      }

      if (pc->type == CT_UNSAFE && next->type != CT_BRACE_OPEN)
      {
         set_chunk_type(pc, CT_QUALIFIER);
      }

      if (  (  pc->type == CT_USING
            || (pc->type == CT_TRY && (cpd.lang_flags & LANG_JAVA)))
         && next->type == CT_PAREN_OPEN)
      {
         set_chunk_type(pc, CT_USING_STMT);
      }

      // Add minimal support for C++0x rvalue references
      if (pc->type == CT_BOOL && (cpd.lang_flags & LANG_CPP) && chunk_is_str(pc, "&&", 2))
      {
         if (prev->type == CT_TYPE)
         {
            // Issue # 1002
            if ((pc->flags & PCF_IN_TEMPLATE) == 0)
            {
               set_chunk_type(pc, CT_BYREF);
            }
         }
      }

      /*
       * HACK: treat try followed by a colon as a qualifier to handle this:
       *   A::A(int) try : B() { } catch (...) { }
       */
      if (  pc->type == CT_TRY
         && chunk_is_str(pc, "try", 3)
         && next != nullptr
         && next->type == CT_COLON)
      {
         set_chunk_type(pc, CT_QUALIFIER);
      }

      /*
       * If Java's 'synchronized' is in a method declaration, it should be
       * a qualifier.
       */
      if (  (cpd.lang_flags & LANG_JAVA)
         && pc->type == CT_SYNCHRONIZED
         && next->type != CT_PAREN_OPEN)
      {
         set_chunk_type(pc, CT_QUALIFIER);
      }

      // change CT_DC_MEMBER + CT_FOR into CT_DC_MEMBER + CT_FUNC_CALL
      if (  pc->type == CT_FOR
         && (pc->prev != nullptr && pc->prev->type == CT_DC_MEMBER))
      {
         set_chunk_type(pc, CT_FUNC_CALL);
      }
      // TODO: determine other stuff here

      prev = pc;
      pc   = next;
      next = chunk_get_next_ncnl(pc);
   }
} // tokenize_cleanup


static void check_template(chunk_t *start)
{
   LOG_FMT(LTEMPL, "%s(%d): orig_line %zu, orig_col %zu:",
           __func__, __LINE__, start->orig_line, start->orig_col);
#ifdef DEBUG
   LOG_FMT(LTEMPL, "\n");
#endif // DEBUG

   chunk_t *prev = chunk_get_prev_ncnl(start, scope_e::PREPROC);
   if (prev == nullptr)
   {
      return;
   }

   chunk_t *end;
   chunk_t *pc;
   if (prev->type == CT_TEMPLATE)
   {
#ifdef DEBUG
      LOG_FMT(LTEMPL, "%s(%d):", __func__, __LINE__);
#endif
      LOG_FMT(LTEMPL, " CT_TEMPLATE:");
#ifdef DEBUG
      LOG_FMT(LTEMPL, "\n");
#endif

      // We have: "template< ... >", which is a template declaration
      size_t level = 1;
      for (pc = chunk_get_next_ncnl(start, scope_e::PREPROC);
           pc != nullptr;
           pc = chunk_get_next_ncnl(pc, scope_e::PREPROC))
      {
         LOG_FMT(LTEMPL, "%s(%d): [%s,%zu]",
                 __func__, __LINE__, get_token_name(pc->type), level);
#ifdef DEBUG
         LOG_FMT(LTEMPL, "\n");
#endif

         if ((pc->str[0] == '>') && (pc->len() > 1))
         {
            LOG_FMT(LTEMPL, "%s(%d): {split '%s' at orig_line %zu, orig_col %zu}",
                    __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
#ifdef DEBUG
            LOG_FMT(LTEMPL, "\n");
#endif
            split_off_angle_close(pc);
         }

         if (chunk_is_str(pc, "<", 1))
         {
            level++;
         }
         else if (chunk_is_str(pc, ">", 1))
         {
            level--;
            if (level == 0)
            {
               break;
            }
         }
      }
      end = pc;
   }
   else
   {
      /*
       * We may have something like "a< ... >", which is a template where
       * '...' may consist of anything except braces {}, a semicolon, and
       * unbalanced parens.
       * if we are inside an 'if' statement and hit a CT_BOOL, then it isn't a
       * template.
       */

      // A template requires a word/type right before the open angle
      if (  prev->type != CT_WORD
         && prev->type != CT_TYPE
         && prev->type != CT_COMMA
         && prev->type != CT_QUALIFIER
         && prev->type != CT_OPERATOR_VAL
         && prev->parent_type != CT_OPERATOR)
      {
         LOG_FMT(LTEMPL, "%s(%d): - after %s + ( - Not a template\n",
                 __func__, __LINE__, get_token_name(prev->type));
#ifdef DEBUG
         LOG_FMT(LTEMPL, "\n");
#endif
         set_chunk_type(start, CT_COMPARE);
         return;
      }

      LOG_FMT(LTEMPL, "%s(%d): - prev %s -",
              __func__, __LINE__, get_token_name(prev->type));
#ifdef DEBUG
      LOG_FMT(LTEMPL, "\n");
#endif

      // Scan back and make sure we aren't inside square parenthesis
      bool in_if         = false;
      bool hit_semicolon = false;
      pc = start;
      while ((pc = chunk_get_prev_ncnl(pc, scope_e::PREPROC)) != nullptr)
      {
         if (  (pc->type == CT_SEMICOLON && hit_semicolon == true)
            || pc->type == CT_BRACE_OPEN
            || pc->type == CT_BRACE_CLOSE
            || pc->type == CT_SQUARE_CLOSE)
         {
            break;
         }
         if (pc->type == CT_SEMICOLON && hit_semicolon == false)
         {
            hit_semicolon = true;
         }
         if (  ((  pc->type == CT_IF
                || pc->type == CT_RETURN
                || pc->type == CT_WHILE
                || pc->type == CT_WHILE_OF_DO) && hit_semicolon == false)
            || (pc->type == CT_FOR && hit_semicolon == true))
         {
            in_if = true;
            break;
         }
      }

      /*
       * Scan forward to the angle close
       * If we have a comparison in there, then it can't be a template.
       */
#define MAX_NUMBER_OF_TOKEN    1024
      c_token_t tokens[MAX_NUMBER_OF_TOKEN];
      size_t    num_tokens = 1;

      tokens[0] = CT_ANGLE_OPEN;
      for (pc = chunk_get_next_ncnl(start, scope_e::PREPROC);
           pc != nullptr;
           pc = chunk_get_next_ncnl(pc, scope_e::PREPROC))
      {
         LOG_FMT(LTEMPL, "%s(%d): [%s,%zu]",
                 __func__, __LINE__, get_token_name(pc->type), num_tokens);
#ifdef DEBUG
         LOG_FMT(LTEMPL, "\n");
#endif

         if (  (tokens[num_tokens - 1] == CT_ANGLE_OPEN)
            && (pc->str[0] == '>')
            && (pc->len() > 1)
            && (  cpd.settings[UO_tok_split_gte].b
               || ((chunk_is_str(pc, ">>", 2) || chunk_is_str(pc, ">>>", 3)) && num_tokens >= 2)))
         {
            LOG_FMT(LTEMPL, "%s(%d): {split '%s' at orig_line %zu, orig_col %zu}",
                    __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
#ifdef DEBUG
            LOG_FMT(LTEMPL, "\n");
#endif
            split_off_angle_close(pc);
         }

         if (chunk_is_str(pc, "<", 1))
         {
            tokens[num_tokens] = CT_ANGLE_OPEN;
            num_tokens++;
         }
         else if (chunk_is_str(pc, ">", 1))
         {
            if (num_tokens > 0 && (tokens[num_tokens - 1] == CT_PAREN_OPEN))
            {
               handle_double_angle_close(pc);
            }
            else if (--num_tokens <= 0)
            {
               break;
            }
            else if (tokens[num_tokens] != CT_ANGLE_OPEN)
            {
               break; // unbalanced parentheses
            }
         }
         else if (  in_if
                 && (pc->type == CT_BOOL || pc->type == CT_COMPARE))
         {
            break;
         }
         else if (  pc->type == CT_BRACE_OPEN
                 || pc->type == CT_BRACE_CLOSE
                 || pc->type == CT_SEMICOLON)
         {
            break;
         }
         else if (pc->type == CT_PAREN_OPEN)
         {
            if (num_tokens >= MAX_NUMBER_OF_TOKEN - 1)
            {
               break;
            }
            tokens[num_tokens] = CT_PAREN_OPEN;
            num_tokens++;
         }
         else if (pc->type == CT_PAREN_CLOSE)
         {
            num_tokens--;
            if (tokens[num_tokens] != CT_PAREN_OPEN)
            {
               break;  // unbalanced parentheses
            }
         }
      }
      end = pc;
   }

   if (end != nullptr && end->type == CT_ANGLE_CLOSE)
   {
      pc = chunk_get_next_ncnl(end, scope_e::PREPROC);
      if (pc == nullptr || pc->type != CT_NUMBER)
      {
#ifdef DEBUG
         LOG_FMT(LTEMPL, "%s(%d):", __func__, __LINE__);
#endif
         LOG_FMT(LTEMPL, " - Template Detected\n");
#ifdef DEBUG
         LOG_FMT(LTEMPL, "%s(%d):", __func__, __LINE__);
#endif
         LOG_FMT(LTEMPL, "     from orig_line %zu, orig_col %zu\n",
                 start->orig_line, start->orig_col);
#ifdef DEBUG
         LOG_FMT(LTEMPL, "%s(%d):", __func__, __LINE__);
#endif
         LOG_FMT(LTEMPL, "     to   orig_line %zu, orig_col %zu\n",
                 end->orig_line, end->orig_col);

         set_chunk_parent(start, CT_TEMPLATE);

         // Issue #1127
         // MyFoo<mySize * 2> foo1;
         // MyFoo<2*mySize * 2> foo1;
         // Issue #1346
         // use it as ONE line:
         //   typename std::enable_if<!std::is_void<T>::value,
         //   QVector<T> >::type dummy(const std::function<T*(const S&)>&
         //   pFunc, const QVector<S>& pItems)
         // we nees two runs
         // 1. run to test if expression is numeric
         bool expressionIsNumeric = false;
         pc = start;
         while (pc != end)
         {
            chunk_t *next = chunk_get_next_ncnl(pc, scope_e::PREPROC);
            // a test "if (next == nullptr)" is not necessary
            chunk_flags_set(pc, PCF_IN_TEMPLATE);
            if (next->type != CT_PAREN_OPEN)
            {
               if (  pc->type == CT_NUMBER
                  || (pc->type == CT_ARITH && pc->type != CT_STAR))
               {
                  expressionIsNumeric = true;
                  break;
               }
            }
            pc = next;
         }
         LOG_FMT(LTEMPL, "%s(%d): expressionIsNumeric is %s\n",
                 __func__, __LINE__, expressionIsNumeric ? "FALSE" : "TRUE");
         // 2. run to do the work
         if (!expressionIsNumeric)
         {
            pc = start;
            while (pc != end)
            {
               chunk_t *next = chunk_get_next_ncnl(pc, scope_e::PREPROC);
               // a test "if (next == nullptr)" is not necessary
               chunk_flags_set(pc, PCF_IN_TEMPLATE);
               if (next->type != CT_PAREN_OPEN)
               {
                  make_type(pc);
               }
               pc = next;
            }
         }
         set_chunk_parent(end, CT_TEMPLATE);
         chunk_flags_set(end, PCF_IN_TEMPLATE);
         return;
      }
   }

   LOG_FMT(LTEMPL, "%s(%d): - Not a template: end = %s\n",
           __func__, __LINE__, (end != NULL) ? get_token_name(end->type) : "<null>");
   set_chunk_type(start, CT_COMPARE);
} // check_template


static void cleanup_objc_property(chunk_t *start)
{
   assert(start && start->type == CT_OC_PROPERTY);

   chunk_t *open_paren = chunk_get_next_type(start, CT_PAREN_OPEN, start->level);

   if (!open_paren)
   {
      LOG_FMT(LTEMPL, "%s(%d): Property is not followed by openning paren\n", __func__, __LINE__);
      return;
   }

   set_chunk_parent(open_paren, start->type);

   chunk_t *tmp = chunk_get_next_type(start, CT_PAREN_CLOSE, start->level);
   if (tmp != NULL)
   {
      set_chunk_parent(tmp, start->type);
      tmp = chunk_get_next_ncnl(tmp);
      if (tmp != NULL)
      {
         chunk_flags_set(tmp, PCF_STMT_START | PCF_EXPR_START);

         tmp = chunk_get_next_type(tmp, CT_SEMICOLON, start->level);
         if (tmp != NULL)
         {
            set_chunk_parent(tmp, start->type);
         }
      }
   }
   mark_selectors_in_property_with_open_paren(open_paren);
   mark_attributes_in_property_with_open_paren(open_paren);
}


static void mark_selectors_in_property_with_open_paren(chunk_t *open_paren)
{
   assert(open_paren && open_paren->type == CT_PAREN_OPEN);

   chunk_t *tmp = open_paren;

   while (tmp && tmp->type != CT_PAREN_CLOSE)
   {
      if (  tmp->type == CT_WORD
         && (chunk_is_str(tmp, "setter", 6) || chunk_is_str(tmp, "getter", 6)))
      {
         tmp = tmp->next;
         while (  tmp
               && tmp->type != CT_COMMA
               && tmp->type != CT_PAREN_CLOSE)
         {
            if (tmp->type == CT_WORD || chunk_is_str(tmp, ":", 1))
            {
               tmp->type = CT_OC_SEL_NAME;
            }
            tmp = tmp->next;
         }
      }
      else
      {
         tmp = tmp->next;
      }
   }
}


static void mark_attributes_in_property_with_open_paren(chunk_t *open_paren)
{
   assert(open_paren && open_paren->type == CT_PAREN_OPEN);

   chunk_t *tmp = open_paren;

   while (tmp && tmp->type != CT_PAREN_CLOSE)
   {
      if (  (tmp->type == CT_COMMA || tmp->type == CT_PAREN_OPEN)
         && tmp->next
         && (tmp->next->type == CT_WORD || tmp->next->type == CT_TYPE))
      {
         tmp->next->type = CT_OC_PROPERTY_ATTR;
      }
      tmp = tmp->next;
   }
}

