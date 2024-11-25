/**
 * @file space.cpp
 * Adds or removes inter-chunk spaces.
 *
 * Information
 *   "Ignore" means do not change it.
 *   "Add" in the context of spaces means make sure there is at least 1.
 *   "Add" elsewhere means make sure one is present.
 *   "Remove" mean remove the space/brace/newline/etc.
 *   "Force" in the context of spaces means ensure that there is exactly 1.
 *   "Force" in other contexts means the same as "add".
 *
 *   Rmk: spaces = space + nl
 *
 * @author  Ben Gardner
 * @author  Guy Maurel, 2015-2023
 * @license GPL v2+
 */
#include "space.h"

#include "add_space_table.h"
#include "log_rules.h"
#include "options.h"
#include "options_for_QT.h"
#include "punctuators.h"
#include "token_is_within_trailing_return.h"

#ifdef WIN32
#include <algorithm>                   // to get max
#endif // ifdef WIN32


constexpr static auto LCURRENT = LSPACE;

using namespace std;
using namespace uncrustify;


/**
 * Ensure to force the space between the \a first and the \a second chunks
 * if the PCF_FORCE_SPACE flag is set in the \a first.
 *
 * @param first   The first chunk
 * @param second  The second chunk
 * @param av      Av from the do_space()
 *
 * @return IARF_IGNORE, IARF_ADD, IARF_REMOVE or IARF_FORCE
 */
static iarf_e ensure_force_space(Chunk *first, Chunk *second, iarf_e av);


/**
 * Decides how to change inter-chunk spacing.
 * Note that the order of the if statements is VERY important.
 *
 * @param first   The first chunk
 * @param second  The second chunk
 *
 * @return IARF_IGNORE, IARF_ADD, IARF_REMOVE or IARF_FORCE
 *
 * This function is called for every chunk in the input file,
 * thus it is important to keep this function efficient
 */
