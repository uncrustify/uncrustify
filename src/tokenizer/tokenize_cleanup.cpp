/**
 * @file tokenize_cleanup.cpp
 * Looks at simple sequences to refine the chunk types.
 * Examples:
 *  - change '[' + ']' into '[]'/
 *  - detect "version = 10;" vs "version (xxx) {"
 *
 * @author  Ben Gardner
 * @author  Guy Maurel 2015, 2025
 * @license GPL v2+
 */
#include "tokenizer/tokenize_cleanup.h"

#include "chunk.h"
#include "keywords.h"
#include "log_rules.h"
#include "punctuators.h"
#include "space.h"
#include "tokenizer/check_template.h"
#include "tokenizer/combine.h"
#include "tokenizer/combine_skip.h"
#include "tokenizer/flag_braced_init_list.h"
#include "tokenizer/flag_decltype.h"
#include "unc_ctype.h"
#include "uncrustify.h"


using namespace uncrustify;


/**
 * Marks ObjC specific chunks in property declaration, by setting
 * parent types and chunk types.
 */
static void cleanup_objc_property(Chunk *start);


/**
 * Marks ObjC specific chunks in property declaration (getter/setter attribute)
 * Will mark 'test4Setter'and ':' in '@property (setter=test4Setter:, strong) int test4;' as E_Token::CT_OC_SEL_NAME
 */
static void mark_selectors_in_property_with_open_paren(Chunk *open_paren);


/**
 * Marks ObjC specific chunks in property declaration ( attributes)
 * Changes all the E_Token::CT_WORD and E_Token::CT_TYPE to E_Token::CT_OC_PROPERTY_ATTR
 */
static void mark_attributes_in_property_with_open_paren(Chunk *open_paren);


