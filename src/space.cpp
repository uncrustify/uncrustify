/**
 * @file space.cpp
 * Adds or removes inter-chunk spaces.
 *
 * Informations
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
 * @author  Guy Maurel, 2015-2021
 * @license GPL v2+
 */

#include "space.h"

#include "log_rules.h"
#include "options_for_QT.h"
#include "punctuators.h"

#ifdef WIN32
#include <algorithm>                   // to get max
#endif // ifdef WIN32

using namespace std;
using namespace uncrustify;


/**
 * Decides how to change inter-chunk spacing.
 * Note that the order of the if statements is VERY important.
 *
 * @param first   The first chunk
 * @param second  The second chunk
 *
 * @return IARF_IGNORE, IARF_ADD, IARF_REMOVE or IARF_FORCE
 */
static iarf_e do_space(Chunk *first, Chunk *second, int &min_sp);

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

//! type that stores two chunks between those no space shall occur
struct no_space_table_t
{
   E_Token first;  //! first  chunk
   E_Token second; //! second chunk
};


/**
 * this table lists out all combos where a space MUST be present
 * CT_UNKNOWN is a wildcard.
 *
 * TODO: some of these are no longer needed.
 */
const no_space_table_t add_space_table[] =
{
#include "add_space_table.h"
};


/**
 * this table lists out all combos where a space should NOT be present
 * CT_UNKNOWN is a wildcard.
 *
 * TODO: some of these are no longer needed.
 */
const no_space_table_t no_space_table[] =
{
   { CT_OC_AT,          CT_UNKNOWN      },
   { CT_INCDEC_BEFORE,  CT_WORD         },
   { CT_UNKNOWN,        CT_INCDEC_AFTER },
   { CT_UNKNOWN,        CT_LABEL_COLON  },
   { CT_UNKNOWN,        CT_ACCESS_COLON },
   { CT_UNKNOWN,        CT_SEMICOLON    },
   { CT_UNKNOWN,        CT_D_TEMPLATE   },
   { CT_D_TEMPLATE,     CT_UNKNOWN      },
   { CT_MACRO_FUNC,     CT_FPAREN_OPEN  },
   { CT_PAREN_OPEN,     CT_UNKNOWN      },
   { CT_UNKNOWN,        CT_PAREN_CLOSE  },
   { CT_FPAREN_OPEN,    CT_UNKNOWN      },
   { CT_UNKNOWN,        CT_SPAREN_CLOSE },
   { CT_SPAREN_OPEN,    CT_UNKNOWN      },
   { CT_UNKNOWN,        CT_FPAREN_CLOSE },
   { CT_UNKNOWN,        CT_COMMA        },
   { CT_POS,            CT_UNKNOWN      },
   { CT_STAR,           CT_UNKNOWN      },
   { CT_VBRACE_CLOSE,   CT_UNKNOWN      },
   { CT_VBRACE_OPEN,    CT_UNKNOWN      },
   { CT_UNKNOWN,        CT_VBRACE_CLOSE },
   { CT_UNKNOWN,        CT_VBRACE_OPEN  },
   { CT_PREPROC,        CT_UNKNOWN      },
   { CT_PREPROC_INDENT, CT_UNKNOWN      },
   { CT_NEG,            CT_UNKNOWN      },
   { CT_UNKNOWN,        CT_SQUARE_OPEN  },
   { CT_UNKNOWN,        CT_SQUARE_CLOSE },
   { CT_SQUARE_OPEN,    CT_UNKNOWN      },
   { CT_PAREN_CLOSE,    CT_WORD         },
   { CT_PAREN_CLOSE,    CT_FUNC_DEF     },
   { CT_PAREN_CLOSE,    CT_FUNC_CALL    },
   { CT_PAREN_CLOSE,    CT_ADDR         },
   { CT_PAREN_CLOSE,    CT_FPAREN_OPEN  },
   { CT_OC_SEL_NAME,    CT_OC_SEL_NAME  },
   { CT_TYPENAME,       CT_TYPE         },
};


bool token_is_within_trailing_return(Chunk *pc)
{
   // look back for '->' type is TRAILING_RET
   // until CT_FPAREN_CLOSE
   //   or  CT_FPAREN_OPEN is found
   Chunk *prev = pc;

   if (prev == nullptr)
   {
      prev = Chunk::NullChunkPtr;
   }

   while (prev->IsNotNullChunk())
   {
      if (chunk_is_token(prev, CT_TRAILING_RET))
      {
         return(true);
      }
      else if (  chunk_is_token(prev, CT_FPAREN_CLOSE)
              || chunk_is_token(prev, CT_FPAREN_OPEN))
      {
         return(false);
      }
      else
      {
         prev = prev->GetPrev();
      }
   }
   return(false);
} // token_is_within_trailing_return


/*
 * this function is called for every chunk in the input file.
 * Thus it is important to keep this function efficient
 */
