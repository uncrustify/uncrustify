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

#include "char_table.h"
#include "chunk_list.h"
#include "combine.h"
#include "combine_skip.h"
#include "error_types.h"
#include "flag_braced_init_list.h"
#include "flag_decltype.h"
#include "keywords.h"
#include "language_tools.h"
#include "log_rules.h"
#include "prototypes.h"
#include "punctuators.h"
#include "space.h"
#include "unc_ctype.h"
#include "uncrustify.h"
#include "uncrustify_types.h"

#include <vector>

#include <cstring>

using namespace uncrustify;


/**
 * Mark types in a single template argument.
 *
 * @param start  chunk to start check at
 * @param end    chunk to end check at
 */
static void check_template_arg(chunk_t *start, chunk_t *end);


/**
 * Mark types in template argument(s).
 *
 * @param start  chunk to start check at
 * @param end    chunk to end check at
 */
static void check_template_args(chunk_t *start, chunk_t *end);


/**
 * If there is nothing but CT_WORD and CT_MEMBER, then it's probably a
 * template thingy.  Otherwise, it's likely a comparison.
 *
 * @param start  chunk to start check at
 */
static void check_template(chunk_t *start, bool in_type_cast);


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
      if (  chunk_is_token(pc, CT_ANGLE_CLOSE)
         && chunk_is_token(next, CT_ANGLE_CLOSE)
         && get_chunk_parent_type(pc) == CT_NONE
         && (pc->orig_col_end + 1) == next->orig_col
         && get_chunk_parent_type(next) == CT_NONE)
      {
         pc->str.append('>');
         set_chunk_type(pc, CT_SHIFT);
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

   set_chunk_type(&nc, ct->type);
   nc.str.pop_front();
   nc.orig_col++;
   nc.column++;
   chunk_add_after(&nc, pc);
}