void split_off_angle_close(Chunk *pc)
{
   const chunk_tag_t *ct = find_punctuator(pc->GetLogText() + 1, cpd.lang_flags);

   if (ct == nullptr)
   {
      return;
   }
   Chunk nc = *pc;

   pc->Text().resize(1);
   pc->SetOrigColEnd(pc->GetOrigCol() + 1);
   pc->SetType(E_Token::CT_ANGLE_CLOSE);

   nc.SetType(ct->type);
   nc.Text().pop_front();
   nc.SetOrigCol(nc.GetOrigCol() + 1);
   nc.SetColumn(nc.GetColumn() + 1);
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
      LOG_FMT(LNOTE, "%s(%d): orig line is %zu, orig col is %zu, text is '%s'\n",
              __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->ElidedText(copy));

      if (  pc->Is(E_Token::CT_MEMBER)
         && (strcmp(pc->GetLogText(), "->") == 0))
      {
         Chunk *tmp = pc->GetPrevNcNnl();
         Chunk *tmp_2;
         Chunk *open_paren;

         if (tmp->Is(E_Token::CT_QUALIFIER))
         {
            // auto max(int a, int b) const -> int;
            // auto f11() const -> bool;
            tmp = tmp->GetPrevNcNnl();
         }
         else if (tmp->Is(E_Token::CT_NOEXCEPT))
         {
            // noexcept is present
            tmp_2 = tmp->GetPrevNcNnl();

            if (tmp_2->Is(E_Token::CT_QUALIFIER))
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
         else if (tmp->Is(E_Token::CT_PAREN_CLOSE))
         {
            open_paren = tmp->GetPrevType(E_Token::CT_PAREN_OPEN, tmp->GetLevel());
            tmp        = open_paren->GetPrevNcNnl();

            if (tmp->Is(E_Token::CT_NOEXCEPT))
            {
               // noexcept is present
               tmp_2 = tmp->GetPrevNcNnl();

               if (tmp_2->Is(E_Token::CT_QUALIFIER))
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
            else if (tmp->Is(E_Token::CT_THROW))
            {
               // throw is present
               tmp_2 = tmp->GetPrevNcNnl();

               if (tmp_2->Is(E_Token::CT_QUALIFIER))
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

         if (  tmp->Is(E_Token::CT_FPAREN_CLOSE)
            && (  tmp->GetParentType() == E_Token::CT_FUNC_PROTO
               || tmp->GetParentType() == E_Token::CT_FUNC_DEF))
         {
            pc->SetType(E_Token::CT_TRAILING_RET);
            LOG_FMT(LNOTE, "%s(%d): set trailing return type for text is '%s'\n",
                    __func__, __LINE__, pc->GetLogText());                  // Issue #3222
            // TODO
            // https://en.cppreference.com/w/cpp/language/function
            // noptr-declarator ( parameter-list ) cv(optional) ref(optional) except(optional) attr(optional) -> trailing
            Chunk *next = pc->GetNextNcNnl();

            if (next->Is(E_Token::CT_DECLTYPE))
            {
               // TODO
            }
            else if (next->Is(E_Token::CT_WORD))
            {
               next->SetType(E_Token::CT_TYPE);                         // Issue #3222
               next = next->GetNextNcNnl();

               if (next->Is(E_Token::CT_ARITH))
               {
                  if (next->GetText()[0] == '*')
                  {
                     next->SetType(E_Token::CT_PTR_TYPE);
                  }
                  else if (next->GetText()[0] == '&')                       // Issue #3407
                  {
                     next->SetType(E_Token::CT_BYREF);
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
      if (pc->Is(E_Token::CT_SQUARE_OPEN))
      {
         next = pc->GetNextNcNnl();

         if (next->Is(E_Token::CT_SQUARE_CLOSE))
         {
            // Change '[' + ']' into '[]'
            pc->SetType(E_Token::CT_TSQUARE);
            pc->Text() = "[]";
            /*
             * bug #664: The original m_origColEnd of E_Token::CT_SQUARE_CLOSE is
             * stored at m_origColEnd of E_Token::CT_TSQUARE.
             * pc->SetOrigColEnd(pc->GetOrigColEnd() + 1);
             */
            pc->SetOrigColEnd(next->GetOrigColEnd());
            Chunk::Delete(next);
         }
      }

      if (  pc->Is(E_Token::CT_SEMICOLON)
         && pc->TestFlags(PCF_IN_PREPROC)
         && !pc->GetNextNcNnl(E_Scope::PREPROC))
      {
         LOG_FMT(LNOTE, "%s(%d): %s:%zu Detected a macro that ends with a semicolon. Possible failures if used.\n",
                 __func__, __LINE__, cpd.filename.c_str(), pc->GetOrigLine());
      }
   }

   // change := to E_Token::CT_SQL_ASSIGN Issue #527
   for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      if (pc->Is(E_Token::CT_COLON))
      {
         next = pc->GetNextNcNnl();

         if (next->Is(E_Token::CT_ASSIGN))
         {
            // Change ':' + '=' into ':='
            pc->SetType(E_Token::CT_SQL_ASSIGN);
            pc->Text() = ":=";
            pc->SetOrigColEnd(next->GetOrigColEnd());
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
      if (  pc->Is(E_Token::CT_DOT)
         && language_is_set(lang_flag_e::LANG_ALLC))
      {
         pc->SetType(E_Token::CT_MEMBER);
      }

      // Determine the version stuff (D only)
      if (pc->Is(E_Token::CT_D_VERSION))
      {
         if (next->Is(E_Token::CT_PAREN_OPEN))
         {
            pc->SetType(E_Token::CT_D_VERSION_IF);
         }
         else
         {
            if (next->IsNot(E_Token::CT_ASSIGN))
            {
               LOG_FMT(LERR, "%s(%d): %s:%zu: version: Unexpected token %s\n",
                       __func__, __LINE__, cpd.filename.c_str(), pc->GetOrigLine(), get_token_name(next->GetType()));
               exit(EX_SOFTWARE);
            }
            pc->SetType(E_Token::CT_WORD);
         }
      }

      // Determine the scope stuff (D only)
      if (pc->Is(E_Token::CT_D_SCOPE))
      {
         if (next->Is(E_Token::CT_PAREN_OPEN))
         {
            pc->SetType(E_Token::CT_D_SCOPE_IF);
         }
         else
         {
            pc->SetType(E_Token::CT_TYPE);
         }
      }

      /*
       * Change E_Token::CT_BASE before E_Token::CT_PAREN_OPEN to E_Token::CT_WORD.
       * public myclass() : base() {}
       * -or-
       * var x = (T)base.y;
       */
      if (  pc->Is(E_Token::CT_BASE)
         && (  next->Is(E_Token::CT_PAREN_OPEN)
            || next->Is(E_Token::CT_DOT)))
      {
         pc->SetType(E_Token::CT_WORD);
      }

      if (  pc->Is(E_Token::CT_ENUM)
         && (  next->Is(E_Token::CT_STRUCT)
            || next->Is(E_Token::CT_CLASS)))
      {
         next->SetType(E_Token::CT_ENUM_CLASS);
      }
      Chunk *next_non_attr = language_is_set(lang_flag_e::LANG_CPP) ? skip_attribute_next(next) : next;

      /*
       * Change E_Token::CT_WORD after E_Token::CT_ENUM, E_Token::CT_UNION, E_Token::CT_STRUCT, or E_Token::CT_CLASS to E_Token::CT_TYPE
       * Change E_Token::CT_WORD before E_Token::CT_WORD to E_Token::CT_TYPE
       */
      if (next_non_attr->Is(E_Token::CT_WORD))
      {
         if (pc->IsClassEnumStructOrUnion())
         {
            next_non_attr->SetType(E_Token::CT_TYPE);
         }

         if (pc->Is(E_Token::CT_WORD))
         {
            pc->SetType(E_Token::CT_TYPE);
         }
      }

      /*
       * change extern to qualifier if extern isn't followed by a string or
       * an open parenthesis
       */
      if (pc->Is(E_Token::CT_EXTERN))
      {
         if (next->Is(E_Token::CT_STRING))
         {
            // Probably 'extern "C"'
         }
         else if (next->Is(E_Token::CT_PAREN_OPEN))
         {
            // Probably 'extern (C)'
         }
         else
         {
            // Something else followed by a open brace
            Chunk *tmp = next->GetNextNcNnl();

            if (  tmp->IsNullChunk()
               || tmp->IsNot(E_Token::CT_BRACE_OPEN))
            {
               pc->SetType(E_Token::CT_QUALIFIER);
            }
         }
      }

      /*
       * Change E_Token::CT_STAR to E_Token::CT_PTR_TYPE if preceded by
       *     E_Token::CT_TYPE, E_Token::CT_QUALIFIER, or E_Token::CT_PTR_TYPE
       * or by a
       *     E_Token::CT_WORD which is preceded by E_Token::CT_DC_MEMBER: '::aaa *b'
       */
      if (  (next->Is(E_Token::CT_STAR))
         || (  language_is_set(lang_flag_e::LANG_CPP)
            && (next->Is(E_Token::CT_CARET)))
         || (  (  language_is_set(lang_flag_e::LANG_CS)
               || language_is_set(lang_flag_e::LANG_VALA))
            && (next->Is(E_Token::CT_QUESTION))
            && (strcmp(pc->GetLogText(), "null") != 0)))
      {
         if (  pc->Is(E_Token::CT_TYPE)
            || pc->Is(E_Token::CT_QUALIFIER)
            || pc->Is(E_Token::CT_PTR_TYPE))
         {
            next->SetType(E_Token::CT_PTR_TYPE);
         }
         else if (  (  language_is_set(lang_flag_e::LANG_CS)
                    || language_is_set(lang_flag_e::LANG_VALA))
                 && next->Is(E_Token::CT_QUESTION))
         {
            Chunk *tmp = next->GetNextNcNnl();

            if (tmp->Is(E_Token::CT_TSQUARE))
            {
               // word?[]   Array of nullables.
               next->SetType(E_Token::CT_PTR_TYPE);
            }
         }
      }

      if (  pc->Is(E_Token::CT_TYPE_CAST)
         && next->Is(E_Token::CT_ANGLE_OPEN))
      {
         next->SetParentType(E_Token::CT_TYPE_CAST);
         in_type_cast = true;
      }

      if (pc->Is(E_Token::CT_DECLTYPE))
      {
         flag_cpp_decltype(pc);
      }

      // Change angle open/close to E_Token::CT_COMPARE, if not a template thingy
      if (  pc->Is(E_Token::CT_ANGLE_OPEN)
         && pc->GetParentType() != E_Token::CT_TYPE_CAST)
      {
         /*
          * pretty much all languages except C use <> for something other than
          * comparisons.  "#include<xxx>" is handled elsewhere.
          */
         if (  language_is_set(lang_flag_e::LANG_OC)
            || language_is_set(lang_flag_e::LANG_CPP)
            || language_is_set(lang_flag_e::LANG_CS)
            || language_is_set(lang_flag_e::LANG_JAVA)
            || language_is_set(lang_flag_e::LANG_VALA))
         {
            // bug #663
            check_template(pc, in_type_cast);
         }
         else
         {
            // convert E_Token::CT_ANGLE_OPEN to E_Token::CT_COMPARE
            pc->SetType(E_Token::CT_COMPARE);
         }
      }

      if (  pc->Is(E_Token::CT_ANGLE_CLOSE)
         && pc->GetParentType() != E_Token::CT_TEMPLATE)
      {
         if (in_type_cast)
         {
            in_type_cast = false;
            pc->SetParentType(E_Token::CT_TYPE_CAST);
         }
         else
         {
            next = handle_double_angle_close(pc);
         }
      }

      if (language_is_set(lang_flag_e::LANG_D))
      {
         // Check for the D string concat symbol '~'
         if (  pc->Is(E_Token::CT_INV)
            && (  prev->Is(E_Token::CT_STRING)
               || prev->Is(E_Token::CT_WORD)
               || next->Is(E_Token::CT_STRING)))
         {
            pc->SetType(E_Token::CT_CONCAT);
         }

         // Check for the D template symbol '!' (word + '!' + word or '(')
         if (  pc->Is(E_Token::CT_NOT)
            && prev->Is(E_Token::CT_WORD)
            && (  next->Is(E_Token::CT_PAREN_OPEN)
               || next->Is(E_Token::CT_WORD)
               || next->Is(E_Token::CT_TYPE)
               || next->Is(E_Token::CT_NUMBER)
               || next->Is(E_Token::CT_NUMBER_FP)
               || next->Is(E_Token::CT_STRING)
               || next->Is(E_Token::CT_STRING_MULTI)))
         {
            pc->SetType(E_Token::CT_D_TEMPLATE);
         }

         // handle "version(unittest) { }" vs "unittest { }"
         if (  pc->Is(E_Token::CT_UNITTEST)
            && prev->Is(E_Token::CT_PAREN_OPEN))
         {
            pc->SetType(E_Token::CT_WORD);
         }

         // handle 'static if' and merge the tokens
         if (  pc->Is(E_Token::CT_IF)
            && prev->IsString("static"))
         {
            // delete PREV and merge with IF
            pc->Text().insert(0, ' ');
            pc->Text().insert(0, prev->GetText());
            pc->SetOrigCol(prev->GetOrigCol());
            pc->SetOrigLine(prev->GetOrigLine());
            Chunk *to_be_deleted = prev;
            prev = prev->GetPrevNcNnl();

            if (prev->IsNotNullChunk())
            {
               Chunk::Delete(to_be_deleted);
            }
         }
      }

      if (language_is_set(lang_flag_e::LANG_CPP))
      {
         // Change Word before '::' into a type
         if (  pc->Is(E_Token::CT_WORD)
            && next->Is(E_Token::CT_DC_MEMBER))
         {
            prev = pc->GetPrev();

            if (prev->IsNullChunk())                  // Issue #3010
            {
               pc->SetType(E_Token::CT_TYPE);
            }
            else
            {
               if (prev->Is(E_Token::CT_COLON))
               {
                  // nothing to do
               }
               else
               {
                  pc->SetType(E_Token::CT_TYPE);
               }
            }
         }

         // Set parent type for 'if constexpr'
         if (  prev->Is(E_Token::CT_IF)
            && pc->Is(E_Token::CT_QUALIFIER)
            && pc->IsString("constexpr"))
         {
            pc->SetType(E_Token::CT_CONSTEXPR);
         }
      }

      // Change get/set to E_Token::CT_WORD if not followed by a brace open
      if (  pc->Is(E_Token::CT_GETSET)
         && next->IsNot(E_Token::CT_BRACE_OPEN))
      {
         if (  next->Is(E_Token::CT_SEMICOLON)
            && (  prev->Is(E_Token::CT_BRACE_CLOSE)
               || prev->Is(E_Token::CT_BRACE_OPEN)
               || prev->Is(E_Token::CT_SEMICOLON)))
         {
            pc->SetType(E_Token::CT_GETSET_EMPTY);
            next->SetParentType(E_Token::CT_GETSET);
         }
         else
         {
            pc->SetType(E_Token::CT_WORD);
         }
      }

      /*
       * Interface is only a keyword in MS land if followed by 'class' or 'struct'
       * likewise, 'class' may be a member name in Java.
       */
      if (  pc->Is(E_Token::CT_CLASS)
         && !CharTable::IsKw1(next->GetText()[0]))
      {
         if (  next->IsNot(E_Token::CT_DC_MEMBER)
            && next->IsNot(E_Token::CT_ATTRIBUTE))                       // Issue #2570
         {
            pc->SetType(E_Token::CT_WORD);
         }
         else if (  prev->Is(E_Token::CT_DC_MEMBER)
                 || prev->Is(E_Token::CT_TYPE))
         {
            pc->SetType(E_Token::CT_TYPE);
         }
         else if (next->Is(E_Token::CT_DC_MEMBER))
         {
            Chunk *next2 = next->GetNextNcNnlNet();

            if (  next2->Is(E_Token::CT_INV)      // E_Token::CT_INV hasn't turned into E_Token::CT_DESTRUCTOR just yet
               || (  next2->Is(E_Token::CT_CLASS) // constructor isn't turned into E_Token::CT_FUNC* just yet
                  && !strcmp(pc->GetLogText(), next2->GetLogText())))
            {
               pc->SetType(E_Token::CT_TYPE);
            }
         }
      }

      /*
       * Change item after operator (>=, ==, etc) to a E_Token::CT_OPERATOR_VAL
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
      if (pc->Is(E_Token::CT_OPERATOR))
      {
         Chunk *tmp2 = next->GetNext();

         // Handle special case of () operator -- [] already handled
         if (next->Is(E_Token::CT_PAREN_OPEN))
         {
            Chunk *tmp = next->GetNext();

            if (tmp->Is(E_Token::CT_PAREN_CLOSE))
            {
               next->Text() = "()";
               next->SetType(E_Token::CT_OPERATOR_VAL);
               Chunk::Delete(tmp);
               next->SetOrigColEnd(next->GetOrigColEnd() + 1);
            }
         }
         else if (  next->Is(E_Token::CT_ANGLE_CLOSE)
                 && tmp2->Is(E_Token::CT_ANGLE_CLOSE)
                 && tmp2->GetOrigCol() == next->GetOrigColEnd())
         {
            next->Text().append('>');
            next->SetOrigColEnd(next->GetOrigColEnd() + 1);
            next->SetType(E_Token::CT_OPERATOR_VAL);
            Chunk::Delete(tmp2);
         }
         else if (next->TestFlags(PCF_PUNCTUATOR))
         {
            next->SetType(E_Token::CT_OPERATOR_VAL);
         }
         else
         {
            next->SetType(E_Token::CT_TYPE);

            /*
             * Replace next with a collection of all tokens that are part of
             * the type.
             */
            tmp2 = next;
            Chunk *tmp;

            while ((tmp = tmp2->GetNext())->IsNotNullChunk())
            {
               if (  tmp->IsNot(E_Token::CT_WORD)
                  && tmp->IsNot(E_Token::CT_TYPE)
                  && tmp->IsNot(E_Token::CT_QUALIFIER)
                  && tmp->IsNot(E_Token::CT_STAR)
                  && tmp->IsNot(E_Token::CT_CARET)
                  && tmp->IsNot(E_Token::CT_AMP)
                  && tmp->IsNot(E_Token::CT_TSQUARE))
               {
                  break;
               }
               // Change tmp into a type so that space_needed() works right
               make_type(tmp);
               size_t num_sp = space_needed(tmp2, tmp);

               while (num_sp-- > 0)
               {
                  next->Text().append(" ");
               }
               next->Text().append(tmp->GetText());
               tmp2 = tmp;
            }

            while ((tmp2 = next->GetNext()) != tmp)
            {
               Chunk::Delete(tmp2);
            }
            next->SetType(E_Token::CT_OPERATOR_VAL);

            next->SetOrigColEnd(next->GetOrigCol() + next->Len());
         }
         next->SetParentType(E_Token::CT_OPERATOR);

         LOG_FMT(LOPERATOR, "%s(%d): %zu:%zu operator '%s'\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), next->GetLogText());
      }

      // Change private, public, protected into either a qualifier or label
      if (pc->Is(E_Token::CT_ACCESS))
      {
         // Handle Qt slots - maybe should just check for a E_Token::CT_WORD?
         if (  next->IsString("slots")
            || next->IsString("Q_SLOTS"))
         {
            Chunk *tmp = next->GetNext();

            if (tmp->Is(E_Token::CT_COLON))
            {
               next = tmp;
            }
         }

         if (next->Is(E_Token::CT_COLON))
         {
            next->SetType(E_Token::CT_ACCESS_COLON);
            Chunk *tmp;

            if ((tmp = next->GetNextNcNnl())->IsNotNullChunk())
            {
               tmp->SetFlagBits(PCF_STMT_START | PCF_EXPR_START);
               log_ruleStart("start statementi/ expression", tmp);
            }
         }
         else
         {
            pc->SetType((  pc->IsString("signals")
                        || pc->IsString("Q_SIGNALS"))
                           ? E_Token::CT_WORD : E_Token::CT_QUALIFIER);
         }
      }

      // Look for <newline> 'EXEC' 'SQL'
      if (  (  pc->IsString("EXEC", false)
            && next->IsString("SQL", false))
         || (  (*pc->GetText().GetLogText() == '$')
            && pc->IsNot(E_Token::CT_SQL_WORD)
               /* but avoid breaking tokenization for C# 6 interpolated strings. */
            && (  !language_is_set(lang_flag_e::LANG_CS)
               || (  pc->Is(E_Token::CT_STRING)
                  && (!pc->GetText().startswith("$\""))
                  && (!pc->GetText().startswith("$@\""))))))
      {
         Chunk *tmp = pc->GetPrev();

         if (tmp->IsNewline())
         {
            if (*pc->GetText().GetLogText() == '$')
            {
               pc->SetType(E_Token::CT_SQL_EXEC);

               if (pc->Len() > 1)
               {
                  // SPLIT OFF '$'
                  Chunk nc;

                  nc = *pc;
                  pc->Text().resize(1);
                  pc->SetOrigColEnd(pc->GetOrigCol() + 1);

                  nc.SetType(E_Token::CT_SQL_WORD);
                  nc.Text().pop_front();
                  nc.SetOrigCol(nc.GetOrigCol() + 1);
                  nc.SetColumn(nc.GetColumn() + 1);
                  nc.CopyAndAddAfter(pc);

                  next = pc->GetNext();
               }
            }
            tmp = next->GetNext();

            if (tmp->IsString("BEGIN", false))
            {
               pc->SetType(E_Token::CT_SQL_BEGIN);
            }
            else if (tmp->IsString("END", false))
            {
               pc->SetType(E_Token::CT_SQL_END);
            }
            else
            {
               pc->SetType(E_Token::CT_SQL_EXEC);
            }

            // Change words into E_Token::CT_SQL_WORD until E_Token::CT_SEMICOLON
            while (tmp->IsNotNullChunk())
            {
               if (tmp->Is(E_Token::CT_SEMICOLON))
               {
                  break;
               }

               if (  (tmp->Len() > 0)
                  && (  unc_isalpha(*tmp->GetText().GetLogText())
                     || (*tmp->GetText().GetLogText() == '$')))
               {
                  tmp->SetType(E_Token::CT_SQL_WORD);
               }
               tmp = tmp->GetNextNcNnl();
            }
         }
      }

      // handle MS abomination 'for each'
      if (  pc->Is(E_Token::CT_FOR)
         && next->IsString("each")
         && (next == pc->GetNext()))
      {
         // merge the two with a space between
         pc->Text().append(' ');
         pc->Text() += next->GetText();
         pc->SetOrigColEnd(next->GetOrigColEnd());
         Chunk::Delete(next);
         next = pc->GetNextNcNnl();

         // label the 'in'
         if (next->Is(E_Token::CT_PAREN_OPEN))
         {
            Chunk *tmp = next->GetNextNcNnl();

            while (  tmp->IsNotNullChunk()
                  && tmp->IsNot(E_Token::CT_PAREN_CLOSE))
            {
               if (tmp->IsString("in"))
               {
                  tmp->SetType(E_Token::CT_IN);
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
      if (language_is_set(lang_flag_e::LANG_OC))
      {
         if (  (  pc->Is(E_Token::CT_IF)
               || pc->Is(E_Token::CT_FOR)
               || pc->Is(E_Token::CT_WHILE))
            && !next->Is(E_Token::CT_PAREN_OPEN))
         {
            pc->SetType(E_Token::CT_WORD);
         }

         if (  pc->Is(E_Token::CT_DO)
            && (  prev->Is(E_Token::CT_MINUS)
               || next->Is(E_Token::CT_SQUARE_CLOSE)))
         {
            pc->SetType(E_Token::CT_WORD);
         }

         // Fix self keyword back to word when mixing c++/objective-c
         if (  pc->Is(E_Token::CT_THIS)
            && !strcmp(pc->GetLogText(), "self")
            && (  next->Is(E_Token::CT_COMMA)
               || next->Is(E_Token::CT_PAREN_CLOSE)))
         {
            pc->SetType(E_Token::CT_WORD);
         }

         // Fix self keyword back to word when mixing c++/objective-c
         if (  pc->Is(E_Token::CT_THIS)
            && !strcmp(pc->GetLogText(), "self")
            && (  next->Is(E_Token::CT_COMMA)
               || next->Is(E_Token::CT_PAREN_CLOSE)))
         {
            pc->SetType(E_Token::CT_WORD);
         }
      }

      // Vala allows keywords to be used as identifiers
      if (language_is_set(lang_flag_e::LANG_VALA))
      {
         if (  find_keyword_type(pc->GetLogText(), pc->Len()) != E_Token::CT_WORD
            && (  prev->Is(E_Token::CT_DOT)
               || next->Is(E_Token::CT_DOT)
               || prev->Is(E_Token::CT_MEMBER)
               || next->Is(E_Token::CT_MEMBER)
               || prev->Is(E_Token::CT_TYPE)))
         {
            pc->SetType(E_Token::CT_WORD);
         }
      }

      // Another hack to clean up more keyword abuse
      if (  pc->Is(E_Token::CT_CLASS)
         && (  prev->Is(E_Token::CT_DOT)
            || next->Is(E_Token::CT_DOT)
            || prev->Is(E_Token::CT_MEMBER)  // Issue #3031
            || next->Is(E_Token::CT_MEMBER)))
      {
         pc->SetType(E_Token::CT_WORD);
      }

      // Detect Objective C class name
      if (  pc->Is(E_Token::CT_OC_IMPL)
         || pc->Is(E_Token::CT_OC_INTF)
         || pc->Is(E_Token::CT_OC_PROTOCOL))
      {
         if (next->IsNot(E_Token::CT_PAREN_OPEN))
         {
            next->SetType(E_Token::CT_OC_CLASS);
         }
         next->SetParentType(pc->GetType());

         Chunk *tmp = next->GetNextNcNnl();

         if (tmp->IsNotNullChunk())
         {
            tmp->SetFlagBits(PCF_STMT_START | PCF_EXPR_START);
            log_ruleStart("start statementi/ expression", tmp);
         }
         tmp = pc->GetNextType(E_Token::CT_OC_END, pc->GetLevel());

         if (tmp->IsNotNullChunk())
         {
            tmp->SetParentType(pc->GetType());
         }
      }

      if (pc->Is(E_Token::CT_OC_INTF))
      {
         Chunk *tmp = pc->GetNextNcNnl(E_Scope::PREPROC);

         while (  tmp->IsNotNullChunk()
               && tmp->IsNot(E_Token::CT_OC_END))
         {
            if (get_token_pattern_class(tmp->GetType()) != pattern_class_e::NONE)
            {
               LOG_FMT(LOBJCWORD, "%s(%d): @interface %zu:%zu change '%s' (%s) to E_Token::CT_WORD\n",
                       __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), tmp->GetLogText(),
                       get_token_name(tmp->GetType()));
               tmp->SetType(E_Token::CT_WORD);
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
      if (  (  pc->GetParentType() == E_Token::CT_OC_IMPL
            || pc->GetParentType() == E_Token::CT_OC_INTF
            || pc->Is(E_Token::CT_OC_CLASS))
         && next->Is(E_Token::CT_PAREN_OPEN))
      {
         next->SetParentType(pc->GetParentType());

         Chunk *tmp = next->GetNext();

         if (  tmp->IsNotNullChunk()
            && tmp->GetNext()->IsNotNullChunk())
         {
            if (tmp->Is(E_Token::CT_PAREN_CLOSE))
            {
               //tmp->SetType(E_Token::CT_OC_CLASS_EXT);
               tmp->SetParentType(pc->GetParentType());
            }
            else
            {
               tmp->SetType(E_Token::CT_OC_CATEGORY);
               tmp->SetParentType(pc->GetParentType());
            }
         }
         tmp = pc->GetNextType(E_Token::CT_PAREN_CLOSE, pc->GetLevel());

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
      if (pc->Is(E_Token::CT_OC_PROPERTY))
      {
         if (next->IsNot(E_Token::CT_PAREN_OPEN))
         {
            next->SetFlagBits(PCF_STMT_START | PCF_EXPR_START);
            log_ruleStart("start statement/ expression", next);
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
      if (  pc->Is(E_Token::CT_OC_SEL)
         && next->Is(E_Token::CT_PAREN_OPEN))
      {
         next->SetParentType(pc->GetType());

         Chunk *tmp = next->GetNext();

         if (tmp->IsNotNullChunk())
         {
            tmp->SetType(E_Token::CT_OC_SEL_NAME);
            tmp->SetParentType(pc->GetType());

            while ((tmp = tmp->GetNextNcNnl())->IsNotNullChunk())
            {
               if (tmp->Is(E_Token::CT_PAREN_CLOSE))
               {
                  tmp->SetParentType(E_Token::CT_OC_SEL);
                  break;
               }
               tmp->SetType(E_Token::CT_OC_SEL_NAME);
               tmp->SetParentType(pc->GetType());
            }
         }
      }

      // Handle special preprocessor junk
      if (pc->Is(E_Token::CT_PREPROC))
      {
         pc->SetParentType(next->GetType());
      }

      // Detect "pragma region" and "pragma endregion"
      if (  pc->Is(E_Token::CT_PP_PRAGMA)
         && next->Is(E_Token::CT_PREPROC_BODY))
      {
         if (  (strncmp(next->GetText().GetLogText(), "region", 6) == 0)
            || (strncmp(next->GetText().GetLogText(), "endregion", 9) == 0))
         // TODO: probably better use strncmp
         {
            pc->SetType((*next->GetText().GetLogText() == 'r') ? E_Token::CT_PP_REGION : E_Token::CT_PP_ENDREGION);

            prev->SetParentType(pc->GetType());
         }
      }

      // Change 'default(' into a sizeof-like statement
      if (  language_is_set(lang_flag_e::LANG_CS)
         && pc->Is(E_Token::CT_DEFAULT)
         && next->Is(E_Token::CT_PAREN_OPEN))
      {
         pc->SetType(E_Token::CT_SIZEOF);
      }

      if (  pc->Is(E_Token::CT_UNSAFE)
         && next->IsNot(E_Token::CT_BRACE_OPEN))
      {
         pc->SetType(E_Token::CT_QUALIFIER);
      }

      if (  (  pc->Is(E_Token::CT_USING)
            || (  pc->Is(E_Token::CT_TRY)
               && language_is_set(lang_flag_e::LANG_JAVA)))
         && next->Is(E_Token::CT_PAREN_OPEN))
      {
         pc->SetType(E_Token::CT_USING_STMT);
      }

      /*
       * HACK: treat try followed by a colon as a qualifier to handle this:
       *   A::A(int) try : B() { } catch (...) { }
       */
      if (  pc->Is(E_Token::CT_TRY)
         && pc->IsString("try")
         && next->Is(E_Token::CT_COLON))
      {
         pc->SetType(E_Token::CT_QUALIFIER);
      }

      /*
       * If Java's 'synchronized' is in a method declaration, it should be
       * a qualifier.
       */
      if (  language_is_set(lang_flag_e::LANG_JAVA)
         && pc->Is(E_Token::CT_SYNCHRONIZED)
         && next->IsNot(E_Token::CT_PAREN_OPEN))
      {
         pc->SetType(E_Token::CT_QUALIFIER);
      }

      // change E_Token::CT_DC_MEMBER + E_Token::CT_FOR into E_Token::CT_DC_MEMBER + E_Token::CT_FUNC_CALL
      if (  pc->Is(E_Token::CT_FOR)
         && pc->GetPrev()->Is(E_Token::CT_DC_MEMBER))
      {
         pc->SetType(E_Token::CT_FUNC_CALL);
      }

      // Mark tokens inside macro-no-format-args macros
      // When we find E_Token::CT_MACRO_NO_FMT_ARGS, mark all tokens inside its parentheses
      // including the macro name, opening paren, all contents, and closing paren
      if (pc->Is(E_Token::CT_MACRO_NO_FMT_ARGS))
      {
         // Find the opening paren
         Chunk *open_paren = next;

         // Skip any whitespace/newlines/comments between macro name and paren
         while (  open_paren->IsNotNullChunk()
               && (  open_paren->Is(E_Token::CT_NEWLINE)
                  || open_paren->Is(E_Token::CT_COMMENT)
                  || open_paren->Is(E_Token::CT_COMMENT_CPP)
                  || open_paren->Is(E_Token::CT_COMMENT_MULTI)))
         {
            open_paren = open_paren->GetNext();
         }

         if (  open_paren->IsNotNullChunk()
            && (  open_paren->Is(E_Token::CT_PAREN_OPEN)
               || open_paren->Is(E_Token::CT_FPAREN_OPEN)))
         {
            // Mark the macro name itself
            pc->SetFlagBits(PCF_IN_MACRO_NO_FMT_ARGS);

            // Mark the opening paren (to prevent space between macro name and paren)
            open_paren->SetFlagBits(PCF_IN_MACRO_NO_FMT_ARGS);

            // Find matching close paren and mark everything in between
            size_t paren_level = 1;
            Chunk  *tmp        = open_paren->GetNext();

            while (tmp->IsNotNullChunk() && paren_level > 0)
            {
               if (  tmp->Is(E_Token::CT_PAREN_OPEN)
                  || tmp->Is(E_Token::CT_FPAREN_OPEN))
               {
                  paren_level++;
               }
               else if (  tmp->Is(E_Token::CT_PAREN_CLOSE)
                       || tmp->Is(E_Token::CT_FPAREN_CLOSE))
               {
                  paren_level--;
               }
               // Mark this chunk as being inside the macro args
               // Also mark the closing paren (paren_level == 0 means we just hit it)
               tmp->SetFlagBits(PCF_IN_MACRO_NO_FMT_ARGS);
               tmp = tmp->GetNext();
            }
         }
      }
      // TODO: determine other stuff here

      prev = pc;
      pc   = next;
      next = pc->GetNextNcNnl();
   }
} // tokenize_cleanup


static void cleanup_objc_property(Chunk *start)
{
   assert(start->Is(E_Token::CT_OC_PROPERTY));

   Chunk *open_paren = start->GetNextType(E_Token::CT_PAREN_OPEN, start->GetLevel());

   if (open_paren->IsNullChunk())
   {
      LOG_FMT(LTEMPL, "%s(%d): Property is not followed by opening paren\n", __func__, __LINE__);
      return;
   }
   open_paren->SetParentType(start->GetType());

   Chunk *tmp = start->GetNextType(E_Token::CT_PAREN_CLOSE, start->GetLevel());

   if (tmp->IsNotNullChunk())
   {
      tmp->SetParentType(start->GetType());
      tmp = tmp->GetNextNcNnl();

      if (tmp->IsNotNullChunk())
      {
         tmp->SetFlagBits(PCF_STMT_START | PCF_EXPR_START);
         log_ruleStart("start statement/ expression", tmp);

         tmp = tmp->GetNextType(E_Token::CT_SEMICOLON, start->GetLevel());

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
   assert(open_paren->Is(E_Token::CT_PAREN_OPEN));

   Chunk *tmp = open_paren;

   while (tmp->IsNot(E_Token::CT_PAREN_CLOSE))
   {
      if (  tmp->Is(E_Token::CT_WORD)
         && (  tmp->IsString("setter")
            || tmp->IsString("getter")))
      {
         tmp = tmp->GetNext();

         while (  tmp->IsNotNullChunk()
               && tmp->IsNot(E_Token::CT_COMMA)
               && tmp->IsNot(E_Token::CT_PAREN_CLOSE))
         {
            if (  tmp->Is(E_Token::CT_WORD)
               || tmp->IsString(":"))
            {
               tmp->SetType(E_Token::CT_OC_SEL_NAME);
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
   assert(open_paren->Is(E_Token::CT_PAREN_OPEN));

   Chunk *tmp = open_paren;

   while (tmp->IsNot(E_Token::CT_PAREN_CLOSE))
   {
      Chunk *next = tmp->GetNext();

      if (  (  tmp->Is(E_Token::CT_COMMA)
            || tmp->Is(E_Token::CT_PAREN_OPEN))
         && (  next->Is(E_Token::CT_WORD)
            || next->Is(E_Token::CT_TYPE)))
      {
         next->SetType(E_Token::CT_OC_PROPERTY_ATTR);
      }
      tmp = next;
   }
}
