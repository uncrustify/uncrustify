/**
 * @file tokenize_cleanup.cpp
 * Looks at simple sequences to refine the chunk types.
 * Examples:
 *  - change '[' + ']' into '[]'/
 *  - detect "version = 10;" vs "version (xxx) {"
 *
 * @author  Ben Gardner
 * @author  Guy Maurel 2015, 2021
 * @license GPL v2+
 */

#include "tokenize_cleanup.h"

#include "combine.h"
#include "combine_skip.h"
#include "flag_braced_init_list.h"
#include "flag_decltype.h"
#include "keywords.h"
#include "prototypes.h"
#include "punctuators.h"
#include "space.h"
#include "unc_ctype.h"


using namespace uncrustify;


/**
 * Mark types in a single template argument.
 *
 * @param start  chunk to start check at
 * @param end    chunk to end check at
 */
static void check_template_arg(Chunk *start, Chunk *end);


/**
 * Mark types in template argument(s).
 *
 * @param start  chunk to start check at
 * @param end    chunk to end check at
 */
static void check_template_args(Chunk *start, Chunk *end);


/**
 * If there is nothing but CT_WORD and CT_MEMBER, then it's probably a
 * template thingy.  Otherwise, it's likely a comparison.
 *
 * @param start  chunk to start check at
 */
static void check_template(Chunk *start, bool in_type_cast);


/**
 * Convert '>' + '>' into '>>'
 * If we only have a single '>', then change it to CT_COMPARE.
 *
 * @param pc  chunk to start at
 */
static Chunk *handle_double_angle_close(Chunk *pc);


/**
 * Marks ObjC specific chunks in property declaration, by setting
 * parent types and chunk types.
 */
static void cleanup_objc_property(Chunk *start);


/**
 * Marks ObjC specific chunks in property declaration (getter/setter attribute)
 * Will mark 'test4Setter'and ':' in '@property (setter=test4Setter:, strong) int test4;' as CT_OC_SEL_NAME
 */
static void mark_selectors_in_property_with_open_paren(Chunk *open_paren);


/**
 * Marks ObjC specific chunks in property declaration ( attributes)
 * Changes all the CT_WORD and CT_TYPE to CT_OC_PROPERTY_ATTR
 */
static void mark_attributes_in_property_with_open_paren(Chunk *open_paren);


static Chunk *handle_double_angle_close(Chunk *pc)
{
   if (pc == nullptr)
   {
      pc = Chunk::NullChunkPtr;
   }
   Chunk *next = pc->GetNext();

   if (next->IsNotNullChunk())
   {
      if (  pc->Is(CT_ANGLE_CLOSE)
         && next->Is(CT_ANGLE_CLOSE)
         && pc->GetParentType() == CT_NONE
         && (pc->orig_col_end + 1) == next->orig_col
         && next->GetParentType() == CT_NONE)
      {
         pc->str.append('>');
         pc->SetType(CT_SHIFT);
         pc->orig_col_end = next->orig_col_end;

         Chunk *tmp = next->GetNextNcNnl();
         Chunk::Delete(next);
         next = tmp;
      }
      else
      {
         // bug #663
         pc->SetType(CT_COMPARE);
      }
   }
   return(next);
}


void split_off_angle_close(Chunk *pc)
{
   const chunk_tag_t *ct = find_punctuator(pc->Text() + 1, cpd.lang_flags);

   if (ct == nullptr)
   {
      return;
   }
   Chunk nc = *pc;

   pc->str.resize(1);
   pc->orig_col_end = pc->orig_col + 1;
   pc->SetType(CT_ANGLE_CLOSE);

   nc.SetType(ct->type);
   nc.str.pop_front();
   nc.orig_col++;
   nc.column++;
   nc.CopyAndAddAfter(pc);
}