static iarf_e do_space(Chunk *first, Chunk *second, int &min_sp)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LSPACE, "%s(%d): orig_line is %zu, orig_col is %zu, first->Text() '%s', type is %s\n",
           __func__, __LINE__, first->orig_line, first->orig_col, first->Text(), get_token_name(first->type));

   min_sp = 1;

   if (  chunk_is_token(first, CT_IGNORED)
      || chunk_is_token(second, CT_IGNORED))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  chunk_is_token(first, CT_PP_IGNORE)
      && chunk_is_token(second, CT_PP_IGNORE))
   {
      // Leave spacing alone between PP_IGNORE tokens as we don't want the default behavior (which is ADD).
      log_rule("PP_IGNORE");
      return(IARF_IGNORE);
   }

   if (  chunk_is_token(first, CT_PP)
      || chunk_is_token(second, CT_PP))
   {
      // Add or remove space around preprocessor '##' concatenation operator.
      log_rule("sp_pp_concat");
      return(options::sp_pp_concat());
   }

   if (chunk_is_token(first, CT_POUND))
   {
      // Add or remove space after preprocessor '#' stringify operator.
      // Also affects the '#@' charizing operator.
      log_rule("sp_pp_stringify");
      return(options::sp_pp_stringify());
   }

   if (  chunk_is_token(second, CT_POUND)
      && second->flags.test(PCF_IN_PREPROC)
      && get_chunk_parent_type(first) != CT_MACRO_FUNC)
   {
      // Add or remove space before preprocessor '#' stringify operator
      // as in '#define x(y) L#y'.
      log_rule("sp_before_pp_stringify");
      return(options::sp_before_pp_stringify());
   }

   if (  chunk_is_token(first, CT_SPACE)
      || chunk_is_token(second, CT_SPACE))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (chunk_is_token(first, CT_DECLSPEC))  // Issue 1289
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  chunk_is_token(second, CT_NEWLINE)
      || chunk_is_token(second, CT_VBRACE_OPEN))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  chunk_is_token(first, CT_VBRACE_OPEN)
      && second->type != CT_NL_CONT
      && second->type != CT_SEMICOLON) // # Issue 1158
   {
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   if (  chunk_is_token(first, CT_VBRACE_CLOSE)
      && second->type != CT_NL_CONT)
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (chunk_is_token(second, CT_VSEMICOLON))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (chunk_is_token(first, CT_MACRO_FUNC))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (chunk_is_token(second, CT_NL_CONT))
   {
      // Add or remove space before a backslash-newline at the end of a line.
      log_rule("sp_before_nl_cont");
      return(options::sp_before_nl_cont());
   }

   if (  language_is_set(LANG_D)
      && (  chunk_is_token(first, CT_D_ARRAY_COLON)
         || chunk_is_token(second, CT_D_ARRAY_COLON)))
   {
      // (D) Add or remove around the D named array initializer ':' operator.
      log_rule("sp_d_array_colon");
      return(options::sp_d_array_colon());
   }

   if (  chunk_is_token(first, CT_CASE)
      && ((  CharTable::IsKw1(second->str[0])
          || chunk_is_token(second, CT_NUMBER))))
   {
      // Fix the spacing between 'case' and the label. Only 'ignore' and 'force' make
      // sense here.
      log_rule("sp_case_label");
      return(options::sp_case_label() | IARF_ADD);
   }

   if (chunk_is_token(first, CT_FOR_COLON))
   {
      // java
      // Add or remove space after ':' in a Java/C++11 range-based 'for',
      // as in 'for (Type var : expr)'.
      log_rule("sp_after_for_colon");
      return(options::sp_after_for_colon());
   }

   if (chunk_is_token(second, CT_FOR_COLON))
   {
      // java
      // Add or remove space before ':' in a Java/C++11 range-based 'for',
      // as in 'for (Type var : expr)'.
      log_rule("sp_before_for_colon");
      return(options::sp_before_for_colon());
   }

   if (  chunk_is_token(first, CT_QUESTION)
      && chunk_is_token(second, CT_COND_COLON))
   {
      // In the abbreviated ternary form '(a ?: b)', add or remove space between '?'
      // and ':'.
      // Overrides all other sp_cond_* options.
      log_rule("sp_cond_ternary_short");
      return(options::sp_cond_ternary_short());
   }

   if (  chunk_is_token(first, CT_QUESTION)                  // see the tests cpp:34513-34516
      || chunk_is_token(second, CT_QUESTION))
   {
      if (  chunk_is_token(second, CT_QUESTION)
         && (options::sp_cond_question_before() != IARF_IGNORE))
      {
         // Add or remove space before the '?' in 'b ? t : f'.
         // Overrides sp_cond_question.
         log_rule("sp_cond_question_before");
         return(options::sp_cond_question_before());
      }

      if (  chunk_is_token(first, CT_QUESTION)
         && (options::sp_cond_question_after() != IARF_IGNORE))
      {
         // Add or remove space after the '?' in 'b ? t : f'.
         // Overrides sp_cond_question.
         log_rule("sp_cond_question_after");
         return(options::sp_cond_question_after());
      }
      log_rule("sp_cond_question");
      return(options::sp_cond_question());
   }

   if (  chunk_is_token(first, CT_COND_COLON)
      || chunk_is_token(second, CT_COND_COLON))
   {
      if (  chunk_is_token(second, CT_COND_COLON)
         && (options::sp_cond_colon_before() != IARF_IGNORE))
      {
         // Add or remove space before the ':' in 'b ? t : f'.
         // Overrides sp_cond_colon.
         log_rule("sp_cond_colon_before");
         return(options::sp_cond_colon_before());
      }

      if (  chunk_is_token(first, CT_COND_COLON)
         && (options::sp_cond_colon_after() != IARF_IGNORE))
      {
         // Add or remove space after the ':' in 'b ? t : f'.
         // Overrides sp_cond_colon.
         log_rule("sp_cond_colon_after");
         return(options::sp_cond_colon_after());
      }
      // Issue #2596
      // replace "if (chunk_is_token(first, CT_WORD) && chunk_is_token(second, CT_COND_COLON))"
      // Add or remove space around the ':' in 'b ? t : f'.
      log_rule("sp_cond_colon");
      return(options::sp_cond_colon());
   }

   if (  language_is_set(LANG_D)
      && (  chunk_is_token(first, CT_RANGE)
         || chunk_is_token(second, CT_RANGE)))
   {
      // (D) Add or remove space around the D '..' operator.
      log_rule("sp_range");
      return(options::sp_range());
   }

   if (  chunk_is_token(first, CT_COLON)
      && get_chunk_parent_type(first) == CT_SQL_EXEC)
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   // Macro stuff can only return IGNORE, ADD, or FORCE
   if (chunk_is_token(first, CT_MACRO))
   {
      // Add or remove space between a macro name and its definition.
      log_rule("sp_macro");
      iarf_e arg = options::sp_macro();
      return(arg | ((arg != IARF_IGNORE) ? IARF_ADD : IARF_IGNORE));
   }

   if (  chunk_is_token(first, CT_FPAREN_CLOSE)
      && get_chunk_parent_type(first) == CT_MACRO_FUNC)
   {
      // Add or remove space between a macro function ')' and its definition.
      log_rule("sp_macro_func");
      iarf_e arg = options::sp_macro_func();
      return(arg | ((arg != IARF_IGNORE) ? IARF_ADD : IARF_IGNORE));
   }

   if (chunk_is_token(first, CT_PREPROC))
   {
      // Remove spaces, unless we are ignoring. See indent_preproc()
      if (options::pp_space() == IARF_IGNORE)
      {
         log_rule("pp_space");
         log_rule("IGNORE");
         return(IARF_IGNORE);
      }
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (chunk_is_token(second, CT_SEMICOLON))                       // see the tests cpp:34517-34519
   {
      if (chunk_is_token(first, CT_VBRACE_OPEN))
      {
         // Add or remove space before ';'.
         log_rule("sp_before_semi");
         return(options::sp_before_semi());
      }

      if (get_chunk_parent_type(second) == CT_FOR)
      {
         if (chunk_is_token(first, CT_SPAREN_OPEN))
         {
            // empty, e.g. for (;;)
            //                 ^ is first
            //                  ^ is second
            // Add or remove space before a semicolon of an empty left part of a for statement.
            log_rule("sp_before_semi_for_empty");
            return(options::sp_before_semi_for_empty());
         }

         if (chunk_is_token(first, CT_SEMICOLON))
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
      iarf_e arg = options::sp_before_semi();                      // see the tests cpp:34517-34519

      if (  chunk_is_token(first, CT_VBRACE_OPEN)                  // Issue #2942
         && chunk_is_token(first->prev, CT_SPAREN_CLOSE)
         && get_chunk_parent_type(first) != CT_WHILE_OF_DO)
      {
         // Add or remove space before empty statement ';' on 'if', 'for' and 'while'.
         log_rule("sp_special_semi");
         arg = arg | options::sp_special_semi();
      }
      else
      {
         // Add or remove space before ';'.
         log_rule("sp_before_semi");
      }
      return(arg);
   }

   if (  (  chunk_is_token(second, CT_COMMENT)
         || chunk_is_token(second, CT_COMMENT_CPP))
      && (  chunk_is_token(first, CT_PP_ELSE)
         || chunk_is_token(first, CT_PP_ENDIF)))
   {
      set_chunk_type(second, CT_COMMENT_ENDIF);
      // Add or remove space between #else or #endif and a trailing comment.
      log_rule("sp_endif_cmt");
      return(options::sp_endif_cmt());
   }

   if (  options::sp_before_tr_cmt() != IARF_IGNORE
      && get_chunk_parent_type(second) == CT_COMMENT_END)
   {
      // Add or remove space before a trailing comment.
      // Number of spaces before a trailing comment.
      log_rule("sp_num_before_tr_cmt");
      min_sp = options::sp_num_before_tr_cmt();
      return(options::sp_before_tr_cmt());
   }

   if (get_chunk_parent_type(second) == CT_COMMENT_END)
   {
      switch (second->orig_prev_sp)
      {
      case 0:
         log_rule("orig_prev_sp-REMOVE");
         return(IARF_REMOVE);

      case 1:
         log_rule("orig_prev_sp-FORCE");
         return(IARF_FORCE);

      default:
         log_rule("orig_prev_sp-ADD");
         return(IARF_ADD);
      }
   }

   // "for (;;)" vs. "for (;; )" and "for (a;b;c)" vs. "for (a; b; c)"
   if (chunk_is_token(first, CT_SEMICOLON))                        // see the tests cpp:34517-34519
   {
      if (get_chunk_parent_type(first) == CT_FOR)
      {
         if (chunk_is_token(second, CT_SPAREN_CLOSE))
         {
            // Add or remove space after the final semicolon of an empty part of a for
            // statement, as in 'for ( ; ; <here> )'.
            log_rule("sp_after_semi_for_empty");
            return(options::sp_after_semi_for_empty());
         }

         if (second->type != CT_SPAREN_CLOSE)  // Issue 1324
         {
            // Add or remove space after ';' in non-empty 'for' statements.
            log_rule("sp_after_semi_for");
            return(options::sp_after_semi_for());
         }
      }
      else if (  !second->IsComment()
              && second->type != CT_BRACE_CLOSE) // issue #197
      {
         // Add or remove space after ';', except when followed by a comment.
         // see the tests cpp:34517-34519
         log_rule("sp_after_semi");
         return(options::sp_after_semi());
      }
      // Let the comment spacing rules handle this
   }

   // puts a space in the rare '+-' or '-+'
   if (  (  chunk_is_token(first, CT_NEG)
         || chunk_is_token(first, CT_POS)
         || chunk_is_token(first, CT_ARITH)
         || chunk_is_token(first, CT_SHIFT))
      && (  chunk_is_token(second, CT_NEG)
         || chunk_is_token(second, CT_POS)
         || chunk_is_token(second, CT_ARITH)
         || chunk_is_token(second, CT_SHIFT)))
   {
      log_rule("ADD");
      return(IARF_ADD);
   }

   // "return(a);" vs. "return (foo_t)a + 3;" vs. "return a;" vs. "return;"
   if (chunk_is_token(first, CT_RETURN))
   {
      if (  chunk_is_token(second, CT_PAREN_OPEN)
         && get_chunk_parent_type(second) == CT_RETURN)
      {
         // Add or remove space between 'return' and '('.
         log_rule("sp_return_paren");
         return(options::sp_return_paren());
      }
      else if (  chunk_is_token(second, CT_BRACE_OPEN)
              && get_chunk_parent_type(second) == CT_BRACED_INIT_LIST)
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
   if (chunk_is_token(first, CT_SIZEOF))
   {
      if (chunk_is_token(second, CT_PAREN_OPEN))
      {
         // Add or remove space between 'sizeof' and '('.
         log_rule("sp_sizeof_paren");
         return(options::sp_sizeof_paren());
      }

      if (chunk_is_token(second, CT_ELLIPSIS))
      {
         // Add or remove space between 'sizeof' and '...'.
         log_rule("sp_sizeof_ellipsis");
         return(options::sp_sizeof_ellipsis());
      }
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   // "decltype(foo_t)" vs. "decltype (foo_t)"
   if (chunk_is_token(first, CT_DECLTYPE))
   {
      if (chunk_is_token(second, CT_PAREN_OPEN))
      {
         // Add or remove space between 'decltype' and '('.
         log_rule("sp_decltype_paren");
         return(options::sp_decltype_paren());
      }
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   // handle '::'
   if (chunk_is_token(first, CT_DC_MEMBER))
   {
      // Add or remove space after the '::' operator.
      log_rule("sp_after_dc");
      return(options::sp_after_dc());
   }

   // Issue #889
   // mapped_file_source abc((int) ::CW2A(sTemp));
   if (  chunk_is_token(first, CT_PAREN_CLOSE)
      && chunk_is_token(second, CT_DC_MEMBER)
      && second->next != nullptr
      && second->next->type == CT_FUNC_CALL)
   {
      log_rule("sp_after_cast");
      return(options::sp_after_cast());
   }

   if (chunk_is_token(second, CT_DC_MEMBER))
   {
      /* '::' at the start of an identifier is not member access, but global scope operator.
       * Detect if previous chunk is keyword
       */
      switch (first->type)
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

      if (  (  chunk_is_token(first, CT_WORD)
            || chunk_is_token(first, CT_TYPE)
            || chunk_is_token(first, CT_PAREN_CLOSE)
            || CharTable::IsKw1(first->str[0]))
         && (strcmp(first->Text(), "void") != 0)) // Issue 1249
      {
         // Add or remove space before the '::' operator.
         log_rule("sp_before_dc");
         return(options::sp_before_dc());
      }
   }

   // "a,b" vs. "a, b"
   if (chunk_is_token(first, CT_COMMA))                         // see the tests cpp:34520-34524
                                                                // see the tests c-sharp:12200-12202
   {
      if (  language_is_set(LANG_CS)
         && get_chunk_parent_type(first) == CT_TYPE)
      {
         // C# multidimensional array type: ',,' vs. ', ,' or ',]' vs. ', ]'
         if (chunk_is_token(second, CT_COMMA))
         {
            // (C#) Add or remove space between ',' in multidimensional array type
            // like 'int[,,]'.
            log_rule("sp_between_mdatype_commas");
            return(options::sp_between_mdatype_commas());
         }
         // (C#) Add or remove space between ',' and ']' in multidimensional array type
         // like 'int[,,]'.
         log_rule("sp_after_mdatype_commas");
         return(options::sp_after_mdatype_commas());
      }

      // Fix for issue #1243
      // Don't add extra space after comma immediately followed by Angle close
      if (chunk_is_token(second, CT_ANGLE_CLOSE))
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
      if (  chunk_is_token(first, CT_FPAREN_CLOSE)
         && (  chunk_is_token(second, CT_FPAREN_CLOSE)
            || chunk_is_token(second, CT_COMMA)))
      {
         if (second->level == QT_SIGNAL_SLOT_level)
         {
            restoreValues = true;
         }
      }
   }

   if (chunk_is_token(second, CT_COMMA))
   {
      if (  language_is_set(LANG_CS)
         && chunk_is_token(first, CT_SQUARE_OPEN)
         && get_chunk_parent_type(first) == CT_TYPE)
      {
         // Only for C#.
         // (C#) Add or remove space between '[' and ',' in multidimensional array type
         // like 'int[,,]'.
         log_rule("sp_before_mdatype_commas");
         return(options::sp_before_mdatype_commas());
      }

      if (  chunk_is_token(first, CT_PAREN_OPEN)
         || chunk_is_token(first, CT_FPAREN_OPEN))
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

   if (chunk_is_token(second, CT_ELLIPSIS))
   {
      // type followed by a ellipsis
      Chunk *tmp = first;

      if (  chunk_is_token(tmp, CT_PTR_TYPE)
         || chunk_is_token(tmp, CT_BYREF))
      {
         tmp = tmp->GetPrevNcNnl();
      }

      if (  chunk_is_token(tmp, CT_TYPE)
         || chunk_is_token(tmp, CT_QUALIFIER))
      {
         // Add or remove space between a type and '...'.
         log_rule("sp_type_ellipsis");
         return(options::sp_type_ellipsis());
      }

      // non-punc followed by a ellipsis
      if (  !first->flags.test(PCF_PUNCTUATOR)
         && (options::sp_before_ellipsis() != IARF_IGNORE))
      {
         // Add or remove space before the variadic '...' when preceded by a
         // non-punctuator.
         log_rule("sp_before_ellipsis");
         return(options::sp_before_ellipsis());
      }

      if (  chunk_is_token(first, CT_FPAREN_CLOSE)
         || chunk_is_token(first, CT_PAREN_CLOSE))
      {
         // Add or remove space between ')' and '...'.
         log_rule("sp_paren_ellipsis");
         return(options::sp_paren_ellipsis());
      }

      if (chunk_is_token(first, CT_TAG_COLON))
      {
         log_rule("FORCE");
         return(IARF_FORCE);
      }

      if (chunk_is_token(first, CT_BYREF))                         // Issue #3309
      {
         log_rule("sp_byref_ellipsis");
         return(options::sp_byref_ellipsis());
      }

      if (chunk_is_token(first, CT_PARAMETER_PACK))                // Issue #3309
      {
         log_rule("sp_parameter_pack_ellipsis");
         return(options::sp_parameter_pack_ellipsis());
      }
   }

   if (chunk_is_token(first, CT_ELLIPSIS))
   {
      if (chunk_is_token(second, CT_PARAMETER_PACK))                // Issue #3309
      {
         log_rule("sp_ellipsis_parameter_pack");
         return(options::sp_ellipsis_parameter_pack());
      }

      if (CharTable::IsKw1(second->str[0]))
      {
         log_rule("FORCE");
         return(IARF_FORCE);
      }

      if (  chunk_is_token(second, CT_PAREN_OPEN)
         && first->prev != nullptr
         && chunk_is_token(first->prev, CT_SIZEOF))
      {
         // Add or remove space between 'sizeof...' and '('.
         log_rule("sp_sizeof_ellipsis_paren");
         return(options::sp_sizeof_ellipsis_paren());
      }
   }

   if (  language_is_set(LANG_PAWN)
      && chunk_is_token(first, CT_TAG_COLON))
   {
      // (Pawn) Add or remove space after the tag keyword.
      log_rule("sp_after_tag");
      return(options::sp_after_tag());
   }

   if (chunk_is_token(second, CT_TAG_COLON))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   // handle '~'
   if (chunk_is_token(first, CT_DESTRUCTOR))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  language_is_set(LANG_OC)
      && chunk_is_token(first, CT_CATCH)
      && chunk_is_token(second, CT_SPAREN_OPEN)
      && (options::sp_oc_catch_paren() != IARF_IGNORE))
   {
      // (OC) Add or remove space between '@catch' and '('
      // in '@catch (something) { }'. If set to ignore, sp_catch_paren is used.
      log_rule("sp_oc_catch_paren");
      return(options::sp_oc_catch_paren());
   }

   if (  language_is_set(LANG_OC)
      && (  chunk_is_token(first, CT_PAREN_CLOSE)
         || chunk_is_token(first, CT_OC_CLASS)
         || chunk_is_token(first, CT_WORD))
      && chunk_is_token(second, CT_ANGLE_OPEN)
      && (  get_chunk_parent_type(second) == CT_OC_PROTO_LIST
         || get_chunk_parent_type(second) == CT_OC_GENERIC_SPEC)
      && (options::sp_before_oc_proto_list() != IARF_IGNORE))
   {
      // (OC) Add or remove space before Objective-C protocol list
      // as in '@protocol Protocol<here><Protocol_A>' or '@interface MyClass : NSObject<here><MyProtocol>'.
      log_rule("sp_before_oc_proto_list");
      return(options::sp_before_oc_proto_list());
   }

   if (  language_is_set(LANG_OC)
      && chunk_is_token(first, CT_OC_CLASS)
      && chunk_is_token(second, CT_PAREN_OPEN)
      && (options::sp_oc_classname_paren() != IARF_IGNORE))
   {
      // (OC) Add or remove space between class name and '('
      // in '@interface className(categoryName)<ProtocolName>:BaseClass'
      log_rule("sp_oc_classname_paren");
      return(options::sp_oc_classname_paren());
   }

   if (  chunk_is_token(first, CT_CATCH)
      && chunk_is_token(second, CT_SPAREN_OPEN)
      && (options::sp_catch_paren() != IARF_IGNORE))
   {
      // Add or remove space between 'catch' and '(' in 'catch (something) { }'.
      // If set to ignore, sp_before_sparen is used.
      log_rule("sp_catch_paren");
      return(options::sp_catch_paren());
   }

   if (  language_is_set(LANG_D)
      && chunk_is_token(first, CT_D_VERSION_IF)
      && chunk_is_token(second, CT_SPAREN_OPEN)
      && (options::sp_version_paren() != IARF_IGNORE))
   {
      // (D) Add or remove space between 'version' and '('
      // in 'version (something) { }'. If set to ignore, sp_before_sparen is used.
      log_rule("sp_version_paren");
      return(options::sp_version_paren());
   }

   if (  language_is_set(LANG_D)
      && chunk_is_token(first, CT_D_SCOPE_IF)
      && chunk_is_token(second, CT_SPAREN_OPEN)
      && (options::sp_scope_paren() != IARF_IGNORE))
   {
      // (D) Add or remove space between 'scope' and '('
      // in 'scope (something) { }'. If set to ignore, sp_before_sparen is used.
      log_rule("sp_scope_paren");
      return(options::sp_scope_paren());
   }

   if (  language_is_set(LANG_OC)
      && chunk_is_token(first, CT_SYNCHRONIZED)
      && chunk_is_token(second, CT_SPAREN_OPEN))
   {
      // (OC) Add or remove space between '@synchronized' and the open parenthesis,
      // i.e. '@synchronized(foo)' vs. '@synchronized (foo)'.
      log_rule("sp_after_oc_synchronized");
      return(options::sp_after_oc_synchronized());
   }

   // "if (" vs. "if("
   if (chunk_is_token(second, CT_SPAREN_OPEN))
   {
      // Add or remove space after 'do' between 'while' and '('. Issue #995
      if (  chunk_is_token(first, CT_WHILE_OF_DO)
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

   if (  chunk_is_token(first, CT_LAMBDA)
      || chunk_is_token(second, CT_LAMBDA))
   {
      // Add or remove space around assignment operator '=', '+=', etc.
      log_rule("sp_assign (lambda)");
      return(options::sp_assign());
   }

   // Handle the special lambda case for C++11:
   //    [=](Something arg){.....}
   // Add or remove space around '=' in C++11 lambda capture specifications.
   // Overrides sp_assign.
   if (  (options::sp_cpp_lambda_assign() != IARF_IGNORE)
      && (  (  chunk_is_token(first, CT_SQUARE_OPEN)
            && get_chunk_parent_type(first) == CT_CPP_LAMBDA
            && chunk_is_token(second, CT_ASSIGN))
         || (  chunk_is_token(first, CT_ASSIGN)
            && chunk_is_token(second, CT_SQUARE_CLOSE)
            && get_chunk_parent_type(second) == CT_CPP_LAMBDA)))
   {
      log_rule("sp_cpp_lambda_assign");
      return(options::sp_cpp_lambda_assign());
   }

   if (  chunk_is_token(first, CT_SQUARE_CLOSE)
      && get_chunk_parent_type(first) == CT_CPP_LAMBDA)
   {
      // Handle the special lambda case for C++11:
      //    [](Something arg){.....}
      // Add or remove space after the capture specification of a C++11 lambda when
      // an argument list is present, as in '[] <here> (int x){ ... }'.
      if (chunk_is_token(second, CT_LPAREN_OPEN))
      {
         log_rule("sp_cpp_lambda_square_paren");
         return(options::sp_cpp_lambda_square_paren());
      }
      else if (chunk_is_token(second, CT_BRACE_OPEN))
      {
         // Add or remove space after the capture specification of a C++11 lambda with
         // no argument list is present, as in '[] <here> { ... }'.
         log_rule("sp_cpp_lambda_square_brace");
         return(options::sp_cpp_lambda_square_brace());
      }
   }

   if (chunk_is_token(first, CT_LPAREN_OPEN))
   {
      // Add or remove space after the opening parenthesis of a argument list
      // of a C++11 lambda, as in '[]( <here> int x ){ ... }'.
      log_rule("sp_cpp_lambda_argument_list");
      return(options::sp_cpp_lambda_argument_list());
   }

   if (chunk_is_token(first, CT_LPAREN_CLOSE))
   {
      if (chunk_is_token(second, CT_BRACE_OPEN))
      {
         // Add or remove space after the argument list of a C++11 lambda, as in
         // '[](int x) <here> { ... }'.
         log_rule("sp_cpp_lambda_paren_brace");
         return(options::sp_cpp_lambda_paren_brace());
      }
   }

   if (chunk_is_token(second, CT_LPAREN_CLOSE))
   {
      // Add or remove space before the closing parenthesis of a argument list
      // of a C++11 lambda, as in '[]( int x <here> ){ ... }'.
      log_rule("sp_cpp_lambda_argument_list");
      return(options::sp_cpp_lambda_argument_list());
   }

   if (  chunk_is_token(first, CT_BRACE_CLOSE)
      && get_chunk_parent_type(first) == CT_CPP_LAMBDA
      && chunk_is_token(second, CT_FPAREN_OPEN))
   {
      // Add or remove space between a lambda body and its call operator of an
      // immediately invoked lambda, as in '[]( ... ){ ... } <here> ( ... )'.
      log_rule("sp_cpp_lambda_fparen");
      return(options::sp_cpp_lambda_fparen());
   }

   if (chunk_is_token(first, CT_ENUM))
   {
      if (chunk_is_token(second, CT_BRACE_OPEN))
      {
         // Add or remove space in 'enum {'.
         log_rule("sp_enum_brace");
         return(options::sp_enum_brace());
      }
      else if (chunk_is_token(second, CT_FPAREN_OPEN))
      {
         // Add or remove space in 'NS_ENUM ('.
         log_rule("sp_enum_paren");
         return(options::sp_enum_paren());
      }
   }

   if (chunk_is_token(second, CT_ASSIGN))
   {
      if (second->flags.test(PCF_IN_ENUM))
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
         && get_chunk_parent_type(second) == CT_FUNC_PROTO)
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

   if (chunk_is_token(second, CT_ASSIGN_DEFAULT_ARG))
   {
      // Add or remove space around assignment operator '=' in a prototype.
      // If set to ignore, use sp_assign.
      if (  (options::sp_assign_default() != IARF_IGNORE)
         && get_chunk_parent_type(second) == CT_FUNC_PROTO)
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

   if (chunk_is_token(first, CT_ASSIGN))
   {
      if (first->flags.test(PCF_IN_ENUM))
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
         && get_chunk_parent_type(first) == CT_FUNC_PROTO)
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

   if (  chunk_is_token(first, CT_TRAILING_RET)
      || chunk_is_token(first, CT_CPP_LAMBDA_RET)
      || chunk_is_token(second, CT_TRAILING_RET)
      || chunk_is_token(second, CT_CPP_LAMBDA_RET))
   {
      // Add or remove space around trailing return operator '->'.
      log_rule("sp_trailing_return");
      return(options::sp_trailing_return());
   }

   if (chunk_is_token(first, CT_ASSIGN_DEFAULT_ARG))
   {
      // Add or remove space around assignment operator '=' in a prototype.
      // If set to ignore, use sp_assign.
      if (  (options::sp_assign_default() != IARF_IGNORE)
         && get_chunk_parent_type(first) == CT_FUNC_PROTO)
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

   if (chunk_is_token(first, CT_BIT_COLON))
   {
      if (  first->flags.test(PCF_IN_ENUM)
         || first->parent_type == CT_ENUM)
      {
         // Add or remove space around assignment ':' in enum.
         log_rule("sp_enum_colon");
         return(options::sp_enum_colon());
      }
   }

   if (chunk_is_token(second, CT_BIT_COLON))
   {
      if (  second->flags.test(PCF_IN_ENUM)
         || second->parent_type == CT_ENUM)
      {
         // Add or remove space around assignment ':' in enum.
         log_rule("sp_enum_colon");
         return(options::sp_enum_colon());
      }
   }

   if (  chunk_is_token(first, CT_OC_AVAILABLE_VALUE)
      || chunk_is_token(second, CT_OC_AVAILABLE_VALUE))
   {
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }

   if (language_is_set(LANG_OC))
   {
      if (chunk_is_token(second, CT_OC_BLOCK_CARET))
      {
         // (OC) Add or remove space before a block pointer caret,
         // i.e. '^int (int arg){...}' vs. ' ^int (int arg){...}'.
         log_rule("sp_before_oc_block_caret");
         return(options::sp_before_oc_block_caret());
      }

      if (chunk_is_token(first, CT_OC_BLOCK_CARET))
      {
         // (OC) Add or remove space after a block pointer caret,
         // i.e. '^int (int arg){...}' vs. '^ int (int arg){...}'.
         log_rule("sp_after_oc_block_caret");
         return(options::sp_after_oc_block_caret());
      }

      if (chunk_is_token(second, CT_OC_MSG_FUNC))
      {
         if (  (options::sp_after_oc_msg_receiver() == IARF_REMOVE)
            && (  (first->type != CT_SQUARE_CLOSE)
               && (first->type != CT_FPAREN_CLOSE)
               && (first->type != CT_PAREN_CLOSE)))
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
   if (  language_is_set(LANG_CPP)
      && (  chunk_is_token(first, CT_BYREF)
         || chunk_is_token(first, CT_QUALIFIER)
         || chunk_is_token(first, CT_TYPE))
      && chunk_is_token(second, CT_SQUARE_OPEN)
      && get_chunk_parent_type(second) != CT_OC_MSG
      && get_chunk_parent_type(second) != CT_CS_SQ_STMT)
   {
      // Add or remove space before C++17 structured bindings.
      log_rule("sp_cpp_before_struct_binding");
      return(options::sp_cpp_before_struct_binding());
   }

   // "a [x]" vs. "a[x]"
   if (  chunk_is_token(second, CT_SQUARE_OPEN)
      && (  get_chunk_parent_type(second) != CT_OC_MSG
         && get_chunk_parent_type(second) != CT_CS_SQ_STMT
         && get_chunk_parent_type(second) != CT_CPP_LAMBDA))
   {
      if (  second->flags.test(PCF_IN_SPAREN)
         && (chunk_is_token(first, CT_IN)))
      {
         log_rule("FORCE");
         return(IARF_FORCE);
      }

      if (chunk_is_token(first, CT_ASM_COLON))
      {
         // Add or remove space before '[' for asm block.
         log_rule("sp_before_square_asm_block");
         return(options::sp_before_square_asm_block());
      }

      if (first->flags.test(PCF_VAR_DEF))
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
   if (chunk_is_token(second, CT_TSQUARE))
   {
      // Add or remove space before '[]'.
      log_rule("sp_before_squares");
      return(options::sp_before_squares());
   }

   if (  (options::sp_angle_shift() != IARF_IGNORE)
      && chunk_is_token(first, CT_ANGLE_CLOSE)
      && chunk_is_token(second, CT_ANGLE_CLOSE))
   {
      // Add or remove space between '>' and '>' in '>>' (template stuff).
      log_rule("sp_angle_shift");
      return(options::sp_angle_shift());
   }

   // spacing around template < > stuff
   if (  chunk_is_token(first, CT_ANGLE_OPEN)
      || chunk_is_token(second, CT_ANGLE_CLOSE))
   {
      if (  chunk_is_token(first, CT_ANGLE_OPEN)
         && chunk_is_token(second, CT_ANGLE_CLOSE))
      {
         // Add or remove space inside '<>'.
         log_rule("sp_inside_angle_empty");
         return(options::sp_inside_angle_empty());
      }
      // Add or remove space inside '<' and '>'.
      log_rule("sp_inside_angle");
      iarf_e op = options::sp_inside_angle();

      // special: if we're not supporting digraphs, then we shouldn't create them!
      if (  (op == IARF_REMOVE)
         && !options::enable_digraphs()
         && chunk_is_token(first, CT_ANGLE_OPEN)
         && chunk_is_token(second, CT_DC_MEMBER))
      {
         op = IARF_IGNORE;
      }
      return(op);
   }

   if (chunk_is_token(second, CT_ANGLE_OPEN))
   {
      if (  chunk_is_token(first, CT_TEMPLATE)
         && (options::sp_template_angle() != IARF_IGNORE))
      {
         // Add or remove space between 'template' and '<'.
         // If set to ignore, sp_before_angle is used.
         log_rule("sp_template_angle");
         return(options::sp_template_angle());
      }

      if (first->type != CT_QUALIFIER)
      {
         // Add or remove space before '<'.
         log_rule("sp_before_angle");
         return(options::sp_before_angle());
      }
   }

   if (chunk_is_token(first, CT_ANGLE_CLOSE))
   {
      if (  chunk_is_token(second, CT_WORD)
         || CharTable::IsKw1(second->str[0]))
      {
         // Add or remove space between '>' and a word as in 'List<byte> m;' or
         // 'template <typename T> static ...'.
         log_rule("sp_angle_word");
         return(options::sp_angle_word());
      }

      if (  chunk_is_token(second, CT_FPAREN_OPEN)
         || chunk_is_token(second, CT_PAREN_OPEN))
      {
         Chunk *next = second->GetNextNcNnl();

         if (chunk_is_token(next, CT_FPAREN_CLOSE))
         {
            // Add or remove space between '>' and '()' as found in 'new List<byte>();'.
            log_rule("sp_angle_paren_empty");
            return(options::sp_angle_paren_empty());
         }
         // Add or remove space between '>' and '(' as found in 'new List<byte>(foo);'.
         log_rule("sp_angle_paren");
         return(options::sp_angle_paren());
      }

      if (chunk_is_token(second, CT_DC_MEMBER))
      {
         // Add or remove space before the '::' operator.
         log_rule("sp_before_dc");
         return(options::sp_before_dc());
      }

      if (  second->type != CT_BYREF
         && second->type != CT_PTR_TYPE
         && second->type != CT_BRACE_OPEN
         && second->type != CT_PAREN_CLOSE)
      {
         if (  chunk_is_token(second, CT_CLASS_COLON)
            && options::sp_angle_colon() != IARF_IGNORE)
         {
            // Add or remove space between '>' and ':'.
            log_rule("sp_angle_colon");
            return(options::sp_angle_colon());
         }

         // Whether sp_after_angle takes precedence over sp_inside_fparen. This was the
         // historic behavior, but is probably not the desired behavior, so this is off
         // by default.
         if (  chunk_is_token(second, CT_FPAREN_CLOSE)
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

   if (chunk_is_token(first, CT_BYREF))                             // see the tests cpp:34509-34512
   {
      if (chunk_is_token(second, CT_PAREN_OPEN))
      {
         // Add or remove space after a reference sign '&', if followed by an open
         // parenthesis, as in 'char& (*)()'.
         log_rule("sp_byref_paren");
         return(options::sp_byref_paren());
      }
      else if (  get_chunk_parent_type(first) == CT_FUNC_DEF        // Issue #3197, #3210
              || get_chunk_parent_type(first) == CT_FUNC_PROTO)
      {
         // Add or remove space after a reference sign '&', if followed by a function
         // prototype or function definition.
         log_rule("sp_after_byref_func");                          // byref 2
         return(options::sp_after_byref_func());
      }
      else if (  CharTable::IsKw1(second->str[0])
              && (  options::sp_after_byref() != IARF_IGNORE
                 || (  !chunk_is_token(second, CT_FUNC_PROTO)
                    && !chunk_is_token(second, CT_FUNC_DEF))))
      {
         // Add or remove space after reference sign '&', if followed by a word.
         log_rule("sp_after_byref");                               // byref 1
         return(options::sp_after_byref());
      }
   }

   if (  chunk_is_token(second, CT_BYREF)
      && !chunk_is_token(first, CT_PAREN_OPEN))              // Issue #1804
   {
      if (  get_chunk_parent_type(second) == CT_FUNC_DEF     // Issue #3197, #3210
         || get_chunk_parent_type(second) == CT_FUNC_PROTO)
      {
         // Add or remove space before a reference sign '&', if followed by a function
         // prototype or function definition.
         log_rule("sp_before_byref_func");                         // byref 4
         return(options::sp_before_byref_func());
      }
      Chunk *next = second->GetNext();

      if (  next->IsNotNullChunk()
         && (  chunk_is_token(next, CT_COMMA)
            || chunk_is_token(next, CT_FPAREN_CLOSE)
            || chunk_is_token(next, CT_SEMICOLON)))
      {
         if (options::sp_before_unnamed_byref() != IARF_IGNORE)
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
      // Add or remove space before a reference sign '&'.
      log_rule("sp_before_byref");                                 // byref 3
      return(options::sp_before_byref());
   }

   if (chunk_is_token(first, CT_SPAREN_CLOSE))
   {
      if (chunk_is_token(second, CT_BRACE_OPEN))
      {
         if (get_chunk_parent_type(second) == CT_CATCH)
         {
            if (  language_is_set(LANG_OC)
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

      if (  !second->IsComment()
         && (options::sp_after_sparen() != IARF_IGNORE))
      {
         // Add or remove space after ')' of control statements.
         log_rule("sp_after_sparen");
         return(options::sp_after_sparen());
      }
   }

   if (  chunk_is_token(second, CT_FPAREN_OPEN)
      && get_chunk_parent_type(first) == CT_OPERATOR
      && (options::sp_after_operator_sym() != IARF_IGNORE))
   {
      if (  (options::sp_after_operator_sym_empty() != IARF_IGNORE)
         && chunk_is_token(second, CT_FPAREN_OPEN))
      {
         Chunk *next = second->GetNextNcNnl();

         if (chunk_is_token(next, CT_FPAREN_CLOSE))
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
   if (  language_is_set(LANG_VALA)
      && chunk_is_token(first, CT_FUNC_CALL))
   {
      if (  chunk_is_str(first, "_")
         && chunk_is_token(second, CT_FPAREN_OPEN)
         && (options::sp_vala_after_translation() != IARF_IGNORE))
      {
         // Add or remove space after '_'.
         log_rule("sp_vala_after_translation");
         return(options::sp_vala_after_translation());
      }
   }

   if (  chunk_is_token(first, CT_MACRO_OPEN)
      || chunk_is_token(first, CT_MACRO_CLOSE)
      || chunk_is_token(first, CT_MACRO_ELSE))
   {
      if (chunk_is_token(second, CT_FPAREN_OPEN))
      {
         // TODO: provide some test data to check this block
         // Add or remove space between function name and '(' on function calls.
         log_rule("sp_func_call_paren");
         return(options::sp_func_call_paren());
      }
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }

   // spaces between function and open paren
   if (  chunk_is_token(first, CT_FUNC_CALL)
      || chunk_is_token(first, CT_FUNCTION)                        // Issue #2665
      || chunk_is_token(first, CT_FUNC_CTOR_VAR)
      || chunk_is_token(first, CT_CNG_HASINC)
      || chunk_is_token(first, CT_CNG_HASINCN)
      || (  chunk_is_token(first, CT_BRACE_CLOSE)
         && first->parent_type == CT_BRACED_INIT_LIST
         && chunk_is_token(second, CT_FPAREN_OPEN)))
   {
      if (  (options::sp_func_call_paren_empty() != IARF_IGNORE)
         && chunk_is_token(second, CT_FPAREN_OPEN))
      {
         Chunk *next = second->GetNextNcNnl();

         if (chunk_is_token(next, CT_FPAREN_CLOSE))
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

   if (chunk_is_token(first, CT_FUNC_CALL_USER))
   {
      // Add or remove space between the user function name and '(' on function
      // calls. You need to set a keyword to be a user function in the config file,
      // like:
      //   set func_call_user tr _ i18n
      log_rule("sp_func_call_user_paren");
      return(options::sp_func_call_user_paren());
   }

   if (  chunk_is_token(first, CT_ATTRIBUTE)
      && chunk_is_paren_open(second))
   {
      // Add or remove space between '__attribute__' and '('.
      log_rule("sp_attribute_paren");
      return(options::sp_attribute_paren());
   }

   if (chunk_is_token(first, CT_FUNC_DEF))
   {
      if (  (options::sp_func_def_paren_empty() != IARF_IGNORE)
         && chunk_is_token(second, CT_FPAREN_OPEN))
      {
         Chunk *next = second->GetNextNcNnl();

         if (chunk_is_token(next, CT_FPAREN_CLOSE))
         {
            // Add or remove space between function name and '()' on function definition
            // without parameters.
            log_rule("sp_func_def_paren_empty");
            return(options::sp_func_def_paren_empty());
         }
      }
      // Add or remove space between function name and '(' on function definition.
      log_rule("sp_func_def_paren");
      return(options::sp_func_def_paren());
   }

   if (  chunk_is_token(first, CT_CPP_CAST)
      || chunk_is_token(first, CT_TYPE_WRAP))
   {
      // Add or remove space between the type and open parenthesis in a C++ cast,
      // i.e. 'int(exp)' vs. 'int (exp)'.
      log_rule("sp_cpp_cast_paren");
      return(options::sp_cpp_cast_paren());
   }

   if (  chunk_is_token(first, CT_SPAREN_CLOSE)
      && chunk_is_token(second, CT_WHEN))
   {
      // TODO: provide some test data to check this block
      log_rule("FORCE");
      return(IARF_FORCE); // TODO: make this configurable?
   }

   if (  chunk_is_token(first, CT_PAREN_CLOSE)
      && (  chunk_is_token(second, CT_PAREN_OPEN)
         || chunk_is_token(second, CT_FPAREN_OPEN)))
   {
      // "(int)a" vs. "(int) a" or "cast(int)a" vs. "cast(int) a"
      if (  get_chunk_parent_type(first) == CT_C_CAST
         || get_chunk_parent_type(first) == CT_D_CAST)
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
   if (chunk_is_token(first, CT_TPAREN_CLOSE))
   {
      // Add or remove space between the ')' and '(' in a function type, as in
      // 'void (*x)(...)'.
      log_rule("sp_after_tparen_close");
      return(options::sp_after_tparen_close());
   }

   // ")(" vs. ") ("
   if (  (  chunk_is_str(first, ")")
         && chunk_is_str(second, "("))
      || (  chunk_is_paren_close(first)
         && chunk_is_paren_open(second)))
   {
      // Add or remove space between back-to-back parentheses, i.e. ')(' vs. ') ('.
      log_rule("sp_cparen_oparen");
      return(options::sp_cparen_oparen());
   }

   if (  chunk_is_token(first, CT_FUNC_PROTO)
      || (  chunk_is_token(second, CT_FPAREN_OPEN)
         && get_chunk_parent_type(second) == CT_FUNC_PROTO))
   {
      if (  (options::sp_func_proto_paren_empty() != IARF_IGNORE)
         && chunk_is_token(second, CT_FPAREN_OPEN))
      {
         Chunk *next = second->GetNextNcNnl();

         if (chunk_is_token(next, CT_FPAREN_CLOSE))
         {
            // Add or remove space between function name and '()' on function declaration
            // without parameters.
            log_rule("sp_func_proto_paren_empty");
            return(options::sp_func_proto_paren_empty());
         }
      }
      // Add or remove space between function name and '(' on function declaration.
      log_rule("sp_func_proto_paren");
      return(options::sp_func_proto_paren());
   }

   // Issue #2437
   if (  chunk_is_token(first, CT_FUNC_TYPE)
      && chunk_is_token(second, CT_FPAREN_OPEN))
   {
      // Add or remove space between function name and '(' with a typedef specifier.
      log_rule("sp_func_type_paren");
      return(options::sp_func_type_paren());
   }

   if (  chunk_is_token(first, CT_FUNC_CLASS_DEF)
      || chunk_is_token(first, CT_FUNC_CLASS_PROTO))
   {
      if (  (options::sp_func_class_paren_empty() != IARF_IGNORE)
         && chunk_is_token(second, CT_FPAREN_OPEN))
      {
         Chunk *next = second->GetNextNcNnl();

         if (chunk_is_token(next, CT_FPAREN_CLOSE))
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

   if (  chunk_is_token(first, CT_CLASS)
      && !first->flags.test(PCF_IN_OC_MSG))
   {
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   if (  chunk_is_token(first, CT_BRACE_OPEN)
      && chunk_is_token(second, CT_BRACE_CLOSE))
   {
      // Add or remove space inside '{}'.
      log_rule("sp_inside_braces_empty");
      return(options::sp_inside_braces_empty());
   }

   if (  (  chunk_is_token(first, CT_TYPE)                           // Issue #2428
         || chunk_is_token(first, CT_ANGLE_CLOSE))
      && chunk_is_token(second, CT_BRACE_OPEN)
      && get_chunk_parent_type(second) == CT_BRACED_INIT_LIST)
   {
      iarf_flags_t arg = iarf_flags_t{ options::sp_type_brace_init_lst() };

      if (  arg != IARF_IGNORE
         || get_chunk_parent_type(first) != CT_DECLTYPE)
      {
         // 'int{9}' vs. 'int {9}'
         // Add or remove space between type and open brace of an unnamed temporary
         // direct-list-initialization.
         log_rule("sp_type_brace_init_lst");
         return(arg);
      }
   }

   if (  (  chunk_is_token(first, CT_WORD)                           // Issue #2428
         || chunk_is_token(first, CT_SQUARE_CLOSE)
         || chunk_is_token(first, CT_TSQUARE))
      && chunk_is_token(second, CT_BRACE_OPEN)
      && get_chunk_parent_type(second) == CT_BRACED_INIT_LIST)
   {
      // Add or remove space between a variable and '{' for C++ uniform
      // initialization.
      auto arg = iarf_flags_t{ options::sp_word_brace_init_lst() };

      if (  arg != IARF_IGNORE
         || get_chunk_parent_type(first) != CT_DECLTYPE)
      {
         // 'a{9}' vs. 'a {9}'
         // Add or remove space between variable/word and open brace of an unnamed
         // temporary direct-list-initialization.
         log_rule("sp_word_brace_init_lst");
         return(arg);
      }
   }

   if (chunk_is_token(second, CT_BRACE_CLOSE))
   {
      if (get_chunk_parent_type(second) == CT_ENUM)
      {
         // Add or remove space inside enum '{' and '}'.
         log_rule("sp_inside_braces_enum");
         return(options::sp_inside_braces_enum());
      }

      if (  get_chunk_parent_type(second) == CT_STRUCT
         || get_chunk_parent_type(second) == CT_UNION)
      {
         // Fix for issue #1240  adding space in struct initializers
         Chunk *tmp = chunk_skip_to_match_rev(second)->GetPrevNcNnl();

         if (chunk_is_token(tmp, CT_ASSIGN))
         {
            // TODO: provide some test data to check this block
            log_rule("IGNORE");
            return(IARF_IGNORE);
         }
         // Add or remove space inside struct/union '{' and '}'.
         log_rule("sp_inside_braces_struct");
         return(options::sp_inside_braces_struct());
      }
      else if (  language_is_set(LANG_OC)
              && get_chunk_parent_type(second) == CT_OC_AT
              && options::sp_inside_braces_oc_dict() != IARF_IGNORE)
      {
         // (OC) Add or remove space inside Objective-C boxed dictionary '{' and '}'
         log_rule("sp_inside_braces_oc_dict");
         return(options::sp_inside_braces_oc_dict());
      }

      if (get_chunk_parent_type(second) == CT_BRACED_INIT_LIST)
      {
         // Add or remove space between nested braces, i.e. '{{' vs. '{ {'.
         // only to help the vim command }}}}
         if (  options::sp_brace_brace() != IARF_IGNORE
            && chunk_is_token(first, CT_BRACE_CLOSE)
            && get_chunk_parent_type(first) == CT_BRACED_INIT_LIST)
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

   if (chunk_is_token(first, CT_D_CAST))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  chunk_is_token(first, CT_PP_DEFINED)
      && chunk_is_token(second, CT_PAREN_OPEN))
   {
      // Add or remove space between 'defined' and '(' in '#if defined (FOO)'.
      log_rule("sp_defined_paren");
      return(options::sp_defined_paren());
   }

   if (chunk_is_token(first, CT_THROW))
   {
      if (chunk_is_token(second, CT_PAREN_OPEN))
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

   if (  chunk_is_token(first, CT_THIS)
      && chunk_is_token(second, CT_PAREN_OPEN))
   {
      // Add or remove space between 'this' and '(' in 'this (something)'.
      log_rule("sp_this_paren");
      return(options::sp_this_paren());
   }

   if (  chunk_is_token(first, CT_STATE)
      && chunk_is_token(second, CT_PAREN_OPEN))
   {
      log_rule("ADD");
      return(IARF_ADD);
   }

   if (  chunk_is_token(first, CT_DELEGATE)
      && chunk_is_token(second, CT_PAREN_OPEN))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  chunk_is_token(first, CT_MEMBER)
      || chunk_is_token(second, CT_MEMBER))
   {
      // Add or remove space around the '.' or '->' operators.
      log_rule("sp_member");
      return(options::sp_member());
   }

   if (chunk_is_token(first, CT_C99_MEMBER))
   {
      // always remove space(s) after then '.' of a C99-member
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  chunk_is_token(first, CT_SUPER)
      && chunk_is_token(second, CT_PAREN_OPEN))
   {
      // Add or remove space between 'super' and '(' in 'super (something)'.
      log_rule("sp_super_paren");
      return(options::sp_super_paren());
   }

   if (  chunk_is_token(first, CT_FPAREN_CLOSE)
      && chunk_is_token(second, CT_BRACE_OPEN))
   {
      if (  language_is_set(LANG_JAVA)
         && get_chunk_parent_type(second) == CT_DOUBLE_BRACE)
      {
         // (Java) Add or remove space between ')' and '{{' of double brace initializer.
         // only to help the vim command }}
         log_rule("sp_fparen_dbrace");
         return(options::sp_fparen_dbrace());
      }

      // To fix issue #1234
      // check for initializers and add space or ignore based on the option.
      if (get_chunk_parent_type(first) == CT_FUNC_CALL)
      {
         Chunk *tmp = first->GetPrevType(get_chunk_parent_type(first), first->level);
         tmp = tmp->GetPrevNcNnl();

         if (chunk_is_token(tmp, CT_NEW))
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

   if (  chunk_is_token(first, CT_D_TEMPLATE)
      || chunk_is_token(second, CT_D_TEMPLATE))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  chunk_is_token(first, CT_ELSE)
      && chunk_is_token(second, CT_BRACE_OPEN))
   {
      // Add or remove space between 'else' and '{' if on the same line.
      log_rule("sp_else_brace");
      return(options::sp_else_brace());
   }

   if (  chunk_is_token(first, CT_ELSE)
      && chunk_is_token(second, CT_ELSEIF))
   {
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   if (  chunk_is_token(first, CT_FINALLY)
      && chunk_is_token(second, CT_BRACE_OPEN))
   {
      // Add or remove space between 'finally' and '{' if on the same line.
      log_rule("sp_finally_brace");
      return(options::sp_finally_brace());
   }

   if (  chunk_is_token(first, CT_TRY)
      && chunk_is_token(second, CT_BRACE_OPEN))
   {
      // Add or remove space between 'try' and '{' if on the same line.
      log_rule("sp_try_brace");
      return(options::sp_try_brace());
   }

   if (  chunk_is_token(first, CT_GETSET)
      && chunk_is_token(second, CT_BRACE_OPEN))
   {
      // Add or remove space between get/set and '{' if on the same line.
      log_rule("sp_getset_brace");
      return(options::sp_getset_brace());
   }

   if (  chunk_is_token(first, CT_WORD)
      && chunk_is_token(second, CT_BRACE_OPEN))
   {
      if (get_chunk_parent_type(first) == CT_NAMESPACE)
      {
         // Add or remove space between a variable and '{' for a namespace.
         log_rule("sp_word_brace_ns");
         return(options::sp_word_brace_ns());
      }
   }

   if (  language_is_set(LANG_D)
      && chunk_is_token(second, CT_PAREN_OPEN)
      && get_chunk_parent_type(second) == CT_INVARIANT)
   {
      // (D) Add or remove space between 'invariant' and '('.
      log_rule("sp_invariant_paren");
      return(options::sp_invariant_paren());
   }

   if (  chunk_is_token(first, CT_PAREN_CLOSE)
      && get_chunk_parent_type(first) != CT_DECLTYPE)
   {
      if (  language_is_set(LANG_D)
         && get_chunk_parent_type(first) == CT_INVARIANT)
      {
         // (D) Add or remove space after the ')' in 'invariant (C) c'.
         log_rule("sp_after_invariant_paren");
         return(options::sp_after_invariant_paren());
      }

      // "(struct foo) {...}" vs. "(struct foo){...}"
      if (chunk_is_token(second, CT_BRACE_OPEN))
      {
         // Add or remove space between ')' and '{'.
         log_rule("sp_paren_brace");
         return(options::sp_paren_brace());
      }

      // D-specific: "delegate(some thing) dg
      if (get_chunk_parent_type(first) == CT_DELEGATE)
      {
         log_rule("ADD");
         return(IARF_ADD);
      }

      // PAWN-specific: "state (condition) next"
      if (get_chunk_parent_type(first) == CT_STATE)
      {
         log_rule("ADD");
         return(IARF_ADD);
      }

      /* C++ new operator: new(bar) Foo */
      if (get_chunk_parent_type(first) == CT_NEW)
      {
         // Add or remove space between ')' and type in 'new(foo) BAR'.
         log_rule("sp_after_newop_paren");
         return(options::sp_after_newop_paren());
      }
   }

   /* "((" vs. "( (" or "))" vs. ") )" */
   // Issue #1342
   if (  (  chunk_is_str(first, "(")
         && chunk_is_str(second, "("))
      || (  chunk_is_str(first, ")")
         && chunk_is_str(second, ")")))
   {
      if (get_chunk_parent_type(second) == CT_FUNC_CALL_USER)
      {
         // Add or remove space between nested parentheses with user functions,
         // i.e. '((' vs. '( ('.
         log_rule("sp_func_call_user_paren_paren");
         return(options::sp_func_call_user_paren_paren());
      }

      if (  options::sp_sparen_paren() != IARF_IGNORE
         && (  chunk_is_token(first, CT_SPAREN_OPEN)
            || chunk_is_token(second, CT_SPAREN_CLOSE)))
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
   if (  chunk_is_token(first, CT_FPAREN_OPEN)
      || chunk_is_token(second, CT_FPAREN_CLOSE))
   {
      if (  (get_chunk_parent_type(first) == CT_FUNC_CALL_USER)
         || (  (get_chunk_parent_type(second) == CT_FUNC_CALL_USER)
            && (  (chunk_is_token(first, CT_WORD))
               || (chunk_is_token(first, CT_SQUARE_CLOSE)))))
      {
         // Add or remove space inside user function '(' and ')'.
         log_rule("sp_func_call_user_inside_fparen");
         return(options::sp_func_call_user_inside_fparen());
      }

      if (  chunk_is_token(first, CT_FPAREN_OPEN)
         && chunk_is_token(second, CT_FPAREN_CLOSE))
      {
         // Add or remove space inside empty function '()'.
         log_rule("sp_inside_fparens");
         return(options::sp_inside_fparens());
      }
      // Add or remove space inside function '(' and ')'.
      log_rule("sp_inside_fparen");
      return(options::sp_inside_fparen());
   }

   // "foo(...)" vs. "foo( ... )"
   if (  chunk_is_token(first, CT_TPAREN_OPEN)
      || chunk_is_token(second, CT_TPAREN_CLOSE))
   {
      // Add or remove space inside the first parentheses in a function type, as in
      // 'void (*x)(...)'.
      log_rule("sp_inside_tparen");
      return(options::sp_inside_tparen());
   }

   if (  language_is_set(LANG_OC)
      && chunk_is_token(first, CT_PAREN_CLOSE))
   {
      if (  first->flags.test(PCF_OC_RTYPE) // == CT_OC_RTYPE)
         && (  get_chunk_parent_type(first) == CT_OC_MSG_DECL
            || get_chunk_parent_type(first) == CT_OC_MSG_SPEC))
      {
         // (OC) Add or remove space after the first (type) in message specs,
         // i.e. '-(int) f:(int)x;' vs. '-(int)f:(int)x;'.
         log_rule("sp_after_oc_return_type");
         return(options::sp_after_oc_return_type());
      }

      if (  get_chunk_parent_type(first) == CT_OC_MSG_SPEC
         || get_chunk_parent_type(first) == CT_OC_MSG_DECL)
      {
         // (OC) Add or remove space after the (type) in message specs,
         // i.e. '-(int)f: (int) x;' vs. '-(int)f: (int)x;'.
         log_rule("sp_after_oc_type");
         return(options::sp_after_oc_type());
      }

      if (  get_chunk_parent_type(first) == CT_OC_SEL
         && second->type != CT_SQUARE_CLOSE)
      {
         // (OC) Add or remove space between '@selector(x)' and the following word,
         // i.e. '@selector(foo) a:' vs. '@selector(foo)a:'.
         log_rule("sp_after_oc_at_sel_parens");
         return(options::sp_after_oc_at_sel_parens());
      }
   }

   if (  language_is_set(LANG_OC)
      && options::sp_inside_oc_at_sel_parens() != IARF_IGNORE)
   {
      if (  (  chunk_is_token(first, CT_PAREN_OPEN)
            && (  get_chunk_parent_type(first) == CT_OC_SEL
               || get_chunk_parent_type(first) == CT_OC_PROTOCOL))
         || (  chunk_is_token(second, CT_PAREN_CLOSE)
            && (  get_chunk_parent_type(second) == CT_OC_SEL
               || get_chunk_parent_type(second) == CT_OC_PROTOCOL)))
      {
         // (OC) Add or remove space inside '@selector' parentheses,
         // i.e. '@selector(foo)' vs. '@selector( foo )'.
         // Also applies to '@protocol()' constructs.
         log_rule("sp_inside_oc_at_sel_parens");
         return(options::sp_inside_oc_at_sel_parens());
      }
   }

   if (  chunk_is_token(second, CT_PAREN_OPEN)
      && (  chunk_is_token(first, CT_OC_SEL)
         || chunk_is_token(first, CT_OC_PROTOCOL)))
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
   if (chunk_is_token(first, CT_PAREN_OPEN))
   {
      if (  get_chunk_parent_type(first) == CT_C_CAST
         || get_chunk_parent_type(first) == CT_CPP_CAST
         || get_chunk_parent_type(first) == CT_D_CAST)
      {
         // Add or remove spaces inside cast parentheses.
         log_rule("sp_inside_paren_cast");
         return(options::sp_inside_paren_cast());
      }

      if (get_chunk_parent_type(first) == CT_NEW)
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
            // Add or remove space inside parenthesis of the new operator
            // as in 'new(foo) BAR'.
            log_rule("sp_inside_newop_paren");
            return(options::sp_inside_newop_paren());
         }
      }
      log_rule("sp_inside_paren");
      return(options::sp_inside_paren());
   }

   if (chunk_is_token(second, CT_PAREN_CLOSE))
   {
      if (  get_chunk_parent_type(second) == CT_C_CAST
         || get_chunk_parent_type(second) == CT_CPP_CAST
         || get_chunk_parent_type(second) == CT_D_CAST)
      {
         // Add or remove spaces inside cast parentheses.
         log_rule("sp_inside_paren_cast");
         return(options::sp_inside_paren_cast());
      }

      if (get_chunk_parent_type(second) == CT_NEW)
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
            // Add or remove space inside parenthesis of the new operator
            // as in 'new(foo) BAR'.
            log_rule("sp_inside_newop_paren");
            return(options::sp_inside_newop_paren());
         }
      }
      // Add or remove space inside '(' and ')'.
      log_rule("sp_inside_paren");
      return(options::sp_inside_paren());
   }

   if (  chunk_is_token(first, CT_SQUARE_OPEN)
      && chunk_is_token(second, CT_SQUARE_CLOSE))
   {
      // Add or remove space inside '[]'.
      log_rule("sp_inside_square_empty");
      return(options::sp_inside_square_empty());
   }

   // "[3]" vs. "[ 3 ]" or for objective-c "@[@3]" vs. "@[ @3 ]"
   if (  chunk_is_token(first, CT_SQUARE_OPEN)
      || chunk_is_token(second, CT_SQUARE_CLOSE))
   {
      if (  language_is_set(LANG_OC)
         && (  (  get_chunk_parent_type(first) == CT_OC_AT
               && chunk_is_token(first, CT_SQUARE_OPEN))
            || (  get_chunk_parent_type(second) == CT_OC_AT
               && chunk_is_token(second, CT_SQUARE_CLOSE)))
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

   if (  chunk_is_token(first, CT_SQUARE_CLOSE)
      && chunk_is_token(second, CT_FPAREN_OPEN))
   {
      // Add or remove space between ']' and '(' when part of a function call.
      log_rule("sp_square_fparen");
      return(options::sp_square_fparen());
   }

   // "if(...)" vs. "if( ... )" etc.
   if (chunk_is_token(second, CT_SPAREN_CLOSE))
   {
      if (  get_chunk_parent_type(second) == CT_FOR
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

   if (chunk_is_token(first, CT_SPAREN_OPEN))
   {
      if (  get_chunk_parent_type(first) == CT_FOR
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

   if (chunk_is_token(first, CT_SPAREN_OPEN))
   {
      if (get_chunk_parent_type(first) == CT_FOR)
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

   if (chunk_is_token(second, CT_SPAREN_CLOSE))
   {
      if (get_chunk_parent_type(second) == CT_FOR)
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

   if (chunk_is_token(first, CT_CLASS_COLON))
   {
      if (  get_chunk_parent_type(first) == CT_OC_CLASS
         && (  first->GetPrevType(CT_OC_INTF, first->level, E_Scope::ALL)->IsNullChunk()
            && first->GetPrevType(CT_OC_IMPL, first->level, E_Scope::ALL)->IsNullChunk()))
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

   if (chunk_is_token(second, CT_CLASS_COLON))
   {
      if (  language_is_set(LANG_OC)
         && get_chunk_parent_type(second) == CT_OC_CLASS
         && (  second->GetPrevType(CT_OC_INTF, second->level, E_Scope::ALL)->IsNullChunk()
            && second->GetPrevType(CT_OC_IMPL, second->level, E_Scope::ALL)->IsNullChunk()))
      {
         if (  get_chunk_parent_type(second) == CT_OC_CLASS
            && second->GetPrevType(CT_OC_INTF, second->level, E_Scope::ALL)->IsNullChunk())
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

   if (chunk_is_token(first, CT_CONSTR_COLON))
   {
      min_sp = options::indent_ctor_init_leading() - 1; // default indent is 1 space
      // Add or remove space after class constructor ':'.
      log_rule("sp_after_constr_colon");
      return(options::sp_after_constr_colon());
   }

   if (chunk_is_token(second, CT_CONSTR_COLON))
   {
      // Add or remove space before class constructor ':'.
      log_rule("sp_before_constr_colon");
      return(options::sp_before_constr_colon());
   }

   if (chunk_is_token(second, CT_CASE_COLON))
   {
      // Add or remove space before case ':'.
      log_rule("sp_before_case_colon");
      return(options::sp_before_case_colon());
   }

   if (chunk_is_token(first, CT_DOT))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (chunk_is_token(second, CT_DOT))
   {
      log_rule("ADD");
      return(IARF_ADD);
   }

   if (  chunk_is_token(first, CT_NULLCOND)
      || chunk_is_token(second, CT_NULLCOND))
   {
      // TODO: provide some test data to check this block
      // LANG_CS  null conditional operator
      // Add or remove space around the '.' or '->' operators.
      log_rule("sp_member");
      return(options::sp_member());
   }

   if (  chunk_is_token(first, CT_ARITH)
      || chunk_is_token(first, CT_SHIFT)
      || chunk_is_token(first, CT_CARET)
      || chunk_is_token(second, CT_ARITH)
      || chunk_is_token(second, CT_SHIFT)
      || chunk_is_token(second, CT_CARET))
   {
      // Add or remove space around arithmetic operators '+' and '-'.
      // Overrides sp_arith.
      if (options::sp_arith_additive() != IARF_IGNORE)
      {
         auto arith_char = (  chunk_is_token(first, CT_ARITH)
                           || chunk_is_token(first, CT_SHIFT)
                           || chunk_is_token(first, CT_CARET))
                           ? first->str[0] : second->str[0];

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

   if (  chunk_is_token(first, CT_BOOL)
      || chunk_is_token(second, CT_BOOL))
   {
      // Add or remove space around boolean operators '&&' and '||'.
      iarf_e arg = options::sp_bool();

      if (  (options::pos_bool() != TP_IGNORE)
         && first->orig_line != second->orig_line)
      {
         arg = arg | IARF_ADD;
      }
      log_rule("sp_bool");
      return(arg);
   }

   if (  chunk_is_token(first, CT_COMPARE)
      || chunk_is_token(second, CT_COMPARE))
   {
      // Add or remove space around compare operator '<', '>', '==', etc.
      log_rule("sp_compare");
      return(options::sp_compare());
   }

   if (  chunk_is_token(first, CT_PAREN_OPEN)
      && chunk_is_token(second, CT_PTR_TYPE))
   {
      // TODO: provide some test data to check this block
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (chunk_is_token(first, CT_PTR_TYPE))                            // see the tests cpp:34505-34508
   {
      if (  chunk_is_token(second, CT_FPAREN_OPEN)
         || chunk_is_token(second, CT_TPAREN_OPEN))
      {
         // Add or remove space after a pointer star '*', if followed by an open
         // parenthesis, as in 'void* (*)()'.
         log_rule("sp_ptr_star_paren");                               // ptr_star 10
         return(options::sp_ptr_star_paren());
      }
      else if (chunk_is_token(second, CT_PTR_TYPE))
      {
         // Add or remove space between pointer stars '*'.
         // as in 'int ***a;'.
         log_rule("sp_between_ptr_star");                             // ptr_star 9
         return(options::sp_between_ptr_star());
      }
      else if (chunk_is_token(second, CT_FUNC_VAR))
      {
         // Add or remove space between the pointer star '*' and the name of the
         // variable in a function pointer definition.
         log_rule("sp_ptr_star_func_var");
         return(options::sp_ptr_star_func_var());
      }
      else if (chunk_is_token(second, CT_FUNC_TYPE))
      {
         // Add or remove space between the pointer star '*' and the name of the
         // type in a function pointer type definition.
         log_rule("sp_ptr_star_func_type");
         return(options::sp_ptr_star_func_type());
      }
      else if (  get_chunk_parent_type(first) == CT_FUNC_DEF
              || get_chunk_parent_type(first) == CT_FUNC_PROTO
              || get_chunk_parent_type(first) == CT_FUNC_VAR)
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
      else if (CharTable::IsKw1(second->str[0]))
      {
         Chunk *prev = first->GetPrev();

         if (chunk_is_token(prev, CT_IN))
         {
            // Add or remove space after the '*' (dereference) unary operator. This does
            // not affect the spacing after a '*' that is part of a type.
            log_rule("sp_deref");
            return(options::sp_deref());
         }
         else if (  get_chunk_parent_type(first) == CT_FUNC_VAR
                 || get_chunk_parent_type(first) == CT_FUNC_TYPE)
         {
            // Add or remove space after pointer caret '^', if followed by a word.
            log_rule("sp_after_ptr_block_caret");
            return(options::sp_after_ptr_block_caret());
         }
         else if (chunk_is_token(second, CT_QUALIFIER))
         {
            // Add or remove space after pointer star '*', if followed by a qualifier.
            log_rule("sp_after_ptr_star_qualifier");                  // ptr_star 4
            return(options::sp_after_ptr_star_qualifier());
         }
         // Add or remove space after pointer star '*', if followed by a word.
         log_rule("sp_after_ptr_star");                               // ptr_star 1
         return(options::sp_after_ptr_star());
      }
      else if (chunk_is_token(second, CT_PAREN_OPEN))
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

   if (  chunk_is_token(second, CT_PTR_TYPE)
      && first->type != CT_IN)
   {
      // look back for '->' type is TRAILING_RET
      if (token_is_within_trailing_return(second))
      {
         log_rule("sp_before_ptr_star_trailing");                     // ptr_star 7
         return(options::sp_before_ptr_star_trailing());
      }

      // Add or remove space before a pointer star '*', if followed by a function
      // prototype or function definition.
      if (options::sp_before_ptr_star_func() != IARF_IGNORE)
      {
         // Find the next non-'*' chunk
         Chunk *next = second;

         do
         {
            next = next->GetNext();
         } while (chunk_is_token(next, CT_PTR_TYPE));

         if (  chunk_is_token(next, CT_FUNC_DEF)
            || chunk_is_token(next, CT_FUNC_PROTO))
         {
            log_rule("sp_before_ptr_star_func");                      // ptr_star 6
            return(options::sp_before_ptr_star_func());
         }
      }

      // Add or remove space before pointer star '*' that isn't followed by a
      // variable name. If set to 'ignore', sp_before_ptr_star is used instead.
      if (options::sp_before_unnamed_ptr_star() != IARF_IGNORE)
      {
         Chunk *next = second->GetNextNc();

         while (chunk_is_token(next, CT_PTR_TYPE))
         {
            next = next->GetNextNc();
         }

         if (  next->IsNotNullChunk()
            && next->type != CT_WORD)
         {
            log_rule("sp_before_unnamed_ptr_star");                   // ptr_star 8
            return(options::sp_before_unnamed_ptr_star());
         }
      }

      // Add or remove space before pointer star '*'.
      if (options::sp_before_ptr_star() != IARF_IGNORE)
      {
         log_rule("sp_before_ptr_star");                              // ptr_star 5
         return(options::sp_before_ptr_star());
      }
   }

   if (chunk_is_token(first, CT_OPERATOR))
   {
      // Add or remove space between 'operator' and operator sign.
      log_rule("sp_after_operator");
      return(options::sp_after_operator());
   }

   if (  chunk_is_token(second, CT_FUNC_PROTO)
      || chunk_is_token(second, CT_FUNC_DEF))
   {
      if (  first->type != CT_PTR_TYPE
         && first->type != CT_BYREF)
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
   if (  (  get_chunk_parent_type(first) == CT_C_CAST
         || get_chunk_parent_type(first) == CT_D_CAST)
      && chunk_is_token(first, CT_PAREN_CLOSE))
   {
      log_rule("sp_after_cast");
      return(options::sp_after_cast());
   }

   if (chunk_is_token(first, CT_BRACE_CLOSE))
   {
      if (chunk_is_token(second, CT_ELSE))
      {
         // Add or remove space between '}' and 'else' if on the same line.
         log_rule("sp_brace_else");
         return(options::sp_brace_else());
      }

      if (  language_is_set(LANG_OC)
         && chunk_is_token(second, CT_CATCH)
         && (options::sp_oc_brace_catch() != IARF_IGNORE))
      {
         // TODO: provide some test data to check this block
         // (OC) Add or remove space between '}' and '@catch' if on the same line.
         // If set to ignore, sp_brace_catch is used.
         log_rule("sp_oc_brace_catch");
         return(options::sp_oc_brace_catch());
      }

      if (chunk_is_token(second, CT_CATCH))
      {
         // TODO: provide some test data to check this block
         // Add or remove space between '}' and 'catch' if on the same line.
         log_rule("sp_brace_catch");
         return(options::sp_brace_catch());
      }

      if (chunk_is_token(second, CT_FINALLY))
      {
         // Add or remove space between '}' and 'finally' if on the same line.
         log_rule("sp_brace_finally");
         return(options::sp_brace_finally());
      }
   }

   if (chunk_is_token(first, CT_BRACE_OPEN))
   {
      if (get_chunk_parent_type(first) == CT_ENUM)
      {
         // Add or remove space inside enum '{' and '}'.
         log_rule("sp_inside_braces_enum");
         return(options::sp_inside_braces_enum());
      }

      if (  get_chunk_parent_type(first) == CT_STRUCT
         || get_chunk_parent_type(first) == CT_UNION)
      {
         // Fix for issue #1240  adding space in struct initializers
         Chunk *tmp = first->GetPrevNcNnl();

         if (chunk_is_token(tmp, CT_ASSIGN))
         {
            // TODO: provide some test data to check this block
            log_rule("IGNORE");
            return(IARF_IGNORE);
         }
         // Add or remove space inside struct/union '{' and '}'.
         log_rule("sp_inside_braces_struct");
         return(options::sp_inside_braces_struct());
      }
      else if (  get_chunk_parent_type(first) == CT_OC_AT
              && options::sp_inside_braces_oc_dict() != IARF_IGNORE)
      {
         // (OC) Add or remove space inside Objective-C boxed dictionary '{' and '}'
         log_rule("sp_inside_braces_oc_dict");
         return(options::sp_inside_braces_oc_dict());
      }

      if (get_chunk_parent_type(first) == CT_BRACED_INIT_LIST)
      {
         // Add or remove space between nested braces, i.e. '{{' vs. '{ {'.
         // only to help the vim command }}}}
         if (  options::sp_brace_brace() != IARF_IGNORE
            && chunk_is_token(second, CT_BRACE_OPEN)
            && get_chunk_parent_type(second) == CT_BRACED_INIT_LIST)
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

   if (  chunk_is_token(first, CT_BRACE_CLOSE)
      && first->flags.test(PCF_IN_TYPEDEF)
      && (  get_chunk_parent_type(first) == CT_ENUM
         || get_chunk_parent_type(first) == CT_STRUCT
         || get_chunk_parent_type(first) == CT_UNION))
   {
      // Add or remove space between '}' and the name of a typedef on the same line.
      log_rule("sp_brace_typedef");
      return(options::sp_brace_typedef());
   }

   if (  language_is_set(LANG_D)
      && chunk_is_token(second, CT_PAREN_OPEN)
      && get_chunk_parent_type(second) == CT_TEMPLATE)
   {
      // (D) Add or remove space before the parenthesis in the D constructs
      // 'template Foo(' and 'class Foo('.
      log_rule("sp_before_template_paren");
      return(options::sp_before_template_paren());
   }

   // Issue #3080
   if (  chunk_is_token(first, CT_PAREN_CLOSE)
      && get_chunk_parent_type(first) == CT_DECLTYPE
      && (  chunk_is_token(second, CT_WORD)
         || chunk_is_token(second, CT_BRACE_OPEN)
         || chunk_is_token(second, CT_FUNC_CALL)))
   {
      iarf_e arg = options::sp_after_decltype();
      // Add or remove space between 'decltype(...)' and word, brace or function call.
      log_rule("sp_after_decltype");
      return(arg);
   }

   // Issue #3080
   if (  !language_is_set(LANG_D)
      && chunk_is_token(first, CT_PAREN_CLOSE)
      && chunk_is_token(second, CT_WORD))
   {
      // Add or remove space between type and word.
      log_rule("sp_after_type");
      return(options::sp_after_type());
   }

   // see if the D template expression is used as a type
   if (  language_is_set(LANG_D)
      && chunk_is_token(first, CT_PAREN_CLOSE)
      && get_chunk_parent_type(first) == CT_D_TEMPLATE)
   {
      if (get_chunk_parent_type(second) == CT_USING_ALIAS)
      {
         log_rule("sp_after_type | ADD");
         return(options::sp_after_type() | IARF_ADD);
      }

      if (chunk_is_token(second, CT_WORD))
      {
         Chunk *open_paren = chunk_skip_to_match_rev(first);

         if (open_paren == nullptr)
         {
            open_paren = Chunk::NullChunkPtr;
         }
         Chunk *type = open_paren->GetPrev()->GetPrev();

         if (chunk_is_token(type, CT_TYPE))
         {
            log_rule("sp_after_type");
            return(options::sp_after_type());
         }
      }
   }

   if (  chunk_is_token(first, CT_TYPE)                   // Issue #3457
      && chunk_is_token(second, CT_COLON))
   {
      log_rule("sp_type_colon");
      return(options::sp_type_colon());
   }

   if (  !chunk_is_token(second, CT_PTR_TYPE)
      && (  chunk_is_token(first, CT_QUALIFIER)
         || chunk_is_token(first, CT_TYPE)))
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

   if (  chunk_is_token(first, CT_PTR_TYPE)
      && chunk_is_token(second, CT_ELLIPSIS))
   {
      log_rule("sp_ptr_type_ellipsis");
      return(options::sp_ptr_type_ellipsis());
   }

   // If nothing claimed the PTR_TYPE, then return ignore
   if (  chunk_is_token(first, CT_PTR_TYPE)
      || chunk_is_token(second, CT_PTR_TYPE))
   {
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }

   if (chunk_is_token(first, CT_NOT))
   {
      if (  chunk_is_token(second, CT_NOT)
         && (options::sp_not_not() != IARF_IGNORE))
      {
         log_rule("sp_not_not");
         return(options::sp_not_not());
      }
      // Add or remove space after the '!' (not) unary operator.
      log_rule("sp_not");
      return(options::sp_not());
   }

   if (chunk_is_token(first, CT_INV))
   {
      // Add or remove space after the '~' (invert) unary operator.
      log_rule("sp_inv");
      return(options::sp_inv());
   }

   if (chunk_is_token(first, CT_ADDR))
   {
      // Add or remove space after the '&' (address-of) unary operator. This does not
      // affect the spacing after a '&' that is part of a type.
      log_rule("sp_addr");
      return(options::sp_addr());
   }

   if (chunk_is_token(first, CT_DEREF))
   {
      // Add or remove space after the '*' (dereference) unary operator. This does
      // not affect the spacing after a '*' that is part of a type.
      log_rule("sp_deref");
      return(options::sp_deref());
   }

   if (  chunk_is_token(first, CT_POS)
      || chunk_is_token(first, CT_NEG))
   {
      // Add or remove space after '+' or '-', as in 'x = -5' or 'y = +7'.
      log_rule("sp_sign");
      return(options::sp_sign());
   }

   if (  chunk_is_token(first, CT_INCDEC_BEFORE)
      || chunk_is_token(second, CT_INCDEC_AFTER))
   {
      // Add or remove space between '++' and '--' the word to which it is being
      // applied, as in '(--x)' or 'y++;'.
      log_rule("sp_incdec");
      return(options::sp_incdec());
   }

   if (chunk_is_token(second, CT_CS_SQ_COLON))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (chunk_is_token(first, CT_CS_SQ_COLON))
   {
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   if (  language_is_set(LANG_OC)
      && chunk_is_token(first, CT_OC_SCOPE))
   {
      // (OC) Add or remove space after the scope '+' or '-', as in '-(void) foo;'
      // or '+(int) bar;'.
      log_rule("sp_after_oc_scope");
      return(options::sp_after_oc_scope());
   }

   if (  language_is_set(LANG_OC)
      && chunk_is_token(first, CT_OC_DICT_COLON))
   {
      // (OC) Add or remove space after the colon in immutable dictionary expression
      // 'NSDictionary *test = @{@"foo" :@"bar"};'.
      log_rule("sp_after_oc_dict_colon");
      return(options::sp_after_oc_dict_colon());
   }

   if (  language_is_set(LANG_OC)
      && chunk_is_token(second, CT_OC_DICT_COLON))
   {
      // (OC) Add or remove space before the colon in immutable dictionary expression
      // 'NSDictionary *test = @{@"foo" :@"bar"};'.
      log_rule("sp_before_oc_dict_colon");
      return(options::sp_before_oc_dict_colon());
   }

   if (  language_is_set(LANG_OC)
      && chunk_is_token(first, CT_OC_COLON))
   {
      if (first->flags.test(PCF_IN_OC_MSG))
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

   if (  language_is_set(LANG_OC)
      && chunk_is_token(second, CT_OC_COLON))
   {
      if (  first->flags.test(PCF_IN_OC_MSG)
         && (  chunk_is_token(first, CT_OC_MSG_FUNC)
            || chunk_is_token(first, CT_OC_MSG_NAME)))
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

   if (  chunk_is_token(second, CT_COMMENT)
      && get_chunk_parent_type(second) == CT_COMMENT_EMBED)
   {
      // Add or remove space before an embedded comment.
      // Number of spaces before an embedded comment.
      log_rule("sp_before_emb_cmt");
      min_sp = options::sp_num_before_emb_cmt();
      return(options::sp_before_emb_cmt());
   }

   if (chunk_is_token(first, CT_COMMENT))
   {
      // Add or remove space after an embedded comment.
      // Number of spaces after an embedded comment.
      log_rule("sp_after_emb_cmt");
      min_sp = options::sp_num_after_emb_cmt();
      return(options::sp_after_emb_cmt());
   }

   if (  chunk_is_token(first, CT_NEW)
      && chunk_is_token(second, CT_PAREN_OPEN))
   {
      // c# new Constraint, c++ new operator
      // Add or remove space between 'new' and '(' in 'new()'.
      log_rule("sp_between_new_paren");
      return(options::sp_between_new_paren());
   }

   if (  chunk_is_token(first, CT_NEW)
      || chunk_is_token(first, CT_DELETE)
      || (  chunk_is_token(first, CT_TSQUARE)
         && get_chunk_parent_type(first) == CT_DELETE))
   {
      // Add or remove space after 'new', 'delete' and 'delete[]'.
      log_rule("sp_after_new");
      return(options::sp_after_new());
   }

   if (  language_is_set(LANG_JAVA)
      && chunk_is_token(first, CT_ANNOTATION)
      && chunk_is_paren_open(second))
   {
      // (Java) Add or remove space between an annotation and the open parenthesis.
      log_rule("sp_annotation_paren");
      return(options::sp_annotation_paren());
   }

   if (  language_is_set(LANG_OC)
      && chunk_is_token(first, CT_OC_PROPERTY))
   {
      // (OC) Add or remove space after '@property'.
      log_rule("sp_after_oc_property");
      return(options::sp_after_oc_property());
   }

   if (  language_is_set(LANG_D)
      && chunk_is_token(first, CT_EXTERN)
      && chunk_is_token(second, CT_PAREN_OPEN))
   {
      // (D) Add or remove space between 'extern' and '(' as in 'extern (C)'.
      log_rule("sp_extern_paren");
      return(options::sp_extern_paren());
   }

   if (  chunk_is_token(second, CT_TYPE)
      && (  (  chunk_is_token(first, CT_STRING)
            && get_chunk_parent_type(first) == CT_EXTERN)
         || (  chunk_is_token(first, CT_FPAREN_CLOSE)
            && get_chunk_parent_type(first) == CT_ATTRIBUTE)))
   {
      log_rule("FORCE");
      return(IARF_FORCE);  /* TODO: make this configurable? */
   }

   if (chunk_is_token(first, CT_NOEXCEPT))
   {
      // Add or remove space after 'noexcept'.
      log_rule("sp_after_noexcept");
      return(options::sp_after_noexcept());
   }

   // Issue #2138
   if (chunk_is_token(first, CT_FPAREN_CLOSE))
   {
      if (chunk_is_token(second, CT_QUALIFIER))
      {
         // Add or remove space between ')' and a qualifier such as 'const'.
         log_rule("sp_paren_qualifier");
         return(options::sp_paren_qualifier());
      }
      else if (chunk_is_token(second, CT_NOEXCEPT))
      {
         // Add or remove space between ')' and 'noexcept'.
         log_rule("sp_paren_noexcept");
         return(options::sp_paren_noexcept());
      }
   }

   // Issue #2098
   if (  chunk_is_token(first, CT_PP_PRAGMA)
      && chunk_is_token(second, CT_PREPROC_BODY))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   // Issue #1733
   if (  chunk_is_token(first, CT_OPERATOR_VAL)
      && chunk_is_token(second, CT_TYPE))
   {
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }

   // Issue #995
   if (  chunk_is_token(first, CT_DO)
      && chunk_is_token(second, CT_BRACE_OPEN))
   {
      // Add or remove space between 'do' and '{'.
      log_rule("sp_do_brace_open");
      return(options::sp_do_brace_open());
   }

   // Issue #995
   if (  chunk_is_token(first, CT_BRACE_CLOSE)
      && chunk_is_token(second, CT_WHILE_OF_DO))
   {
      // Add or remove space between '}' and 'while.
      log_rule("sp_brace_close_while");
      return(options::sp_brace_close_while());
   }

   // TODO: have a look to Issue #2186, why NEWLINE?
   // Issue #2524
   if (  chunk_is_token(first, CT_NEWLINE)
      && chunk_is_token(second, CT_BRACE_OPEN))
   {
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }

   // =============================================================
   // categorie 1
   // this table lists out all combos where a space should NOT be present
   // CT_UNKNOWN is a wildcard.
   for (auto it : no_space_table)
   {
      if (  (  it.first == CT_UNKNOWN
            || it.first == first->type)
         && (  it.second == CT_UNKNOWN
            || it.second == second->type))
      {
         log_rule("REMOVE from no_space_table");
         return(IARF_REMOVE);
      }
   }

   // =============================================================
   // categorie 2
   // this table lists out all combos where a space MUST be present
   for (auto it : add_space_table)
   {
      if (  it.first == first->type
         && it.second == second->type)
      {
         // TODO: if necessary create a new option
         log_rule("ADD from add_space_table");
         return(IARF_ADD);
      }
   }

   // Issue #2386
   if (  chunk_is_token(first, CT_FORM_FEED)
      || chunk_is_token(second, CT_FORM_FEED))
   {
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }

   // TODO: if necessary create a new option
   if (  chunk_is_token(first, CT_MACRO_FUNC_CALL)
      && chunk_is_token(second, CT_FPAREN_OPEN))
   {
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }

   if (  chunk_is_token(first, CT_CASE_ELLIPSIS)
      && chunk_is_token(second, CT_NUMBER))
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

   if (  chunk_is_token(first, CT_NUMBER)
      && chunk_is_token(second, CT_CASE_ELLIPSIS))
   {
      // Add or remove space before the variadic '...' when preceded by a
      // non-punctuator.
      // The value REMOVE will be overridden with FORCE
      if (options::sp_before_ellipsis() == IARF_REMOVE)
      {
         log_rule("sp_before_ellipsis/FORCE");
         return(IARF_FORCE);
      }
      else
      {
         log_rule("sp_before_ellipsis");
         return(options::sp_before_ellipsis());
      }
   }
   // =============================================================
   // categorie 3
   // these lines are only useful for debugging uncrustify itself
   LOG_FMT(LSPACE, "\n\n%s(%d): WARNING: unrecognize do_space:\n",
           __func__, __LINE__);
   LOG_FMT(LSPACE, "   first->orig_line  is %zu, first->orig_col  is %zu, first->Text()  '%s', first->type is  %s\n",
           first->orig_line, first->orig_col, first->Text(), get_token_name(first->type));
   LOG_FMT(LSPACE, "   second->orig_line is %zu, second->orig_col is %zu, second->Text() '%s', second->type is %s\n",
           second->orig_line, second->orig_col, second->Text(), get_token_name(second->type));
   LOG_FMT(LSPACE, "   Please make a call at https://github.com/uncrustify/uncrustify/issues/new\n");
   LOG_FMT(LSPACE, "   or merge the line:\n");
   LOG_FMT(LSPACE, "   {%s,    %s},\n",
           get_token_name(first->type), get_token_name(second->type));
   LOG_FMT(LSPACE, "   in the file <Path_to_uncrustify>/src/add_space_table.h\n");

   log_rule("ADD as default value");
   return(IARF_ADD);
} // do_space


static iarf_e ensure_force_space(Chunk *first, Chunk *second, iarf_e av)
{
   if (first->flags.test(PCF_FORCE_SPACE))
   {
      LOG_FMT(LSPACE, "%s(%d): <force between '%s' and '%s'>",
              __func__, __LINE__, first->Text(), second->Text());
      return(av | IARF_ADD);
   }
   return(av);
}


static iarf_e do_space_ensured(Chunk *first, Chunk *second, int &min_sp)
{
   iarf_e aa = ensure_force_space(first, second, do_space(first, second, min_sp));

   return(aa);
}


void space_text(void)
{
   LOG_FUNC_ENTRY();

   Chunk  *pc = Chunk::GetHead();
   Chunk  *next;
   size_t prev_column;
   size_t column = pc->column;

   while (pc->IsNotNullChunk())
   {
      if (chunk_is_token(pc, CT_NEWLINE))
      {
         LOG_FMT(LSPACE, "%s(%d): orig_line is %zu, orig_col is %zu, <Newline>, nl is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->nl_count);
      }
      else
      {
         char copy[1000];
         LOG_FMT(LSPACE, "%s(%d): orig_line is %zu, orig_col is %zu, '%s' type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->ElidedText(copy), get_token_name(pc->type));
      }

      if (  (options::use_options_overriding_for_qt_macros())
         && (  (strcmp(pc->Text(), "SIGNAL") == 0)
            || (strcmp(pc->Text(), "SLOT") == 0)))
      {
         LOG_FMT(LSPACE, "%s(%d): orig_col is %zu, type is %s SIGNAL/SLOT found\n",
                 __func__, __LINE__, pc->orig_line, get_token_name(pc->type));
         chunk_flags_set(pc, PCF_IN_QT_MACRO); // flag the chunk for a second processing

         // save the values
         save_set_options_for_QT(pc->level);
      }

      // Bug # 637
      // If true, vbrace tokens are dropped to the previous token and skipped.
      if (options::sp_skip_vbrace_tokens())
      {
         next = pc->GetNext();

         while (  next->IsEmptyText()
               && !chunk_is_newline(next)
               && (  chunk_is_token(next, CT_VBRACE_OPEN)
                  || chunk_is_token(next, CT_VBRACE_CLOSE)))
         {
            LOG_FMT(LSPACE, "%s(%d): orig_line is %zu, orig_col is %zu, Skip %s (%zu+%zu)\n",
                    __func__, __LINE__, next->orig_line, next->orig_col, get_token_name(next->type),
                    pc->column, pc->str.size());
            next->column = pc->column + pc->str.size();
            next         = next->GetNext();
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
      if (  QT_SIGNAL_SLOT_found
         && options::sp_balance_nested_parens())
      {
         Chunk *nn = next->GetNext();                                    // Issue #2734

         if (  nn->IsNotNullChunk()
            && chunk_is_token(nn, CT_SPACE))
         {
            chunk_del(nn); // remove the space
         }
      }

      /*
       * If the current chunk contains a newline, do not change the column
       * of the next item
       */
      if (  chunk_is_token(pc, CT_NEWLINE)
         || chunk_is_token(pc, CT_NL_CONT)
         || chunk_is_token(pc, CT_COMMENT_MULTI))
      {
         column = next->column;
      }
      else
      {
         // Set to the minimum allowed column
         if (pc->nl_count == 0)
         {
            column += pc->Len();
         }
         else
         {
            column = pc->orig_col_end;
         }
         prev_column = column;

         /*
          * Apply a general safety check
          * If the two chunks combined will tokenize differently, then we
          * must force a space.
          * Two chunks -- "()" and "[]" will always tokenize differently.
          * They are always safe to not have a space after them.
          */
         chunk_flags_clr(pc, PCF_FORCE_SPACE);

         if (  (pc->Len() > 0)
            && !chunk_is_str(pc, "[]")
            && !chunk_is_str(pc, "{{")
            && !chunk_is_str(pc, "}}")
            && !chunk_is_str(pc, "()")
            && !pc->str.startswith("@\""))
         {
            // Find the next non-empty chunk on this line
            Chunk *tmp = next;

            while (  tmp->IsNotNullChunk()
                  && (tmp->Len() == 0)
                  && !chunk_is_newline(tmp))
            {
               tmp = tmp->GetNext();
            }

            if (  tmp->IsNotNullChunk()
               && tmp->Len() > 0)
            {
               bool kw1 = CharTable::IsKw2(pc->str[pc->Len() - 1]);
               bool kw2 = CharTable::IsKw1(next->str[0]);

               if (  kw1
                  && kw2)
               {
                  // back-to-back words need a space
                  LOG_FMT(LSPACE, "%s(%d): back-to-back words need a space: pc->Text() '%s', next->Text() '%s'\n",
                          __func__, __LINE__, pc->Text(), next->Text());
                  chunk_flags_set(pc, PCF_FORCE_SPACE);
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
                     if (  (  (  language_is_set(LANG_CPP)
                              && options::sp_permit_cpp11_shift())
                           || (language_is_set(LANG_JAVA | LANG_CS | LANG_VALA | LANG_OC)))
                        && chunk_is_token(pc, CT_ANGLE_CLOSE)
                        && chunk_is_token(next, CT_ANGLE_CLOSE))
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
                        chunk_flags_set(pc, PCF_FORCE_SPACE);
                     }
                  }
               }
            }
         }
         int min_sp;
         LOG_FMT(LSPACE, "%s(%d): orig_line is %zu, orig_col is %zu, pc-Text() '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), get_token_name(pc->type));
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

            if (  next->orig_col >= pc->orig_col_end
               && pc->orig_col_end != 0)
            {
               // Keep the same relative spacing, minimum 1
               delta = next->orig_col - pc->orig_col_end;

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
            if (  next->orig_col >= pc->orig_col_end
               && pc->orig_col_end != 0)
            {
               column += next->orig_col - pc->orig_col_end;
            }
            else
            {
               // preserve the position if virtual brace
               // Issue #1854
               if (chunk_is_token(pc, CT_VBRACE_OPEN))
               {
                  column = next->orig_col;
               }
            }
            break;

         default:
            // If we got here, something is wrong...
            break;
         } // switch

         if (  next->IsComment()
            && chunk_is_newline(next->GetNext())
            && column < next->orig_col)
         {
            /*
             * do some comment adjustments if sp_before_tr_cmt and sp_endif_cmt
             * did not apply.
             */
            // Add or remove space between #else or #endif and a trailing comment.
            if (  (  options::sp_before_tr_cmt() == IARF_IGNORE
                  || get_chunk_parent_type(next) != CT_COMMENT_END)
               && (  options::sp_endif_cmt() == IARF_IGNORE
                  || (  pc->type != CT_PP_ELSE
                     && pc->type != CT_PP_ENDIF)))
            {
               if (options::indent_relative_single_line_comments())
               {
                  // Try to keep relative spacing between tokens
                  LOG_FMT(LSPACE, "%s(%d): <relative adj>", __func__, __LINE__);
                  LOG_FMT(LSPACE, "%s(%d): pc is '%s', pc->orig_col is %zu, next->orig_col is %zu, pc->orig_col_end is %zu\n",
                          __func__, __LINE__, pc->Text(),
                          pc->orig_col, next->orig_col, pc->orig_col_end);
                  column = pc->column + (next->orig_col - pc->orig_col_end);
               }
               else
               {
                  /*
                   * If there was a space, we need to force one, otherwise
                   * try to keep the comment in the same column.
                   */
                  size_t col_min = pc->column + pc->Len() + ((next->orig_prev_sp > 0) ? 1 : 0);
                  column = next->orig_col;

                  if (column < col_min)
                  {
                     column = col_min;
                  }
                  LOG_FMT(LSPACE, "%s(%d): <relative set>", __func__, __LINE__);
               }
            }
         }
         next->column = column;

         LOG_FMT(LSPACE, " rule = %s @ %zu => %zu\n",
                 (av == IARF_IGNORE) ? "IGNORE" :
                 (av == IARF_ADD) ? "ADD" :
                 (av == IARF_REMOVE) ? "REMOVE" : "FORCE",
                 column - prev_column, next->column);

         if (restoreValues)    // guy 2015-09-22
         {
            restore_options_for_QT();
         }
      }
      pc = next;

      if (QT_SIGNAL_SLOT_found)
      {
         // flag the chunk for a second processing
         chunk_flags_set(pc, PCF_IN_QT_MACRO);
      }
   }
} // space_text


void space_text_balance_nested_parens(void)
{
   LOG_FUNC_ENTRY();

   Chunk *first = Chunk::GetHead();

   while (first->IsNotNullChunk())
   {
      Chunk *next = first->GetNext();

      if (next->IsNullChunk())
      {
         break;
      }

      // if there are two successive opening parenthesis
      if (  chunk_is_str(first, "(")
         && chunk_is_str(next, "("))
      {
         // insert a space between them
         space_add_after(first, 1);

         // test after the closing parens   Issue #1703
         Chunk *closing = first->GetNextType((E_Token)(first->type + 1), first->level);

         if (closing->orig_col == closing->prev->orig_col_end)
         {
            space_add_after(closing->prev, 1);
         }
      }
      else if (  chunk_is_str(first, ")")
              && chunk_is_str(next, ")"))
      {
         // insert a space between the two closing parens
         space_add_after(first, 1);

         // test after the opening parens   Issue #1703
         Chunk *opening = next->GetPrevType((E_Token)(next->type - 1), next->level);

         if (opening->orig_col_end == opening->GetNext()->orig_col)
         {
            space_add_after(opening, 1);
         }
      }
      first = next;
   }
} // space_text_balance_nested_parens


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
      return(second->orig_col > (first->orig_col + first->Len()));
   }
}


size_t space_col_align(Chunk *first, Chunk *second)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LSPACE, "%s(%d): first->orig_line is %zu, orig_col is %zu, [%s/%s], Text() '%s' <==>\n",
           __func__, __LINE__, first->orig_line, first->orig_col,
           get_token_name(first->type), get_token_name(get_chunk_parent_type(first)),
           first->Text());
   LOG_FMT(LSPACE, "%s(%d): second->orig_line is %zu, orig_col is %zu [%s/%s], Text() '%s',",
           __func__, __LINE__, second->orig_line, second->orig_col,
           get_token_name(second->type), get_token_name(get_chunk_parent_type(second)),
           second->Text());
   log_func_stack_inline(LSPACE);

   int    min_sp;
   iarf_e av = do_space_ensured(first, second, min_sp);

   LOG_FMT(LSPACE, "%s(%d): av is %s\n", __func__, __LINE__, to_string(av));
   size_t coldiff;

   if (first->nl_count)
   {
      LOG_FMT(LSPACE, "%s(%d):    nl_count is %zu, orig_col_end is %zu\n", __func__, __LINE__, first->nl_count, first->orig_col_end);
      coldiff = first->orig_col_end - 1;
   }
   else
   {
      LOG_FMT(LSPACE, "%s(%d):    Len is %zu\n", __func__, __LINE__, first->Len());
      coldiff = first->Len();
   }
   LOG_FMT(LSPACE, "%s(%d):    => coldiff is %zu\n", __func__, __LINE__, coldiff);

   LOG_FMT(LSPACE, "%s(%d):    => av is %s\n", __func__, __LINE__,
           (av == IARF_IGNORE) ? "IGNORE" :
           (av == IARF_ADD) ? "ADD" :
           (av == IARF_REMOVE) ? "REMOVE" : "FORCE");

   switch (av)
   {
   case IARF_ADD:
   case IARF_FORCE:
      coldiff++;
      break;

   case IARF_REMOVE:
      break;

   case IARF_IGNORE:                // Issue #2064
      LOG_FMT(LSPACE, "%s(%d):    => first->orig_line  is %zu\n", __func__, __LINE__, first->orig_line);
      LOG_FMT(LSPACE, "%s(%d):    => second->orig_line is %zu\n", __func__, __LINE__, second->orig_line);
      LOG_FMT(LSPACE, "%s(%d):    => first->Text()     is '%s'\n", __func__, __LINE__, first->Text());
      LOG_FMT(LSPACE, "%s(%d):    => second->Text()    is '%s'\n", __func__, __LINE__, second->Text());
      LOG_FMT(LSPACE, "%s(%d):    => first->orig_col   is %zu\n", __func__, __LINE__, first->orig_col);
      LOG_FMT(LSPACE, "%s(%d):    => second->orig_col  is %zu\n", __func__, __LINE__, second->orig_col);
      LOG_FMT(LSPACE, "%s(%d):    => first->Len()      is %zu\n", __func__, __LINE__, first->Len());

      if (  first->orig_line == second->orig_line
         && second->orig_col > (first->orig_col + first->Len()))
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
      || chunk_is_newline(next))
   {
      return;
   }

   // Limit to 16 spaces
   if (count > 16)
   {
      count = 16;
   }

   // Two CT_SPACE in a row -- use the max of the two
   if (chunk_is_token(next, CT_SPACE))
   {
      if (next->Len() < count)
      {
         while (next->Len() < count)
         {
            next->str.append(' ');
         }
      }
      return;
   }
   Chunk sp;

   set_chunk_type(&sp, CT_SPACE);
   sp.flags = pc->flags & PCF_COPY_FLAGS;
   sp.str   = "                ";       // 16 spaces
   sp.str.resize(count);
   sp.level       = pc->level;
   sp.brace_level = pc->brace_level;
   sp.pp_level    = pc->pp_level;
   sp.column      = pc->column + pc->Len();
   sp.orig_line   = pc->orig_line;
   sp.orig_col    = pc->orig_col;

   chunk_add_after(&sp, pc);
} // space_add_after