void tokenize_trailing_return_types(void)
{
   // Issue #2330
   // auto max(int a, int b) -> int;
   // Issue #2460
   // auto f01() -> bool;
   // auto f02() noexcept -> bool;
   // auto f03() noexcept(true) -> bool;
   // auto f04() noexcept(false) -> bool;
   // auto f05() noexcept -> bool = delete;
   // auto f06() noexcept(true) -> bool = delete;
   // auto f07() noexcept(false) -> bool = delete;
   // auto f11() const -> bool;
   // auto f12() const noexcept -> bool;
   // auto f13() const noexcept(true) -> bool;
   // auto f14() const noexcept(false) -> bool;
   // auto f15() const noexcept -> bool = delete;
   // auto f16() const noexcept(true) -> bool = delete;
   // auto f17() const noexcept(false) -> bool = delete;
   // auto f21() throw() -> bool;
   // auto f22() throw() -> bool = delete;
   // auto f23() const throw() -> bool;
   // auto f24() const throw() -> bool = delete;
   chunk_t *pc;

   for (pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      LOG_FMT(LNOTE, "%s(%d): orig_line is %zu, orig_col is %zu, text() is '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text());

      if (  chunk_is_token(pc, CT_MEMBER)
         && (strcmp(pc->text(), "->") == 0))
      {
         chunk_t *tmp = chunk_get_prev_ncnl(pc);
         chunk_t *tmp_2;
         chunk_t *open_paren;

         if (chunk_is_token(tmp, CT_QUALIFIER))
         {
            // auto max(int a, int b) const -> int;
            // auto f11() const -> bool;
            tmp = chunk_get_prev_ncnl(tmp);
         }
         else if (chunk_is_token(tmp, CT_NOEXCEPT))
         {
            // noexcept is present
            tmp_2 = chunk_get_prev_ncnl(tmp);

            if (chunk_is_token(tmp_2, CT_QUALIFIER))
            {
               // auto f12() const noexcept -> bool;
               // auto f15() const noexcept -> bool = delete;
               tmp = chunk_get_prev_ncnl(tmp_2);
            }
            else
            {
               // auto f02() noexcept -> bool;
               // auto f05() noexcept -> bool = delete;
               tmp = tmp_2;
            }
         }
         else if (chunk_is_token(tmp, CT_PAREN_CLOSE))
         {
            open_paren = chunk_get_prev_type(tmp, CT_PAREN_OPEN, tmp->level);
            tmp        = chunk_get_prev_ncnl(open_paren);

            if (chunk_is_token(tmp, CT_NOEXCEPT))
            {
               // noexcept is present
               tmp_2 = chunk_get_prev_ncnl(tmp);

               if (chunk_is_token(tmp_2, CT_QUALIFIER))
               {
                  // auto f13() const noexcept(true) -> bool;
                  // auto f14() const noexcept(false) -> bool;
                  // auto f16() const noexcept(true) -> bool = delete;
                  // auto f17() const noexcept(false) -> bool = delete;
                  tmp = chunk_get_prev_ncnl(tmp_2);
               }
               else
               {
                  // auto f03() noexcept(true) -> bool;
                  // auto f04() noexcept(false) -> bool;
                  // auto f06() noexcept(true) -> bool = delete;
                  // auto f07() noexcept(false) -> bool = delete;
                  tmp = tmp_2;
               }
            }
            else if (chunk_is_token(tmp, CT_THROW))
            {
               // throw is present
               tmp_2 = chunk_get_prev_ncnl(tmp);

               if (chunk_is_token(tmp_2, CT_QUALIFIER))
               {
                  // auto f23() const throw() -> bool;
                  // auto f24() const throw() -> bool = delete;
                  tmp = chunk_get_prev_ncnl(tmp_2);
               }
               else
               {
                  // auto f21() throw() -> bool;
                  // auto f22() throw() -> bool = delete;
                  tmp = tmp_2;
               }
            }
            else
            {
               LOG_FMT(LNOTE, "%s(%d): NOT COVERED\n", __func__, __LINE__);
            }
         }
         else
         {
            LOG_FMT(LNOTE, "%s(%d): NOT COVERED\n", __func__, __LINE__);
         }

         if (  chunk_is_token(tmp, CT_FPAREN_CLOSE)
            && (get_chunk_parent_type(tmp) == CT_FUNC_PROTO || get_chunk_parent_type(tmp) == CT_FUNC_DEF))
         {
            set_chunk_type(pc, CT_TRAILING_RET);
            LOG_FMT(LNOTE, "%s(%d): set trailing return type for text() is '%s'\n",
                    __func__, __LINE__, tmp->text());
         }
      }
   }
} // tokenize_trailing_return_types


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
      if (chunk_is_token(pc, CT_SQUARE_OPEN))
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

      if (  chunk_is_token(pc, CT_SEMICOLON)
         && pc->flags.test(PCF_IN_PREPROC)
         && !chunk_get_next_ncnl(pc, scope_e::PREPROC))
      {
         LOG_FMT(LNOTE, "%s(%d): %s:%zu Detected a macro that ends with a semicolon. Possible failures if used.\n",
                 __func__, __LINE__, cpd.filename.c_str(), pc->orig_line);
      }
   }

   // change := to CT_SQL_ASSIGN Issue #527
   for (pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next_ncnl(pc))
   {
      if (chunk_is_token(pc, CT_COLON))
      {
         next = chunk_get_next_ncnl(pc);

         if (chunk_is_token(next, CT_ASSIGN))
         {
            // Change ':' + '=' into ':='
            set_chunk_type(pc, CT_SQL_ASSIGN);
            pc->str          = ":=";
            pc->orig_col_end = next->orig_col_end;
            chunk_del(next);
         }
      }
   }

   // We can handle everything else in the second pass
   pc   = chunk_get_head();
   next = chunk_get_next_ncnl(pc);

   while (pc != nullptr && next != nullptr)
   {
      if (chunk_is_token(pc, CT_DOT) && language_is_set(LANG_ALLC))
      {
         set_chunk_type(pc, CT_MEMBER);
      }

      if (chunk_is_token(pc, CT_NULLCOND) && language_is_set(LANG_CS))
      {
         set_chunk_type(pc, CT_MEMBER);
      }

      // Determine the version stuff (D only)
      if (chunk_is_token(pc, CT_D_VERSION))
      {
         if (chunk_is_token(next, CT_PAREN_OPEN))
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
      if (chunk_is_token(pc, CT_D_SCOPE))
      {
         if (chunk_is_token(next, CT_PAREN_OPEN))
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
      if (  chunk_is_token(pc, CT_BASE)
         && (chunk_is_token(next, CT_PAREN_OPEN) || chunk_is_token(next, CT_DOT)))
      {
         set_chunk_type(pc, CT_WORD);
      }

      if (  chunk_is_token(pc, CT_ENUM)
         && (chunk_is_token(next, CT_STRUCT) || chunk_is_token(next, CT_CLASS)))
      {
         set_chunk_type(next, CT_ENUM_CLASS);
      }
      chunk_t *next_non_attr = language_is_set(LANG_CPP) ? skip_attribute_next(next) : next;

      /*
       * Change CT_WORD after CT_ENUM, CT_UNION, CT_STRUCT, or CT_CLASS to CT_TYPE
       * Change CT_WORD before CT_WORD to CT_TYPE
       */
      if (chunk_is_token(next_non_attr, CT_WORD))
      {
         if (  chunk_is_token(pc, CT_ENUM)
            || chunk_is_token(pc, CT_ENUM_CLASS)
            || chunk_is_token(pc, CT_UNION)
            || chunk_is_token(pc, CT_STRUCT)
            || chunk_is_token(pc, CT_CLASS))
         {
            set_chunk_type(next_non_attr, CT_TYPE);
         }

         if (chunk_is_token(pc, CT_WORD))
         {
            set_chunk_type(pc, CT_TYPE);
         }
      }

      /*
       * change extern to qualifier if extern isn't followed by a string or
       * an open parenthesis
       */
      if (chunk_is_token(pc, CT_EXTERN))
      {
         if (chunk_is_token(next, CT_STRING))
         {
            // Probably 'extern "C"'
         }
         else if (chunk_is_token(next, CT_PAREN_OPEN))
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
      if (  (chunk_is_token(next, CT_STAR))
         || (language_is_set(LANG_CPP) && (chunk_is_token(next, CT_CARET)))
         || (language_is_set(LANG_CS) && (chunk_is_token(next, CT_QUESTION)) && (strcmp(pc->text(), "null") != 0)))
      {
         if (  chunk_is_token(pc, CT_TYPE)
            || chunk_is_token(pc, CT_QUALIFIER)
            || chunk_is_token(pc, CT_PTR_TYPE))
         {
            set_chunk_type(next, CT_PTR_TYPE);
         }
      }

      if (  chunk_is_token(pc, CT_TYPE_CAST)
         && chunk_is_token(next, CT_ANGLE_OPEN))
      {
         set_chunk_parent(next, CT_TYPE_CAST);
         in_type_cast = true;
      }

      if (chunk_is_token(pc, CT_DECLTYPE))
      {
         flag_cpp_decltype(pc);
      }

      // Change angle open/close to CT_COMPARE, if not a template thingy
      if (  chunk_is_token(pc, CT_ANGLE_OPEN)
         && pc->parent_type != CT_TYPE_CAST)
      {
         /*
          * pretty much all languages except C use <> for something other than
          * comparisons.  "#include<xxx>" is handled elsewhere.
          */
         if (language_is_set(LANG_OC | LANG_CPP | LANG_CS | LANG_JAVA | LANG_VALA))
         {
            // bug #663
            check_template(pc, in_type_cast);
         }
         else
         {
            // convert CT_ANGLE_OPEN to CT_COMPARE
            set_chunk_type(pc, CT_COMPARE);
         }
      }

      if (  chunk_is_token(pc, CT_ANGLE_CLOSE)
         && pc->parent_type != CT_TEMPLATE)
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

      if (language_is_set(LANG_D))
      {
         // Check for the D string concat symbol '~'
         if (  chunk_is_token(pc, CT_INV)
            && (  chunk_is_token(prev, CT_STRING)
               || chunk_is_token(prev, CT_WORD)
               || chunk_is_token(next, CT_STRING)))
         {
            set_chunk_type(pc, CT_CONCAT);
         }

         // Check for the D template symbol '!' (word + '!' + word or '(')
         if (  chunk_is_token(pc, CT_NOT)
            && chunk_is_token(prev, CT_WORD)
            && (  chunk_is_token(next, CT_PAREN_OPEN)
               || chunk_is_token(next, CT_WORD)
               || chunk_is_token(next, CT_TYPE)
               || chunk_is_token(next, CT_NUMBER)
               || chunk_is_token(next, CT_NUMBER_FP)
               || chunk_is_token(next, CT_STRING)
               || chunk_is_token(next, CT_STRING_MULTI)))
         {
            set_chunk_type(pc, CT_D_TEMPLATE);
         }

         // handle "version(unittest) { }" vs "unittest { }"
         if (  chunk_is_token(pc, CT_UNITTEST)
            && chunk_is_token(prev, CT_PAREN_OPEN))
         {
            set_chunk_type(pc, CT_WORD);
         }

         // handle 'static if' and merge the tokens
         if (  chunk_is_token(pc, CT_IF)
            && chunk_is_str(prev, "static", 6))
         {
            // delete PREV and merge with IF
            pc->str.insert(0, ' ');
            pc->str.insert(0, prev->str);
            pc->orig_col  = prev->orig_col;
            pc->orig_line = prev->orig_line;
            chunk_t *to_be_deleted = prev;
            prev = chunk_get_prev_ncnl(prev);

            if (prev != nullptr)
            {
               chunk_del(to_be_deleted);
            }
         }
      }

      if (language_is_set(LANG_CPP))
      {
         // Change Word before '::' into a type
         if (  chunk_is_token(pc, CT_WORD)
            && chunk_is_token(next, CT_DC_MEMBER))
         {
            set_chunk_type(pc, CT_TYPE);
         }

         // Set parent type for 'if constexpr'
         if (  chunk_is_token(prev, CT_IF)
            && chunk_is_token(pc, CT_QUALIFIER)
            && chunk_is_str(pc, "constexpr", 9))
         {
            set_chunk_type(pc, CT_CONSTEXPR);
         }
      }

      // Change get/set to CT_WORD if not followed by a brace open
      if (  chunk_is_token(pc, CT_GETSET)
         && next->type != CT_BRACE_OPEN)
      {
         if (  chunk_is_token(next, CT_SEMICOLON)
            && (  chunk_is_token(prev, CT_BRACE_CLOSE)
               || chunk_is_token(prev, CT_BRACE_OPEN)
               || chunk_is_token(prev, CT_SEMICOLON)))
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
      if (  chunk_is_token(pc, CT_CLASS)
         && !CharTable::IsKw1(next->str[0]))
      {
         if (  chunk_is_not_token(next, CT_DC_MEMBER)
            && chunk_is_not_token(next, CT_ATTRIBUTE))                       // Issue #2570
         {
            set_chunk_type(pc, CT_WORD);
         }
         else if (  chunk_is_token(prev, CT_DC_MEMBER)
                 || chunk_is_token(prev, CT_TYPE))
         {
            set_chunk_type(pc, CT_TYPE);
         }
         else if (chunk_is_token(next, CT_DC_MEMBER))
         {
            chunk_t *next2 = chunk_get_next_nblank(next);

            if (  chunk_is_token(next2, CT_INV)      // CT_INV hasn't turned into CT_DESTRUCTOR just yet
               || (  chunk_is_token(next2, CT_CLASS) // constructor isn't turned into CT_FUNC* just yet
                  && !strcmp(pc->text(), next2->text())))
            {
               set_chunk_type(pc, CT_TYPE);
            }
         }
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
      if (chunk_is_token(pc, CT_OPERATOR))
      {
         chunk_t *tmp2 = chunk_get_next(next);

         // Handle special case of () operator -- [] already handled
         if (chunk_is_token(next, CT_PAREN_OPEN))
         {
            chunk_t *tmp = chunk_get_next(next);

            if (chunk_is_token(tmp, CT_PAREN_CLOSE))
            {
               next->str = "()";
               set_chunk_type(next, CT_OPERATOR_VAL);
               chunk_del(tmp);
               next->orig_col_end += 1;
            }
         }
         else if (  chunk_is_token(next, CT_ANGLE_CLOSE)
                 && chunk_is_token(tmp2, CT_ANGLE_CLOSE)
                 && tmp2->orig_col == next->orig_col_end)
         {
            next->str.append('>');
            next->orig_col_end++;
            set_chunk_type(next, CT_OPERATOR_VAL);
            chunk_del(tmp2);
         }
         else if (next->flags.test(PCF_PUNCTUATOR))
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
      if (chunk_is_token(pc, CT_ACCESS))
      {
         // Handle Qt slots - maybe should just check for a CT_WORD?
         if (chunk_is_str(next, "slots", 5) || chunk_is_str(next, "Q_SLOTS", 7))
         {
            chunk_t *tmp = chunk_get_next(next);

            if (chunk_is_token(tmp, CT_COLON))
            {
               next = tmp;
            }
         }

         if (chunk_is_token(next, CT_COLON))
         {
            set_chunk_type(next, CT_ACCESS_COLON);
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
            && (  !language_is_set(LANG_CS)
               || ((chunk_is_token(pc, CT_STRING)) && (!pc->str.startswith("$\"")) && (!pc->str.startswith("$@\""))))))
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

                  set_chunk_type(&nc, CT_SQL_WORD);
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
               if (chunk_is_token(tmp, CT_SEMICOLON))
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
      if (  chunk_is_token(pc, CT_FOR)
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
         if (chunk_is_token(next, CT_PAREN_OPEN))
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
      if (language_is_set(LANG_OC))
      {
         if (  (  chunk_is_token(pc, CT_IF)
               || chunk_is_token(pc, CT_FOR)
               || chunk_is_token(pc, CT_WHILE))
            && !chunk_is_token(next, CT_PAREN_OPEN))
         {
            set_chunk_type(pc, CT_WORD);
         }

         if (  chunk_is_token(pc, CT_DO)
            && (  chunk_is_token(prev, CT_MINUS)
               || chunk_is_token(next, CT_SQUARE_CLOSE)))
         {
            set_chunk_type(pc, CT_WORD);
         }

         // Fix self keyword back to word when mixing c++/objective-c
         if (  chunk_is_token(pc, CT_THIS) && !strcmp(pc->text(), "self")
            && (chunk_is_token(next, CT_COMMA) || chunk_is_token(next, CT_PAREN_CLOSE)))
         {
            set_chunk_type(pc, CT_WORD);
         }

         // Fix self keyword back to word when mixing c++/objective-c
         if (  chunk_is_token(pc, CT_THIS)
            && !strcmp(pc->text(), "self")
            && (chunk_is_token(next, CT_COMMA) || chunk_is_token(next, CT_PAREN_CLOSE)))
         {
            set_chunk_type(pc, CT_WORD);
         }
      }

      // Another hack to clean up more keyword abuse
      if (  chunk_is_token(pc, CT_CLASS)
         && (  chunk_is_token(prev, CT_DOT)
            || chunk_is_token(next, CT_DOT)
            || chunk_is_token(prev, CT_MEMBER)  // Issue #3031
            || chunk_is_token(next, CT_MEMBER)))
      {
         set_chunk_type(pc, CT_WORD);
      }

      // Detect Objective C class name
      if (  chunk_is_token(pc, CT_OC_IMPL)
         || chunk_is_token(pc, CT_OC_INTF)
         || chunk_is_token(pc, CT_OC_PROTOCOL))
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

      if (chunk_is_token(pc, CT_OC_INTF))
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
      if (  (  get_chunk_parent_type(pc) == CT_OC_IMPL
            || get_chunk_parent_type(pc) == CT_OC_INTF
            || chunk_is_token(pc, CT_OC_CLASS))
         && chunk_is_token(next, CT_PAREN_OPEN))
      {
         set_chunk_parent(next, get_chunk_parent_type(pc));

         chunk_t *tmp = chunk_get_next(next);

         if (tmp != nullptr && tmp->next != nullptr)
         {
            if (chunk_is_token(tmp, CT_PAREN_CLOSE))
            {
               //set_chunk_type(tmp, CT_OC_CLASS_EXT);
               set_chunk_parent(tmp, get_chunk_parent_type(pc));
            }
            else
            {
               set_chunk_type(tmp, CT_OC_CATEGORY);
               set_chunk_parent(tmp, get_chunk_parent_type(pc));
            }
         }
         tmp = chunk_get_next_type(pc, CT_PAREN_CLOSE, pc->level);

         if (tmp != nullptr)
         {
            set_chunk_parent(tmp, get_chunk_parent_type(pc));
         }
      }

      /*
       * Detect Objective C @property:
       *   @property NSString *stringProperty;
       *   @property(nonatomic, retain) NSMutableDictionary *shareWith;
       */
      if (chunk_is_token(pc, CT_OC_PROPERTY))
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
      if (  chunk_is_token(pc, CT_OC_SEL)
         && chunk_is_token(next, CT_PAREN_OPEN))
      {
         set_chunk_parent(next, pc->type);

         chunk_t *tmp = chunk_get_next(next);

         if (tmp != nullptr)
         {
            set_chunk_type(tmp, CT_OC_SEL_NAME);
            set_chunk_parent(tmp, pc->type);

            while ((tmp = chunk_get_next_ncnl(tmp)) != nullptr)
            {
               if (chunk_is_token(tmp, CT_PAREN_CLOSE))
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
      if (chunk_is_token(pc, CT_PREPROC))
      {
         set_chunk_parent(pc, next->type);
      }

      // Detect "pragma region" and "pragma endregion"
      if (  chunk_is_token(pc, CT_PP_PRAGMA)
         && chunk_is_token(next, CT_PREPROC_BODY))
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
      if (  language_is_set(LANG_CS)
         && chunk_is_token(pc, CT_DEFAULT)
         && chunk_is_token(next, CT_PAREN_OPEN))
      {
         set_chunk_type(pc, CT_SIZEOF);
      }

      if (chunk_is_token(pc, CT_UNSAFE) && next->type != CT_BRACE_OPEN)
      {
         set_chunk_type(pc, CT_QUALIFIER);
      }

      if (  (  chunk_is_token(pc, CT_USING)
            || (chunk_is_token(pc, CT_TRY) && language_is_set(LANG_JAVA)))
         && chunk_is_token(next, CT_PAREN_OPEN))
      {
         set_chunk_type(pc, CT_USING_STMT);
      }

      // Add minimal support for C++0x rvalue references
      if (  chunk_is_token(pc, CT_BOOL)
         && language_is_set(LANG_CPP)
         && chunk_is_str(pc, "&&", 2))
      {
         if (chunk_is_token(prev, CT_TYPE))
         {
            // Issue # 1002
            if (!pc->flags.test(PCF_IN_TEMPLATE))
            {
               set_chunk_type(pc, CT_BYREF);
            }
         }
      }

      /*
       * HACK: treat try followed by a colon as a qualifier to handle this:
       *   A::A(int) try : B() { } catch (...) { }
       */
      if (  chunk_is_token(pc, CT_TRY)
         && chunk_is_str(pc, "try", 3)
         && chunk_is_token(next, CT_COLON))
      {
         set_chunk_type(pc, CT_QUALIFIER);
      }

      /*
       * If Java's 'synchronized' is in a method declaration, it should be
       * a qualifier.
       */
      if (  language_is_set(LANG_JAVA)
         && chunk_is_token(pc, CT_SYNCHRONIZED)
         && next->type != CT_PAREN_OPEN)
      {
         set_chunk_type(pc, CT_QUALIFIER);
      }

      // change CT_DC_MEMBER + CT_FOR into CT_DC_MEMBER + CT_FUNC_CALL
      if (  chunk_is_token(pc, CT_FOR)
         && chunk_is_token(pc->prev, CT_DC_MEMBER))
      {
         set_chunk_type(pc, CT_FUNC_CALL);
      }
      // TODO: determine other stuff here

      prev = pc;
      pc   = next;
      next = chunk_get_next_ncnl(pc);
   }
} // tokenize_cleanup


static void check_template(chunk_t *start, bool in_type_cast)
{
   LOG_FMT(LTEMPL, "%s(%d): orig_line %zu, orig_col %zu:\n",
           __func__, __LINE__, start->orig_line, start->orig_col);

   chunk_t *prev = chunk_get_prev_ncnl(start, scope_e::PREPROC);

   if (prev == nullptr)
   {
      return;
   }
   chunk_t *end;
   chunk_t *pc;

   if (chunk_is_token(prev, CT_TEMPLATE))
   {
      LOG_FMT(LTEMPL, "%s(%d): CT_TEMPLATE:\n", __func__, __LINE__);

      // We have: "template< ... >", which is a template declaration
      size_t level  = 1;
      size_t parens = 0;

      for (pc = chunk_get_next_ncnl(start, scope_e::PREPROC);
           pc != nullptr;
           pc = chunk_get_next_ncnl(pc, scope_e::PREPROC))
      {
         LOG_FMT(LTEMPL, "%s(%d): type is %s, level is %zu\n",
                 __func__, __LINE__, get_token_name(pc->type), level);

         if ((pc->str[0] == '>') && (pc->len() > 1))
         {
            if (pc->str[1] == '=')                         // Issue #1462 and #2565
            {
               LOG_FMT(LTEMPL, "%s(%d): do not split '%s' at orig_line %zu, orig_col %zu\n",
                       __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
            }
            else
            {
               LOG_FMT(LTEMPL, "%s(%d): {split '%s' at orig_line %zu, orig_col %zu}\n",
                       __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);
               split_off_angle_close(pc);
            }
         }

         if (chunk_is_token(pc, CT_DECLTYPE))
         {
            flag_cpp_decltype(pc);
         }
         else if (chunk_is_token(pc, CT_PAREN_OPEN))
         {
            ++parens;
         }
         else if (chunk_is_token(pc, CT_PAREN_CLOSE))
         {
            --parens;
         }

         if (parens == 0)
         {
            if (chunk_is_str(pc, "<", 1))
            {
               level++;
            }
            else if (chunk_is_str(pc, ">", 1))
            {
               if (level == 0)
               {
                  fprintf(stderr, "%s(%d): level is ZERO, cannot be decremented, at line %zu, column %zu\n",
                          __func__, __LINE__, pc->orig_line, pc->orig_col);
                  log_flush(true);
                  exit(EX_SOFTWARE);
               }
               level--;

               if (level == 0)
               {
                  break;
               }
            }
         }
      }

      end = pc;
   }
   else
   {
      /*
       * We may have something like "a< ... >", which is a template where
       * '...' may consist of anything except a semicolon, unbalanced
       * parens, or braces (with one exception being braced initializers
       * embedded within decltypes).
       *
       * For example, braces may be encountered as such in the following
       * snippet of valid C++ code:
       *
       * template<typename T,
       *          typename = enable_if_t<is_same<typename decay<T>::type,
       *                                          decltype (make_index_sequence<5> { })>::value>>
       * void foo(T &&arg)
       * {
       *
       * }
       *
       * Finally, if we are inside an 'if' statement and hit a CT_BOOL,
       * then it isn't a template.
       */

      // A template requires a word/type right before the open angle
      if (  prev->type != CT_WORD
         && prev->type != CT_TYPE
         && prev->type != CT_COMMA
         && prev->type != CT_QUALIFIER
         && prev->type != CT_OPERATOR_VAL
         && get_chunk_parent_type(prev) != CT_OPERATOR)
      {
         LOG_FMT(LTEMPL, "%s(%d): - after type %s + ( - Not a template\n",
                 __func__, __LINE__, get_token_name(prev->type));
         set_chunk_type(start, CT_COMPARE);
         return;
      }
      LOG_FMT(LTEMPL, "%s(%d): - prev->type is %s -\n",
              __func__, __LINE__, get_token_name(prev->type));

      // Scan back and make sure we aren't inside square parenthesis
      bool in_if         = false;
      bool hit_semicolon = false;
      pc = start;

      while ((pc = chunk_get_prev_ncnl(pc, scope_e::PREPROC)) != nullptr)
      {
         if (  (chunk_is_token(pc, CT_SEMICOLON) && hit_semicolon)
            || chunk_is_token(pc, CT_SQUARE_CLOSE))
         {
            break;
         }

         if (chunk_is_token(pc, CT_DECLTYPE))
         {
            flag_cpp_decltype(pc);
         }

         if (chunk_is_token(pc, CT_BRACE_OPEN))
         {
            if (  !pc->flags.test(PCF_IN_DECLTYPE)
               || !detect_cpp_braced_init_list(pc->prev, pc))
            {
               break;
            }
            flag_cpp_braced_init_list(pc->prev, pc);
         }

         if (  chunk_is_token(pc, CT_BRACE_CLOSE)
            && get_chunk_parent_type(pc) != CT_BRACED_INIT_LIST
            && !pc->flags.test(PCF_IN_DECLTYPE))
         {
            break;
         }

         if (chunk_is_token(pc, CT_SEMICOLON) && !hit_semicolon)
         {
            hit_semicolon = true;
         }

         if (  ((  chunk_is_token(pc, CT_IF)
                || chunk_is_token(pc, CT_RETURN)
                || chunk_is_token(pc, CT_WHILE)
                || chunk_is_token(pc, CT_WHILE_OF_DO)) && hit_semicolon == false)
            || (chunk_is_token(pc, CT_FOR) && hit_semicolon == true))
         {
            in_if = true;
            break;
         }
      }
      /*
       * Scan forward to the angle close
       * If we have a comparison in there, then it can't be a template.
       */
      const int max_token_count = 1024;
      c_token_t tokens[max_token_count];
      size_t    num_tokens = 1;

      tokens[0] = CT_ANGLE_OPEN;

      for (pc = chunk_get_next_ncnl(start, scope_e::PREPROC);
           pc != nullptr;
           pc = chunk_get_next_ncnl(pc, scope_e::PREPROC))
      {
         LOG_FMT(LTEMPL, "%s(%d): type is %s, num_tokens is %zu\n",
                 __func__, __LINE__, get_token_name(pc->type), num_tokens);

         log_rule_B("tok_split_gte");

         if (  (tokens[num_tokens - 1] == CT_ANGLE_OPEN)
            && (pc->str[0] == '>')
            && (pc->len() > 1)
            && (  options::tok_split_gte()
               || (  (chunk_is_str(pc, ">>", 2) || chunk_is_str(pc, ">>>", 3))
                  && (num_tokens >= 2 || (num_tokens >= 1 && in_type_cast)))))
         {
            LOG_FMT(LTEMPL, "%s(%d): {split '%s' at orig_line %zu, orig_col %zu}\n",
                    __func__, __LINE__, pc->text(), pc->orig_line, pc->orig_col);

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
                 && (chunk_is_token(pc, CT_BOOL) || chunk_is_token(pc, CT_COMPARE)))
         {
            break;
         }
         else if (chunk_is_token(pc, CT_BRACE_OPEN))
         {
            if (  !pc->flags.test(PCF_IN_DECLTYPE)
               || !detect_cpp_braced_init_list(pc->prev, pc))
            {
               break;
            }
            auto brace_open  = chunk_get_next_ncnl(pc);
            auto brace_close = chunk_skip_to_match(brace_open);

            set_chunk_parent(brace_open, CT_BRACED_INIT_LIST);
            set_chunk_parent(brace_close, CT_BRACED_INIT_LIST);
         }
         else if (  chunk_is_token(pc, CT_BRACE_CLOSE)
                 && get_chunk_parent_type(pc) != CT_BRACED_INIT_LIST
                 && !pc->flags.test(PCF_IN_DECLTYPE))
         {
            break;
         }
         else if (chunk_is_token(pc, CT_SEMICOLON))
         {
            break;
         }
         else if (chunk_is_token(pc, CT_PAREN_OPEN))
         {
            if (num_tokens >= max_token_count - 1)
            {
               break;
            }
            tokens[num_tokens] = CT_PAREN_OPEN;
            num_tokens++;
         }
         else if (  chunk_is_token(pc, CT_QUESTION)                    // Issue #2949
                 && language_is_set(LANG_CPP))
         {
            break;
         }
         else if (chunk_is_token(pc, CT_PAREN_CLOSE))
         {
            if (num_tokens == 0)
            {
               fprintf(stderr, "%s(%d): num_tokens is ZERO, cannot be decremented, at line %zu, column %zu\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col);
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            num_tokens--;

            if (tokens[num_tokens] != CT_PAREN_OPEN)
            {
               break;  // unbalanced parentheses
            }
         }
      }

      end = pc;
   }

   if (chunk_is_token(end, CT_ANGLE_CLOSE))
   {
      pc = chunk_get_next_ncnl(end, scope_e::PREPROC);

      if (pc == nullptr || pc->type != CT_NUMBER)
      {
         LOG_FMT(LTEMPL, "%s(%d): Template detected\n", __func__, __LINE__);
         LOG_FMT(LTEMPL, "%s(%d):     from orig_line %zu, orig_col %zu\n",
                 __func__, __LINE__, start->orig_line, start->orig_col);
         LOG_FMT(LTEMPL, "%s(%d):     to   orig_line %zu, orig_col %zu\n",
                 __func__, __LINE__, end->orig_line, end->orig_col);
         set_chunk_parent(start, CT_TEMPLATE);

         check_template_args(start, end);

         set_chunk_parent(end, CT_TEMPLATE);
         chunk_flags_set(end, PCF_IN_TEMPLATE);
         return;
      }
   }
   LOG_FMT(LTEMPL, "%s(%d): - Not a template: end = %s\n",
           __func__, __LINE__, (end != NULL) ? get_token_name(end->type) : "<null>");
   set_chunk_type(start, CT_COMPARE);
} // check_template


static void check_template_arg(chunk_t *start, chunk_t *end)
{
   LOG_FMT(LTEMPL, "%s(%d): Template argument detected\n", __func__, __LINE__);
   LOG_FMT(LTEMPL, "%s(%d):     from orig_line %zu, orig_col %zu\n",
           __func__, __LINE__, start->orig_line, start->orig_col);
   LOG_FMT(LTEMPL, "%s(%d):     to   orig_line %zu, orig_col %zu\n",
           __func__, __LINE__, end->orig_line, end->orig_col);

   // Issue #1127
   // MyFoo<mySize * 2> foo1;
   // MyFoo<2*mySize * 2> foo1;
   // Issue #1346
   // use it as ONE line:
   //   typename std::enable_if<!std::is_void<T>::value,
   //   QVector<T> >::type dummy(const std::function<T*(const S&)>&
   //   pFunc, const QVector<S>& pItems)
   // we need two runs
   // 1. run to test if expression is numeric
   bool    expressionIsNumeric = false;
   chunk_t *pc                 = start;

   while (pc != end)
   {
      chunk_t *next = chunk_get_next_ncnl(pc, scope_e::PREPROC);
      // a test "if (next == nullptr)" is not necessary
      chunk_flags_set(pc, PCF_IN_TEMPLATE);

      if (chunk_is_token(pc, CT_DECLTYPE) || chunk_is_token(pc, CT_SIZEOF))
      {
         expressionIsNumeric = true;
         break;
      }

      if (next->type != CT_PAREN_OPEN)
      {
         if (  chunk_is_token(pc, CT_NUMBER)
            || chunk_is_token(pc, CT_ARITH)
            || chunk_is_token(pc, CT_SHIFT))
         {
            expressionIsNumeric = true;
            break;
         }
      }
      pc = next;
   }
   LOG_FMT(LTEMPL, "%s(%d): expressionIsNumeric is %s\n",
           __func__, __LINE__, expressionIsNumeric ? "TRUE" : "FALSE");

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
} // check_template_arg


static void check_template_args(chunk_t *start, chunk_t *end)
{
   std::vector<c_token_t> tokens;

   // Scan for commas
   chunk_t *pc;

   for (pc = chunk_get_next_ncnl(start, scope_e::PREPROC);
        pc != nullptr && pc != end;
        pc = chunk_get_next_ncnl(pc, scope_e::PREPROC))
   {
      switch (pc->type)
      {
      case CT_COMMA:

         if (tokens.empty())
         {
            // Check current argument
            check_template_args(start, pc);
            start = pc;
         }
         break;

      case CT_ANGLE_OPEN:
      case CT_PAREN_OPEN:
         tokens.push_back(pc->type);
         break;

      case CT_ANGLE_CLOSE:

         if (!tokens.empty() && tokens.back() == CT_ANGLE_OPEN)
         {
            tokens.pop_back();
         }
         break;

      case CT_PAREN_CLOSE:

         if (!tokens.empty() && tokens.back() == CT_PAREN_OPEN)
         {
            tokens.pop_back();
         }
         break;

      default:
         break;
      }
   }

   // Check whatever is left
   check_template_arg(start, end);
} // check_template_args


static void cleanup_objc_property(chunk_t *start)
{
   assert(chunk_is_token(start, CT_OC_PROPERTY));

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
   assert(chunk_is_token(open_paren, CT_PAREN_OPEN));

   chunk_t *tmp = open_paren;

   while (tmp && tmp->type != CT_PAREN_CLOSE)
   {
      if (  chunk_is_token(tmp, CT_WORD)
         && (chunk_is_str(tmp, "setter", 6) || chunk_is_str(tmp, "getter", 6)))
      {
         tmp = tmp->next;

         while (  tmp
               && tmp->type != CT_COMMA
               && tmp->type != CT_PAREN_CLOSE)
         {
            if (chunk_is_token(tmp, CT_WORD) || chunk_is_str(tmp, ":", 1))
            {
               set_chunk_type(tmp, CT_OC_SEL_NAME);
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
   assert(chunk_is_token(open_paren, CT_PAREN_OPEN));

   chunk_t *tmp = open_paren;

   while (tmp && tmp->type != CT_PAREN_CLOSE)
   {
      if (  (chunk_is_token(tmp, CT_COMMA) || chunk_is_token(tmp, CT_PAREN_OPEN))
         && (chunk_is_token(tmp->next, CT_WORD) || chunk_is_token(tmp->next, CT_TYPE)))
      {
         set_chunk_type(tmp->next, CT_OC_PROPERTY_ATTR);
      }
      tmp = tmp->next;
   }
}