static iarf_e do_space(Chunk *first, Chunk *second, int &min_sp)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LSPACE, "%s(%d): first:  orig line %zu, orig col %zu, text '%s', type %s\n",
           __func__, __LINE__, first->GetOrigLine(), first->GetOrigCol(), first->Text(), get_token_name(first->GetType()));
   LOG_FMT(LSPACE, "%s(%d): second: orig line %zu, orig col %zu, text '%s', type %s\n",
           __func__, __LINE__, second->GetOrigLine(), second->GetOrigCol(), second->Text(), get_token_name(second->GetType()));

   min_sp = 1;

   if (  first->Is(CT_COMMENT)                                      // Issue #4327
      && first->GetParentType() == CT_COMMENT_EMBED
      && (options::sp_emb_cmt_priority()))
   {
      // Add or remove space bewteen an embedded comment and a close parenthesis.
      log_rule("sp_emb_cmt_priority (after)");
      min_sp = options::sp_num_after_emb_cmt();
      return(options::sp_after_emb_cmt());
   }

   if (  second->Is(CT_COMMENT)                                     // Issue #4327
      && second->GetParentType() == CT_COMMENT_EMBED
      && (options::sp_emb_cmt_priority()))
   {
      // Add or remove space bewteen an open parenthesis and an embedded comment.
      log_rule("sp_between_open_paren_and_emb_cmt (before)");
      min_sp = options::sp_num_before_emb_cmt();
      return(options::sp_before_emb_cmt());
   }

   if (  first->Is(CT_VBRACE_OPEN)
      && first->GetPrev()->Is(CT_SPAREN_CLOSE)
      && second->IsNot(CT_SEMICOLON))
   {
      // Add or remove space after ')' of control statements.
      log_rule("sp_after_sparen");
      return(options::sp_after_sparen());
   }

   if (first->Is(CT_SPAREN_CLOSE))
   {
      if (second->Is(CT_VBRACE_OPEN))
      {
         // Fix spacing between SPAREN_CLOSE and VBRACE_OPEN tokens as we don't want the default behavior (which is ADD).
         log_rule("REMOVE");
         return(IARF_REMOVE);
      }

      if (  options::sp_sparen_brace() == IARF_IGNORE
         && second->Is(CT_BRACE_OPEN))
      {
         // Do sp_after_sparen if sp_sparen_brace is ignored. No need for VBRACE_OPEN spacing.
         log_rule("sp_after_sparen");
         return(options::sp_after_sparen());
      }
   }

   if (  first->Is(CT_PP_IGNORE)
      && second->Is(CT_PP_IGNORE))
   {
      // Leave spacing alone between PP_IGNORE tokens as we don't want the default behavior (which is ADD).
      log_rule("PP_IGNORE");
      return(IARF_IGNORE);
   }

   if (  first->Is(CT_PP)
      || second->Is(CT_PP))
   {
      // Add or remove space around preprocessor '##' concatenation operator.
      log_rule("sp_pp_concat");
      return(options::sp_pp_concat());
   }

   if (first->Is(CT_POUND))
   {
      // Add or remove space after preprocessor '#' stringify operator.
      // Also affects the '#@' charizing operator.
      log_rule("sp_pp_stringify");
      return(options::sp_pp_stringify());
   }

   if (  second->Is(CT_POUND)
      && second->TestFlags(PCF_IN_PREPROC)
      && first->GetParentType() != CT_MACRO_FUNC)
   {
      // Add or remove space before preprocessor '#' stringify operator
      // as in '#define x(y) L#y'.
      log_rule("sp_before_pp_stringify");
      return(options::sp_before_pp_stringify());
   }

   if (first->Is(CT_DECLSPEC))  // Issue 1289
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  second->Is(CT_NEWLINE)
      || second->Is(CT_VBRACE_OPEN))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  first->Is(CT_VBRACE_OPEN)
      && second->IsNot(CT_NL_CONT)
      && second->IsNot(CT_SEMICOLON)) // # Issue 1158
   {
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   if (  first->Is(CT_VBRACE_CLOSE)
      && second->IsNot(CT_NL_CONT))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (second->Is(CT_VSEMICOLON))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (first->Is(CT_MACRO_FUNC))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (second->Is(CT_NL_CONT))
   {
      // Add or remove space before a backslash-newline at the end of a line.
      log_rule("sp_before_nl_cont");
      return(options::sp_before_nl_cont());
   }

   if (  language_is_set(lang_flag_e::LANG_D)
      && (  first->Is(CT_D_ARRAY_COLON)
         || second->Is(CT_D_ARRAY_COLON)))
   {
      // (D) Add or remove around the D named array initializer ':' operator.
      log_rule("sp_d_array_colon");
      return(options::sp_d_array_colon());
   }

   if (  first->Is(CT_CASE)
      && ((  CharTable::IsKw1(second->GetStr()[0])
          || second->Is(CT_NUMBER))))
   {
      // Fix the spacing between 'case' and the label. Only 'ignore' and 'force' make
      // sense here.
      log_rule("sp_case_label");
      return(options::sp_case_label() | IARF_ADD);
   }

   if (first->Is(CT_FOR_COLON))
   {
      // java
      // Add or remove space after ':' in a Java/C++11 range-based 'for',
      // as in 'for (Type var : expr)'.
      log_rule("sp_after_for_colon");
      return(options::sp_after_for_colon());
   }

   if (second->Is(CT_FOR_COLON))
   {
      // java
      // Add or remove space before ':' in a Java/C++11 range-based 'for',
      // as in 'for (Type var : expr)'.
      log_rule("sp_before_for_colon");
      return(options::sp_before_for_colon());
   }

   if (  first->Is(CT_QUESTION)
      && second->Is(CT_COND_COLON))
   {
      // In the abbreviated ternary form '(a ?: b)', add or remove space between '?'
      // and ':'.
      // Overrides all other sp_cond_* options.
      log_rule("sp_cond_ternary_short");
      return(options::sp_cond_ternary_short());
   }

   if (  first->Is(CT_QUESTION)                  // see the tests cpp:34513-34516
      || second->Is(CT_QUESTION))
   {
      if (  second->Is(CT_QUESTION)
         && (options::sp_cond_question_before() != IARF_IGNORE))
      {
         if (second->TestFlags(PCF_IN_TEMPLATE))
         {
            // do nothing
         }
         else
         {
            // Add or remove space before the '?' in 'b ? t : f'.
            // Overrides sp_cond_question.
            log_rule("sp_cond_question_before");
            return(options::sp_cond_question_before());
         }
      }

      if (  first->Is(CT_QUESTION)
         && (options::sp_cond_question_after() != IARF_IGNORE))
      {
         if (first->TestFlags(PCF_IN_TEMPLATE))
         {
            // do nothing
         }
         else
         {
            // Add or remove space after the '?' in 'b ? t : f'.
            // Overrides sp_cond_question.
            log_rule("sp_cond_question_after");
            return(options::sp_cond_question_after());
         }
      }

      if (  first->TestFlags(PCF_IN_TEMPLATE)
         || second->TestFlags(PCF_IN_TEMPLATE))
      {
         // do nothing
      }
      else
      {
         log_rule("sp_cond_question");
         return(options::sp_cond_question());
      }
   }

   if (  first->Is(CT_COND_COLON)
      || second->Is(CT_COND_COLON))
   {
      if (  second->Is(CT_COND_COLON)
         && (options::sp_cond_colon_before() != IARF_IGNORE))
      {
         // Add or remove space before the ':' in 'b ? t : f'.
         // Overrides sp_cond_colon.
         log_rule("sp_cond_colon_before");
         return(options::sp_cond_colon_before());
      }

      if (  first->Is(CT_COND_COLON)
         && (options::sp_cond_colon_after() != IARF_IGNORE))
      {
         // Add or remove space after the ':' in 'b ? t : f'.
         // Overrides sp_cond_colon.
         log_rule("sp_cond_colon_after");
         return(options::sp_cond_colon_after());
      }
      // Issue #2596
      // replace "if (first->Is(CT_WORD) && second->Is(CT_COND_COLON))"
      // Add or remove space around the ':' in 'b ? t : f'.
      log_rule("sp_cond_colon");
      return(options::sp_cond_colon());
   }

   if (  language_is_set(lang_flag_e::LANG_D)
      && (  first->Is(CT_RANGE)
         || second->Is(CT_RANGE)))
   {
      // (D) Add or remove space around the D '..' operator.
      log_rule("sp_range");
      return(options::sp_range());
   }

   if (  first->Is(CT_COLON)
      && first->GetParentType() == CT_SQL_EXEC)
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   // Macro stuff can only return IGNORE, ADD, or FORCE
   if (first->Is(CT_MACRO))
   {
      // Add or remove space between a macro name and its definition.
      log_rule("sp_macro");
      iarf_e arg = options::sp_macro();
      return(arg | ((arg != IARF_IGNORE) ? IARF_ADD : IARF_IGNORE));
   }

   if (  first->Is(CT_FPAREN_CLOSE)
      && first->GetParentType() == CT_MACRO_FUNC)
   {
      // Add or remove space between a macro function ')' and its definition.
      log_rule("sp_macro_func");
      iarf_e arg = options::sp_macro_func();
      return(arg | ((arg != IARF_IGNORE) ? IARF_ADD : IARF_IGNORE));
   }

   if (first->Is(CT_PREPROC))
   {
      // Remove spaces, unless we are ignoring. See indent_preproc()
      log_rule("pp_space_after");

      if (options::pp_space_after() == IARF_IGNORE)
      {
         log_rule("IGNORE");
         return(IARF_IGNORE);
      }
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (second->Is(CT_PREPROC))
   {
      // Remove spaces, unless we are ignoring. See indent_preproc()
      log_rule("pp_indent");

      if (options::pp_indent() == IARF_IGNORE)
      {
         log_rule("IGNORE");
         return(IARF_IGNORE);
      }
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (second->Is(CT_SEMICOLON))                       // see the tests cpp:34517-34519
   {
      if (  first->Is(CT_VBRACE_OPEN)                  // Issue #2942
         && first->GetPrev()->Is(CT_SPAREN_CLOSE)
         && (  first->GetParentType() == CT_IF
            || first->GetParentType() == CT_FOR
            || first->GetParentType() == CT_WHILE))
      {
         // Add or remove space before empty statement ';' on 'if', 'for' and 'while'.
         log_rule("sp_special_semi");
         return(options::sp_special_semi());
      }

      // Issue #4094-03
      if (second->GetParentType() == CT_FOR)
      {
         if (first->Is(CT_SPAREN_OPEN))
         {
            // empty, e.g. for (;;)
            //                 ^ is first
            //                  ^ is second
            // Add or remove space before a semicolon of an empty left part of a for statement.
            log_rule("sp_before_semi_for_empty");
            return(options::sp_before_semi_for_empty());
         }

         if (first->Is(CT_SEMICOLON))
         {
            // empty, e.g. for (;;)
            //                  ^ is first
            //                   ^ is second
            // Add or remove space between semicolons of an empty middle part of a for statement.
            log_rule("sp_between_semi_for_empty");
            return(options::sp_between_semi_for_empty());
         }
         // Add or remove space before ';' in non-empty 'for' statements.
         log_rule("sp_before_semi_for");
         return(options::sp_before_semi_for());
      }
      else if (  first->Is(CT_VBRACE_OPEN)                  // Issue #2942
              && first->GetPrev()->Is(CT_SPAREN_CLOSE)
              && first->GetParentType() != CT_WHILE_OF_DO)
      {
         // Add or remove space before empty statement ';' on 'if', 'for' and 'while'.
         log_rule("sp_special_semi");
         return(options::sp_special_semi());
      }
      else
      {
         // Add or remove space before ';'.
         log_rule("sp_before_semi");
         return(options::sp_before_semi());
      }
   }

   if (  (  second->Is(CT_COMMENT)
         || second->Is(CT_COMMENT_CPP))
      && (  first->Is(CT_PP_ELSE)
         || first->Is(CT_PP_ENDIF)))
   {
      if (second->Is(CT_COMMENT_CPP))
      {
         second->SetType(CT_COMMENT_CPP_ENDIF);
      }
      else
      {
         second->SetType(CT_COMMENT_ENDIF);
      }
      // Add or remove space between #else or #endif and a trailing comment.
      log_rule("sp_endif_cmt");
      return(options::sp_endif_cmt());
   }

   if (  options::sp_before_tr_cmt() != IARF_IGNORE
      && second->GetParentType() == CT_COMMENT_END)
   {
      // Add or remove space before a trailing comment.
      // Number of spaces before a trailing comment.
      log_rule("sp_before_tr_cmt");
      log_rule("sp_num_before_tr_cmt");
      min_sp = options::sp_num_before_tr_cmt();
      return(options::sp_before_tr_cmt());
   }

   if (second->GetParentType() == CT_COMMENT_END)
   {
      switch (second->GetOrigPrevSp())
      {
      case 0:
         log_rule("orig prev sp - REMOVE");
         return(IARF_REMOVE);

      case 1:
         log_rule("orig prev sp - FORCE");
         return(IARF_FORCE);

      default:
         log_rule("orig prev sp - ADD");
         return(IARF_ADD);
      }
   }

   // Issue #4094-05
   // "for (;;)" vs. "for (;; )" and "for (a;b;c)" vs. "for (a; b; c)"
   if (first->Is(CT_SEMICOLON))                        // see the tests cpp:34517-34519
   {
      if (first->GetParentType() == CT_FOR)
      {
         if (second->Is(CT_SPAREN_CLOSE))
         {
            // Add or remove space after the final semicolon of an empty part of a for
            // statement, as in 'for ( ; ; <here> )'.
            log_rule("sp_after_semi_for_empty");
            return(options::sp_after_semi_for_empty());
         }

         if (second->IsNot(CT_SPAREN_CLOSE))  // Issue 1324
         {
            // Add or remove space after ';' in non-empty 'for' statements.
            log_rule("sp_after_semi_for");
            return(options::sp_after_semi_for());
         }
      }
      else if (  !second->IsComment()
              && second->IsNot(CT_BRACE_CLOSE)) // issue #197
      {
         // Add or remove space after ';', except when followed by a comment.
         // see the tests cpp:34517-34519
         log_rule("sp_after_semi");
         return(options::sp_after_semi());
      }
      // Let the comment spacing rules handle this
   }

   // puts a space in the rare '+-' or '-+'
   if (  (  first->Is(CT_NEG)
         || first->Is(CT_POS)
         || first->Is(CT_ARITH)
         || first->Is(CT_SHIFT))
      && (  second->Is(CT_NEG)
         || second->Is(CT_POS)
         || second->Is(CT_ARITH)
         || second->Is(CT_SHIFT)))
   {
      log_rule("ADD");
      return(IARF_ADD);
   }

   // "return(a);" vs. "return (foo_t)a + 3;" vs. "return a;" vs. "return;"
   if (first->Is(CT_RETURN))
   {
      if (  second->Is(CT_PAREN_OPEN)
         && second->GetParentType() == CT_RETURN)
      {
         // Add or remove space between 'return' and '('.
         log_rule("sp_return_paren");
         return(options::sp_return_paren());
      }
      else if (  second->Is(CT_BRACE_OPEN)
              && second->GetParentType() == CT_BRACED_INIT_LIST)
      {
         // Add or remove space between 'return' and '{'.
         log_rule("sp_return_brace");
         return(options::sp_return_brace());
      }
      // Everything else requires a space
      // The value REMOVE will be overridden with FORCE
      log_rule("sp_return");

      if (options::sp_return() == IARF_REMOVE)
      {
         return(IARF_FORCE);
      }
      return(options::sp_return());
   }

   // "sizeof(foo_t)" vs. "sizeof (foo_t)"
   if (first->Is(CT_SIZEOF))
   {
      if (second->Is(CT_PAREN_OPEN))
      {
         // Add or remove space between 'sizeof' and '('.
         log_rule("sp_sizeof_paren");
         return(options::sp_sizeof_paren());
      }

      if (second->Is(CT_ELLIPSIS))
      {
         // Add or remove space between 'sizeof' and '...'.
         log_rule("sp_sizeof_ellipsis");
         return(options::sp_sizeof_ellipsis());
      }
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   // "decltype(foo_t)" vs. "decltype (foo_t)"
   if (first->Is(CT_DECLTYPE))
   {
      if (second->Is(CT_PAREN_OPEN))
      {
         // Add or remove space between 'decltype' and '('.
         log_rule("sp_decltype_paren");
         return(options::sp_decltype_paren());
      }
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   // handle '::'
   if (first->Is(CT_DC_MEMBER))
   {
      // Add or remove space after the '::' operator.
      log_rule("sp_after_dc");
      return(options::sp_after_dc());
   }

   // Issue #889
   // mapped_file_source abc((int) ::CW2A(sTemp));
   if (  first->Is(CT_PAREN_CLOSE)
      && second->Is(CT_DC_MEMBER)
      && second->GetNext()->GetType() == CT_FUNC_CALL)
   {
      log_rule("sp_after_cast");
      return(options::sp_after_cast());
   }

   if (second->Is(CT_DC_MEMBER))
   {
      /* '::' at the start of an identifier is not member access, but global scope operator.
       * Detect if previous chunk is keyword
       */
      switch (first->GetType())
      {
      case CT_SBOOL:
      case CT_SASSIGN:
      case CT_ARITH:
      case CT_SHIFT:
      case CT_CASE:
      case CT_CLASS:
      case CT_DELETE:
      case CT_FRIEND:
      case CT_NAMESPACE:
      case CT_NEW:
      case CT_SARITH:
      case CT_SCOMPARE:
      case CT_OPERATOR:
      case CT_ACCESS:
      case CT_QUALIFIER:
      case CT_RETURN:
      case CT_SIZEOF:
      case CT_DECLTYPE:
      case CT_STRUCT:
      case CT_THROW:
      case CT_TYPEDEF:
      case CT_TYPENAME:
      case CT_UNION:
      case CT_USING:
         log_rule("FORCE");
         return(IARF_FORCE);

      default:
         break;
      }

      if (  (  first->Is(CT_WORD)
            || first->Is(CT_TYPE)
            || first->Is(CT_PAREN_CLOSE)
            || CharTable::IsKw1(first->GetStr()[0]))
         && (strcmp(first->Text(), "void") != 0)) // Issue 1249
      {
         // Add or remove space before the '::' operator.
         log_rule("sp_before_dc");
         return(options::sp_before_dc());
      }
   }

   // "a,b" vs. "a, b"
   if (first->Is(CT_COMMA))                         // see the tests cpp:34520-34524
   // see the tests c-sharp:12200-12202
   {
      if (  (  language_is_set(lang_flag_e::LANG_CS)
            || language_is_set(lang_flag_e::LANG_VALA))
         && first->GetParentType() == CT_TYPE)
      {
         // (C#, Vala) multidimensional array type: ',,' vs. ', ,' or ',]' vs. ', ]'
         if (second->Is(CT_COMMA))
         {
            // (C#, Vala) Add or remove space between ',' in multidimensional array type
            // like 'int[,,]'.
            log_rule("sp_between_mdatype_commas");
            return(options::sp_between_mdatype_commas());
         }
         // (C#, Vala) Add or remove space between ',' and ']' in multidimensional array type
         // like 'int[,,]'.
         log_rule("sp_after_mdatype_commas");
         return(options::sp_after_mdatype_commas());
      }

      // Fix for issue #1243
      // Don't add extra space after comma immediately followed by Angle close
      if (second->Is(CT_ANGLE_CLOSE))
      {
         log_rule("IGNORE");
         return(IARF_IGNORE);
      }
      // Add or remove space after ',', i.e. 'a,b' vs. 'a, b'.
      log_rule("sp_after_comma");
      return(options::sp_after_comma());
   }

   // test if we are within a SIGNAL/SLOT call
   if (QT_SIGNAL_SLOT_found)
   {
      if (  first->Is(CT_FPAREN_CLOSE)
         && (  second->Is(CT_FPAREN_CLOSE)
            || second->Is(CT_COMMA)))
      {
         if (second->GetLevel() == QT_SIGNAL_SLOT_level)
         {
            restoreValues = true;
         }
      }
   }

   if (second->Is(CT_COMMA))
   {
      if (  (  language_is_set(lang_flag_e::LANG_CS)
            || language_is_set(lang_flag_e::LANG_VALA))
         && first->Is(CT_SQUARE_OPEN)
         && first->GetParentType() == CT_TYPE)
      {
         // (C#, Vala) Add or remove space between '[' and ',' in multidimensional array type
         // like 'int[,,]'.
         log_rule("sp_before_mdatype_commas");
         return(options::sp_before_mdatype_commas());
      }

      if (  first->Is(CT_PAREN_OPEN)
         || first->Is(CT_FPAREN_OPEN))
      {
         // Add or remove space between an open parenthesis and comma,
         // i.e. '(,' vs. '( ,'.
         log_rule("sp_paren_comma");
         return(options::sp_paren_comma());
      }
      // Add or remove space before ',', i.e. 'a,b' vs. 'a ,b'.
      log_rule("sp_before_comma");
      return(options::sp_before_comma());
   }

   if (second->Is(CT_ELLIPSIS))
   {
      // type followed by a ellipsis
      Chunk *tmp = first;

      if (  tmp->Is(CT_PTR_TYPE)
         || tmp->Is(CT_BYREF))
      {
         tmp = tmp->GetPrevNcNnl();
      }

      if (  tmp->Is(CT_TYPE)
         || tmp->Is(CT_QUALIFIER))
      {
         // Add or remove space between a type and '...'.
         log_rule("sp_type_ellipsis");
         return(options::sp_type_ellipsis());
      }

      // non-punc followed by a ellipsis
      if (  !first->TestFlags(PCF_PUNCTUATOR)
         && (options::sp_before_ellipsis() != IARF_IGNORE))
      {
         // Add or remove space before the variadic '...' when preceded by a
         // non-punctuator.
         log_rule("sp_before_ellipsis");
         return(options::sp_before_ellipsis());
      }

      if (  first->Is(CT_FPAREN_CLOSE)
         || first->Is(CT_PAREN_CLOSE))
      {
         // Add or remove space between ')' and '...'.
         log_rule("sp_paren_ellipsis");
         return(options::sp_paren_ellipsis());
      }

      if (first->Is(CT_TAG_COLON))
      {
         log_rule("FORCE");
         return(IARF_FORCE);
      }

      if (first->Is(CT_BYREF))                         // Issue #3309
      {
         log_rule("sp_byref_ellipsis");
         return(options::sp_byref_ellipsis());
      }

      if (first->Is(CT_PARAMETER_PACK))                // Issue #3309
      {
         log_rule("sp_parameter_pack_ellipsis");
         return(options::sp_parameter_pack_ellipsis());
      }
   }

   if (first->Is(CT_ELLIPSIS))
   {
      if (second->Is(CT_PARAMETER_PACK))                // Issue #3309
      {
         log_rule("sp_ellipsis_parameter_pack");
         return(options::sp_ellipsis_parameter_pack());
      }

      if (CharTable::IsKw1(second->GetStr()[0]))
      {
         log_rule("FORCE");
         return(IARF_FORCE);
      }

      if (  second->Is(CT_PAREN_OPEN)
         && first->GetPrev()->Is(CT_SIZEOF))
      {
         // Add or remove space between 'sizeof...' and '('.
         log_rule("sp_sizeof_ellipsis_paren");
         return(options::sp_sizeof_ellipsis_paren());
      }
   }

   if (  language_is_set(lang_flag_e::LANG_PAWN)
      && first->Is(CT_TAG_COLON))
   {
      // (Pawn) Add or remove space after the tag keyword.
      log_rule("sp_after_tag");
      return(options::sp_after_tag());
   }

   if (second->Is(CT_TAG_COLON))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   // handle '~'
   if (first->Is(CT_DESTRUCTOR))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  language_is_set(lang_flag_e::LANG_OC)
      && first->Is(CT_CATCH)
      && second->Is(CT_SPAREN_OPEN)
      && (options::sp_oc_catch_paren() != IARF_IGNORE))
   {
      // (OC) Add or remove space between '@catch' and '('
      // in '@catch (something) { }'. If set to ignore, sp_catch_paren is used.
      log_rule("sp_oc_catch_paren");
      return(options::sp_oc_catch_paren());
   }

   if (  language_is_set(lang_flag_e::LANG_OC)
      && (  first->Is(CT_PAREN_CLOSE)
         || first->Is(CT_OC_CLASS)
         || first->Is(CT_WORD))
      && second->Is(CT_ANGLE_OPEN)
      && (  second->GetParentType() == CT_OC_PROTO_LIST
         || second->GetParentType() == CT_OC_GENERIC_SPEC)
      && (options::sp_before_oc_proto_list() != IARF_IGNORE))
   {
      // (OC) Add or remove space before Objective-C protocol list
      // as in '@protocol Protocol<here><Protocol_A>' or '@interface MyClass : NSObject<here><MyProtocol>'.
      log_rule("sp_before_oc_proto_list");
      return(options::sp_before_oc_proto_list());
   }

   if (  language_is_set(lang_flag_e::LANG_OC)
      && first->Is(CT_OC_CLASS)
      && second->Is(CT_PAREN_OPEN)
      && (options::sp_oc_classname_paren() != IARF_IGNORE))
   {
      // (OC) Add or remove space between class name and '('
      // in '@interface className(categoryName)<ProtocolName>:BaseClass'
      log_rule("sp_oc_classname_paren");
      return(options::sp_oc_classname_paren());
   }

   if (  first->Is(CT_CATCH)
      && second->Is(CT_SPAREN_OPEN)
      && (options::sp_catch_paren() != IARF_IGNORE))
   {
      // Add or remove space between 'catch' and '(' in 'catch (something) { }'.
      // If set to ignore, sp_before_sparen is used.
      log_rule("sp_catch_paren");
      return(options::sp_catch_paren());
   }

   if (  language_is_set(lang_flag_e::LANG_D)
      && first->Is(CT_D_VERSION_IF)
      && second->Is(CT_SPAREN_OPEN)
      && (options::sp_version_paren() != IARF_IGNORE))
   {
      // (D) Add or remove space between 'version' and '('
      // in 'version (something) { }'. If set to ignore, sp_before_sparen is used.
      log_rule("sp_version_paren");
      return(options::sp_version_paren());
   }

   if (  language_is_set(lang_flag_e::LANG_D)
      && first->Is(CT_D_SCOPE_IF)
      && second->Is(CT_SPAREN_OPEN)
      && (options::sp_scope_paren() != IARF_IGNORE))
   {
      // (D) Add or remove space between 'scope' and '('
      // in 'scope (something) { }'. If set to ignore, sp_before_sparen is used.
      log_rule("sp_scope_paren");
      return(options::sp_scope_paren());
   }

   if (  language_is_set(lang_flag_e::LANG_OC)
      && first->Is(CT_SYNCHRONIZED)
      && second->Is(CT_SPAREN_OPEN))
   {
      // (OC) Add or remove space between '@synchronized' and the open parenthesis,
      // i.e. '@synchronized(foo)' vs. '@synchronized (foo)'.
      log_rule("sp_after_oc_synchronized");
      return(options::sp_after_oc_synchronized());
   }

   // "if (" vs. "if("
   if (second->Is(CT_SPAREN_OPEN))
   {
      // Add or remove space after 'do' between 'while' and '('. Issue #995
      if (  first->Is(CT_WHILE_OF_DO)
         && options::sp_while_paren_open() != IARF_IGNORE)
      {
         log_rule("sp_while_paren_open");
         return(options::sp_while_paren_open());
      }
      // Add or remove space before '(' of other control statements ('if', 'for',
      // 'switch', 'while', etc.).
      log_rule("sp_before_sparen");
      return(options::sp_before_sparen());
   }

   if (  first->Is(CT_LAMBDA)
      || second->Is(CT_LAMBDA))
   {
      // Add or remove space around assignment operator '=', '+=', etc.
      log_rule("sp_assign");
      return(options::sp_assign());
   }

   // Handle the special lambda case for C++11:
   //    [=](Something arg){.....}
   // Add or remove space around '=' in C++11 lambda capture specifications.
   // Overrides sp_assign.
   if (  (options::sp_cpp_lambda_assign() != IARF_IGNORE)
      && (  (  first->Is(CT_SQUARE_OPEN)
            && first->GetParentType() == CT_CPP_LAMBDA
            && second->Is(CT_ASSIGN))
         || (  first->Is(CT_ASSIGN)
            && second->Is(CT_SQUARE_CLOSE)
            && second->GetParentType() == CT_CPP_LAMBDA)))
   {
      log_rule("sp_cpp_lambda_assign");
      return(options::sp_cpp_lambda_assign());
   }

   if (  first->Is(CT_SQUARE_CLOSE)
      && first->GetParentType() == CT_CPP_LAMBDA)
   {
      // Handle the special lambda case for C++11:
      //    [](Something arg){.....}
      // Add or remove space after the capture specification of a C++11 lambda when
      // an argument list is present, as in '[] <here> (int x){ ... }'.
      if (second->Is(CT_LPAREN_OPEN))
      {
         log_rule("sp_cpp_lambda_square_paren");
         return(options::sp_cpp_lambda_square_paren());
      }
      else if (second->Is(CT_BRACE_OPEN))
      {
         // Add or remove space after the capture specification of a C++11 lambda with
         // no argument list is present, as in '[] <here> { ... }'.
         log_rule("sp_cpp_lambda_square_brace");
         return(options::sp_cpp_lambda_square_brace());
      }
   }

   if (first->Is(CT_LPAREN_OPEN))
   {
      if (second->Is(CT_LPAREN_CLOSE))
      {
         // Add or remove space after the opening parenthesis and before the closing
         // parenthesis of a argument list of a C++11 lambda, as in
         // '[]( <here> ){ ... }'
         // with an empty list.
         log_rule("sp_cpp_lambda_argument_list_empty");
         return(options::sp_cpp_lambda_argument_list_empty());
      }
      // Add or remove space after the opening parenthesis of a argument list
      // of a C++11 lambda, as in '[]( <here> int x ){ ... }'.
      log_rule("sp_cpp_lambda_argument_list");
      return(options::sp_cpp_lambda_argument_list());
   }

   if (first->Is(CT_LPAREN_CLOSE))
   {
      if (second->Is(CT_BRACE_OPEN))
      {
         // Add or remove space after the argument list of a C++11 lambda, as in
         // '[](int x) <here> { ... }'.
         log_rule("sp_cpp_lambda_paren_brace");
         return(options::sp_cpp_lambda_paren_brace());
      }
   }

   if (second->Is(CT_LPAREN_CLOSE))
   {
      // Add or remove space before the closing parenthesis of a argument list
      // of a C++11 lambda, as in '[]( int x <here> ){ ... }'.
      log_rule("sp_cpp_lambda_argument_list");
      return(options::sp_cpp_lambda_argument_list());
   }

   if (  first->Is(CT_BRACE_CLOSE)
      && first->GetParentType() == CT_CPP_LAMBDA
      && second->Is(CT_FPAREN_OPEN))
   {
      // Add or remove space between a lambda body and its call operator of an
      // immediately invoked lambda, as in '[]( ... ){ ... } <here> ( ... )'.
      log_rule("sp_cpp_lambda_fparen");
      return(options::sp_cpp_lambda_fparen());
   }

   if (first->Is(CT_ENUM))
   {
      if (second->Is(CT_BRACE_OPEN))
      {
         // Add or remove space in 'enum {'.
         log_rule("sp_enum_brace");
         return(options::sp_enum_brace());
      }
      else if (second->Is(CT_FPAREN_OPEN))
      {
         // Add or remove space in 'NS_ENUM ('.
         log_rule("sp_enum_paren");
         return(options::sp_enum_paren());
      }
   }

   if (second->Is(CT_ASSIGN))
   {
      if (second->TestFlags(PCF_IN_ENUM))
      {
         // Add or remove space before assignment '=' in enum.
         // Overrides sp_enum_assign.
         if (options::sp_enum_before_assign() != IARF_IGNORE)
         {
            log_rule("sp_enum_before_assign");
            return(options::sp_enum_before_assign());
         }
         // Add or remove space around assignment '=' in enum.
         log_rule("sp_enum_assign");
         return(options::sp_enum_assign());
      }

      // Add or remove space around assignment operator '=' in a prototype.
      // If set to ignore, use sp_assign.
      if (  (options::sp_assign_default() != IARF_IGNORE)
         && second->GetParentType() == CT_FUNC_PROTO)
      {
         log_rule("sp_assign_default");
         return(options::sp_assign_default());
      }

      // Add or remove space before assignment operator '=', '+=', etc.
      // Overrides sp_assign.
      if (options::sp_before_assign() != IARF_IGNORE)
      {
         log_rule("sp_before_assign");
         return(options::sp_before_assign());
      }
      // Add or remove space around assignment operator '=', '+=', etc.
      log_rule("sp_assign");
      return(options::sp_assign());
   }

   if (second->Is(CT_ASSIGN_DEFAULT_ARG))
   {
      // Add or remove space around assignment operator '=' in a prototype.
      // If set to ignore, use sp_assign.
      if (  (options::sp_assign_default() != IARF_IGNORE)
         && second->GetParentType() == CT_FUNC_PROTO)
      {
         log_rule("sp_assign_default");
         return(options::sp_assign_default());
      }

      // Add or remove space before assignment operator '=', '+=', etc.
      // Overrides sp_assign.
      if (options::sp_before_assign() != IARF_IGNORE)
      {
         log_rule("sp_before_assign");
         return(options::sp_before_assign());
      }
      // Add or remove space around assignment operator '=', '+=', etc.
      log_rule("sp_assign");
      return(options::sp_assign());
   }

   if (first->Is(CT_ASSIGN))
   {
      if (first->TestFlags(PCF_IN_ENUM))
      {
         // Add or remove space after assignment '=' in enum.
         // Overrides sp_enum_assign.
         if (options::sp_enum_after_assign() != IARF_IGNORE)
         {
            log_rule("sp_enum_after_assign");
            return(options::sp_enum_after_assign());
         }
         // Add or remove space around assignment '=' in enum.
         log_rule("sp_enum_assign");
         return(options::sp_enum_assign());
      }

      // Add or remove space around assignment operator '=' in a prototype.
      // If set to ignore, use sp_assign.
      if (  (options::sp_assign_default() != IARF_IGNORE)
         && first->GetParentType() == CT_FUNC_PROTO)
      {
         log_rule("sp_assign_default");
         return(options::sp_assign_default());
      }

      // Add or remove space after assignment operator '=', '+=', etc.
      // Overrides sp_assign.
      if (options::sp_after_assign() != IARF_IGNORE)
      {
         log_rule("sp_after_assign");
         return(options::sp_after_assign());
      }
      // Add or remove space around assignment operator '=', '+=', etc.
      log_rule("sp_assign");
      return(options::sp_assign());
   }

   if (  first->Is(CT_TRAILING_RET)
      || first->Is(CT_CPP_LAMBDA_RET)
      || second->Is(CT_TRAILING_RET)
      || second->Is(CT_CPP_LAMBDA_RET))
   {
      // Add or remove space around trailing return operator '->'.
      log_rule("sp_trailing_return");
      return(options::sp_trailing_return());
   }

   if (first->Is(CT_ASSIGN_DEFAULT_ARG))
   {
      // Add or remove space around assignment operator '=' in a prototype.
      // If set to ignore, use sp_assign.
      if (  (options::sp_assign_default() != IARF_IGNORE)
         && first->GetParentType() == CT_FUNC_PROTO)
      {
         log_rule("sp_assign_default");
         return(options::sp_assign_default());
      }

      // Add or remove space after assignment operator '=', '+=', etc.
      // Overrides sp_assign.
      if (options::sp_after_assign() != IARF_IGNORE)
      {
         log_rule("sp_after_assign");
         return(options::sp_after_assign());
      }
      // Add or remove space around assignment operator '=', '+=', etc.
      log_rule("sp_assign");
      return(options::sp_assign());
   }

   if (first->Is(CT_ENUM_COLON))                      // Issue #4040
   {
      // Add or remove space around assignment ':' in enum.
      log_rule("sp_enum_colon");
      return(options::sp_enum_colon());
   }

   if (second->Is(CT_ENUM_COLON))                    // Issue #4040
   {
      // Add or remove space around assignment ':' in enum.
      log_rule("sp_enum_colon");
      return(options::sp_enum_colon());
   }

   if (first->Is(CT_BIT_COLON))                      // Issue #4040
   {
      // Add or remove space after a bit colon ':'.
      log_rule("sp_after_bit_colon");
      return(options::sp_after_bit_colon());
   }

   if (second->Is(CT_BIT_COLON))                    // Issue #4040
   {
      // Add or remove space before a bit colon ':'.
      log_rule("sp_before_bit_colon");
      return(options::sp_before_bit_colon());
   }

   if (  first->Is(CT_OC_AVAILABLE_VALUE)
      || second->Is(CT_OC_AVAILABLE_VALUE))
   {
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }

   if (language_is_set(lang_flag_e::LANG_OC))
   {
      if (second->Is(CT_OC_BLOCK_CARET))
      {
         // (OC) Add or remove space before a block pointer caret,
         // i.e. '^int (int arg){...}' vs. ' ^int (int arg){...}'.
         log_rule("sp_before_oc_block_caret");
         return(options::sp_before_oc_block_caret());
      }

      if (first->Is(CT_OC_BLOCK_CARET))
      {
         // (OC) Add or remove space after a block pointer caret,
         // i.e. '^int (int arg){...}' vs. '^ int (int arg){...}'.
         log_rule("sp_after_oc_block_caret");
         return(options::sp_after_oc_block_caret());
      }

      if (second->Is(CT_OC_MSG_FUNC))
      {
         if (  (options::sp_after_oc_msg_receiver() == IARF_REMOVE)
            && (  first->IsNot(CT_SQUARE_CLOSE)
               && first->IsNot(CT_FPAREN_CLOSE)
               && first->IsNot(CT_PAREN_CLOSE)))
         {
            log_rule("FORCE");
            return(IARF_FORCE);
         }
         // (OC) Add or remove space between the receiver and selector in a message,
         // as in '[receiver selector ...]'.
         log_rule("sp_after_oc_msg_receiver");
         return(options::sp_after_oc_msg_receiver());
      }
   }

   // c++17 structured bindings e.g., "auto [x, y, z]" vs. a[x, y, z]" or "auto const [x, y, z]" vs. "auto const[x, y, z]"
   // after byref.
   if (  language_is_set(lang_flag_e::LANG_CPP)
      && first->Is(CT_BYREF)
      && second->Is(CT_SQUARE_OPEN)
      && second->GetParentType() != CT_OC_MSG
      && second->GetParentType() != CT_CS_SQ_STMT)
   {
      // Add or remove space before C++17 structured bindings.
      // after byref.
      log_rule("sp_cpp_before_struct_binding_after_byref");
      return(options::sp_cpp_before_struct_binding_after_byref());
   }

   // c++17 structured bindings e.g., "auto [x, y, z]" vs. a[x, y, z]" or "auto const [x, y, z]" vs. "auto const[x, y, z]"
   if (  language_is_set(lang_flag_e::LANG_CPP)
      && (  first->Is(CT_QUALIFIER)
         || first->Is(CT_TYPE))
      && second->Is(CT_SQUARE_OPEN)
      && second->GetParentType() != CT_OC_MSG
      && second->GetParentType() != CT_CS_SQ_STMT)
   {
      // Add or remove space before C++17 structured bindings.
      log_rule("sp_cpp_before_struct_binding");
      return(options::sp_cpp_before_struct_binding());
   }

   // "a [x]" vs. "a[x]"
   if (  second->Is(CT_SQUARE_OPEN)
      && (  second->GetParentType() != CT_OC_MSG
         && second->GetParentType() != CT_CS_SQ_STMT
         && second->GetParentType() != CT_CPP_LAMBDA))
   {
      if (  second->TestFlags(PCF_IN_SPAREN)
         && (first->Is(CT_IN)))
      {
         log_rule("FORCE");
         return(IARF_FORCE);
      }

      if (first->Is(CT_ASM_COLON))
      {
         // Add or remove space before '[' for asm block.
         log_rule("sp_before_square_asm_block");
         return(options::sp_before_square_asm_block());
      }

      if (first->TestFlags(PCF_VAR_DEF))
      {
         // Add or remove space before '[' for a variable definition.
         log_rule("sp_before_vardef_square");
         return(options::sp_before_vardef_square());
      }
      // Add or remove space before '[' (except '[]').
      log_rule("sp_before_square");
      return(options::sp_before_square());
   }

   // "byte[]" vs. "byte []"
   if (second->Is(CT_TSQUARE))
   {
      // Add or remove space before '[]'.
      log_rule("sp_before_squares");
      return(options::sp_before_squares());
   }

   if (  (options::sp_angle_shift() != IARF_IGNORE)
      && first->Is(CT_ANGLE_CLOSE)
      && second->Is(CT_ANGLE_CLOSE))
   {
      // Add or remove space between '>' and '>' in '>>' (template stuff).
      log_rule("sp_angle_shift");
      return(options::sp_angle_shift());
   }

   // Issue #4094-01
   // spacing around template < > stuff
   if (  first->Is(CT_ANGLE_OPEN)
      || second->Is(CT_ANGLE_CLOSE))
   {
      if (  first->Is(CT_ANGLE_OPEN)
         && second->Is(CT_ANGLE_CLOSE))
      {
         // Add or remove space inside '<>'.
         // if empty.
         log_rule("sp_inside_angle_empty");
         return(options::sp_inside_angle_empty());
      }
      // Add or remove space inside '<' and '>'.
      log_rule("sp_inside_angle");
      iarf_e op = options::sp_inside_angle();

      // special: if we're not supporting digraphs, then we shouldn't create them!
      if (  (op == IARF_REMOVE)
         && !options::enable_digraphs()
         && first->Is(CT_ANGLE_OPEN)
         && second->Is(CT_DC_MEMBER))
      {
         op = IARF_IGNORE;
      }
      return(op);
   }

   if (second->Is(CT_ANGLE_OPEN))
   {
      if (  first->Is(CT_TEMPLATE)
         && (options::sp_template_angle() != IARF_IGNORE))
      {
         // Add or remove space between 'template' and '<'.
         // If set to ignore, sp_before_angle is used.
         log_rule("sp_template_angle");
         return(options::sp_template_angle());
      }

      if (first->IsNot(CT_QUALIFIER))
      {
         // Add or remove space before '<'.
         log_rule("sp_before_angle");
         return(options::sp_before_angle());
      }
   }

   if (first->Is(CT_ANGLE_CLOSE))
   {
      if (  second->Is(CT_WORD)
         || CharTable::IsKw1(second->GetStr()[0]))
      {
         // Add or remove space between '>' and a word as in 'List<byte> m;' or
         // 'template <typename T> static ...'.
         log_rule("sp_angle_word");
         return(options::sp_angle_word());
      }

      // Issue #4094-02
      if (  second->Is(CT_FPAREN_OPEN)
         || second->Is(CT_PAREN_OPEN))
      {
         Chunk *next = second->GetNextNcNnl();

         if (next->Is(CT_FPAREN_CLOSE))
         {
            // Add or remove space between '>' and '()' as found in 'new List<byte>();'.
            log_rule("sp_angle_paren_empty");
            return(options::sp_angle_paren_empty());
         }
         // Add or remove space between '>' and '(' as found in 'new List<byte>(foo);'.
         log_rule("sp_angle_paren");
         return(options::sp_angle_paren());
      }

      if (second->Is(CT_DC_MEMBER))
      {
         // Add or remove space before the '::' operator.
         log_rule("sp_before_dc");
         return(options::sp_before_dc());
      }

      if (  second->IsNot(CT_BYREF)
         && second->IsNot(CT_PTR_TYPE)
         && second->IsNot(CT_BRACE_OPEN)
         && second->IsNot(CT_PAREN_CLOSE))
      {
         if (  second->Is(CT_CLASS_COLON)
            && options::sp_angle_colon() != IARF_IGNORE)
         {
            // Add or remove space between '>' and ':'.
            log_rule("sp_angle_colon");
            return(options::sp_angle_colon());
         }

         // Whether sp_after_angle takes precedence over sp_inside_fparen. This was the
         // historic behavior, but is probably not the desired behavior, so this is off
         // by default.
         if (  second->Is(CT_FPAREN_CLOSE)
            && options::sp_inside_fparen() != IARF_IGNORE
            && !options::use_sp_after_angle_always())
         {
            // Add or remove space between '>' and ')'.
            log_rule("sp_inside_fparen");
            return(options::sp_inside_fparen());
         }
         // Add or remove space after '>'.
         log_rule("sp_after_angle");
         return(options::sp_after_angle());
      }
   }

   if (first->Is(CT_BYREF))                             // see the tests cpp:34509-34512
   {
      if (second->Is(CT_PAREN_OPEN))
      {
         // Add or remove space after a reference sign '&', if followed by an open
         // parenthesis, as in 'char& (*)()'.
         log_rule("sp_byref_paren");
         return(options::sp_byref_paren());
      }
      else if (  first->GetParentType() == CT_FUNC_DEF        // Issue #3197, #3210
              || first->GetParentType() == CT_FUNC_PROTO)
      {
         // Add or remove space after a reference sign '&', if followed by a function
         // prototype or function definition.
         log_rule("sp_after_byref_func");                          // byref 2
         return(options::sp_after_byref_func());
      }
      else if (  CharTable::IsKw1(second->GetStr()[0])
              && (  options::sp_after_byref() != IARF_IGNORE
                 || (  !second->Is(CT_FUNC_PROTO)
                    && !second->Is(CT_FUNC_DEF))))
      {
         // Add or remove space after reference sign '&', if followed by a word.
         log_rule("sp_after_byref");                               // byref 1
         return(options::sp_after_byref());
      }
   }

   if (  second->Is(CT_BYREF)
      && !first->Is(CT_PAREN_OPEN))              // Issue #1804
   {
      if (  second->GetParentType() == CT_FUNC_DEF     // Issue #3197, #3210
         || second->GetParentType() == CT_FUNC_PROTO)
      {
         // Add or remove space before a reference sign '&', if followed by a function
         // prototype or function definition.
         log_rule("sp_before_byref_func");                         // byref 4
         return(options::sp_before_byref_func());
      }
      Chunk *next = second->GetNext();

      if (  next->IsNotNullChunk()
         && (  next->Is(CT_COMMA)
            || next->Is(CT_PAREN_CLOSE)                            // Issue #3691
            || next->Is(CT_FPAREN_CLOSE)
            || next->Is(CT_SEMICOLON)
            || next->Is(CT_ANGLE_CLOSE)))                          // Issue #4064
      {
         if (options::sp_before_unnamed_byref() != IARF_IGNORE)    // Issue #3691
         {
            // Add or remove space before a reference sign '&' that isn't followed by a
            // variable name. If set to 'ignore', sp_before_byref is used instead.
            log_rule("sp_before_unnamed_byref");                   // byref 5
            return(options::sp_before_unnamed_byref());
         }
         else
         {
            // Add or remove space before a reference sign '&'.
            log_rule("sp_before_byref");                           // byref 3
            return(options::sp_before_byref());
         }
      }

      if (first->Is(CT_PTR_TYPE))
      {
         // Add or remove space between pointer and Ref.
         // as in 'int *& a'.
         log_rule("sp_between_ptr_ref");                           // ptr_ref 1
         return(options::sp_between_ptr_ref());
      }
      // Add or remove space before a reference sign '&'.
      log_rule("sp_before_byref");                                 // byref 3
      return(options::sp_before_byref());
   }

   if (first->Is(CT_SPAREN_CLOSE))
   {
      if (second->Is(CT_BRACE_OPEN))
      {
         if (second->GetParentType() == CT_CATCH)
         {
            if (  language_is_set(lang_flag_e::LANG_OC)
               && (options::sp_oc_catch_brace() != IARF_IGNORE))
            {
               // (OC) Add or remove space before the '{' of a '@catch' statement, if the '{'
               // and '@catch' are on the same line, as in '@catch (decl) <here> {'.
               // If set to ignore, sp_catch_brace is used.
               // only to help the vim command }}
               log_rule("sp_oc_catch_brace");
               return(options::sp_oc_catch_brace());
            }

            if (options::sp_catch_brace() != IARF_IGNORE)
            {
               // Add or remove space before the '{' of a 'catch' statement, if the '{' and
               // 'catch' are on the same line, as in 'catch (decl) <here> {'.
               log_rule("sp_catch_brace");
               return(options::sp_catch_brace());
            }
         }

         if (options::sp_sparen_brace() != IARF_IGNORE)
         {
            // Add or remove space between ')' and '{' of control statements.
            log_rule("sp_sparen_brace");
            return(options::sp_sparen_brace());
         }
      }
   }

   // Issue #4094-07
   if (  second->Is(CT_FPAREN_OPEN)
      && first->GetParentType() == CT_OPERATOR
      && (options::sp_after_operator_sym() != IARF_IGNORE))
   {
      if (  (options::sp_after_operator_sym_empty() != IARF_IGNORE)
         && second->Is(CT_FPAREN_OPEN))
      {
         Chunk *next = second->GetNextNcNnl();

         if (next->Is(CT_FPAREN_CLOSE))
         {
            // Overrides sp_after_operator_sym when the operator has no arguments, as in
            // 'operator *()'.
            log_rule("sp_after_operator_sym_empty");
            return(options::sp_after_operator_sym_empty());
         }
      }
      // Add or remove space between the operator symbol and the open parenthesis, as
      // in 'operator ++('.
      log_rule("sp_after_operator_sym");
      return(options::sp_after_operator_sym());
   }

   // Issue #2270
   // Translations under vala
   if (  language_is_set(lang_flag_e::LANG_VALA)
      && first->Is(CT_FUNC_CALL))
   {
      if (  first->IsString("_")
         && second->Is(CT_FPAREN_OPEN)
         && (options::sp_vala_after_translation() != IARF_IGNORE))
      {
         // Add or remove space after '_'.
         log_rule("sp_vala_after_translation");
         return(options::sp_vala_after_translation());
      }
   }

   if (  first->Is(CT_MACRO_OPEN)
      || first->Is(CT_MACRO_CLOSE)
      || first->Is(CT_MACRO_ELSE))
   {
      if (second->Is(CT_FPAREN_OPEN))
      {
         // TODO: provide some test data to check this block
         // Add or remove space between function name and '(' on function calls.
         log_rule("sp_func_call_paren");
         return(options::sp_func_call_paren());
      }
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }

   // Issue #4094-11
   // spaces between function and open paren
   if (  first->Is(CT_FUNC_CALL)
      || first->Is(CT_FUNCTION)                        // Issue #2665
      || first->Is(CT_FUNC_CTOR_VAR)
      || first->Is(CT_CNG_HASINC)
      || first->Is(CT_CNG_HASINCN)
      || (  first->Is(CT_BRACE_CLOSE)
         && first->GetParentType() == CT_BRACED_INIT_LIST
         && second->Is(CT_FPAREN_OPEN))
      || (  first->Is(CT_FUNC_VAR)                     // Issue #3852
         && second->Is(CT_PAREN_OPEN)))
   {
      if (  (options::sp_func_call_paren_empty() != IARF_IGNORE)
         && second->Is(CT_FPAREN_OPEN))
      {
         Chunk *next = second->GetNextNcNnl();

         if (next->Is(CT_FPAREN_CLOSE))
         {
            // Add or remove space between function name and '()' on function calls without
            // parameters. If set to 'ignore' (the default), sp_func_call_paren is used.
            log_rule("sp_func_call_paren_empty");
            return(options::sp_func_call_paren_empty());
         }
      }
      // Add or remove space between function name and '(' on function calls.
      log_rule("sp_func_call_paren");
      return(options::sp_func_call_paren());
   }

   if (first->Is(CT_FUNC_CALL_USER))
   {
      // Add or remove space between the user function name and '(' on function
      // calls. You need to set a keyword to be a user function in the config file,
      // like:
      //   set func_call_user tr _ i18n
      log_rule("sp_func_call_user_paren");
      return(options::sp_func_call_user_paren());
   }

   if (  first->Is(CT_ATTRIBUTE)
      && second->IsParenOpen())
   {
      // Add or remove space between '__attribute__' and '('.
      log_rule("sp_attribute_paren");
      return(options::sp_attribute_paren());
   }

   // Issue #4094-10
   if (first->Is(CT_FUNC_DEF))
   {
      if (  (options::sp_func_def_paren_empty() != IARF_IGNORE)
         && second->Is(CT_FPAREN_OPEN))
      {
         Chunk *next = second->GetNextNcNnl();

         if (next->Is(CT_FPAREN_CLOSE))
         {
            // Add or remove space between function name and '()' on function definition
            // if empty.
            log_rule("sp_func_def_paren_empty");
            return(options::sp_func_def_paren_empty());
         }
      }
      // Add or remove space between function name and '(' on function definition.
      log_rule("sp_func_def_paren");
      return(options::sp_func_def_paren());
   }

   if (  first->Is(CT_CPP_CAST)
      || first->Is(CT_TYPE_WRAP))
   {
      // Add or remove space between the type and open parenthesis in a C++ cast,
      // i.e. 'int(exp)' vs. 'int (exp)'.
      log_rule("sp_cpp_cast_paren");
      return(options::sp_cpp_cast_paren());
   }

   if (  first->Is(CT_SPAREN_CLOSE)
      && second->Is(CT_WHEN))
   {
      // TODO: provide some test data to check this block
      log_rule("FORCE");
      return(IARF_FORCE); // TODO: make this configurable?
   }

   if (  first->Is(CT_PAREN_CLOSE)
      && (  second->Is(CT_PAREN_OPEN)
         || second->Is(CT_FPAREN_OPEN)))
   {
      // "(int)a" vs. "(int) a" or "cast(int)a" vs. "cast(int) a"
      if (  first->GetParentType() == CT_C_CAST
         || first->GetParentType() == CT_D_CAST)
      {
         // Add or remove space after C/D cast, i.e. 'cast(int)a' vs. 'cast(int) a' or
         // '(int)a' vs. '(int) a'.
         log_rule("sp_after_cast");
         return(options::sp_after_cast());
      }
      // Probably a parenthesized indirect function call or similar (issue #3260)
      log_rule("sp_cparen_oparen");
      return(options::sp_cparen_oparen());
   }

   // handle the space between parens in fcn type 'void (*f)(void)'
   if (first->Is(CT_TPAREN_CLOSE))
   {
      // Add or remove space between the ')' and '(' in a function type, as in
      // 'void (*x)(...)'.
      log_rule("sp_after_tparen_close");
      return(options::sp_after_tparen_close());
   }

   // ")(" vs. ") ("
   if (  (  first->IsString(")")
         && second->IsString("("))
      || (  first->IsParenClose()
         && second->IsParenOpen()))
   {
      // Add or remove space between back-to-back parentheses, i.e. ')(' vs. ') ('.
      log_rule("sp_cparen_oparen");
      return(options::sp_cparen_oparen());
   }

   // Issue #4094-09
   if (  first->Is(CT_FUNC_PROTO)
      || (  second->Is(CT_FPAREN_OPEN)
         && second->GetParentType() == CT_FUNC_PROTO))
   {
      if (  (options::sp_func_proto_paren_empty() != IARF_IGNORE)
         && second->Is(CT_FPAREN_OPEN))
      {
         Chunk *next = second->GetNextNcNnl();

         if (next->Is(CT_FPAREN_CLOSE))
         {
            // Add or remove space between function name and '()' on function declaration
            // if empty.
            log_rule("sp_func_proto_paren_empty");
            return(options::sp_func_proto_paren_empty());
         }
      }
      // Add or remove space between function name and '(' on function declaration.
      log_rule("sp_func_proto_paren");
      return(options::sp_func_proto_paren());
   }

   // Issue #2437
   if (  first->Is(CT_FUNC_TYPE)
      && second->Is(CT_FPAREN_OPEN))
   {
      // Add or remove space between function name and '(' with a typedef specifier.
      log_rule("sp_func_type_paren");
      return(options::sp_func_type_paren());
   }

   // Issue #4094-12
   if (  first->Is(CT_FUNC_CLASS_DEF)
      || first->Is(CT_FUNC_CLASS_PROTO))
   {
      if (  (options::sp_func_class_paren_empty() != IARF_IGNORE)
         && second->Is(CT_FPAREN_OPEN))
      {
         Chunk *next = second->GetNextNcNnl();

         if (next->Is(CT_FPAREN_CLOSE))
         {
            // Add or remove space between a constructor without parameters or destructor
            // and '()'.
            log_rule("sp_func_class_paren_empty");
            return(options::sp_func_class_paren_empty());
         }
      }
      // Add or remove space between a constructor/destructor and the open
      // parenthesis.
      log_rule("sp_func_class_paren");
      return(options::sp_func_class_paren());
   }

   if (  first->Is(CT_CLASS)
      && !first->TestFlags(PCF_IN_OC_MSG))
   {
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   // Issue #4094-08
   if (  first->Is(CT_BRACE_OPEN)
      && second->Is(CT_BRACE_CLOSE))
   {
      // Add or remove space inside '{}'.
      // if empty.
      log_rule("sp_inside_braces_empty");
      return(options::sp_inside_braces_empty());
   }

   if (  (  first->Is(CT_TYPE)                           // Issue #2428
         || first->Is(CT_ANGLE_CLOSE))
      && second->Is(CT_BRACE_OPEN)
      && second->GetParentType() == CT_BRACED_INIT_LIST)
   {
      iarf_flags_t arg = iarf_flags_t{ options::sp_type_brace_init_lst() };

      if (  arg != IARF_IGNORE
         || first->GetParentType() != CT_DECLTYPE)
      {
         // 'int{9}' vs. 'int {9}'
         // Add or remove space between type and open brace of an unnamed temporary
         // direct-list-initialization.
         log_rule("sp_type_brace_init_lst");
         return(arg);
      }
   }

   if (  (  first->Is(CT_WORD)                           // Issue #2428
         || first->Is(CT_SQUARE_CLOSE)
         || first->Is(CT_TSQUARE))
      && second->Is(CT_BRACE_OPEN)
      && second->GetParentType() == CT_BRACED_INIT_LIST)
   {
      // Add or remove space between a variable and '{' for C++ uniform
      // initialization.
      auto arg = iarf_flags_t{ options::sp_word_brace_init_lst() };

      if (  arg != IARF_IGNORE
         || first->GetParentType() != CT_DECLTYPE)
      {
         // 'a{9}' vs. 'a {9}'
         // Add or remove space between variable/word and open brace of an unnamed
         // temporary direct-list-initialization.
         log_rule("sp_word_brace_init_lst");
         return(arg);
      }
   }

   if (second->Is(CT_BRACE_CLOSE))
   {
      if (second->GetParentType() == CT_ENUM)
      {
         // Add or remove space inside enum '{' and '}'.
         log_rule("sp_inside_braces_enum");
         return(options::sp_inside_braces_enum());
      }

      if (  second->GetParentType() == CT_STRUCT
         || second->GetParentType() == CT_UNION)
      {
         // Fix for issue #1240  adding space in struct initializers
         Chunk *tmp = second->GetOpeningParen()->GetPrevNcNnl();

         if (tmp->Is(CT_ASSIGN))
         {
            // TODO: provide some test data to check this block
            log_rule("IGNORE");
            return(IARF_IGNORE);
         }
         // Add or remove space inside struct/union '{' and '}'.
         log_rule("sp_inside_braces_struct");
         return(options::sp_inside_braces_struct());
      }
      else if (  language_is_set(lang_flag_e::LANG_OC)
              && second->GetParentType() == CT_OC_AT
              && options::sp_inside_braces_oc_dict() != IARF_IGNORE)
      {
         // (OC) Add or remove space inside Objective-C boxed dictionary '{' and '}'
         log_rule("sp_inside_braces_oc_dict");
         return(options::sp_inside_braces_oc_dict());
      }

      if (second->GetParentType() == CT_BRACED_INIT_LIST)
      {
         // Add or remove space between nested braces, i.e. '{{' vs. '{ {'.
         // only to help the vim command }}}}
         if (  options::sp_brace_brace() != IARF_IGNORE
            && first->Is(CT_BRACE_CLOSE)
            && first->GetParentType() == CT_BRACED_INIT_LIST)
         {
            log_rule("sp_brace_brace");
            return(options::sp_brace_brace());
         }

         if (options::sp_before_type_brace_init_lst_close() != IARF_IGNORE)
         {
            // Add or remove space before close brace in an unnamed temporary
            // direct-list-initialization
            // if statement is a brace_init_lst
            // works only if sp_brace_brace is set to ignore.
            log_rule("sp_before_type_brace_init_lst_close");
            return(options::sp_before_type_brace_init_lst_close());
         }

         if (options::sp_inside_type_brace_init_lst() != IARF_IGNORE)
         {
            // Add or remove space inside an unnamed temporary direct-list-initialization.
            // if statement is a brace_init_lst
            // works only if sp_brace_brace is set to ignore
            // works only if sp_before_type_brace_init_lst_close is set to ignore.
            log_rule("sp_inside_type_brace_init_lst");
            return(options::sp_inside_type_brace_init_lst());
         }
      }
      // Add or remove space inside '{' and '}'.
      log_rule("sp_inside_braces");
      return(options::sp_inside_braces());
   }

   if (first->Is(CT_D_CAST))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  first->Is(CT_PP_DEFINED)
      && second->Is(CT_PAREN_OPEN))
   {
      // Add or remove space between 'defined' and '(' in '#if defined (FOO)'.
      log_rule("sp_defined_paren");
      return(options::sp_defined_paren());
   }

   if (first->Is(CT_THROW))
   {
      if (second->Is(CT_PAREN_OPEN))
      {
         // Add or remove space between 'throw' and '(' in 'throw (something)'.
         log_rule("sp_throw_paren");
         return(options::sp_throw_paren());
      }
      // Add or remove space between 'throw' and anything other than '(' as in
      // '@throw [...];'.
      log_rule("sp_after_throw");
      return(options::sp_after_throw());
   }

   if (  first->Is(CT_THIS)
      && second->Is(CT_PAREN_OPEN))
   {
      // Add or remove space between 'this' and '(' in 'this (something)'.
      log_rule("sp_this_paren");
      return(options::sp_this_paren());
   }

   if (  first->Is(CT_STATE)
      && second->Is(CT_PAREN_OPEN))
   {
      log_rule("ADD");
      return(IARF_ADD);
   }

   if (  first->Is(CT_DELEGATE)
      && second->Is(CT_PAREN_OPEN))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  first->Is(CT_MEMBER)
      || second->Is(CT_MEMBER))
   {
      // Add or remove space around the '.' or '->' operators.
      log_rule("sp_member");
      return(options::sp_member());
   }

   if (first->Is(CT_C99_MEMBER))
   {
      // always remove space(s) after then '.' of a C99-member
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  first->Is(CT_SUPER)
      && second->Is(CT_PAREN_OPEN))
   {
      // Add or remove space between 'super' and '(' in 'super (something)'.
      log_rule("sp_super_paren");
      return(options::sp_super_paren());
   }

   if (  first->Is(CT_FPAREN_CLOSE)
      && second->Is(CT_BRACE_OPEN))
   {
      if (  language_is_set(lang_flag_e::LANG_JAVA)
         && second->GetParentType() == CT_DOUBLE_BRACE)
      {
         // (Java) Add or remove space between ')' and '{{' of double brace initializer.
         // only to help the vim command }}
         log_rule("sp_fparen_dbrace");
         return(options::sp_fparen_dbrace());
      }

      // To fix issue #1234
      // check for initializers and add space or ignore based on the option.
      if (first->GetParentType() == CT_FUNC_CALL)
      {
         Chunk *tmp = first->GetPrevType(first->GetParentType(), first->GetLevel());
         tmp = tmp->GetPrevNcNnl();

         if (tmp->Is(CT_NEW))
         {
            // Add or remove space between ')' and '{' of s function call in object
            // initialization.
            // Overrides sp_fparen_brace.
            log_rule("sp_fparen_brace_initializer");
            return(options::sp_fparen_brace_initializer());
         }
      }
      // Add or remove space between ')' and '{' of function.
      log_rule("sp_fparen_brace");
      return(options::sp_fparen_brace());
   }

   if (  first->Is(CT_D_TEMPLATE)
      || second->Is(CT_D_TEMPLATE))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  first->Is(CT_ELSE)
      && second->Is(CT_BRACE_OPEN))
   {
      // Add or remove space between 'else' and '{' if on the same line.
      log_rule("sp_else_brace");
      return(options::sp_else_brace());
   }

   if (  first->Is(CT_ELSE)
      && second->Is(CT_ELSEIF))
   {
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   if (  first->Is(CT_FINALLY)
      && second->Is(CT_BRACE_OPEN))
   {
      // Add or remove space between 'finally' and '{' if on the same line.
      log_rule("sp_finally_brace");
      return(options::sp_finally_brace());
   }

   if (  first->Is(CT_TRY)
      && second->Is(CT_BRACE_OPEN))
   {
      // Add or remove space between 'try' and '{' if on the same line.
      log_rule("sp_try_brace");
      return(options::sp_try_brace());
   }

   if (  first->Is(CT_GETSET)
      && second->Is(CT_BRACE_OPEN))
   {
      // Add or remove space between get/set and '{' if on the same line.
      log_rule("sp_getset_brace");
      return(options::sp_getset_brace());
   }

   if (  first->Is(CT_WORD)
      && second->Is(CT_BRACE_OPEN))
   {
      if (first->GetParentType() == CT_NAMESPACE)
      {
         // Add or remove space between a variable and '{' for a namespace.
         log_rule("sp_word_brace_ns");
         return(options::sp_word_brace_ns());
      }
   }

   if (  language_is_set(lang_flag_e::LANG_D)
      && second->Is(CT_PAREN_OPEN)
      && second->GetParentType() == CT_INVARIANT)
   {
      // (D) Add or remove space between 'invariant' and '('.
      log_rule("sp_invariant_paren");
      return(options::sp_invariant_paren());
   }

   if (  first->Is(CT_PAREN_CLOSE)
      && first->GetParentType() != CT_DECLTYPE)
   {
      if (  language_is_set(lang_flag_e::LANG_D)
         && first->GetParentType() == CT_INVARIANT)
      {
         // (D) Add or remove space after the ')' in 'invariant (C) c'.
         log_rule("sp_after_invariant_paren");
         return(options::sp_after_invariant_paren());
      }

      // "(struct foo) {...}" vs. "(struct foo){...}"
      if (second->Is(CT_BRACE_OPEN))
      {
         // Add or remove space between ')' and '{'.
         log_rule("sp_paren_brace");
         return(options::sp_paren_brace());
      }

      // D-specific: "delegate(some thing) dg
      if (first->GetParentType() == CT_DELEGATE)
      {
         log_rule("ADD");
         return(IARF_ADD);
      }

      // PAWN-specific: "state (condition) next"
      if (first->GetParentType() == CT_STATE)
      {
         log_rule("ADD");
         return(IARF_ADD);
      }

      /* C++ new operator: new(bar) Foo */
      if (first->GetParentType() == CT_NEW)
      {
         // Add or remove space between ')' and type in 'new(foo) BAR'.
         log_rule("sp_after_newop_paren");
         return(options::sp_after_newop_paren());
      }
   }

   /* "((" vs. "( (" or "))" vs. ") )" */
   // Issue #1342
   if (  (  first->IsString("(")
         && second->IsString("("))
      || (  first->IsString(")")
         && second->IsString(")")))
   {
      if (second->GetParentType() == CT_FUNC_CALL_USER)
      {
         // Add or remove space between nested parentheses with user functions,
         // i.e. '((' vs. '( ('.
         log_rule("sp_func_call_user_paren_paren");
         return(options::sp_func_call_user_paren_paren());
      }

      if (  options::sp_sparen_paren() != IARF_IGNORE
         && (  first->Is(CT_SPAREN_OPEN)
            || second->Is(CT_SPAREN_CLOSE)))
      {
         // Add or remove space between nested parentheses with control
         // statements, i.e. 'if ((' vs. 'if ( ('. Issue #3209
         log_rule("sp_sparen_paren");
         return(options::sp_sparen_paren());
      }
      // Add or remove space between nested parentheses, i.e. '((' vs. ') )'.
      log_rule("sp_paren_paren");
      return(options::sp_paren_paren());
   }

   // "foo(...)" vs. "foo( ... )"
   if (  first->Is(CT_FPAREN_OPEN)
      || second->Is(CT_FPAREN_CLOSE))
   {
      if (  (first->GetParentType() == CT_FUNC_CALL_USER)
         || (  (second->GetParentType() == CT_FUNC_CALL_USER)
            && (  (first->Is(CT_WORD))
               || (first->Is(CT_SQUARE_CLOSE)))))
      {
         // Add or remove space inside user function '(' and ')'.
         log_rule("sp_func_call_user_inside_fparen");
         return(options::sp_func_call_user_inside_fparen());
      }

      if (  first->Is(CT_FPAREN_OPEN)
         && second->Is(CT_FPAREN_CLOSE))
      {
         // Add or remove space inside empty function '()'.
         log_rule("sp_inside_fparens");
         return(options::sp_inside_fparens());
      }
      // Add or remove space inside function '(' and ')'.
      log_rule("sp_inside_fparen");
      return(options::sp_inside_fparen());
   }

   // functor "foo(...)" vs. "foo( ... )"
   if (  first->Is(CT_RPAREN_OPEN)
      || second->Is(CT_RPAREN_CLOSE))                  // Issue #3914
   {
      if (  (first->GetParentType() == CT_FUNC_CALL_USER)
         || (  (second->GetParentType() == CT_FUNC_CALL_USER)
            && (  (first->Is(CT_WORD))
               || (first->Is(CT_SQUARE_CLOSE)))))
      {
         // Add or remove space inside user function '(' and ')'.
         log_rule("sp_func_call_user_inside_rparen");
         return(options::sp_func_call_user_inside_rparen());
      }

      if (  first->Is(CT_RPAREN_OPEN)
         && second->Is(CT_RPAREN_CLOSE))                  // Issue #3914
      {
         // Add or remove space inside empty function '()'.
         log_rule("sp_inside_rparens");
         return(options::sp_inside_rparens());
      }
      // Add or remove space inside function '(' and ')'.
      log_rule("sp_inside_rparen");
      return(options::sp_inside_rparen());
   }

   // "foo(...)" vs. "foo( ... )"
   if (  first->Is(CT_TPAREN_OPEN)
      || second->Is(CT_TPAREN_CLOSE))
   {
      // Add or remove space inside the first parentheses in a function type, as in
      // 'void (*x)(...)'.
      log_rule("sp_inside_tparen");
      return(options::sp_inside_tparen());
   }

   if (  language_is_set(lang_flag_e::LANG_OC)
      && first->Is(CT_PAREN_CLOSE))
   {
      if (  first->TestFlags(PCF_OC_RTYPE) // == CT_OC_RTYPE)
         && (  first->GetParentType() == CT_OC_MSG_DECL
            || first->GetParentType() == CT_OC_MSG_SPEC))
      {
         // (OC) Add or remove space after the first (type) in message specs,
         // i.e. '-(int) f:(int)x;' vs. '-(int)f:(int)x;'.
         log_rule("sp_after_oc_return_type");
         return(options::sp_after_oc_return_type());
      }

      if (  first->GetParentType() == CT_OC_MSG_SPEC
         || first->GetParentType() == CT_OC_MSG_DECL)
      {
         // (OC) Add or remove space after the (type) in message specs,
         // i.e. '-(int)f: (int) x;' vs. '-(int)f: (int)x;'.
         log_rule("sp_after_oc_type");
         return(options::sp_after_oc_type());
      }

      if (  first->GetParentType() == CT_OC_SEL
         && second->IsNot(CT_SQUARE_CLOSE))
      {
         // (OC) Add or remove space between '@selector(x)' and the following word,
         // i.e. '@selector(foo) a:' vs. '@selector(foo)a:'.
         log_rule("sp_after_oc_at_sel_parens");
         return(options::sp_after_oc_at_sel_parens());
      }
   }

   if (  language_is_set(lang_flag_e::LANG_OC)
      && options::sp_inside_oc_at_sel_parens() != IARF_IGNORE)
   {
      if (  (  first->Is(CT_PAREN_OPEN)
            && (  first->GetParentType() == CT_OC_SEL
               || first->GetParentType() == CT_OC_PROTOCOL))
         || (  second->Is(CT_PAREN_CLOSE)
            && (  second->GetParentType() == CT_OC_SEL
               || second->GetParentType() == CT_OC_PROTOCOL)))
      {
         // (OC) Add or remove space inside '@selector' parentheses,
         // i.e. '@selector(foo)' vs. '@selector( foo )'.
         // Also applies to '@protocol()' constructs.
         log_rule("sp_inside_oc_at_sel_parens");
         return(options::sp_inside_oc_at_sel_parens());
      }
   }

   if (  second->Is(CT_PAREN_OPEN)
      && (  first->Is(CT_OC_SEL)
         || first->Is(CT_OC_PROTOCOL)))
   {
      // (OC) Add or remove space between '@selector' and '(',
      // i.e. '@selector(msgName)' vs. '@selector (msgName)'.
      // Also applies to '@protocol()' constructs.
      log_rule("sp_after_oc_at_sel");
      return(options::sp_after_oc_at_sel());
   }

   /*
    * C cast:   "(int)"      vs. "( int )"
    * D cast:   "cast(int)"  vs. "cast( int )"
    * CPP cast: "int(a + 3)" vs. "int( a + 3 )"
    */
   if (first->Is(CT_PAREN_OPEN))
   {
      if (  first->GetParentType() == CT_C_CAST
         || first->GetParentType() == CT_CPP_CAST
         || first->GetParentType() == CT_D_CAST)
      {
         // Add or remove spaces inside cast parentheses.
         log_rule("sp_inside_paren_cast");
         return(options::sp_inside_paren_cast());
      }

      if (first->GetParentType() == CT_NEW)
      {
         if (options::sp_inside_newop_paren_open() != IARF_IGNORE)
         {
            // Add or remove space after the open parenthesis of the new operator,
            // as in 'new(foo) BAR'.
            // Overrides sp_inside_newop_paren.
            log_rule("sp_inside_newop_paren_open");
            return(options::sp_inside_newop_paren_open());
         }

         if (options::sp_inside_newop_paren() != IARF_IGNORE)
         {
            // Add or remove space inside parentheses of the new operator
            // as in 'new(foo) BAR'.
            log_rule("sp_inside_newop_paren");
            return(options::sp_inside_newop_paren());
         }
      }
      log_rule("sp_inside_paren");
      return(options::sp_inside_paren());
   }

   if (second->Is(CT_PAREN_CLOSE))
   {
      if (  second->GetParentType() == CT_C_CAST
         || second->GetParentType() == CT_CPP_CAST
         || second->GetParentType() == CT_D_CAST)
      {
         // Add or remove spaces inside cast parentheses.
         log_rule("sp_inside_paren_cast");
         return(options::sp_inside_paren_cast());
      }

      if (second->GetParentType() == CT_NEW)
      {
         if (options::sp_inside_newop_paren_close() != IARF_IGNORE)
         {
            // Add or remove space before the close parenthesis of the new operator,
            // as in 'new(foo) BAR'.
            // Overrides sp_inside_newop_paren.
            log_rule("sp_inside_newop_paren_close");
            return(options::sp_inside_newop_paren_close());
         }

         if (options::sp_inside_newop_paren() != IARF_IGNORE)
         {
            // Add or remove space inside parentheses of the new operator
            // as in 'new(foo) BAR'.
            log_rule("sp_inside_newop_paren");
            return(options::sp_inside_newop_paren());
         }
      }
      // Add or remove space inside '(' and ')'.
      log_rule("sp_inside_paren");
      return(options::sp_inside_paren());
   }

   // Issue #4094-06
   if (  first->Is(CT_SQUARE_OPEN)
      && second->Is(CT_SQUARE_CLOSE))
   {
      // Add or remove space inside '[]'.
      // if empty.
      log_rule("sp_inside_square_empty");
      return(options::sp_inside_square_empty());
   }

   // "[3]" vs. "[ 3 ]" or for objective-c "@[@3]" vs. "@[ @3 ]"
   if (  first->Is(CT_SQUARE_OPEN)
      || second->Is(CT_SQUARE_CLOSE))
   {
      if (  language_is_set(lang_flag_e::LANG_OC)
         && (  (  first->GetParentType() == CT_OC_AT
               && first->Is(CT_SQUARE_OPEN))
            || (  second->GetParentType() == CT_OC_AT
               && second->Is(CT_SQUARE_CLOSE)))
         && (options::sp_inside_square_oc_array() != IARF_IGNORE))
      {
         // (OC) Add or remove space inside a non-empty Objective-C boxed array '@[' and
         // ']'. If set to ignore, sp_inside_square is used.
         log_rule("sp_inside_square_oc_array");
         return(options::sp_inside_square_oc_array());
      }
      // Add or remove space inside a non-empty '[' and ']'.
      log_rule("sp_inside_square");
      return(options::sp_inside_square());
   }

   if (  first->Is(CT_SQUARE_CLOSE)
      && second->Is(CT_FPAREN_OPEN))
   {
      // Add or remove space between ']' and '(' when part of a function call.
      log_rule("sp_square_fparen");
      return(options::sp_square_fparen());
   }

   // "if(...)" vs. "if( ... )" etc.
   if (second->Is(CT_SPAREN_CLOSE))
   {
      if (  second->GetParentType() == CT_FOR
         && options::sp_inside_for_open() != IARF_IGNORE)
      {
         // Add or remove space before ')' of 'for' statements.
         // Overrides sp_inside_for.
         log_rule("sp_inside_for_close");
         return(options::sp_inside_for_close());
      }
      else if (options::sp_inside_sparen_open() != IARF_IGNORE)
      {
         // Add or remove space before ')' of other control statements.
         // Overrides sp_inside_sparen.
         log_rule("sp_inside_sparen_close");
         return(options::sp_inside_sparen_close());
      }
   }

   if (first->Is(CT_SPAREN_OPEN))
   {
      if (  first->GetParentType() == CT_FOR
         && options::sp_inside_for_open() != IARF_IGNORE)
      {
         // Add or remove space before ')' of 'for' statements.
         // Overrides sp_inside_for.
         log_rule("sp_inside_for_close");
         return(options::sp_inside_for_close());
      }
      else if (options::sp_inside_sparen_open() != IARF_IGNORE)
      {
         // Add or remove space after '(' of other control statements.
         // Overrides sp_inside_sparen.
         log_rule("sp_inside_sparen_open");
         return(options::sp_inside_sparen_open());
      }
   }

   if (first->Is(CT_SPAREN_OPEN))
   {
      if (first->GetParentType() == CT_FOR)
      {
         // Add or remove space inside '(' and ')' of 'for' statements.
         log_rule("sp_inside_for");
         return(options::sp_inside_for());
      }
      else
      {
         // Add or remove space inside '(' and ')' of other control statements.
         log_rule("sp_inside_sparen");
         return(options::sp_inside_sparen());
      }
   }

   if (second->Is(CT_SPAREN_CLOSE))
   {
      if (second->GetParentType() == CT_FOR)
      {
         // Add or remove space inside '(' and ')' of 'for' statements.
         log_rule("sp_inside_for");
         return(options::sp_inside_for());
      }
      else
      {
         // Add or remove space inside '(' and ')' of other control statements.
         log_rule("sp_inside_sparen");
         return(options::sp_inside_sparen());
      }
   }

   if (first->Is(CT_CLASS_COLON))
   {
      if (  first->GetParentType() == CT_OC_CLASS
         && (  first->GetPrevType(CT_OC_INTF, first->GetLevel(), E_Scope::ALL)->IsNullChunk()
            && first->GetPrevType(CT_OC_IMPL, first->GetLevel(), E_Scope::ALL)->IsNullChunk()))
      {
         if (options::sp_after_oc_colon() != IARF_IGNORE)
         {
            // TODO: provide some test data to check this block
            // (OC) Add or remove space after the colon in message specs,
            // i.e. '-(int) f:(int) x;' vs. '-(int) f: (int) x;'.
            log_rule("sp_after_oc_colon");
            return(options::sp_after_oc_colon());
         }
      }

      if (options::sp_after_class_colon() != IARF_IGNORE)
      {
         // Add or remove space after class ':'.
         log_rule("sp_after_class_colon");
         return(options::sp_after_class_colon());
      }
   }

   if (second->Is(CT_CLASS_COLON))
   {
      if (  language_is_set(lang_flag_e::LANG_OC)
         && second->GetParentType() == CT_OC_CLASS
         && (  second->GetPrevType(CT_OC_INTF, second->GetLevel(), E_Scope::ALL)->IsNullChunk()
            && second->GetPrevType(CT_OC_IMPL, second->GetLevel(), E_Scope::ALL)->IsNullChunk()))
      {
         if (  second->GetParentType() == CT_OC_CLASS
            && second->GetPrevType(CT_OC_INTF, second->GetLevel(), E_Scope::ALL)->IsNullChunk())
         {
            if (options::sp_before_oc_colon() != IARF_IGNORE)
            {
               // TODO: provide some test data to check this block
               // (OC) Add or remove space before the colon in message specs,
               // i.e. '-(int) f: (int) x;' vs. '-(int) f : (int) x;'.
               log_rule("sp_before_oc_colon");
               return(options::sp_before_oc_colon());
            }
         }
      }

      if (options::sp_before_class_colon() != IARF_IGNORE)
      {
         // Add or remove space before class ':'.
         log_rule("sp_before_class_colon");
         return(options::sp_before_class_colon());
      }
   }

   if (first->Is(CT_CONSTR_COLON))
   {
      min_sp = options::indent_ctor_init_leading() - 1; // default indent is 1 space
      // Add or remove space after class constructor ':'.
      log_rule("sp_after_constr_colon");
      return(options::sp_after_constr_colon());
   }

   if (second->Is(CT_CONSTR_COLON))
   {
      // Add or remove space before class constructor ':'.
      log_rule("sp_before_constr_colon");
      return(options::sp_before_constr_colon());
   }

   if (second->Is(CT_CASE_COLON))
   {
      // Add or remove space before case ':'.
      log_rule("sp_before_case_colon");
      return(options::sp_before_case_colon());
   }

   if (first->Is(CT_DOT))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (second->Is(CT_DOT))
   {
      log_rule("ADD");
      return(IARF_ADD);
   }

   if (  first->Is(CT_NULLCOND)
      || second->Is(CT_NULLCOND))
   {
      // TODO: provide some test data to check this block
      // lang_flag_e::LANG_CS  null conditional operator
      // Add or remove space around the '.' or '->' operators.
      log_rule("sp_member");
      return(options::sp_member());
   }

   if (  first->Is(CT_ARITH)
      || first->Is(CT_SHIFT)
      || first->Is(CT_CARET)
      || second->Is(CT_ARITH)
      || second->Is(CT_SHIFT)
      || second->Is(CT_CARET))
   {
      // Add or remove space around arithmetic operators '+' and '-'.
      // Overrides sp_arith.
      if (options::sp_arith_additive() != IARF_IGNORE)
      {
         auto arith_char = (  first->Is(CT_ARITH)
                           || first->Is(CT_SHIFT)
                           || first->Is(CT_CARET))
                           ? first->GetStr()[0] : second->GetStr()[0];

         if (  arith_char == '+'
            || arith_char == '-')
         {
            log_rule("sp_arith_additive");
            return(options::sp_arith_additive());
         }
      }
      // Add or remove space around non-assignment symbolic operators ('+', '/', '%',
      // '<<', and so forth).
      log_rule("sp_arith");
      return(options::sp_arith());
   }

   if (  first->Is(CT_BOOL)
      || second->Is(CT_BOOL))
   {
      // Add or remove space around boolean operators '&&' and '||'.
      iarf_e arg = options::sp_bool();

      if (  (options::pos_bool() != TP_IGNORE)
         && first->GetOrigLine() != second->GetOrigLine())
      {
         arg = arg | IARF_ADD;
      }
      log_rule("sp_bool");
      return(arg);
   }

   if (  first->Is(CT_COMPARE)
      || second->Is(CT_COMPARE))
   {
      // Add or remove space around compare operator '<', '>', '==', etc.
      log_rule("sp_compare");
      return(options::sp_compare());
   }

   if (  first->Is(CT_PAREN_OPEN)
      && second->Is(CT_PTR_TYPE))
   {
      // TODO: provide some test data to check this block
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (first->Is(CT_PTR_TYPE))                            // see the tests cpp:34505-34508
   {
      if (  second->Is(CT_FPAREN_OPEN)
         || second->Is(CT_TPAREN_OPEN))
      {
         // Add or remove space after a pointer star '*', if followed by an open
         // parenthesis, as in 'void* (*)()'.
         log_rule("sp_ptr_star_paren");                               // ptr_star 10
         return(options::sp_ptr_star_paren());
      }
      else if (second->Is(CT_PTR_TYPE))
      {
         // Add or remove space between pointer stars '*'.
         // as in 'int ***a;'.
         log_rule("sp_between_ptr_star");                             // ptr_star 9
         return(options::sp_between_ptr_star());
      }
      else if (second->Is(CT_BYREF))
      {
         // Add or remove space between pointer and Ref.
         // as in 'int *& a'.
         log_rule("sp_between_ptr_ref");                             // ptr_ref 1
         return(options::sp_between_ptr_ref());
      }
      else if (second->Is(CT_FUNC_VAR))
      {
         // Add or remove space between the pointer star '*' and the name of the
         // variable in a function pointer definition.
         log_rule("sp_ptr_star_func_var");
         return(options::sp_ptr_star_func_var());
      }
      else if (second->Is(CT_FUNC_TYPE))
      {
         // Add or remove space between the pointer star '*' and the name of the
         // type in a function pointer type definition.
         log_rule("sp_ptr_star_func_type");
         return(options::sp_ptr_star_func_type());
      }
      else if (  first->GetParentType() == CT_FUNC_DEF
              || first->GetParentType() == CT_FUNC_PROTO
              || first->GetParentType() == CT_FUNC_VAR)
      {
         if (token_is_within_trailing_return(first))
         {
            // Add or remove space after a pointer star '*', in the trailing return
            // a function prototype or function definition.
            log_rule("sp_after_ptr_star_trailing");                   // ptr_star 3
            return(options::sp_after_ptr_star_trailing());
         }
         else
         {
            // Add or remove space after a pointer star '*', if followed by a function
            // prototype or function definition.
            log_rule("sp_after_ptr_star_func");                       // ptr_star 2
            return(options::sp_after_ptr_star_func());
         }
      }
      else if (CharTable::IsKw1(second->GetStr()[0]))
      {
         Chunk *prev = first->GetPrev();

         if (prev->Is(CT_IN))
         {
            // Add or remove space after the '*' (dereference) unary operator. This does
            // not affect the spacing after a '*' that is part of a type.
            log_rule("sp_deref");
            return(options::sp_deref());
         }
         else if (  first->GetParentType() == CT_FUNC_VAR
                 || first->GetParentType() == CT_FUNC_TYPE)
         {
            // Add or remove space after pointer caret '^', if followed by a word.
            log_rule("sp_after_ptr_block_caret");
            return(options::sp_after_ptr_block_caret());
         }
         else if (second->Is(CT_QUALIFIER))
         {
            // Add or remove space after pointer star '*', if followed by a qualifier.
            log_rule("sp_after_ptr_star_qualifier");                  // ptr_star 4
            return(options::sp_after_ptr_star_qualifier());
         }
         // Add or remove space after pointer star '*', if followed by a word.
         log_rule("sp_after_ptr_star");                               // ptr_star 1
         return(options::sp_after_ptr_star());
      }
      else if (second->Is(CT_PAREN_OPEN))
      {
         // Add or remove space after pointer star '*', if followed by a word.
         log_rule("sp_after_ptr_star");                               // ptr_star 1
         return(options::sp_after_ptr_star());
      }

      // must be placed at the end of the block
      // look back for '->' type is TRAILING_RET
      if (token_is_within_trailing_return(first))
      {
         log_rule("sp_after_ptr_star_trailing");                      // ptr_star 3
         return(options::sp_after_ptr_star_trailing());
      }
   }

   if (  second->Is(CT_PTR_TYPE)
      && first->IsNot(CT_IN))
   {
      // look back for '->' type is TRAILING_RET
      if (token_is_within_trailing_return(second))
      {
         if (first->Is(CT_QUALIFIER))
         {
            log_rule("sp_qualifier_ptr_star_trailing");
            return(options::sp_qualifier_ptr_star_trailing());
         }
         else
         {
            log_rule("sp_before_ptr_star_trailing");                  // ptr_star 7
            return(options::sp_before_ptr_star_trailing());
         }
      }
      // Find the next non-'*' chunk
      Chunk *next = second;

      do
      {
         next = next->GetNext();
      } while (next->Is(CT_PTR_TYPE));

      if (  next->Is(CT_FUNC_DEF)
         || next->Is(CT_FUNC_PROTO))
      {
         // Add or remove space before a pointer star '*', if followed by a function
         // prototype or function definition. If set to ignore, sp_before_ptr_star is
         // used instead.

         if (first->Is(CT_QUALIFIER))
         {
            if (options::sp_qualifier_ptr_star_func() != IARF_IGNORE)
            {
               log_rule("sp_qualifier_ptr_star_func");
               return(options::sp_qualifier_ptr_star_func());
            }
         }
         else
         {
            if (options::sp_before_ptr_star_func() != IARF_IGNORE)
            {
               log_rule("sp_before_ptr_star_func");                   // ptr_star 6
               return(options::sp_before_ptr_star_func());
            }
         }
      }
      else
      {
         // Add or remove space before pointer star '*' that isn't followed by a
         // variable name. If set to 'ignore', sp_before_ptr_star is used instead.

         next = second->GetNextNc();

         while (next->Is(CT_PTR_TYPE))
         {
            next = next->GetNextNc();
         }

         if (  next->IsNotNullChunk()
            && next->IsNot(CT_WORD))
         {
            if (first->Is(CT_QUALIFIER))
            {
               if (options::sp_qualifier_unnamed_ptr_star() != IARF_IGNORE)
               {
                  log_rule("sp_qualifier_unnamed_ptr_star");
                  return(options::sp_qualifier_unnamed_ptr_star());
               }
            }
            else
            {
               if (next->Is(CT_QUALIFIER))
               {
                  if (options::sp_before_qualifier_ptr_star() != IARF_IGNORE)
                  {
                     log_rule("sp_before_qualifier_ptr_star");        // ptr_star 11
                     return(options::sp_before_qualifier_ptr_star());
                  }
               }
               else if (next->Is(CT_OPERATOR))
               {
                  if (options::sp_before_operator_ptr_star() != IARF_IGNORE)
                  {
                     log_rule("sp_before_operator_ptr_star");         // ptr_star 14
                     return(options::sp_before_operator_ptr_star());
                  }
               }
               else if (next->Is(CT_DC_MEMBER))
               {
                  if (options::sp_before_global_scope_ptr_star() != IARF_IGNORE)
                  {
                     log_rule("sp_before_global_scope_ptr_star");     // ptr_star 13
                     return(options::sp_before_global_scope_ptr_star());
                  }
               }
               else
               {
                  Chunk *next_next = next->GetNextNc();

                  if (next_next->IsNotNullChunk() && next_next->Is(CT_DC_MEMBER))
                  {
                     if (options::sp_before_scope_ptr_star() != IARF_IGNORE)
                     {
                        log_rule("sp_before_scope_ptr_star");         // ptr_star 12
                        return(options::sp_before_scope_ptr_star());
                     }
                  }
               }

               if (options::sp_before_unnamed_ptr_star() != IARF_IGNORE)
               {
                  log_rule("sp_before_unnamed_ptr_star");             // ptr_star 8
                  return(options::sp_before_unnamed_ptr_star());
               }
            }
         }
      }

      // Add or remove space before pointer star '*'.
      if (options::sp_before_ptr_star() != IARF_IGNORE)
      {
         log_rule("sp_before_ptr_star");                              // ptr_star 5
         return(options::sp_before_ptr_star());
      }
   }

   if (first->Is(CT_OPERATOR))
   {
      // Add or remove space between 'operator' and operator sign.
      log_rule("sp_after_operator");
      return(options::sp_after_operator());
   }

   if (  second->Is(CT_FUNC_PROTO)
      || second->Is(CT_FUNC_DEF))
   {
      if (  first->IsNot(CT_PTR_TYPE)
         && first->IsNot(CT_BYREF))
      {
         // Add or remove space between return type and function name. A
         // minimum of 1 is forced except for pointer/reference return types.
         log_rule("sp_type_func | ADD");
         return(options::sp_type_func() | IARF_ADD);
      }
      // Add or remove space between return type and function name. A
      // minimum of 1 is forced except for pointer/reference return types.
      log_rule("sp_type_func");
      return(options::sp_type_func());
   }

   // "(int)a" vs. "(int) a" or "cast(int)a" vs. "cast(int) a"
   if (  (  first->GetParentType() == CT_C_CAST
         || first->GetParentType() == CT_D_CAST)
      && first->Is(CT_PAREN_CLOSE))
   {
      log_rule("sp_after_cast");
      return(options::sp_after_cast());
   }

   if (first->Is(CT_BRACE_CLOSE))
   {
      if (second->Is(CT_ELSE))
      {
         // Add or remove space between '}' and 'else' if on the same line.
         log_rule("sp_brace_else");
         return(options::sp_brace_else());
      }

      if (  language_is_set(lang_flag_e::LANG_OC)
         && second->Is(CT_CATCH)
         && (options::sp_oc_brace_catch() != IARF_IGNORE))
      {
         // TODO: provide some test data to check this block
         // (OC) Add or remove space between '}' and '@catch' if on the same line.
         // If set to ignore, sp_brace_catch is used.
         log_rule("sp_oc_brace_catch");
         return(options::sp_oc_brace_catch());
      }

      if (second->Is(CT_CATCH))
      {
         // TODO: provide some test data to check this block
         // Add or remove space between '}' and 'catch' if on the same line.
         log_rule("sp_brace_catch");
         return(options::sp_brace_catch());
      }

      if (second->Is(CT_FINALLY))
      {
         // Add or remove space between '}' and 'finally' if on the same line.
         log_rule("sp_brace_finally");
         return(options::sp_brace_finally());
      }
   }

   if (first->Is(CT_BRACE_OPEN))
   {
      if (first->GetParentType() == CT_ENUM)
      {
         // Add or remove space inside enum '{' and '}'.
         log_rule("sp_inside_braces_enum");
         return(options::sp_inside_braces_enum());
      }

      if (  first->GetParentType() == CT_STRUCT
         || first->GetParentType() == CT_UNION)
      {
         // Fix for issue #1240  adding space in struct initializers
         Chunk *tmp = first->GetPrevNcNnl();

         if (tmp->Is(CT_ASSIGN))
         {
            // TODO: provide some test data to check this block
            log_rule("IGNORE");
            return(IARF_IGNORE);
         }
         // Add or remove space inside struct/union '{' and '}'.
         log_rule("sp_inside_braces_struct");
         return(options::sp_inside_braces_struct());
      }
      else if (  first->GetParentType() == CT_OC_AT
              && options::sp_inside_braces_oc_dict() != IARF_IGNORE)
      {
         // (OC) Add or remove space inside Objective-C boxed dictionary '{' and '}'
         log_rule("sp_inside_braces_oc_dict");
         return(options::sp_inside_braces_oc_dict());
      }

      if (first->GetParentType() == CT_BRACED_INIT_LIST)
      {
         // Add or remove space between nested braces, i.e. '{{' vs. '{ {'.
         // only to help the vim command }}}}
         if (  options::sp_brace_brace() != IARF_IGNORE
            && second->Is(CT_BRACE_OPEN)
            && second->GetParentType() == CT_BRACED_INIT_LIST)
         {
            log_rule("sp_brace_brace");
            return(options::sp_brace_brace());
         }

         if (options::sp_after_type_brace_init_lst_open() != IARF_IGNORE)
         {
            // Add or remove space after open brace in an unnamed temporary
            // direct-list-initialization
            // if statement is a brace_init_lst
            // works only if sp_brace_brace is set to ignore.
            log_rule("sp_after_type_brace_init_lst_open");
            return(options::sp_after_type_brace_init_lst_open());
         }

         if (options::sp_inside_type_brace_init_lst() != IARF_IGNORE)
         {
            // Add or remove space inside an unnamed temporary direct-list-initialization
            // if statement is a brace_init_lst
            // works only if sp_brace_brace is set to ignore
            // works only if sp_after_type_brace_init_lst_close is set to ignore.
            log_rule("sp_inside_type_brace_init_lst");
            return(options::sp_inside_type_brace_init_lst());
         }
      }

      if (!second->IsComment())
      {
         // Add or remove space inside '{' and '}'.
         log_rule("sp_inside_braces");
         return(options::sp_inside_braces());
      }
   }

   if (  first->Is(CT_BRACE_CLOSE)
      && first->TestFlags(PCF_IN_TYPEDEF)
      && (  first->GetParentType() == CT_ENUM
         || first->GetParentType() == CT_STRUCT
         || first->GetParentType() == CT_UNION))
   {
      // Add or remove space between '}' and the name of a typedef on the same line.
      log_rule("sp_brace_typedef");
      return(options::sp_brace_typedef());
   }

   if (  language_is_set(lang_flag_e::LANG_D)
      && second->Is(CT_PAREN_OPEN)
      && second->GetParentType() == CT_TEMPLATE)
   {
      // (D) Add or remove space before the parenthesis in the D constructs
      // 'template Foo(' and 'class Foo('.
      log_rule("sp_before_template_paren");
      return(options::sp_before_template_paren());
   }

   // Issue #3080
   if (  first->Is(CT_PAREN_CLOSE)
      && first->GetParentType() == CT_DECLTYPE
      && (  second->Is(CT_WORD)
         || second->Is(CT_BRACE_OPEN)
         || second->Is(CT_FUNC_CALL)))
   {
      iarf_e arg = options::sp_after_decltype();
      // Add or remove space between 'decltype(...)' and word, brace or function call.
      log_rule("sp_after_decltype");
      return(arg);
   }

   // Issue #3080
   if (  !language_is_set(lang_flag_e::LANG_D)
      && first->Is(CT_PAREN_CLOSE)
      && second->Is(CT_WORD))
   {
      // Add or remove space between type and word.
      log_rule("sp_after_type");
      return(options::sp_after_type());
   }

   // see if the D template expression is used as a type
   if (  language_is_set(lang_flag_e::LANG_D)
      && first->Is(CT_PAREN_CLOSE)
      && first->GetParentType() == CT_D_TEMPLATE)
   {
      if (second->GetParentType() == CT_USING_ALIAS)
      {
         log_rule("sp_after_type | ADD");
         return(options::sp_after_type() | IARF_ADD);
      }

      if (second->Is(CT_WORD))
      {
         Chunk *open_paren = first->GetOpeningParen();
         Chunk *type       = open_paren->GetPrev()->GetPrev();

         if (type->Is(CT_TYPE))
         {
            log_rule("sp_after_type");
            return(options::sp_after_type());
         }
      }
   }

   if (  first->Is(CT_TYPE)                   // Issue #3457
      && second->Is(CT_COLON))
   {
      log_rule("sp_type_colon");
      return(options::sp_type_colon());
   }

   if (  !second->Is(CT_PTR_TYPE)
      && (  first->Is(CT_QUALIFIER)
         || first->Is(CT_TYPE)))
   {
      // Add or remove space between type and word. In cases where total removal of
      // whitespace would be a syntax error, a value of 'remove' is treated the same
      // as 'force'.
      //
      // This also affects some other instances of space following a type that are
      // not covered by other options; for example, between the return type and
      // parenthesis of a function type template argument, between the type and
      // parenthesis of an array parameter, or between 'decltype(...)' and the
      // following word.
      iarf_e arg = options::sp_after_type();
      log_rule("sp_after_type");
      return(arg);
   }

   if (  first->Is(CT_PTR_TYPE)
      && second->Is(CT_ELLIPSIS))
   {
      log_rule("sp_ptr_type_ellipsis");
      return(options::sp_ptr_type_ellipsis());
   }

   // If nothing claimed the PTR_TYPE, then return ignore
   if (  first->Is(CT_PTR_TYPE)
      || second->Is(CT_PTR_TYPE))
   {
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }

   if (first->Is(CT_NOT))
   {
      if (  second->Is(CT_NOT)
         && (options::sp_not_not() != IARF_IGNORE))
      {
         log_rule("sp_not_not");
         return(options::sp_not_not());
      }
      // Add or remove space after the '!' (not) unary operator.
      log_rule("sp_not");
      return(options::sp_not());
   }

   if (first->Is(CT_INV))
   {
      // Add or remove space after the '~' (invert) unary operator.
      log_rule("sp_inv");
      return(options::sp_inv());
   }

   if (first->Is(CT_ADDR))
   {
      // Add or remove space after the '&' (address-of) unary operator. This does not
      // affect the spacing after a '&' that is part of a type.
      log_rule("sp_addr");
      return(options::sp_addr());
   }

   if (first->Is(CT_DEREF))
   {
      // Add or remove space after the '*' (dereference) unary operator. This does
      // not affect the spacing after a '*' that is part of a type.
      log_rule("sp_deref");
      return(options::sp_deref());
   }

   if (  first->Is(CT_POS)
      || first->Is(CT_NEG))
   {
      // Add or remove space after '+' or '-', as in 'x = -5' or 'y = +7'.
      log_rule("sp_sign");
      return(options::sp_sign());
   }

   if (  first->Is(CT_INCDEC_BEFORE)
      || second->Is(CT_INCDEC_AFTER))
   {
      // Add or remove space between '++' and '--' the word to which it is being
      // applied, as in '(--x)' or 'y++;'.
      log_rule("sp_incdec");
      return(options::sp_incdec());
   }

   if (second->Is(CT_CS_SQ_COLON))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (first->Is(CT_CS_SQ_COLON))
   {
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   if (  language_is_set(lang_flag_e::LANG_OC)
      && first->Is(CT_OC_SCOPE))
   {
      // (OC) Add or remove space after the scope '+' or '-', as in '-(void) foo;'
      // or '+(int) bar;'.
      log_rule("sp_after_oc_scope");
      return(options::sp_after_oc_scope());
   }

   if (  language_is_set(lang_flag_e::LANG_OC)
      && first->Is(CT_OC_DICT_COLON))
   {
      // (OC) Add or remove space after the colon in immutable dictionary expression
      // 'NSDictionary *test = @{@"foo" :@"bar"};'.
      log_rule("sp_after_oc_dict_colon");
      return(options::sp_after_oc_dict_colon());
   }

   if (  language_is_set(lang_flag_e::LANG_OC)
      && second->Is(CT_OC_DICT_COLON))
   {
      // (OC) Add or remove space before the colon in immutable dictionary expression
      // 'NSDictionary *test = @{@"foo" :@"bar"};'.
      log_rule("sp_before_oc_dict_colon");
      return(options::sp_before_oc_dict_colon());
   }

   if (  language_is_set(lang_flag_e::LANG_OC)
      && first->Is(CT_OC_COLON))
   {
      if (first->TestFlags(PCF_IN_OC_MSG))
      {
         // (OC) Add or remove space after the colon in message specs,
         // i.e. '[object setValue:1];' vs. '[object setValue: 1];'.
         log_rule("sp_after_send_oc_colon");
         return(options::sp_after_send_oc_colon());
      }
      // (OC) Add or remove space after the colon in message specs,
      // i.e. '-(int) f:(int) x;' vs. '-(int) f: (int) x;'.
      log_rule("sp_after_oc_colon");
      return(options::sp_after_oc_colon());
   }

   if (  language_is_set(lang_flag_e::LANG_OC)
      && second->Is(CT_OC_COLON))
   {
      if (  first->TestFlags(PCF_IN_OC_MSG)
         && (  first->Is(CT_OC_MSG_FUNC)
            || first->Is(CT_OC_MSG_NAME)))
      {
         // (OC) Add or remove space before the colon in message specs,
         // i.e. '[object setValue:1];' vs. '[object setValue :1];'.
         log_rule("sp_before_send_oc_colon");
         return(options::sp_before_send_oc_colon());
      }
      // (OC) Add or remove space before the colon in message specs,
      // i.e. '-(int) f: (int) x;' vs. '-(int) f : (int) x;'.
      log_rule("sp_before_oc_colon");
      return(options::sp_before_oc_colon());
   }

   if (  second->Is(CT_COMMENT)
      && second->GetParentType() == CT_COMMENT_EMBED)
   {
      // Add or remove space before an embedded comment.
      // Number of spaces before an embedded comment.
      log_rule("sp_before_emb_cmt");
      log_rule("sp_num_before_emb_cmt");
      min_sp = options::sp_num_before_emb_cmt();
      return(options::sp_before_emb_cmt());
   }

   if (  first->Is(CT_COMMENT)
      && first->GetParentType() == CT_COMMENT_EMBED)
   {
      // Add or remove space after an embedded comment.
      // Number of spaces after an embedded comment.
      log_rule("sp_after_emb_cmt");
      log_rule("sp_num_after_emb_cmt");
      min_sp = options::sp_num_after_emb_cmt();
      return(options::sp_after_emb_cmt());
   }

   if (  first->Is(CT_NEW)
      && second->Is(CT_PAREN_OPEN))
   {
      // c# new Constraint, c++ new operator
      // Add or remove space between 'new' and '(' in 'new()'.
      log_rule("sp_between_new_paren");
      return(options::sp_between_new_paren());
   }

   if (  first->Is(CT_NEW)
      || first->Is(CT_DELETE)
      || (  first->Is(CT_TSQUARE)
         && first->GetParentType() == CT_DELETE))
   {
      // Add or remove space after 'new', 'delete' and 'delete[]'.
      log_rule("sp_after_new");
      return(options::sp_after_new());
   }

   if (  language_is_set(lang_flag_e::LANG_JAVA)
      && first->Is(CT_ANNOTATION)
      && second->IsParenOpen())
   {
      // (Java) Add or remove space between an annotation and the open parenthesis.
      log_rule("sp_annotation_paren");
      return(options::sp_annotation_paren());
   }

   if (  language_is_set(lang_flag_e::LANG_OC)
      && first->Is(CT_OC_PROPERTY))
   {
      // (OC) Add or remove space after '@property'.
      log_rule("sp_after_oc_property");
      return(options::sp_after_oc_property());
   }

   if (  language_is_set(lang_flag_e::LANG_D)
      && first->Is(CT_EXTERN)
      && second->Is(CT_PAREN_OPEN))
   {
      // (D) Add or remove space between 'extern' and '(' as in 'extern (C)'.
      log_rule("sp_extern_paren");
      return(options::sp_extern_paren());
   }

   if (  second->Is(CT_TYPE)
      && (  (  first->Is(CT_STRING)
            && first->GetParentType() == CT_EXTERN)
         || (  first->Is(CT_FPAREN_CLOSE)
            && first->GetParentType() == CT_ATTRIBUTE)))
   {
      log_rule("FORCE");
      return(IARF_FORCE);  /* TODO: make this configurable? */
   }

   if (  first->Is(CT_STRING)                        // Issue #4176
      && second->Is(CT_STRING))
   {
      log_rule("sp_string_string");
      return(options::sp_string_string());
   }

   if (first->Is(CT_NOEXCEPT))
   {
      // Add or remove space after 'noexcept'.
      log_rule("sp_after_noexcept");
      return(options::sp_after_noexcept());
   }

   // Issue #2138
   if (first->Is(CT_FPAREN_CLOSE))
   {
      if (second->Is(CT_QUALIFIER))
      {
         // Add or remove space between ')' and a qualifier such as 'const'.
         log_rule("sp_paren_qualifier");
         return(options::sp_paren_qualifier());
      }
      else if (second->Is(CT_NOEXCEPT))
      {
         // Add or remove space between ')' and 'noexcept'.
         log_rule("sp_paren_noexcept");
         return(options::sp_paren_noexcept());
      }
   }

   // Issue #2098
   if (  first->Is(CT_PP_PRAGMA)
      && second->Is(CT_PREPROC_BODY))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   // Issue #1733
   if (  first->Is(CT_OPERATOR_VAL)
      && second->Is(CT_TYPE))
   {
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }

   // Issue #995
   if (  first->Is(CT_DO)
      && second->Is(CT_BRACE_OPEN))
   {
      // Add or remove space between 'do' and '{'.
      log_rule("sp_do_brace_open");
      return(options::sp_do_brace_open());
   }

   // Issue #995
   if (  first->Is(CT_BRACE_CLOSE)
      && second->Is(CT_WHILE_OF_DO))
   {
      // Add or remove space between '}' and 'while.
      log_rule("sp_brace_close_while");
      return(options::sp_brace_close_while());
   }

   // TODO: have a look to Issue #2186, why NEWLINE?
   // Issue #2524
   if (  first->Is(CT_NEWLINE)
      && second->Is(CT_BRACE_OPEN))
   {
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }

   // Issue #4376
   if (  first->Is(CT_STRUCT)
      && second->Is(CT_TYPE))
   {
      // Add or remove space 'struct' and a type.
      log_rule("sp_struct_type");
      return(options::sp_struct_type());
   }

   // =============================================================
   // category 0
   // this table lists out all combos where nothing is to do
   // CT_UNKNOWN is a wildcard.
   for (auto it : IGNORE_space_table)
   {
      if (  (  it.first == CT_UNKNOWN
            || it.first == first->GetType())
         && (  it.second == CT_UNKNOWN
            || it.second == second->GetType()))
      {
         log_rule("IGNORE from IGNORE_space_table");
         return(IARF_IGNORE);
      }
   }

   // =============================================================
   // category 1
   // this table lists out all combos where a space should NOT be present
   // CT_UNKNOWN is a wildcard.
   size_t number = 0;

   for (auto it : no_space_table)
   {
      if (  (  it.first == CT_UNKNOWN
            || it.first == first->GetType())
         && (  it.second == CT_UNKNOWN
            || it.second == second->GetType()))
      {
         char text[80];
         snprintf(text, sizeof(text), "REMOVE from no_space_table @ %zu.", number);
         log_rule(text);
         return(IARF_REMOVE);
      }
      number++;
   }

   // =============================================================
   // category 2
   // this table lists out all combos where a space MUST be present
   number = 0;

   for (auto it : add_space_table)
   {
      if (  it.first == first->GetType()
         && it.second == second->GetType())
      {
         char text[80];
         snprintf(text, sizeof(text), "ADD from add_space_table @ %zu.", number);
         log_rule(text);
         return(IARF_ADD);
      }
      number++;
   }

   // Issue #2386
   if (  first->Is(CT_FORM_FEED)
      || second->Is(CT_FORM_FEED))
   {
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }

   // TODO: if necessary create a new option
   if (  first->Is(CT_MACRO_FUNC_CALL)
      && second->Is(CT_FPAREN_OPEN))
   {
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }
   //// TODO: if necessary create a new option
   //if (  first->Is(CT_RPAREN_OPEN)
   //   && second->Is(CT_RPAREN_CLOSE))
   //{
   //   log_rule("IGNORE");
   //   return(IARF_IGNORE);
   //}

   //// TODO: if necessary create a new option
   //if (  first->Is(CT_RPAREN_OPEN)
   //   && second->Is(CT_UNKNOWN))
   //{
   //   log_rule("IGNORE");
   //   return(IARF_IGNORE);
   //}

   //// TODO: if necessary create a new option
   //if (  first->Is(CT_UNKNOWN)
   //   && second->Is(CT_RPAREN_CLOSE))
   //{
   //   log_rule("IGNORE");
   //   return(IARF_IGNORE);
   //}

   if (  first->Is(CT_CASE_ELLIPSIS)
      && second->Is(CT_NUMBER))
   {
      // Add or remove space after the variadic '...' when preceded by a
      // non-punctuator.
      // The value REMOVE will be overridden with FORCE
      if (options::sp_after_ellipsis() == IARF_REMOVE)
      {
         log_rule("sp_after_ellipsis/FORCE");
         return(IARF_FORCE);
      }
      else
      {
         log_rule("sp_after_ellipsis");
         return(options::sp_after_ellipsis());
      }
   }

   if (  first->Is(CT_NUMBER)
      && second->Is(CT_CASE_ELLIPSIS))
   {
      // Add or remove space before the variadic '...' when preceded by a
      // non-punctuator.
      // The value REMOVE will be overridden with FORCE
      if (options::sp_before_ellipsis() == IARF_REMOVE)
      {
         log_rule("sp_before_ellipsis");
         return(IARF_FORCE);
      }
      else
      {
         log_rule("sp_before_ellipsis");
         return(options::sp_before_ellipsis());
      }
   }
   // =============================================================
   // category 3
   // these lines are only useful for debugging uncrustify itself
   LOG_FMT(LSPACE, "\n\n%s(%d): WARNING: unrecognize do_space:\n",
           __func__, __LINE__);
   LOG_FMT(LSPACE, "   first orig line  is %zu, orig col  is %zu, Text()  '%s', GetType() is  %s\n",
           first->GetOrigLine(), first->GetOrigCol(), first->Text(), get_token_name(first->GetType()));
   LOG_FMT(LSPACE, "   second orig line is %zu, orig col is %zu, Text() '%s', GetType() is %s\n",
           second->GetOrigLine(), second->GetOrigCol(), second->Text(), get_token_name(second->GetType()));
   LOG_FMT(LSPACE, "   Please make a call at https://github.com/uncrustify/uncrustify/issues/new\n");
   LOG_FMT(LSPACE, "   or merge the line:\n");
   LOG_FMT(LSPACE, "   { CT_%s,    CT_%s},\n",
           get_token_name(first->GetType()), get_token_name(second->GetType()));
   LOG_FMT(LSPACE, "   in the file <Path_to_uncrustify>/src/add_space_table.h\n");

   log_rule("ADD as default value");
   return(IARF_ADD);
} // do_space


static iarf_e ensure_force_space(Chunk *first, Chunk *second, iarf_e av)
{
   if (first->TestFlags(PCF_FORCE_SPACE))
   {
      LOG_FMT(LSPACE, "%s(%d): force between '%s' and '%s'\n",
              __func__, __LINE__, first->Text(), second->Text());
      return(av | IARF_ADD);
   }
   return(av);
}


const char *decode_IARF(iarf_e av)
{
   switch (av)
   {
   case IARF_IGNORE:
      return("IGNORE");

   case IARF_ADD:
      return("ADD");

   case IARF_REMOVE:
      return("REMOVE");

   case IARF_FORCE:
      return("FORCE");
   }
   return("???????");
} // decode_IARF


static iarf_e do_space_ensured(Chunk *first, Chunk *second, int &min_sp)
{
   return(ensure_force_space(first, second, do_space(first, second, min_sp)));
}


void space_text()
{
   LOG_FUNC_ENTRY();

   Chunk  *pc = Chunk::GetHead();
   Chunk  *next;
   size_t prev_column;
   size_t column = pc->GetColumn();

   while (pc->IsNotNullChunk())
   {
      if (pc->Is(CT_NEWLINE))
      {
         LOG_FMT(LSPACE, "%s(%d): orig line is %zu, orig col is %zu, <Newline>, nl is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetNlCount());
      }
      else
      {
         char copy[1000];
         LOG_FMT(LSPACE, "%s(%d): orig line is %zu, orig col is %zu, '%s' type is %s\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->ElidedText(copy), get_token_name(pc->GetType()));
      }

      if (  (options::use_options_overriding_for_qt_macros())
         && (  (strcmp(pc->Text(), "SIGNAL") == 0)
            || (strcmp(pc->Text(), "SLOT") == 0)))
      {
         LOG_FMT(LSPACE, "%s(%d): orig col is %zu, type is %s SIGNAL/SLOT found\n",
                 __func__, __LINE__, pc->GetOrigLine(), get_token_name(pc->GetType()));
         pc->SetFlagBits(PCF_IN_QT_MACRO); // flag the chunk for a second processing

         // save the values
         save_set_options_for_QT(pc->GetLevel());
      }
      log_rule_B("sp_skip_vbrace_tokens");

      // Bug # 637
      // If true, vbrace tokens are dropped to the previous token and skipped.
      if (options::sp_skip_vbrace_tokens())
      {
         next = pc->GetNext();

         while (  next->IsEmptyText()
               && !next->IsNewline()
               && next->IsVBrace())
         {
            log_rule_B("sp_skip_vbrace_tokens JA 3");
            LOG_FMT(LSPACE, "%s(%d): orig line is %zu, orig col is %zu, Skip %s (%zu+%zu)\n",
                    __func__, __LINE__, next->GetOrigLine(), next->GetOrigCol(), get_token_name(next->GetType()),
                    pc->GetColumn(), pc->GetStr().size());
            next->SetColumn(pc->GetColumn() + pc->GetStr().size());
            next = next->GetNext();
         }
      }
      else
      {
         next = pc->GetNext();
      }

      if (next->IsNullChunk())
      {
         break;
      }

      // Issue # 481
      // Whether to balance spaces inside nested parentheses.
      if (QT_SIGNAL_SLOT_found)
      {
         Chunk *nn = next->GetNext();                                    // Issue #2734

         if (  nn->IsNotNullChunk()
            && nn->Is(CT_SPACE))
         {
            Chunk::Delete(nn); // remove the space
         }
      }

      /*
       * If the current chunk contains a newline, do not change the column
       * of the next item
       */
      if (  pc->IsNewline()
         || pc->Is(CT_COMMENT_MULTI))
      {
         column = next->GetColumn();
      }
      else
      {
         // Set to the minimum allowed column
         if (pc->GetNlCount() == 0)
         {
            column += pc->Len();
         }
         else
         {
            column = pc->GetOrigColEnd();
         }
         prev_column = column;

         /*
          * Apply a general safety check
          * If the two chunks combined will tokenize differently, then we
          * must force a space.
          * Two chunks -- "()" and "[]" will always tokenize differently.
          * They are always safe to not have a space after them.
          */
         pc->ResetFlagBits(PCF_FORCE_SPACE);

         if (  (pc->Len() > 0)
            && !pc->IsString("[]")
            && !pc->IsString("{{")
            && !pc->IsString("}}")
            && !pc->IsString("()")
            && !pc->GetStr().startswith("@\""))
         {
            // Find the next non-empty chunk on this line
            Chunk *tmp = next;

            while (  tmp->IsNotNullChunk()
                  && (tmp->Len() == 0)
                  && !tmp->IsNewline())
            {
               tmp = tmp->GetNext();
            }

            if (  tmp->IsNotNullChunk()
               && tmp->Len() > 0)
            {
               bool kw1 = CharTable::IsKw2(pc->GetStr()[pc->Len() - 1]);
               bool kw2 = CharTable::IsKw1(next->GetStr()[0]);

               if (  kw1
                  && kw2)
               {
                  // back-to-back words need a space
                  LOG_FMT(LSPACE, "%s(%d): back-to-back words need a space: pc->Text() '%s', next->Text() '%s'\n",
                          __func__, __LINE__, pc->Text(), next->Text());
                  pc->SetFlagBits(PCF_FORCE_SPACE);
               }
               // TODO:  what is the meaning of 4
               else if (  !kw1
                       && !kw2
                       && (pc->Len() < 4)
                       && (next->Len() < 4))
               {
                  // We aren't dealing with keywords. concat and try punctuators
                  char buf[9];
                  memcpy(buf, pc->Text(), pc->Len());
                  memcpy(buf + pc->Len(), next->Text(), next->Len());
                  buf[pc->Len() + next->Len()] = 0;

                  const chunk_tag_t *ct;
                  ct = find_punctuator(buf, cpd.lang_flags);

                  if (  ct != nullptr
                     && (strlen(ct->tag) != pc->Len()))
                  {
                     // punctuator parsed to a different size..

                     /*
                      * C++11 allows '>>' to mean '> >' in templates:
                      *   some_func<vector<string>>();
                      */
                     // (C++11) Permit removal of the space between '>>' in 'foo<bar<int> >'. Note
                     // that sp_angle_shift cannot remove the space without this option.
                     log_rule_B("sp_permit_cpp11_shift");

                     if (  (  (  language_is_set(lang_flag_e::LANG_CPP)
                              && options::sp_permit_cpp11_shift())
                           || language_is_set(lang_flag_e::LANG_JAVA)
                           || language_is_set(lang_flag_e::LANG_CS)
                           || language_is_set(lang_flag_e::LANG_VALA)
                           || language_is_set(lang_flag_e::LANG_OC))
                        && pc->Is(CT_ANGLE_CLOSE)
                        && next->Is(CT_ANGLE_CLOSE))
                     {
                        // allow '>' and '>' to become '>>'
                     }
                     else if (strcmp(ct->tag, "[]") == 0)
                     {
                        // this is OK
                     }
                     else
                     {
                        LOG_FMT(LSPACE, "%s(%d): : pc->Text() is %s, next->Text() is %s\n",
                                __func__, __LINE__, pc->Text(), next->Text());
                        pc->SetFlagBits(PCF_FORCE_SPACE);
                     }
                  }
               }
            }
         }
         int min_sp;
         LOG_FMT(LSPACE, "%s(%d): orig line is %zu, orig col is %zu, pc-Text() '%s', type is %s\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
         iarf_e av = do_space_ensured(pc, next, min_sp);
         min_sp = max(1, min_sp);

         switch (av)
         {
         case IARF_FORCE:
            column += min_sp;  // add exactly the specified number of spaces
            break;

         case IARF_ADD:
         {
            int delta = min_sp;

            // Handle the condition of a CT_VBRACE_OPEN with more complicated spacing mechanics.
            if (  pc->Is(CT_VBRACE_OPEN)
               && next->GetOrigCol() >= pc->GetPrev()->GetOrigCol())
            {
               delta = next->GetOrigCol() - pc->GetPrev()->GetOrigCol();

               if ((delta - 1) < min_sp)
               {
                  column += min_sp;
                  break;
               }
               column += (delta - 1);
               break;
            }

            if (  next->GetOrigCol() >= pc->GetOrigColEnd()
               && pc->GetOrigColEnd() != 0)
            {
               // Keep the same relative spacing, minimum 1
               delta = next->GetOrigCol() - pc->GetOrigColEnd();

               if (delta < min_sp)
               {
                  delta = min_sp;
               }
            }
            column += delta;
            break;
         }

         case IARF_REMOVE:
            // the symbols will be back-to-back "a+3"
            break;

         case IARF_IGNORE:

            // Keep the same relative spacing, if possible
            if (  next->GetOrigCol() >= pc->GetOrigColEnd()
               && pc->GetOrigColEnd() != 0)
            {
               column += next->GetOrigCol() - pc->GetOrigColEnd();
            }
            else
            {
               // preserve the position if virtual brace
               // Issue #1854
               if (pc->Is(CT_VBRACE_OPEN))
               {
                  column = next->GetOrigCol();
               }
            }
            break;

         default:
            // If we got here, something is wrong...     #Issue 4253
            LOG_FMT(LSPACE, "%s(%d): the av value is wrong %zu\n",
                    __func__, __LINE__, (size_t)av);
            LOG_FMT(LSPACE, "Please make a call at https://github.com/uncrustify/uncrustify/issues/new\n");
            exit(EX_SOFTWARE);
         } // switch

         if (  next->IsComment()
            && next->GetNext()->IsNewline()
            && column < next->GetOrigCol())
         {
            /*
             * do some comment adjustments if sp_before_tr_cmt and sp_endif_cmt
             * did not apply.
             */
            // Add or remove space between #else or #endif and a trailing comment.
            if (  (  options::sp_before_tr_cmt() == IARF_IGNORE
                  || next->GetParentType() != CT_COMMENT_END)
               && (  options::sp_endif_cmt() == IARF_IGNORE
                  || (  pc->IsNot(CT_PP_ELSE)
                     && pc->IsNot(CT_PP_ENDIF))))
            {
               if (options::indent_relative_single_line_comments())
               {
                  // Try to keep relative spacing between tokens
                  LOG_FMT(LSPACE, "%s(%d): <relative adj>", __func__, __LINE__);
                  LOG_FMT(LSPACE, "%s(%d): pc is '%s', orig col is %zu, next orig col is %zu, pc orig col end is %zu\n",
                          __func__, __LINE__, pc->Text(),
                          pc->GetOrigCol(), next->GetOrigCol(), pc->GetOrigColEnd());
                  column = pc->GetColumn() + (next->GetOrigCol() - pc->GetOrigColEnd());
               }
               else
               {
                  /*
                   * If there was a space, we need to force one, otherwise
                   * try to keep the comment in the same column.
                   */
                  size_t col_min = pc->GetColumn() + pc->Len() + ((next->GetOrigPrevSp() > 0) ? 1 : 0);
                  column = next->GetOrigCol();

                  if (column < col_min)
                  {
                     column = col_min;
                  }
                  LOG_FMT(LSPACE, "%s(%d): <relative set>", __func__, __LINE__);
               }
            }
         }
         next->SetColumn(column);
         LOG_FMT(LSPACE, "%s(%d): orig line is %zu, orig col is %zu, pc-Text() '%s', type is %s\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text(), get_token_name(pc->GetType()));
         LOG_FMT(LSPACE, "%s(%d): ",
                 __func__, __LINE__);
         LOG_FMT(LSPACE, "   rule = %s @ %zu => %zu\n",
                 decode_IARF(av),
                 column - prev_column, next->GetColumn());

         if (restoreValues)    // guy 2015-09-22
         {
            restore_options_for_QT();
         }
      }
      pc = next;

      if (QT_SIGNAL_SLOT_found)
      {
         // flag the chunk for a second processing
         pc->SetFlagBits(PCF_IN_QT_MACRO);
      }
   }
} // space_text


size_t space_needed(Chunk *first, Chunk *second)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPACE, "%s(%d)\n", __func__, __LINE__);

   int min_sp;

   switch (do_space_ensured(first, second, min_sp))
   {
   case IARF_ADD:
   case IARF_FORCE:
      return(max(1, min_sp));

   case IARF_REMOVE:
      return(0);

   case IARF_IGNORE:
   default:
      return(second->GetOrigCol() > (first->GetOrigCol() + first->Len()));
   }
}


size_t space_col_align(Chunk *first, Chunk *second)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LSPACE, "%s(%d): 1st orig line %zu, orig col %zu, [%s/%s], text '%s' <==>\n",
           __func__, __LINE__, first->GetOrigLine(), first->GetOrigCol(),
           get_token_name(first->GetType()), get_token_name(first->GetParentType()),
           first->Text());
   LOG_FMT(LSPACE, "%s(%d): 2nd orig line %zu, orig col %zu, [%s/%s], text '%s'\n",
           __func__, __LINE__, second->GetOrigLine(), second->GetOrigCol(),
           get_token_name(second->GetType()), get_token_name(second->GetParentType()),
           second->Text());
   log_func_stack_inline(LSPACE);

   int    min_sp;
   iarf_e av = do_space_ensured(first, second, min_sp);

   LOG_FMT(LSPACE, "%s(%d): av is %s\n", __func__, __LINE__, to_string(av));
   size_t coldiff;

   if (first->GetNlCount() > 0)
   {
      LOG_FMT(LSPACE, "%s(%d):    new line count %zu, orig col end %zu\n",
              __func__, __LINE__, first->GetNlCount(), first->GetOrigColEnd());
      coldiff = first->GetOrigColEnd() - 1;
   }
   else
   {
      LOG_FMT(LSPACE, "%s(%d):    '1st' len is %zu\n", __func__, __LINE__, first->Len());
      coldiff = first->Len();
   }
   LOG_FMT(LSPACE, "%s(%d):    => coldiff is %zu\n", __func__, __LINE__, coldiff);

   LOG_FMT(LSPACE, "%s(%d):    => av is %s\n", __func__, __LINE__,
           decode_IARF(av));

   switch (av)
   {
   case IARF_ADD:
   case IARF_FORCE:
      coldiff++;
      break;

   case IARF_REMOVE:
      break;

   case IARF_IGNORE:                // Issue #2064
      LOG_FMT(LSPACE, "%s(%d):    => first orig line  is %zu\n", __func__, __LINE__, first->GetOrigLine());
      LOG_FMT(LSPACE, "%s(%d):    => second orig line is %zu\n", __func__, __LINE__, second->GetOrigLine());
      LOG_FMT(LSPACE, "%s(%d):    => first text       is '%s'\n", __func__, __LINE__, first->Text());
      LOG_FMT(LSPACE, "%s(%d):    => second text      is '%s'\n", __func__, __LINE__, second->Text());
      LOG_FMT(LSPACE, "%s(%d):    => first orig col   is %zu\n", __func__, __LINE__, first->GetOrigCol());
      LOG_FMT(LSPACE, "%s(%d):    => second orig col  is %zu\n", __func__, __LINE__, second->GetOrigCol());
      LOG_FMT(LSPACE, "%s(%d):    => first len        is %zu\n", __func__, __LINE__, first->Len());

      if (  first->GetOrigLine() == second->GetOrigLine()
         && second->GetOrigCol() > (first->GetOrigCol() + first->Len()))
      {
         coldiff++;
      }
      break;

   default:
      // If we got here, something is wrong...
      break;
   }
   LOG_FMT(LSPACE, "%s(%d):    => coldiff is %zu\n", __func__, __LINE__, coldiff);
   return(coldiff);
} // space_col_align


void space_add_after(Chunk *pc, size_t count)
{
   LOG_FUNC_ENTRY();

   Chunk *next = pc->GetNext();

   // don't add at the end of the file or before a newline
   if (  next->IsNullChunk()
      || next->IsNewline())
   {
      return;
   }

   // Limit to 16 spaces
   if (count > 16)
   {
      count = 16;
   }

   // Two CT_SPACE in a row -- use the max of the two
   if (next->Is(CT_SPACE))
   {
      if (next->Len() < count)
      {
         while (next->Len() < count)
         {
            next->Str().append(' ');
         }
      }
      return;
   }
   Chunk sp;

   sp.SetType(CT_SPACE);
   sp.SetFlags(pc->GetFlags() & PCF_COPY_FLAGS);
   sp.Str() = "                ";         // 16 spaces
   sp.Str().resize(count);
   sp.SetLevel(pc->GetLevel());
   sp.SetBraceLevel(pc->GetBraceLevel());
   sp.SetPpLevel(pc->GetPpLevel());
   sp.SetColumn(pc->GetColumn() + pc->Len());
   sp.SetOrigLine(pc->GetOrigLine());
   sp.SetOrigCol(pc->GetOrigCol());

   sp.CopyAndAddAfter(pc);
} // space_add_after