void tokenize_trailing_return_types()
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

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      char copy[1000];
      LOG_FMT(LNOTE, "%s(%d): orig_line is %zu, orig_col is %zu, Text() is '%s'\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->ElidedText(copy));

      if (  pc->Is(CT_MEMBER)
         && (strcmp(pc->Text(), "->") == 0))
      {
         Chunk *tmp = pc->GetPrevNcNnl();
         Chunk *tmp_2;
         Chunk *open_paren;

         if (tmp->Is(CT_QUALIFIER))
         {
            // auto max(int a, int b) const -> int;
            // auto f11() const -> bool;
            tmp = tmp->GetPrevNcNnl();
         }
         else if (tmp->Is(CT_NOEXCEPT))
         {
            // noexcept is present
            tmp_2 = tmp->GetPrevNcNnl();

            if (tmp_2->Is(CT_QUALIFIER))
            {
               // auto f12() const noexcept -> bool;
               // auto f15() const noexcept -> bool = delete;
               tmp = tmp_2->GetPrevNcNnl();
            }
            else
            {
               // auto f02() noexcept -> bool;
               // auto f05() noexcept -> bool = delete;
               tmp = tmp_2;
            }
         }
         else if (tmp->Is(CT_PAREN_CLOSE))
         {
            open_paren = tmp->GetPrevType(CT_PAREN_OPEN, tmp->level);
            tmp        = open_paren->GetPrevNcNnl();

            if (tmp->Is(CT_NOEXCEPT))
            {
               // noexcept is present
               tmp_2 = tmp->GetPrevNcNnl();

               if (tmp_2->Is(CT_QUALIFIER))
               {
                  // auto f13() const noexcept(true) -> bool;
                  // auto f14() const noexcept(false) -> bool;
                  // auto f16() const noexcept(true) -> bool = delete;
                  // auto f17() const noexcept(false) -> bool = delete;
                  tmp = tmp_2->GetPrevNcNnl();
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
            else if (tmp->Is(CT_THROW))
            {
               // throw is present
               tmp_2 = tmp->GetPrevNcNnl();

               if (tmp_2->Is(CT_QUALIFIER))
               {
                  // auto f23() const throw() -> bool;
                  // auto f24() const throw() -> bool = delete;
                  tmp = tmp_2->GetPrevNcNnl();
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

         if (  tmp->Is(CT_FPAREN_CLOSE)
            && (  tmp->GetParentType() == CT_FUNC_PROTO
               || tmp->GetParentType() == CT_FUNC_DEF))
         {
            pc->SetType(CT_TRAILING_RET);
            LOG_FMT(LNOTE, "%s(%d): set trailing return type for Text() is '%s'\n",
                    __func__, __LINE__, pc->Text());                  // Issue #3222
            // TODO
            // https://en.cppreference.com/w/cpp/language/function
            // noptr-declarator ( parameter-list ) cv(optional) ref(optional) except(optional) attr(optional) -> trailing
            Chunk *next = pc->GetNextNcNnl();

            if (next->Is(CT_DECLTYPE))
            {
               // TODO
            }
            else if (next->Is(CT_WORD))
            {
               next->SetType(CT_TYPE);                         // Issue #3222
               next = next->GetNextNcNnl();

               if (next->Is(CT_ARITH))
               {
                  if (next->str[0] == '*')
                  {
                     next->SetType(CT_PTR_TYPE);
                  }
                  else if (next->str[0] == '&')                       // Issue #3407
                  {
                     next->SetType(CT_BYREF);
                  }
               }
            }
            else
            {
               // TODO
            }
         }
      }
   }
} // tokenize_trailing_return_types


void tokenize_cleanup()
{
   LOG_FUNC_ENTRY();

   Chunk *prev = Chunk::NullChunkPtr;
   Chunk *next;
   bool  in_type_cast = false;

   cpd.unc_stage = unc_stage_e::TOKENIZE_CLEANUP;

   /*
    * Since [] is expected to be TSQUARE for the 'operator', we need to make
    * this change in the first pass.
    */
   Chunk *pc;

   for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (pc->Is(CT_SQUARE_OPEN))
      {
         next = pc->GetNextNcNnl();

         if (next->Is(CT_SQUARE_CLOSE))
         {
            // Change '[' + ']' into '[]'
            pc->SetType(CT_TSQUARE);
            pc->str = "[]";
            /*
             * bug #664: The original orig_col_end of CT_SQUARE_CLOSE is
             * stored at orig_col_end of CT_TSQUARE.
             * pc->orig_col_end += 1;
             */
            pc->orig_col_end = next->orig_col_end;
            Chunk::Delete(next);
         }
      }

      if (  pc->Is(CT_SEMICOLON)
         && pc->TestFlags(PCF_IN_PREPROC)
         && !pc->GetNextNcNnl(E_Scope::PREPROC))
      {
         LOG_FMT(LNOTE, "%s(%d): %s:%zu Detected a macro that ends with a semicolon. Possible failures if used.\n",
                 __func__, __LINE__, cpd.filename.c_str(), pc->orig_line);
      }
   }

   // change := to CT_SQL_ASSIGN Issue #527
   for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (pc->Is(CT_COLON))
      {
         next = pc->GetNextNcNnl();

         if (next->Is(CT_ASSIGN))
         {
            // Change ':' + '=' into ':='
            pc->SetType(CT_SQL_ASSIGN);
            pc->str          = ":=";
            pc->orig_col_end = next->orig_col_end;
            Chunk::Delete(next);
         }
      }
   }

   // We can handle everything else in the second pass
   pc   = Chunk::GetHead();
   next = pc->GetNextNcNnl();

   while (  pc->IsNotNullChunk()
         && next->IsNotNullChunk())
   {
      if (  pc->Is(CT_DOT)
         && language_is_set(LANG_ALLC))
      {
         pc->SetType(CT_MEMBER);
      }

      if (  pc->Is(CT_NULLCOND)
         && language_is_set(LANG_CS))
      {
         pc->SetType(CT_MEMBER);
      }

      // Determine the version stuff (D only)
      if (pc->Is(CT_D_VERSION))
      {
         if (next->Is(CT_PAREN_OPEN))
         {
            pc->SetType(CT_D_VERSION_IF);
         }
         else
         {
            if (next->IsNot(CT_ASSIGN))
            {
               LOG_FMT(LERR, "%s(%d): %s:%zu: version: Unexpected token %s\n",
                       __func__, __LINE__, cpd.filename.c_str(), pc->orig_line, get_token_name(next->GetType()));
               cpd.error_count++;
            }
            pc->SetType(CT_WORD);
         }
      }

      // Determine the scope stuff (D only)
      if (pc->Is(CT_D_SCOPE))
      {
         if (next->Is(CT_PAREN_OPEN))
         {
            pc->SetType(CT_D_SCOPE_IF);
         }
         else
         {
            pc->SetType(CT_TYPE);
         }
      }

      /*
       * Change CT_BASE before CT_PAREN_OPEN to CT_WORD.
       * public myclass() : base() {}
       * -or-
       * var x = (T)base.y;
       */
      if (  pc->Is(CT_BASE)
         && (  next->Is(CT_PAREN_OPEN)
            || next->Is(CT_DOT)))
      {
         pc->SetType(CT_WORD);
      }

      if (  pc->Is(CT_ENUM)
         && (  next->Is(CT_STRUCT)
            || next->Is(CT_CLASS)))
      {
         next->SetType(CT_ENUM_CLASS);
      }
      Chunk *next_non_attr = language_is_set(LANG_CPP) ? skip_attribute_next(next) : next;

      /*
       * Change CT_WORD after CT_ENUM, CT_UNION, CT_STRUCT, or CT_CLASS to CT_TYPE
       * Change CT_WORD before CT_WORD to CT_TYPE
       */
      if (next_non_attr->Is(CT_WORD))
      {
         if (pc->IsClassEnumStructOrUnion())
         {
            next_non_attr->SetType(CT_TYPE);
         }

         if (pc->Is(CT_WORD))
         {
            pc->SetType(CT_TYPE);
         }
      }

      /*
       * change extern to qualifier if extern isn't followed by a string or
       * an open parenthesis
       */
      if (pc->Is(CT_EXTERN))
      {
         if (next->Is(CT_STRING))
         {
            // Probably 'extern "C"'
         }
         else if (next->Is(CT_PAREN_OPEN))
         {
            // Probably 'extern (C)'
         }
         else
         {
            // Something else followed by a open brace
            Chunk *tmp = next->GetNextNcNnl();

            if (  tmp->IsNullChunk()
               || tmp->IsNot(CT_BRACE_OPEN))
            {
               pc->SetType(CT_QUALIFIER);
            }
         }
      }

      /*
       * Change CT_STAR to CT_PTR_TYPE if preceded by
       *     CT_TYPE, CT_QUALIFIER, or CT_PTR_TYPE
       * or by a
       *     CT_WORD which is preceded by CT_DC_MEMBER: '::aaa *b'
       */
      if (  (next->Is(CT_STAR))
         || (  language_is_set(LANG_CPP)
            && (next->Is(CT_CARET)))
         || (  language_is_set(LANG_CS | LANG_VALA)
            && (next->Is(CT_QUESTION))
            && (strcmp(pc->Text(), "null") != 0)))
      {
         if (  pc->Is(CT_TYPE)
            || pc->Is(CT_QUALIFIER)
            || pc->Is(CT_PTR_TYPE))
         {
            next->SetType(CT_PTR_TYPE);
         }
      }

      if (  pc->Is(CT_TYPE_CAST)
         && next->Is(CT_ANGLE_OPEN))
      {
         next->SetParentType(CT_TYPE_CAST);
         in_type_cast = true;
      }

      if (pc->Is(CT_DECLTYPE))
      {
         flag_cpp_decltype(pc);
      }

      // Change angle open/close to CT_COMPARE, if not a template thingy
      if (  pc->Is(CT_ANGLE_OPEN)
         && pc->GetParentType() != CT_TYPE_CAST)
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
            pc->SetType(CT_COMPARE);
         }
      }

      if (  pc->Is(CT_ANGLE_CLOSE)
         && pc->GetParentType() != CT_TEMPLATE)
      {
         if (in_type_cast)
         {
            in_type_cast = false;
            pc->SetParentType(CT_TYPE_CAST);
         }
         else
         {
            next = handle_double_angle_close(pc);
         }
      }

      if (language_is_set(LANG_D))
      {
         // Check for the D string concat symbol '~'
         if (  pc->Is(CT_INV)
            && (  prev->Is(CT_STRING)
               || prev->Is(CT_WORD)
               || next->Is(CT_STRING)))
         {
            pc->SetType(CT_CONCAT);
         }

         // Check for the D template symbol '!' (word + '!' + word or '(')
         if (  pc->Is(CT_NOT)
            && prev->Is(CT_WORD)
            && (  next->Is(CT_PAREN_OPEN)
               || next->Is(CT_WORD)
               || next->Is(CT_TYPE)
               || next->Is(CT_NUMBER)
               || next->Is(CT_NUMBER_FP)
               || next->Is(CT_STRING)
               || next->Is(CT_STRING_MULTI)))
         {
            pc->SetType(CT_D_TEMPLATE);
         }

         // handle "version(unittest) { }" vs "unittest { }"
         if (  pc->Is(CT_UNITTEST)
            && prev->Is(CT_PAREN_OPEN))
         {
            pc->SetType(CT_WORD);
         }

         // handle 'static if' and merge the tokens
         if (  pc->Is(CT_IF)
            && prev->IsString("static"))
         {
            // delete PREV and merge with IF
            pc->str.insert(0, ' ');
            pc->str.insert(0, prev->str);
            pc->orig_col  = prev->orig_col;
            pc->orig_line = prev->orig_line;
            Chunk *to_be_deleted = prev;
            prev = prev->GetPrevNcNnl();

            if (prev->IsNotNullChunk())
            {
               Chunk::Delete(to_be_deleted);
            }
         }
      }

      if (language_is_set(LANG_CPP))
      {
         // Change Word before '::' into a type
         if (  pc->Is(CT_WORD)
            && next->Is(CT_DC_MEMBER))
         {
            prev = pc->GetPrev();

            if (prev->IsNullChunk())                  // Issue #3010
            {
               pc->SetType(CT_TYPE);
            }
            else
            {
               if (prev->Is(CT_COLON))
               {
                  // nothing to do
               }
               else
               {
                  pc->SetType(CT_TYPE);
               }
            }
         }

         // Set parent type for 'if constexpr'
         if (  prev->Is(CT_IF)
            && pc->Is(CT_QUALIFIER)
            && pc->IsString("constexpr"))
         {
            pc->SetType(CT_CONSTEXPR);
         }
      }

      // Change get/set to CT_WORD if not followed by a brace open
      if (  pc->Is(CT_GETSET)
         && next->IsNot(CT_BRACE_OPEN))
      {
         if (  next->Is(CT_SEMICOLON)
            && (  prev->Is(CT_BRACE_CLOSE)
               || prev->Is(CT_BRACE_OPEN)
               || prev->Is(CT_SEMICOLON)))
         {
            pc->SetType(CT_GETSET_EMPTY);
            next->SetParentType(CT_GETSET);
         }
         else
         {
            pc->SetType(CT_WORD);
         }
      }

      /*
       * Interface is only a keyword in MS land if followed by 'class' or 'struct'
       * likewise, 'class' may be a member name in Java.
       */
      if (  pc->Is(CT_CLASS)
         && !CharTable::IsKw1(next->str[0]))
      {
         if (  next->IsNot(CT_DC_MEMBER)
            && next->IsNot(CT_ATTRIBUTE))                       // Issue #2570
         {
            pc->SetType(CT_WORD);
         }
         else if (  prev->Is(CT_DC_MEMBER)
                 || prev->Is(CT_TYPE))
         {
            pc->SetType(CT_TYPE);
         }
         else if (next->Is(CT_DC_MEMBER))
         {
            Chunk *next2 = next->GetNextNcNnlNet();

            if (  next2->Is(CT_INV)      // CT_INV hasn't turned into CT_DESTRUCTOR just yet
               || (  next2->Is(CT_CLASS) // constructor isn't turned into CT_FUNC* just yet
                  && !strcmp(pc->Text(), next2->Text())))
            {
               pc->SetType(CT_TYPE);
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
      if (pc->Is(CT_OPERATOR))
      {
         Chunk *tmp2 = next->GetNext();

         // Handle special case of () operator -- [] already handled
         if (next->Is(CT_PAREN_OPEN))
         {
            Chunk *tmp = next->GetNext();

            if (tmp->Is(CT_PAREN_CLOSE))
            {
               next->str = "()";
               next->SetType(CT_OPERATOR_VAL);
               Chunk::Delete(tmp);
               next->orig_col_end += 1;
            }
         }
         else if (  next->Is(CT_ANGLE_CLOSE)
                 && tmp2->Is(CT_ANGLE_CLOSE)
                 && tmp2->orig_col == next->orig_col_end)
         {
            next->str.append('>');
            next->orig_col_end++;
            next->SetType(CT_OPERATOR_VAL);
            Chunk::Delete(tmp2);
         }
         else if (next->TestFlags(PCF_PUNCTUATOR))
         {
            next->SetType(CT_OPERATOR_VAL);
         }
         else
         {
            next->SetType(CT_TYPE);

            /*
             * Replace next with a collection of all tokens that are part of
             * the type.
             */
            tmp2 = next;
            Chunk *tmp;

            while ((tmp = tmp2->GetNext())->IsNotNullChunk())
            {
               if (  tmp->IsNot(CT_WORD)
                  && tmp->IsNot(CT_TYPE)
                  && tmp->IsNot(CT_QUALIFIER)
                  && tmp->IsNot(CT_STAR)
                  && tmp->IsNot(CT_CARET)
                  && tmp->IsNot(CT_AMP)
                  && tmp->IsNot(CT_TSQUARE))
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

            while ((tmp2 = next->GetNext()) != tmp)
            {
               Chunk::Delete(tmp2);
            }
            next->SetType(CT_OPERATOR_VAL);

            next->orig_col_end = next->orig_col + next->Len();
         }
         next->SetParentType(CT_OPERATOR);

         LOG_FMT(LOPERATOR, "%s(%d): %zu:%zu operator '%s'\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, next->Text());
      }

      // Change private, public, protected into either a qualifier or label
      if (pc->Is(CT_ACCESS))
      {
         // Handle Qt slots - maybe should just check for a CT_WORD?
         if (  next->IsString("slots")
            || next->IsString("Q_SLOTS"))
         {
            Chunk *tmp = next->GetNext();

            if (tmp->Is(CT_COLON))
            {
               next = tmp;
            }
         }

         if (next->Is(CT_COLON))
         {
            next->SetType(CT_ACCESS_COLON);
            Chunk *tmp;

            if ((tmp = next->GetNextNcNnl())->IsNotNullChunk())
            {
               tmp->SetFlagBits(PCF_STMT_START | PCF_EXPR_START);
            }
         }
         else
         {
            pc->SetType((  pc->IsString("signals")
                        || pc->IsString("Q_SIGNALS"))
                           ? CT_WORD : CT_QUALIFIER);
         }
      }

      // Look for <newline> 'EXEC' 'SQL'
      if (  (  pc->IsString("EXEC", false)
            && next->IsString("SQL", false))
         || (  (*pc->str.c_str() == '$')
            && pc->IsNot(CT_SQL_WORD)
               /* but avoid breaking tokenization for C# 6 interpolated strings. */
            && (  !language_is_set(LANG_CS)
               || (  pc->Is(CT_STRING)
                  && (!pc->str.startswith("$\""))
                  && (!pc->str.startswith("$@\""))))))
      {
         Chunk *tmp = pc->GetPrev();

         if (tmp->IsNewline())
         {
            if (*pc->str.c_str() == '$')
            {
               pc->SetType(CT_SQL_EXEC);

               if (pc->Len() > 1)
               {
                  // SPLIT OFF '$'
                  Chunk nc;

                  nc = *pc;
                  pc->str.resize(1);
                  pc->orig_col_end = pc->orig_col + 1;

                  nc.SetType(CT_SQL_WORD);
                  nc.str.pop_front();
                  nc.orig_col++;
                  nc.column++;
                  nc.CopyAndAddAfter(pc);

                  next = pc->GetNext();
               }
            }
            tmp = next->GetNext();

            if (tmp->IsString("BEGIN", false))
            {
               pc->SetType(CT_SQL_BEGIN);
            }
            else if (tmp->IsString("END", false))
            {
               pc->SetType(CT_SQL_END);
            }
            else
            {
               pc->SetType(CT_SQL_EXEC);
            }

            // Change words into CT_SQL_WORD until CT_SEMICOLON
            while (tmp->IsNotNullChunk())
            {
               if (tmp->Is(CT_SEMICOLON))
               {
                  break;
               }

               if (  (tmp->Len() > 0)
                  && (  unc_isalpha(*tmp->str.c_str())
                     || (*tmp->str.c_str() == '$')))
               {
                  tmp->SetType(CT_SQL_WORD);
               }
               tmp = tmp->GetNextNcNnl();
            }
         }
      }

      // handle MS abomination 'for each'
      if (  pc->Is(CT_FOR)
         && next->IsString("each")
         && (next == pc->GetNext()))
      {
         // merge the two with a space between
         pc->str.append(' ');
         pc->str         += next->str;
         pc->orig_col_end = next->orig_col_end;
         Chunk::Delete(next);
         next = pc->GetNextNcNnl();

         // label the 'in'
         if (next->Is(CT_PAREN_OPEN))
         {
            Chunk *tmp = next->GetNextNcNnl();

            while (  tmp->IsNotNullChunk()
                  && tmp->IsNot(CT_PAREN_CLOSE))
            {
               if (tmp->IsString("in"))
               {
                  tmp->SetType(CT_IN);
                  break;
               }
               tmp = tmp->GetNextNcNnl();
            }
         }
      }

      /*
       * ObjectiveC allows keywords to be used as identifiers in some situations
       * This is a dirty hack to allow some of the more common situations.
       */
      if (language_is_set(LANG_OC))
      {
         if (  (  pc->Is(CT_IF)
               || pc->Is(CT_FOR)
               || pc->Is(CT_WHILE))
            && !next->Is(CT_PAREN_OPEN))
         {
            pc->SetType(CT_WORD);
         }

         if (  pc->Is(CT_DO)
            && (  prev->Is(CT_MINUS)
               || next->Is(CT_SQUARE_CLOSE)))
         {
            pc->SetType(CT_WORD);
         }

         // Fix self keyword back to word when mixing c++/objective-c
         if (  pc->Is(CT_THIS)
            && !strcmp(pc->Text(), "self")
            && (  next->Is(CT_COMMA)
               || next->Is(CT_PAREN_CLOSE)))
         {
            pc->SetType(CT_WORD);
         }

         // Fix self keyword back to word when mixing c++/objective-c
         if (  pc->Is(CT_THIS)
            && !strcmp(pc->Text(), "self")
            && (  next->Is(CT_COMMA)
               || next->Is(CT_PAREN_CLOSE)))
         {
            pc->SetType(CT_WORD);
         }
      }

      // Vala allows keywords to be used as identifiers
      if (language_is_set(LANG_VALA))
      {
         if (  find_keyword_type(pc->Text(), pc->Len()) != CT_WORD
            && (  prev->Is(CT_DOT)
               || next->Is(CT_DOT)
               || prev->Is(CT_MEMBER)
               || next->Is(CT_MEMBER)
               || prev->Is(CT_TYPE)))
         {
            pc->SetType(CT_WORD);
         }
      }

      // Another hack to clean up more keyword abuse
      if (  pc->Is(CT_CLASS)
         && (  prev->Is(CT_DOT)
            || next->Is(CT_DOT)
            || prev->Is(CT_MEMBER)  // Issue #3031
            || next->Is(CT_MEMBER)))
      {
         pc->SetType(CT_WORD);
      }

      // Detect Objective C class name
      if (  pc->Is(CT_OC_IMPL)
         || pc->Is(CT_OC_INTF)
         || pc->Is(CT_OC_PROTOCOL))
      {
         if (next->IsNot(CT_PAREN_OPEN))
         {
            next->SetType(CT_OC_CLASS);
         }
         next->SetParentType(pc->GetType());

         Chunk *tmp = next->GetNextNcNnl();

         if (tmp->IsNotNullChunk())
         {
            tmp->SetFlagBits(PCF_STMT_START | PCF_EXPR_START);
         }
         tmp = pc->GetNextType(CT_OC_END, pc->level);

         if (tmp->IsNotNullChunk())
         {
            tmp->SetParentType(pc->GetType());
         }
      }

      if (pc->Is(CT_OC_INTF))
      {
         Chunk *tmp = pc->GetNextNcNnl(E_Scope::PREPROC);

         while (  tmp->IsNotNullChunk()
               && tmp->IsNot(CT_OC_END))
         {
            if (get_token_pattern_class(tmp->GetType()) != pattern_class_e::NONE)
            {
               LOG_FMT(LOBJCWORD, "%s(%d): @interface %zu:%zu change '%s' (%s) to CT_WORD\n",
                       __func__, __LINE__, pc->orig_line, pc->orig_col, tmp->Text(),
                       get_token_name(tmp->GetType()));
               tmp->SetType(CT_WORD);
            }
            tmp = tmp->GetNextNcNnl(E_Scope::PREPROC);
         }
      }

      /*
       * Detect Objective-C categories and class extensions:
       *   @interface ClassName (CategoryName)
       *   @implementation ClassName (CategoryName)
       *   @interface ClassName ()
       *   @implementation ClassName ()
       */
      if (  (  pc->GetParentType() == CT_OC_IMPL
            || pc->GetParentType() == CT_OC_INTF
            || pc->Is(CT_OC_CLASS))
         && next->Is(CT_PAREN_OPEN))
      {
         next->SetParentType(pc->GetParentType());

         Chunk *tmp = next->GetNext();

         if (  tmp->IsNotNullChunk()
            && tmp->GetNext()->IsNotNullChunk())
         {
            if (tmp->Is(CT_PAREN_CLOSE))
            {
               //tmp->SetType(CT_OC_CLASS_EXT);
               tmp->SetParentType(pc->GetParentType());
            }
            else
            {
               tmp->SetType(CT_OC_CATEGORY);
               tmp->SetParentType(pc->GetParentType());
            }
         }
         tmp = pc->GetNextType(CT_PAREN_CLOSE, pc->level);

         if (tmp->IsNotNullChunk())
         {
            tmp->SetParentType(pc->GetParentType());
         }
      }

      /*
       * Detect Objective C @property:
       *   @property NSString *stringProperty;
       *   @property(nonatomic, retain) NSMutableDictionary *shareWith;
       */
      if (pc->Is(CT_OC_PROPERTY))
      {
         if (next->IsNot(CT_PAREN_OPEN))
         {
            next->SetFlagBits(PCF_STMT_START | PCF_EXPR_START);
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
      if (  pc->Is(CT_OC_SEL)
         && next->Is(CT_PAREN_OPEN))
      {
         next->SetParentType(pc->GetType());

         Chunk *tmp = next->GetNext();

         if (tmp->IsNotNullChunk())
         {
            tmp->SetType(CT_OC_SEL_NAME);
            tmp->SetParentType(pc->GetType());

            while ((tmp = tmp->GetNextNcNnl())->IsNotNullChunk())
            {
               if (tmp->Is(CT_PAREN_CLOSE))
               {
                  tmp->SetParentType(CT_OC_SEL);
                  break;
               }
               tmp->SetType(CT_OC_SEL_NAME);
               tmp->SetParentType(pc->GetType());
            }
         }
      }

      // Handle special preprocessor junk
      if (pc->Is(CT_PREPROC))
      {
         pc->SetParentType(next->GetType());
      }

      // Detect "pragma region" and "pragma endregion"
      if (  pc->Is(CT_PP_PRAGMA)
         && next->Is(CT_PREPROC_BODY))
      {
         if (  (strncmp(next->str.c_str(), "region", 6) == 0)
            || (strncmp(next->str.c_str(), "endregion", 9) == 0))
         // TODO: probably better use strncmp
         {
            pc->SetType((*next->str.c_str() == 'r') ? CT_PP_REGION : CT_PP_ENDREGION);

            prev->SetParentType(pc->GetType());
         }
      }

      // Change 'default(' into a sizeof-like statement
      if (  language_is_set(LANG_CS)
         && pc->Is(CT_DEFAULT)
         && next->Is(CT_PAREN_OPEN))
      {
         pc->SetType(CT_SIZEOF);
      }

      if (  pc->Is(CT_UNSAFE)
         && next->IsNot(CT_BRACE_OPEN))
      {
         pc->SetType(CT_QUALIFIER);
      }

      if (  (  pc->Is(CT_USING)
            || (  pc->Is(CT_TRY)
               && language_is_set(LANG_JAVA)))
         && next->Is(CT_PAREN_OPEN))
      {
         pc->SetType(CT_USING_STMT);
      }

      // Add minimal support for C++0x rvalue references
      if (  pc->Is(CT_BOOL)
         && language_is_set(LANG_CPP)
         && pc->IsString("&&"))
      {
         if (prev->Is(CT_TYPE))
         {
            // Issue # 1002
            if (!pc->TestFlags(PCF_IN_TEMPLATE))
            {
               pc->SetType(CT_BYREF);
            }
         }
      }

      /*
       * HACK: treat try followed by a colon as a qualifier to handle this:
       *   A::A(int) try : B() { } catch (...) { }
       */
      if (  pc->Is(CT_TRY)
         && pc->IsString("try")
         && next->Is(CT_COLON))
      {
         pc->SetType(CT_QUALIFIER);
      }

      /*
       * If Java's 'synchronized' is in a method declaration, it should be
       * a qualifier.
       */
      if (  language_is_set(LANG_JAVA)
         && pc->Is(CT_SYNCHRONIZED)
         && next->IsNot(CT_PAREN_OPEN))
      {
         pc->SetType(CT_QUALIFIER);
      }

      // change CT_DC_MEMBER + CT_FOR into CT_DC_MEMBER + CT_FUNC_CALL
      if (  pc->Is(CT_FOR)
         && pc->GetPrev()->Is(CT_DC_MEMBER))
      {
         pc->SetType(CT_FUNC_CALL);
      }
      // TODO: determine other stuff here

      prev = pc;
      pc   = next;
      next = pc->GetNextNcNnl();
   }
} // tokenize_cleanup


bool invalid_open_angle_template(Chunk *prev)
{
   if (prev == nullptr)
   {
      return(false);
   }
   // A template requires a word/type right before the open angle
   return(  prev->IsNot(CT_WORD)
         && prev->IsNot(CT_TYPE)
         && prev->IsNot(CT_COMMA)
         && prev->IsNot(CT_QUALIFIER)
         && prev->IsNot(CT_OPERATOR_VAL)
         && prev->GetParentType() != CT_OPERATOR);
}


static void check_template(Chunk *start, bool in_type_cast)
{
   LOG_FMT(LTEMPL, "%s(%d): orig_line %zu, orig_col %zu:\n",
           __func__, __LINE__, start->orig_line, start->orig_col);

   Chunk *prev = start->GetPrevNcNnl(E_Scope::PREPROC);

   if (prev->IsNullChunk())
   {
      return;
   }
   Chunk *end;
   Chunk *pc;

   if (prev->Is(CT_TEMPLATE))
   {
      LOG_FMT(LTEMPL, "%s(%d): CT_TEMPLATE:\n", __func__, __LINE__);

      // We have: "template< ... >", which is a template declaration
      size_t level  = 1;
      size_t parens = 0;

      for (pc = start->GetNextNcNnl(E_Scope::PREPROC);
           pc->IsNotNullChunk();
           pc = pc->GetNextNcNnl(E_Scope::PREPROC))
      {
         LOG_FMT(LTEMPL, "%s(%d): type is %s, level is %zu\n",
                 __func__, __LINE__, get_token_name(pc->GetType()), level);

         if (  (pc->str[0] == '>')
            && (pc->Len() > 1))
         {
            if (pc->str[1] == '=')                         // Issue #1462 and #2565
            {
               LOG_FMT(LTEMPL, "%s(%d): do not split '%s' at orig_line %zu, orig_col %zu\n",
                       __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);
            }
            else
            {
               LOG_FMT(LTEMPL, "%s(%d): {split '%s' at orig_line %zu, orig_col %zu}\n",
                       __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);
               split_off_angle_close(pc);
            }
         }

         if (pc->Is(CT_DECLTYPE))
         {
            flag_cpp_decltype(pc);
         }
         else if (pc->Is(CT_PAREN_OPEN))
         {
            ++parens;
         }
         else if (pc->Is(CT_PAREN_CLOSE))
         {
            --parens;
         }

         if (parens == 0)
         {
            if (pc->IsString("<"))
            {
               level++;
            }
            else if (pc->IsString(">"))
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

      if (invalid_open_angle_template(prev))
      {
         LOG_FMT(LTEMPL, "%s(%d): - after type %s + ( - Not a template\n",
                 __func__, __LINE__, get_token_name(prev->GetType()));
         start->SetType(CT_COMPARE);
         return;
      }
      LOG_FMT(LTEMPL, "%s(%d): - prev->GetType() is %s -\n",
              __func__, __LINE__, get_token_name(prev->GetType()));

      // Scan back and make sure we aren't inside square parenthesis
      bool in_if         = false;
      bool hit_semicolon = false;
      pc = start->GetPrevNcNnl(E_Scope::PREPROC);

      while (pc->IsNotNullChunk())
      {
         if (  (  pc->Is(CT_SEMICOLON)
               && hit_semicolon)
            || pc->Is(CT_SQUARE_CLOSE))
         {
            break;
         }

         if (pc->Is(CT_DECLTYPE))
         {
            flag_cpp_decltype(pc);
         }

         if (pc->Is(CT_BRACE_OPEN))
         {
            if (  !pc->TestFlags(PCF_IN_DECLTYPE)
               || !detect_cpp_braced_init_list(pc->GetPrev(), pc))
            {
               break;
            }
            flag_cpp_braced_init_list(pc->GetPrev(), pc);
         }

         if (  pc->Is(CT_BRACE_CLOSE)
            && pc->GetParentType() != CT_BRACED_INIT_LIST
            && !pc->TestFlags(PCF_IN_DECLTYPE))
         {
            break;
         }

         if (  pc->Is(CT_SEMICOLON)
            && !hit_semicolon)
         {
            hit_semicolon = true;
         }

         if (  (  (  pc->Is(CT_IF)
                  || pc->Is(CT_RETURN)
                  || pc->Is(CT_WHILE)
                  || pc->Is(CT_WHILE_OF_DO))
               && !hit_semicolon)
            || (  pc->Is(CT_FOR)
               && hit_semicolon))
         {
            in_if = true;
            break;
         }
         pc = pc->GetPrevNcNnl(E_Scope::PREPROC);
      }
      /*
       * Scan forward to the angle close
       * If we have a comparison in there, then it can't be a template.
       */
      const int max_token_count = 1024;
      E_Token   tokens[max_token_count];
      size_t    num_tokens = 1;

      tokens[0] = CT_ANGLE_OPEN;

      for (pc = start->GetNextNcNnl(E_Scope::PREPROC);
           pc->IsNotNullChunk();
           pc = pc->GetNextNcNnl(E_Scope::PREPROC))
      {
         constexpr static auto LCURRENT = LTEMPL;

         LOG_FMT(LTEMPL, "%s(%d): pc->orig_line is %zu, pc->orig_col is %zu, type is %s, num_tokens is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, get_token_name(pc->GetType()), num_tokens);

         log_rule_B("tok_split_gte");

         if (pc->Is(CT_BRACE_OPEN))                     // Issue #2886
         {
            // look for the closing brace
            Chunk *A = pc->SkipToMatch();
            LOG_FMT(LTEMPL, "%s(%d): A->orig_line is %zu, A->orig_col is %zu, type is %s\n",
                    __func__, __LINE__, A->orig_line, A->orig_col, get_token_name(A->GetType()));
            pc = A->GetNext();
         }

         if (  (tokens[num_tokens - 1] == CT_ANGLE_OPEN)
            && (pc->str[0] == '>')
            && (pc->Len() > 1)
            && (  options::tok_split_gte()
               || (  (  pc->IsString(">>")
                     || pc->IsString(">>>"))
                  && (  num_tokens >= 2
                     || (  num_tokens >= 1
                        && in_type_cast)))))
         {
            LOG_FMT(LTEMPL, "%s(%d): {split '%s' at orig_line %zu, orig_col %zu}\n",
                    __func__, __LINE__, pc->Text(), pc->orig_line, pc->orig_col);

            split_off_angle_close(pc);
         }

         if (pc->IsString("<"))
         {
            if (  num_tokens > 0 && (tokens[num_tokens - 1] == CT_PAREN_OPEN)
               && invalid_open_angle_template(pc->GetPrev()))
            {
               pc->SetType(CT_COMPARE); // Issue #3127
            }
            else
            {
               tokens[num_tokens] = CT_ANGLE_OPEN;
               num_tokens++;
            }
         }
         else if (pc->IsString(">"))
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
                 && (  pc->Is(CT_BOOL)
                    || pc->Is(CT_COMPARE)))
         {
            break;
         }
         else if (pc->Is(CT_BRACE_OPEN))
         {
            if (  !pc->TestFlags(PCF_IN_DECLTYPE)
               || !detect_cpp_braced_init_list(pc->GetPrev(), pc))
            {
               break;
            }
            auto brace_open  = pc->GetNextNcNnl();
            auto brace_close = brace_open->SkipToMatch();

            brace_open->SetParentType(CT_BRACED_INIT_LIST);
            brace_close->SetParentType(CT_BRACED_INIT_LIST);
         }
         else if (  pc->Is(CT_BRACE_CLOSE)
                 && pc->GetParentType() != CT_BRACED_INIT_LIST
                 && !pc->TestFlags(PCF_IN_DECLTYPE))
         {
            break;
         }
         else if (pc->Is(CT_SEMICOLON))
         {
            break;
         }
         else if (pc->Is(CT_PAREN_OPEN))
         {
            if (num_tokens >= max_token_count - 1)
            {
               break;
            }
            tokens[num_tokens] = CT_PAREN_OPEN;
            num_tokens++;
         }
         else if (  pc->Is(CT_QUESTION)                    // Issue #2949
                 && language_is_set(LANG_CPP))
         {
            break;
         }
         else if (pc->Is(CT_PAREN_CLOSE))
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

   if (end->Is(CT_ANGLE_CLOSE))
   {
      pc = end->GetNextNcNnl(E_Scope::PREPROC);

      if (  pc->IsNullChunk()
         || pc->IsNot(CT_NUMBER))
      {
         LOG_FMT(LTEMPL, "%s(%d): Template detected\n", __func__, __LINE__);
         LOG_FMT(LTEMPL, "%s(%d):     from orig_line %zu, orig_col %zu\n",
                 __func__, __LINE__, start->orig_line, start->orig_col);
         LOG_FMT(LTEMPL, "%s(%d):     to   orig_line %zu, orig_col %zu\n",
                 __func__, __LINE__, end->orig_line, end->orig_col);
         start->SetParentType(CT_TEMPLATE);

         check_template_args(start, end);

         end->SetParentType(CT_TEMPLATE);
         end->SetFlagBits(PCF_IN_TEMPLATE);
         return;
      }
   }
   LOG_FMT(LTEMPL, "%s(%d): - Not a template: end = %s\n",
           __func__, __LINE__, (end != nullptr) ? get_token_name(end->GetType()) : "<null>");
   start->SetType(CT_COMPARE);
} // check_template


static void check_template_arg(Chunk *start, Chunk *end)
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
   bool  expressionIsNumeric = false;
   Chunk *pc                 = start;

   while (pc != end)
   {
      Chunk *next = pc->GetNextNcNnl(E_Scope::PREPROC);
      // a test "if (next == nullptr)" is not necessary
      pc->SetFlagBits(PCF_IN_TEMPLATE);

      if (  pc->Is(CT_DECLTYPE)
         || pc->Is(CT_SIZEOF))
      {
         expressionIsNumeric = true;
         break;
      }

      if (next->IsNot(CT_PAREN_OPEN))
      {
         if (  pc->Is(CT_NUMBER)
            || pc->Is(CT_ARITH)
            || pc->Is(CT_SHIFT))
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
         Chunk *next = pc->GetNextNcNnl(E_Scope::PREPROC);
         // a test "if (next == nullptr)" is not necessary
         pc->SetFlagBits(PCF_IN_TEMPLATE);

         Chunk *prev  = pc->GetPrevNcNnl(E_Scope::PREPROC);
         Chunk *prev2 = prev->GetPrevNcNnl(E_Scope::PREPROC);

         if (  prev->Is(CT_ELLIPSIS)                 // Issue #3309
            && prev2->Is(CT_TYPENAME))
         {
            pc->SetType(CT_PARAMETER_PACK);
         }
         else
         {
            make_type(pc);
         }
         pc = next;
      }
   }
} // check_template_arg


static void check_template_args(Chunk *start, Chunk *end)
{
   std::vector<E_Token> tokens;

   // Scan for commas
   Chunk *pc;

   for (pc = start->GetNextNcNnl(E_Scope::PREPROC);
        pc->IsNotNullChunk() && pc != end;
        pc = pc->GetNextNcNnl(E_Scope::PREPROC))
   {
      switch (pc->GetType())
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
         tokens.push_back(pc->GetType());
         break;

      case CT_ANGLE_CLOSE:

         if (  !tokens.empty()
            && tokens.back() == CT_ANGLE_OPEN)
         {
            tokens.pop_back();
         }
         break;

      case CT_PAREN_CLOSE:

         if (  !tokens.empty()
            && tokens.back() == CT_PAREN_OPEN)
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


static void cleanup_objc_property(Chunk *start)
{
   assert(start->Is(CT_OC_PROPERTY));

   Chunk *open_paren = start->GetNextType(CT_PAREN_OPEN, start->level);

   if (open_paren->IsNullChunk())
   {
      LOG_FMT(LTEMPL, "%s(%d): Property is not followed by opening paren\n", __func__, __LINE__);
      return;
   }
   open_paren->SetParentType(start->GetType());

   Chunk *tmp = start->GetNextType(CT_PAREN_CLOSE, start->level);

   if (tmp->IsNotNullChunk())
   {
      tmp->SetParentType(start->GetType());
      tmp = tmp->GetNextNcNnl();

      if (tmp->IsNotNullChunk())
      {
         tmp->SetFlagBits(PCF_STMT_START | PCF_EXPR_START);

         tmp = tmp->GetNextType(CT_SEMICOLON, start->level);

         if (tmp->IsNotNullChunk())
         {
            tmp->SetParentType(start->GetType());
         }
      }
   }
   mark_selectors_in_property_with_open_paren(open_paren);
   mark_attributes_in_property_with_open_paren(open_paren);
}


static void mark_selectors_in_property_with_open_paren(Chunk *open_paren)
{
   assert(open_paren->Is(CT_PAREN_OPEN));

   Chunk *tmp = open_paren;

   while (tmp->IsNot(CT_PAREN_CLOSE))
   {
      if (  tmp->Is(CT_WORD)
         && (  tmp->IsString("setter")
            || tmp->IsString("getter")))
      {
         tmp = tmp->GetNext();

         while (  tmp->IsNotNullChunk()
               && tmp->IsNot(CT_COMMA)
               && tmp->IsNot(CT_PAREN_CLOSE))
         {
            if (  tmp->Is(CT_WORD)
               || tmp->IsString(":"))
            {
               tmp->SetType(CT_OC_SEL_NAME);
            }
            tmp = tmp->GetNext();
         }
      }
      else
      {
         tmp = tmp->GetNext();
      }
   }
}


static void mark_attributes_in_property_with_open_paren(Chunk *open_paren)
{
   assert(open_paren->Is(CT_PAREN_OPEN));

   Chunk *tmp = open_paren;

   while (tmp->IsNot(CT_PAREN_CLOSE))
   {
      Chunk *next = tmp->GetNext();

      if (  (  tmp->Is(CT_COMMA)
            || tmp->Is(CT_PAREN_OPEN))
         && (  next->Is(CT_WORD)
            || next->Is(CT_TYPE)))
      {
         next->SetType(CT_OC_PROPERTY_ATTR);
      }
      tmp = next;
   }
}
