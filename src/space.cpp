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
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */

#include "space.h"

#include "char_table.h"
#include "chunk_list.h"
#include "language_tools.h"
#include "options_for_QT.h"
#include "prototypes.h"
#include "punctuators.h"
#include "unc_ctype.h"
#include "uncrustify.h"
#include "uncrustify_types.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>


using namespace std;
using namespace uncrustify;


static void log_rule2(size_t line, const char *rule, chunk_t *first, chunk_t *second);


/**
 * Decides how to change inter-chunk spacing.
 * Note that the order of the if statements is VERY important.
 *
 * @param first   The first chunk
 * @param second  The second chunk
 *
 * @return IARF_IGNORE, IARF_ADD, IARF_REMOVE or IARF_FORCE
 */
static iarf_e do_space(chunk_t *first, chunk_t *second, int &min_sp);

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
static iarf_e ensure_force_space(chunk_t *first, chunk_t *second, iarf_e av);

//! type that stores two chunks between those no space shall occur
struct no_space_table_t
{
   c_token_t first;  //! first  chunk
   c_token_t second; //! second chunk
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

#define log_rule(rule)                                   \
   do { if (log_sev_on(LSPACE)) {                        \
           log_rule2(__LINE__, (rule), first, second); } \
   } while (0)


static void log_rule2(size_t line, const char *rule, chunk_t *first, chunk_t *second)
{
   LOG_FUNC_ENTRY();
   if (second->type != CT_NEWLINE)
   {
      LOG_FMT(LSPACE, "%s(%d): Spacing: first->orig_line is %zu, first->orig_col is %zu, first->text() is '%s', [%s/%s] <===>\n",
              __func__, __LINE__, first->orig_line, first->orig_col, first->text(),
              get_token_name(first->type), get_token_name(first->parent_type));
      LOG_FMT(LSPACE, "   second->orig_line is %zu, second->orig_col is %zu, second->text() '%s', [%s/%s] : rule %s[line %zu]\n",
              second->orig_line, second->orig_col, second->text(),
              get_token_name(second->type), get_token_name(second->parent_type),
              rule, line);
   }
}


/*
 * this function is called for every chunk in the input file.
 * Thus it is important to keep this function efficient
 */
static iarf_e do_space(chunk_t *first, chunk_t *second, int &min_sp)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LSPACE, "%s(%d): orig_line is %zu, orig_col is %zu, first->text() '%s', type is %s\n",
           __func__, __LINE__, first->orig_line, first->orig_col, first->text(), get_token_name(first->type));

   min_sp = 1;
   if (chunk_is_token(first, CT_IGNORED) || chunk_is_token(second, CT_IGNORED))
   {
      log_rule("IGNORED");
      return(IARF_REMOVE);
   }
   if (chunk_is_token(first, CT_PP_IGNORE) && chunk_is_token(second, CT_PP_IGNORE))
   {
      // Leave spacing alone between PP_IGNORE tokens as we don't want the default behavior (which is ADD).
      log_rule("PP_IGNORE");
      return(IARF_IGNORE);
   }
   if (chunk_is_token(first, CT_PP) || chunk_is_token(second, CT_PP))
   {
      log_rule("sp_pp_concat");
      return(options::sp_pp_concat());
   }
   if (chunk_is_token(first, CT_POUND))
   {
      log_rule("sp_pp_stringify");
      return(options::sp_pp_stringify());
   }
   if (  chunk_is_token(second, CT_POUND)
      && (second->flags & PCF_IN_PREPROC)
      && first->parent_type != CT_MACRO_FUNC)
   {
      log_rule("sp_before_pp_stringify");
      return(options::sp_before_pp_stringify());
   }

   if (chunk_is_token(first, CT_SPACE) || chunk_is_token(second, CT_SPACE))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (chunk_is_token(first, CT_DECLSPEC))  // Issue 1289
   {
      log_rule("Remove");
      return(IARF_REMOVE);
   }
   // there are not any data for this case, this block will never been reached
   //if (chunk_is_token(first, CT_PAREN_CLOSE) && first->parent_type == CT_DECLSPEC)
   //{
   //   log_rule("Ignore");
   //   return(IARF_IGNORE);
   //}
   if (chunk_is_token(second, CT_NEWLINE) || chunk_is_token(second, CT_VBRACE_OPEN))
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
   if (chunk_is_token(first, CT_VBRACE_CLOSE) && second->type != CT_NL_CONT)
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
      log_rule("sp_before_nl_cont");
      return(options::sp_before_nl_cont());
   }

   if (chunk_is_token(first, CT_D_ARRAY_COLON) || chunk_is_token(second, CT_D_ARRAY_COLON))
   {
      log_rule("sp_d_array_colon");
      return(options::sp_d_array_colon());
   }

   if (  chunk_is_token(first, CT_CASE)
      && ((CharTable::IsKw1(second->str[0]) || chunk_is_token(second, CT_NUMBER))))
   {
      log_rule("sp_case_label");
      return(options::sp_case_label() | IARF_ADD);
   }

   if (chunk_is_token(first, CT_FOR_COLON))
   {
      // java
      log_rule("sp_after_for_colon");
      return(options::sp_after_for_colon());
   }
   if (chunk_is_token(second, CT_FOR_COLON))
   {
      // java
      log_rule("sp_before_for_colon");
      return(options::sp_before_for_colon());
   }

   if (chunk_is_token(first, CT_QUESTION) && chunk_is_token(second, CT_COND_COLON))
   {
      if (options::sp_cond_ternary_short() != IARF_IGNORE)
      {
         log_rule("sp_cond_ternary_short");
         return(options::sp_cond_ternary_short());
      }
   }

   if (chunk_is_token(first, CT_QUESTION) || chunk_is_token(second, CT_QUESTION))
   {
      if (  chunk_is_token(second, CT_QUESTION)
         && (options::sp_cond_question_before() != IARF_IGNORE))
      {
         log_rule("sp_cond_question_before");
         return(options::sp_cond_question_before());
      }
      if (  chunk_is_token(first, CT_QUESTION)
         && (options::sp_cond_question_after() != IARF_IGNORE))
      {
         log_rule("sp_cond_question_after");
         return(options::sp_cond_question_after());
      }
      if (options::sp_cond_question() != IARF_IGNORE)
      {
         log_rule("sp_cond_question");
         return(options::sp_cond_question());
      }
   }

   if (chunk_is_token(first, CT_COND_COLON) || chunk_is_token(second, CT_COND_COLON))
   {
      if (  chunk_is_token(second, CT_COND_COLON)
         && (options::sp_cond_colon_before() != IARF_IGNORE))
      {
         log_rule("sp_cond_colon_before");
         return(options::sp_cond_colon_before());
      }
      if (  chunk_is_token(first, CT_COND_COLON)
         && (options::sp_cond_colon_after() != IARF_IGNORE))
      {
         log_rule("sp_cond_colon_after");
         return(options::sp_cond_colon_after());
      }
      if (options::sp_cond_colon() != IARF_IGNORE)
      {
         log_rule("sp_cond_colon");
         return(options::sp_cond_colon());
      }
   }

   if (chunk_is_token(first, CT_RANGE) || chunk_is_token(second, CT_RANGE))
   {
      log_rule("sp_range");
      return(options::sp_range());
   }

   if (chunk_is_token(first, CT_COLON) && first->parent_type == CT_SQL_EXEC)
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   // Macro stuff can only return IGNORE, ADD, or FORCE
   if (chunk_is_token(first, CT_MACRO))
   {
      log_rule("sp_macro");
      iarf_e arg = options::sp_macro();
      return(arg | ((arg != IARF_IGNORE) ? IARF_ADD : IARF_IGNORE));
   }

   if (chunk_is_token(first, CT_FPAREN_CLOSE) && first->parent_type == CT_MACRO_FUNC)
   {
      log_rule("sp_macro_func");
      iarf_e arg = options::sp_macro_func();
      return(arg | ((arg != IARF_IGNORE) ? IARF_ADD : IARF_IGNORE));
   }

   if (chunk_is_token(first, CT_PREPROC))
   {
      // Remove spaces, unless we are ignoring. See indent_preproc()
      if (options::pp_space() == IARF_IGNORE)
      {
         log_rule("IGNORE");
         return(IARF_IGNORE);
      }
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (chunk_is_token(second, CT_SEMICOLON))
   {
      if (second->parent_type == CT_FOR)
      {
         if (  chunk_is_token(first, CT_SPAREN_OPEN) // a
            || chunk_is_token(first, CT_SEMICOLON))  // b
         {
            // empty, ie for (;;)
            //               ^ is first    // a
            //                ^ is second  // a
            // or
            //                ^ is first   // b
            //                 ^ is second // b
            log_rule("sp_before_semi_for_empty");
            return(options::sp_before_semi_for_empty());
         }
         if (options::sp_before_semi_for() != IARF_IGNORE)
         {
            log_rule("sp_before_semi_for");
            return(options::sp_before_semi_for());
         }
      }

      iarf_e arg = options::sp_before_semi();
      if (  chunk_is_token(first, CT_SPAREN_CLOSE)
         && first->parent_type != CT_WHILE_OF_DO)
      {
         log_rule("sp_before_semi|sp_special_semi");
         arg = arg | options::sp_special_semi();
      }
      else
      {
         log_rule("sp_before_semi");
      }
      return(arg);
   }

   if (  (  chunk_is_token(second, CT_COMMENT)
         || chunk_is_token(second, CT_COMMENT_CPP))
      && (  chunk_is_token(first, CT_PP_ELSE)
         || chunk_is_token(first, CT_PP_ENDIF)))
   {
      if (options::sp_endif_cmt() != IARF_IGNORE)
      {
         set_chunk_type(second, CT_COMMENT_ENDIF);
         log_rule("sp_endif_cmt");
         return(options::sp_endif_cmt());
      }
   }

   if (  (options::sp_before_tr_emb_cmt() != IARF_IGNORE)
      && (  second->parent_type == CT_COMMENT_END
         || second->parent_type == CT_COMMENT_EMBED))
   {
      log_rule("sp_before_tr_emb_cmt");
      min_sp = options::sp_num_before_tr_emb_cmt();
      return(options::sp_before_tr_emb_cmt());
   }

   if (second->parent_type == CT_COMMENT_END)
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

   // "for (;;)" vs "for (;; )" and "for (a;b;c)" vs "for (a; b; c)"
   if (chunk_is_token(first, CT_SEMICOLON))
   {
      if (first->parent_type == CT_FOR)
      {
         if (  (options::sp_after_semi_for_empty() != IARF_IGNORE)
            && chunk_is_token(second, CT_SPAREN_CLOSE))
         {
            log_rule("sp_after_semi_for_empty");
            return(options::sp_after_semi_for_empty());
         }
         if (  (options::sp_after_semi_for() != IARF_IGNORE)
            && second->type != CT_SPAREN_CLOSE)  // Issue 1324
         {
            log_rule("sp_after_semi_for");
            return(options::sp_after_semi_for());
         }
      }
      else if (!chunk_is_comment(second) && second->type != CT_BRACE_CLOSE) // issue #197
      {
         log_rule("sp_after_semi");
         return(options::sp_after_semi());
      }
      // Let the comment spacing rules handle this
   }

   // puts a space in the rare '+-' or '-+'
   if (  (  chunk_is_token(first, CT_NEG)
         || chunk_is_token(first, CT_POS)
         || chunk_is_token(first, CT_ARITH))
      && (  chunk_is_token(second, CT_NEG)
         || chunk_is_token(second, CT_POS)
         || chunk_is_token(second, CT_ARITH)))
   {
      log_rule("ADD");
      return(IARF_ADD);
   }

   // "return(a);" vs "return (foo_t)a + 3;" vs "return a;" vs "return;"
   if (chunk_is_token(first, CT_RETURN))
   {
      if (  chunk_is_token(second, CT_PAREN_OPEN)
         && second->parent_type == CT_RETURN)
      {
         log_rule("sp_return_paren");
         return(options::sp_return_paren());
      }
      else if (  chunk_is_token(second, CT_BRACE_OPEN)
              && second->parent_type == CT_BRACED_INIT_LIST)
      {
         log_rule("sp_return_brace");
         return(options::sp_return_brace());
      }
      // everything else requires a space
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   // "sizeof(foo_t)" vs "sizeof (foo_t)"
   if (chunk_is_token(first, CT_SIZEOF))
   {
      if (chunk_is_token(second, CT_PAREN_OPEN))
      {
         log_rule("sp_sizeof_paren");
         return(options::sp_sizeof_paren());
      }
      if (chunk_is_token(second, CT_ELLIPSIS))
      {
         log_rule("sp_sizeof_ellipsis");
         return(options::sp_sizeof_ellipsis());
      }
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   // "decltype(foo_t)" vs "decltype (foo_t)"
   if (chunk_is_token(first, CT_DECLTYPE))
   {
      if (chunk_is_token(second, CT_PAREN_OPEN))
      {
         log_rule("sp_decltype_paren");
         return(options::sp_decltype_paren());
      }
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   // handle '::'
   if (chunk_is_token(first, CT_DC_MEMBER))
   {
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

      // Issue #1005
      /* '::' at the start of an identifier is not member access, but global scope operator.
       * Detect if previous chunk is a type and previous-previous is "friend"
       */
      if (  chunk_is_token(first, CT_TYPE)
         && first->prev != nullptr
         && (first->prev->type == CT_FRIEND && first->next->type != CT_DC_MEMBER))
      {
         log_rule("FORCE");
         return(IARF_FORCE);
      }

      if (  (  chunk_is_token(first, CT_WORD)
            || chunk_is_token(first, CT_TYPE)
            || chunk_is_token(first, CT_PAREN_CLOSE)
            || CharTable::IsKw1(first->str[0]))
         && (strcmp(first->text(), "void") != 0)) // Issue 1249
      {
         log_rule("sp_before_dc");
         return(options::sp_before_dc());
      }
   }

   // "a,b" vs "a, b"
   if (chunk_is_token(first, CT_COMMA))
   {
      if (first->parent_type == CT_TYPE)
      {
         // C# multidimensional array type: ',,' vs ', ,' or ',]' vs ', ]'
         if (chunk_is_token(second, CT_COMMA))
         {
            log_rule("sp_between_mdatype_commas");
            return(options::sp_between_mdatype_commas());
         }

         log_rule("sp_after_mdatype_commas");
         return(options::sp_after_mdatype_commas());
      }
      // Fix for issue #1243
      // Don't add extra space after comma immediately followed by Angle close
      if (chunk_is_token(second, CT_ANGLE_CLOSE))
      {
         return(IARF_IGNORE);
      }

      log_rule("sp_after_comma");
      return(options::sp_after_comma());
   }
   // test if we are within a SIGNAL/SLOT call
   if (QT_SIGNAL_SLOT_found)
   {
      if (  chunk_is_token(first, CT_FPAREN_CLOSE)
         && (chunk_is_token(second, CT_FPAREN_CLOSE) || chunk_is_token(second, CT_COMMA)))
      {
         if (second->level == QT_SIGNAL_SLOT_level)
         {
            restoreValues = true;
         }
      }
   }
   if (chunk_is_token(second, CT_COMMA))
   {
      if (chunk_is_token(first, CT_SQUARE_OPEN) && first->parent_type == CT_TYPE)
      {
         // Only for C#.
         log_rule("sp_before_mdatype_commas");
         return(options::sp_before_mdatype_commas());
      }
      if (  chunk_is_token(first, CT_PAREN_OPEN)
         && (options::sp_paren_comma() != IARF_IGNORE))
      {
         log_rule("sp_paren_comma");
         return(options::sp_paren_comma());
      }
      log_rule("sp_before_comma");
      return(options::sp_before_comma());
   }

   if (chunk_is_token(second, CT_ELLIPSIS))
   {
      // non-punc followed by a ellipsis
      if (  chunk_is_token(first, CT_TYPE)
         || chunk_is_token(first, CT_QUALIFIER))
      {
         log_rule("sp_type_ellipsis");
         return(options::sp_type_ellipsis());
      }

      if (  ((first->flags & PCF_PUNCTUATOR) == 0)
         && (options::sp_before_ellipsis() != IARF_IGNORE))
      {
         log_rule("sp_before_ellipsis");
         return(options::sp_before_ellipsis());
      }

      if (  chunk_is_token(first, CT_FPAREN_CLOSE)
         || chunk_is_token(first, CT_PAREN_CLOSE))
      {
         log_rule("sp_paren_ellipsis");
         return(options::sp_paren_ellipsis());
      }

      if (chunk_is_token(first, CT_TAG_COLON))
      {
         log_rule("FORCE");
         return(IARF_FORCE);
      }
   }
   if (chunk_is_token(first, CT_ELLIPSIS))
   {
      if (CharTable::IsKw1(second->str[0]))
      {
         log_rule("FORCE");
         return(IARF_FORCE);
      }
      if (  chunk_is_token(second, CT_PAREN_OPEN)
         && first->prev && chunk_is_token(first->prev, CT_SIZEOF))
      {
         log_rule("sp_sizeof_ellipsis_paren");
         return(options::sp_sizeof_ellipsis_paren());
      }
   }
   if (chunk_is_token(first, CT_TAG_COLON))
   {
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
      log_rule("sp_oc_catch_paren");
      return(options::sp_oc_catch_paren());
   }

   if (  language_is_set(LANG_OC)
      && chunk_is_token(first, CT_OC_CLASS)
      && chunk_is_token(second, CT_PAREN_OPEN)
      && (options::sp_oc_classname_paren() != IARF_IGNORE))
   {
      log_rule("sp_oc_classname_paren");
      return(options::sp_oc_classname_paren());
   }

   if (  chunk_is_token(first, CT_CATCH)
      && chunk_is_token(second, CT_SPAREN_OPEN)
      && (options::sp_catch_paren() != IARF_IGNORE))
   {
      log_rule("sp_catch_paren");
      return(options::sp_catch_paren());
   }

   if (  chunk_is_token(first, CT_D_VERSION_IF)
      && chunk_is_token(second, CT_SPAREN_OPEN)
      && (options::sp_version_paren() != IARF_IGNORE))
   {
      log_rule("sp_version_paren");
      return(options::sp_version_paren());
   }

   if (  chunk_is_token(first, CT_D_SCOPE_IF)
      && chunk_is_token(second, CT_SPAREN_OPEN)
      && (options::sp_scope_paren() != IARF_IGNORE))
   {
      log_rule("sp_scope_paren");
      return(options::sp_scope_paren());
   }

   if (  language_is_set(LANG_OC)
      && chunk_is_token(first, CT_SYNCHRONIZED) && chunk_is_token(second, CT_SPAREN_OPEN))
   {
      log_rule("sp_after_oc_synchronized");
      return(options::sp_after_oc_synchronized());
   }

   // "if (" vs "if("
   if (chunk_is_token(second, CT_SPAREN_OPEN))
   {
      log_rule("sp_before_sparen");
      return(options::sp_before_sparen());
   }

   if (chunk_is_token(first, CT_LAMBDA) || chunk_is_token(second, CT_LAMBDA))
   {
      log_rule("sp_assign (lambda)");
      return(options::sp_assign());
   }

   // Handle the special lambda case for C++11:
   //    [=](Something arg){.....}
   if (  (options::sp_cpp_lambda_assign() != IARF_IGNORE)
      && (  (  chunk_is_token(first, CT_SQUARE_OPEN)
            && first->parent_type == CT_CPP_LAMBDA
            && chunk_is_token(second, CT_ASSIGN))
         || (  chunk_is_token(first, CT_ASSIGN)
            && chunk_is_token(second, CT_SQUARE_CLOSE)
            && second->parent_type == CT_CPP_LAMBDA)))
   {
      log_rule("sp_cpp_lambda_assign");
      return(options::sp_cpp_lambda_assign());
   }

   // Handle the special lambda case for C++11:
   //    [](Something arg){.....}
   if (  (options::sp_cpp_lambda_paren() != IARF_IGNORE)
      && chunk_is_token(first, CT_SQUARE_CLOSE)
      && first->parent_type == CT_CPP_LAMBDA
      && chunk_is_token(second, CT_FPAREN_OPEN))
   {
      log_rule("sp_cpp_lambda_paren");
      return(options::sp_cpp_lambda_paren());
   }

   if (chunk_is_token(first, CT_ENUM) && chunk_is_token(second, CT_FPAREN_OPEN))
   {
      if (options::sp_enum_paren() != IARF_IGNORE)
      {
         log_rule("sp_enum_paren");
         return(options::sp_enum_paren());
      }
   }

   if (chunk_is_token(second, CT_ASSIGN))
   {
      if (second->flags & PCF_IN_ENUM)
      {
         if (options::sp_enum_before_assign() != IARF_IGNORE)
         {
            log_rule("sp_enum_before_assign");
            return(options::sp_enum_before_assign());
         }
         log_rule("sp_enum_assign");
         return(options::sp_enum_assign());
      }
      if (  (options::sp_assign_default() != IARF_IGNORE)
         && second->parent_type == CT_FUNC_PROTO)
      {
         log_rule("sp_assign_default");
         return(options::sp_assign_default());
      }
      if (options::sp_before_assign() != IARF_IGNORE)
      {
         log_rule("sp_before_assign");
         return(options::sp_before_assign());
      }
      log_rule("sp_assign");
      return(options::sp_assign());
   }

   if (chunk_is_token(second, CT_ASSIGN_DEFAULT_ARG))
   {
      if (  (options::sp_assign_default() != IARF_IGNORE)
         && second->parent_type == CT_FUNC_PROTO)
      {
         log_rule("sp_assign_default");
         return(options::sp_assign_default());
      }
      if (options::sp_before_assign() != IARF_IGNORE)
      {
         log_rule("sp_before_assign");
         return(options::sp_before_assign());
      }
      log_rule("sp_assign");
      return(options::sp_assign());
   }

   if (chunk_is_token(first, CT_ASSIGN))
   {
      if (first->flags & PCF_IN_ENUM)
      {
         if (options::sp_enum_after_assign() != IARF_IGNORE)
         {
            log_rule("sp_enum_after_assign");
            return(options::sp_enum_after_assign());
         }
         log_rule("sp_enum_assign");
         return(options::sp_enum_assign());
      }
      if (  (options::sp_assign_default() != IARF_IGNORE)
         && first->parent_type == CT_FUNC_PROTO)
      {
         log_rule("sp_assign_default");
         return(options::sp_assign_default());
      }
      if (options::sp_after_assign() != IARF_IGNORE)
      {
         log_rule("sp_after_assign");
         return(options::sp_after_assign());
      }
      log_rule("sp_assign");
      return(options::sp_assign());
   }

   if (chunk_is_token(first, CT_ASSIGN_DEFAULT_ARG))
   {
      if (  (options::sp_assign_default() != IARF_IGNORE)
         && first->parent_type == CT_FUNC_PROTO)
      {
         log_rule("sp_assign_default");
         return(options::sp_assign_default());
      }
      if (options::sp_after_assign() != IARF_IGNORE)
      {
         log_rule("sp_after_assign");
         return(options::sp_after_assign());
      }
      log_rule("sp_assign");
      return(options::sp_assign());
   }

   if (chunk_is_token(first, CT_BIT_COLON))
   {
      if (first->flags & PCF_IN_ENUM)
      {
         log_rule("sp_enum_colon");
         return(options::sp_enum_colon());
      }
   }

   if (chunk_is_token(second, CT_BIT_COLON))
   {
      if (second->flags & PCF_IN_ENUM)
      {
         log_rule("sp_enum_colon");
         return(options::sp_enum_colon());
      }
   }

   if (chunk_is_token(first, CT_OC_AVAILABLE_VALUE) || chunk_is_token(second, CT_OC_AVAILABLE_VALUE))
   {
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }
   if (chunk_is_token(second, CT_OC_BLOCK_CARET))
   {
      log_rule("sp_before_oc_block_caret");
      return(options::sp_before_oc_block_caret());
   }
   if (chunk_is_token(first, CT_OC_BLOCK_CARET))
   {
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
         return(IARF_FORCE);
      }

      log_rule("sp_after_oc_msg_receiver");
      return(options::sp_after_oc_msg_receiver());
   }

   // c++17 structured bindings e.g., "auto [x, y, z]" vs a[x, y, z]" or "auto const [x, y, z]" vs "auto const[x, y, z]"
   if (  language_is_set(LANG_CPP)
      && (  chunk_is_token(first, CT_BYREF)
         || chunk_is_token(first, CT_QUALIFIER)
         || chunk_is_token(first, CT_TYPE))
      && chunk_is_token(second, CT_SQUARE_OPEN)
      && second->parent_type != CT_OC_MSG
      && second->parent_type != CT_CS_SQ_STMT)
   {
      log_rule("sp_cpp_before_struct_binding");
      return(options::sp_cpp_before_struct_binding());
   }

   // "a [x]" vs "a[x]"
   if (  chunk_is_token(second, CT_SQUARE_OPEN)
      && (second->parent_type != CT_OC_MSG && second->parent_type != CT_CS_SQ_STMT))
   {
      if (((second->flags & PCF_IN_SPAREN) != 0) && (chunk_is_token(first, CT_IN)))
      {
         return(IARF_FORCE);
      }

      log_rule("sp_before_square");
      return(options::sp_before_square());
   }

   // "byte[]" vs "byte []"
   if (chunk_is_token(second, CT_TSQUARE))
   {
      log_rule("sp_before_squares");
      return(options::sp_before_squares());
   }

   if (  (options::sp_angle_shift() != IARF_IGNORE)
      && chunk_is_token(first, CT_ANGLE_CLOSE)
      && chunk_is_token(second, CT_ANGLE_CLOSE))
   {
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
         log_rule("sp_inside_angle_empty");

         return(options::sp_inside_angle_empty());
      }

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
         log_rule("sp_template_angle");
         return(options::sp_template_angle());
      }
      if (first->type != CT_QUALIFIER)
      {
         log_rule("sp_before_angle");
         return(options::sp_before_angle());
      }
   }
   if (chunk_is_token(first, CT_ANGLE_CLOSE))
   {
      if (chunk_is_token(second, CT_WORD) || CharTable::IsKw1(second->str[0]))
      {
         if (options::sp_angle_word() != IARF_IGNORE)
         {
            log_rule("sp_angle_word");
            return(options::sp_angle_word());
         }
      }
      if (chunk_is_token(second, CT_FPAREN_OPEN) || chunk_is_token(second, CT_PAREN_OPEN))
      {
         chunk_t *next = chunk_get_next_ncnl(second);
         if (chunk_is_token(next, CT_FPAREN_CLOSE))
         {
            log_rule("sp_angle_paren_empty");
            return(options::sp_angle_paren_empty());
         }

         log_rule("sp_angle_paren");
         return(options::sp_angle_paren());
      }
      if (chunk_is_token(second, CT_DC_MEMBER))
      {
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
            log_rule("sp_angle_colon");
            return(options::sp_angle_colon());
         }
         log_rule("sp_after_angle");
         return(options::sp_after_angle());
      }
   }

   if (  chunk_is_token(first, CT_BYREF)
      && (options::sp_after_byref_func() != IARF_IGNORE)
      && (  first->parent_type == CT_FUNC_DEF
         || first->parent_type == CT_FUNC_PROTO))
   {
      log_rule("sp_after_byref_func");
      return(options::sp_after_byref_func());
   }

   if (  chunk_is_token(first, CT_BYREF)
      && (CharTable::IsKw1(second->str[0]) || chunk_is_token(second, CT_PAREN_OPEN)))
   {
      log_rule("sp_after_byref");
      return(options::sp_after_byref());
   }

   if (  chunk_is_token(second, CT_BYREF)
      && !chunk_is_token(first, CT_PAREN_OPEN))
   {
      if (options::sp_before_byref_func() != IARF_IGNORE)
      {
         chunk_t *next = chunk_get_next(second);
         if (  next != nullptr
            && (  next->parent_type == CT_FUNC_DEF
               || next->parent_type == CT_FUNC_PROTO))
         {
            log_rule("sp_before_byref_func");
            return(options::sp_before_byref_func());
         }
      }

      if (options::sp_before_unnamed_byref() != IARF_IGNORE)
      {
         chunk_t *next = chunk_get_next_nc(second);
         if (next != nullptr && next->type != CT_WORD)
         {
            log_rule("sp_before_unnamed_byref");
            return(options::sp_before_unnamed_byref());
         }
      }
      log_rule("sp_before_byref");
      return(options::sp_before_byref());
   }

   if (chunk_is_token(first, CT_SPAREN_CLOSE))
   {
      if (chunk_is_token(second, CT_BRACE_OPEN))
      {
         if (second->parent_type == CT_CATCH)
         {
            if (language_is_set(LANG_OC) && (options::sp_oc_catch_brace() != IARF_IGNORE))
            {
               log_rule("sp_oc_catch_brace");
               return(options::sp_oc_catch_brace());
            }
            if (options::sp_catch_brace() != IARF_IGNORE)
            {
               log_rule("sp_catch_brace");
               return(options::sp_catch_brace());
            }
         }
         if (options::sp_sparen_brace() != IARF_IGNORE)
         {
            log_rule("sp_sparen_brace");
            return(options::sp_sparen_brace());
         }
      }
      if (  !chunk_is_comment(second)
         && (options::sp_after_sparen() != IARF_IGNORE))
      {
         log_rule("sp_after_sparen");
         return(options::sp_after_sparen());
      }
   }
   if (  chunk_is_token(first, CT_VBRACE_OPEN)
      && chunk_is_token(second, CT_SEMICOLON)) // Issue # 1158
   {
      log_rule("sp_before_semi");
      return(options::sp_before_semi());
   }

   if (  chunk_is_token(second, CT_FPAREN_OPEN)
      && first->parent_type == CT_OPERATOR
      && (options::sp_after_operator_sym() != IARF_IGNORE))
   {
      if (  (options::sp_after_operator_sym_empty() != IARF_IGNORE)
         && chunk_is_token(second, CT_FPAREN_OPEN))
      {
         chunk_t *next = chunk_get_next_ncnl(second);
         if (chunk_is_token(next, CT_FPAREN_CLOSE))
         {
            log_rule("sp_after_operator_sym_empty");
            return(options::sp_after_operator_sym_empty());
         }
      }

      log_rule("sp_after_operator_sym");
      return(options::sp_after_operator_sym());
   }

   // Issue #2270
   // Translations under vala
   if (  language_is_set(LANG_VALA)
      && chunk_is_token(first, CT_FUNC_CALL))
   {
      if (  chunk_is_str(first, "_", 1)
         && chunk_is_token(second, CT_FPAREN_OPEN)
         && (options::sp_vala_after_translation() != IARF_IGNORE))
      {
         log_rule("sp_vala_after_translation");
         return(options::sp_vala_after_translation());
      }
   }

   // spaces between function and open paren
   if (  chunk_is_token(first, CT_FUNC_CALL)
      || chunk_is_token(first, CT_FUNC_CTOR_VAR)
      || chunk_is_token(first, CT_CNG_HASINC)
      || chunk_is_token(first, CT_CNG_HASINCN))
   {
      if (  (options::sp_func_call_paren_empty() != IARF_IGNORE)
         && chunk_is_token(second, CT_FPAREN_OPEN))
      {
         chunk_t *next = chunk_get_next_ncnl(second);
         if (chunk_is_token(next, CT_FPAREN_CLOSE))
         {
            log_rule("sp_func_call_paren_empty");
            return(options::sp_func_call_paren_empty());
         }
      }
      log_rule("sp_func_call_paren");
      return(options::sp_func_call_paren());
   }
   if (chunk_is_token(first, CT_FUNC_CALL_USER))
   {
      log_rule("sp_func_call_user_paren");
      return(options::sp_func_call_user_paren());
   }
   if (chunk_is_token(first, CT_ATTRIBUTE) && chunk_is_paren_open(second))
   {
      log_rule("sp_attribute_paren");
      return(options::sp_attribute_paren());
   }
   if (chunk_is_token(first, CT_FUNC_DEF))
   {
      if (  (options::sp_func_def_paren_empty() != IARF_IGNORE)
         && chunk_is_token(second, CT_FPAREN_OPEN))
      {
         chunk_t *next = chunk_get_next_ncnl(second);
         if (chunk_is_token(next, CT_FPAREN_CLOSE))
         {
            log_rule("sp_func_def_paren_empty");
            return(options::sp_func_def_paren_empty());
         }
      }
      log_rule("sp_func_def_paren");
      return(options::sp_func_def_paren());
   }
   if (chunk_is_token(first, CT_CPP_CAST) || chunk_is_token(first, CT_TYPE_WRAP))
   {
      log_rule("sp_cpp_cast_paren");
      return(options::sp_cpp_cast_paren());
   }

   if (chunk_is_token(first, CT_SPAREN_CLOSE) && chunk_is_token(second, CT_WHEN))
   {
      log_rule("FORCE");
      return(IARF_FORCE); // TODO: make this configurable?
   }

   if (  chunk_is_token(first, CT_PAREN_CLOSE)
      && (chunk_is_token(second, CT_PAREN_OPEN) || chunk_is_token(second, CT_FPAREN_OPEN)))
   {
      // "(int)a" vs "(int) a" or "cast(int)a" vs "cast(int) a"
      if (first->parent_type == CT_C_CAST || first->parent_type == CT_D_CAST)
      {
         log_rule("sp_after_cast");
         return(options::sp_after_cast());
      }

      // Must be an indirect/chained function call?
      log_rule("REMOVE");
      return(IARF_REMOVE);  // TODO: make this configurable?
   }

   // handle the space between parens in fcn type 'void (*f)(void)'
   if (chunk_is_token(first, CT_TPAREN_CLOSE))
   {
      log_rule("sp_after_tparen_close");
      return(options::sp_after_tparen_close());
   }

   // ")(" vs ") ("
   if (  (  chunk_is_str(first, ")", 1)
         && chunk_is_str(second, "(", 1))
      || (chunk_is_paren_close(first) && chunk_is_paren_open(second)))
   {
      log_rule("sp_cparen_oparen");
      return(options::sp_cparen_oparen());
   }

   if (  chunk_is_token(first, CT_FUNC_PROTO)
      || (  chunk_is_token(second, CT_FPAREN_OPEN)
         && second->parent_type == CT_FUNC_PROTO))
   {
      if (  (options::sp_func_proto_paren_empty() != IARF_IGNORE)
         && chunk_is_token(second, CT_FPAREN_OPEN))
      {
         chunk_t *next = chunk_get_next_ncnl(second);
         if (chunk_is_token(next, CT_FPAREN_CLOSE))
         {
            log_rule("sp_func_proto_paren_empty");
            return(options::sp_func_proto_paren_empty());
         }
      }
      log_rule("sp_func_proto_paren");
      return(options::sp_func_proto_paren());
   }
   if (chunk_is_token(first, CT_FUNC_CLASS_DEF) || chunk_is_token(first, CT_FUNC_CLASS_PROTO))
   {
      if (  (options::sp_func_class_paren_empty() != IARF_IGNORE)
         && chunk_is_token(second, CT_FPAREN_OPEN))
      {
         chunk_t *next = chunk_get_next_ncnl(second);
         if (chunk_is_token(next, CT_FPAREN_CLOSE))
         {
            log_rule("sp_func_class_paren_empty");
            return(options::sp_func_class_paren_empty());
         }
      }
      log_rule("sp_func_class_paren");
      return(options::sp_func_class_paren());
   }
   if (chunk_is_token(first, CT_CLASS) && !(first->flags & PCF_IN_OC_MSG))
   {
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   if (chunk_is_token(first, CT_BRACE_OPEN) && chunk_is_token(second, CT_BRACE_CLOSE))
   {
      log_rule("sp_inside_braces_empty");
      return(options::sp_inside_braces_empty());
   }

   if (  !chunk_is_token(first, CT_BRACE_OPEN)
      && !chunk_is_token(first, CT_FPAREN_OPEN)
      && chunk_is_token(second, CT_BRACE_OPEN)
      && second->parent_type == CT_BRACED_INIT_LIST)
   {
      auto arg = iarf_flags_t{ options::sp_type_brace_init_lst() };
      if (arg || first->parent_type != CT_DECLTYPE)
      {
         // 'int{9}' vs 'int {9}'
         log_rule("sp_type_brace_init_lst");
         return(arg);
      }
   }

   if (chunk_is_token(second, CT_BRACE_CLOSE))
   {
      if (second->parent_type == CT_ENUM)
      {
         log_rule("sp_inside_braces_enum");
         return(options::sp_inside_braces_enum());
      }
      if (second->parent_type == CT_STRUCT || second->parent_type == CT_UNION)
      {
         // Fix for issue #1240  adding space in struct initializers
         chunk_t *tmp = chunk_get_prev_ncnl(chunk_skip_to_match_rev(second));
         if (chunk_is_token(tmp, CT_ASSIGN))
         {
            return(IARF_IGNORE);
         }
         log_rule("sp_inside_braces_struct");
         return(options::sp_inside_braces_struct());
      }
      else if (  second->parent_type == CT_OC_AT
              && options::sp_inside_braces_oc_dict() != IARF_IGNORE)
      {
         log_rule("sp_inside_braces_oc_dict");
         return(options::sp_inside_braces_oc_dict());
      }

      if (second->parent_type == CT_BRACED_INIT_LIST)
      {
         if (  options::sp_brace_brace() != IARF_IGNORE
            && chunk_is_token(first, CT_BRACE_CLOSE)
            && first->parent_type == CT_BRACED_INIT_LIST)
         {
            log_rule("sp_brace_brace");
            return(options::sp_brace_brace());
         }
         if (options::sp_before_type_brace_init_lst_close() != IARF_IGNORE)
         {
            log_rule("sp_before_type_brace_init_lst_close");
            return(options::sp_before_type_brace_init_lst_close());
         }
         if (options::sp_inside_type_brace_init_lst() != IARF_IGNORE)
         {
            log_rule("sp_inside_type_brace_init_lst");
            return(options::sp_inside_type_brace_init_lst());
         }
      }

      log_rule("sp_inside_braces");
      return(options::sp_inside_braces());
   }

   if (chunk_is_token(first, CT_D_CAST))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (chunk_is_token(first, CT_PP_DEFINED) && chunk_is_token(second, CT_PAREN_OPEN))
   {
      log_rule("sp_defined_paren");
      return(options::sp_defined_paren());
   }

   if (chunk_is_token(first, CT_THROW))
   {
      if (chunk_is_token(second, CT_PAREN_OPEN))
      {
         log_rule("sp_throw_paren");
         return(options::sp_throw_paren());
      }
      log_rule("sp_after_throw");
      return(options::sp_after_throw());
   }

   if (chunk_is_token(first, CT_THIS) && chunk_is_token(second, CT_PAREN_OPEN))
   {
      log_rule("sp_this_paren");
      return(options::sp_this_paren());
   }

   if (chunk_is_token(first, CT_STATE) && chunk_is_token(second, CT_PAREN_OPEN))
   {
      log_rule("ADD");
      return(IARF_ADD);
   }

   if (chunk_is_token(first, CT_DELEGATE) && chunk_is_token(second, CT_PAREN_OPEN))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (chunk_is_token(first, CT_MEMBER) || chunk_is_token(second, CT_MEMBER))
   {
      log_rule("sp_member");
      return(options::sp_member());
   }

   if (chunk_is_token(first, CT_C99_MEMBER))
   {
      // always remove space(s) after then '.' of a C99-member
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (chunk_is_token(first, CT_SUPER) && chunk_is_token(second, CT_PAREN_OPEN))
   {
      log_rule("sp_super_paren");
      return(options::sp_super_paren());
   }

   if (chunk_is_token(first, CT_FPAREN_CLOSE) && chunk_is_token(second, CT_BRACE_OPEN))
   {
      if (second->parent_type == CT_DOUBLE_BRACE)
      {
         log_rule("sp_fparen_dbrace");
         return(options::sp_fparen_dbrace());
      }
      // To fix issue #1234
      // check for initializers and add space or ignore based on the option.
      if (first->parent_type == CT_FUNC_CALL)
      {
         chunk_t *tmp = chunk_get_prev_type(first, first->parent_type, first->level);
         tmp = chunk_get_prev_ncnl(tmp);
         if (chunk_is_token(tmp, CT_NEW))
         {
            log_rule("sp_fparen_brace_initializer");
            return(options::sp_fparen_brace_initializer());
         }
      }
      log_rule("sp_fparen_brace");
      return(options::sp_fparen_brace());
   }

   if (chunk_is_token(first, CT_D_TEMPLATE) || chunk_is_token(second, CT_D_TEMPLATE))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (chunk_is_token(first, CT_ELSE) && chunk_is_token(second, CT_BRACE_OPEN))
   {
      log_rule("sp_else_brace");
      return(options::sp_else_brace());
   }

   if (chunk_is_token(first, CT_ELSE) && chunk_is_token(second, CT_ELSEIF))
   {
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   if (chunk_is_token(first, CT_FINALLY) && chunk_is_token(second, CT_BRACE_OPEN))
   {
      log_rule("sp_finally_brace");
      return(options::sp_finally_brace());
   }

   if (chunk_is_token(first, CT_TRY) && chunk_is_token(second, CT_BRACE_OPEN))
   {
      log_rule("sp_try_brace");
      return(options::sp_try_brace());
   }

   if (chunk_is_token(first, CT_GETSET) && chunk_is_token(second, CT_BRACE_OPEN))
   {
      log_rule("sp_getset_brace");
      return(options::sp_getset_brace());
   }

   if (chunk_is_token(first, CT_WORD) && chunk_is_token(second, CT_BRACE_OPEN))
   {
      if (first->parent_type == CT_NAMESPACE)
      {
         log_rule("sp_word_brace_ns");
         return(options::sp_word_brace_ns());
      }
      if (first->parent_type == CT_NONE && second->parent_type == CT_NONE)
      {
         log_rule("sp_word_brace");
         return(options::sp_word_brace());
      }
   }

   if (chunk_is_token(second, CT_PAREN_OPEN) && second->parent_type == CT_INVARIANT)
   {
      log_rule("sp_invariant_paren");
      return(options::sp_invariant_paren());
   }

   if (chunk_is_token(first, CT_PAREN_CLOSE) && first->parent_type != CT_DECLTYPE)
   {
      if (first->parent_type == CT_D_TEMPLATE)
      {
         log_rule("FORCE");
         return(IARF_FORCE);
      }

      if (first->parent_type == CT_INVARIANT)
      {
         log_rule("sp_after_invariant_paren");
         return(options::sp_after_invariant_paren());
      }

      // "(struct foo) {...}" vs "(struct foo){...}"
      if (chunk_is_token(second, CT_BRACE_OPEN))
      {
         log_rule("sp_paren_brace");
         return(options::sp_paren_brace());
      }

      // D-specific: "delegate(some thing) dg
      if (first->parent_type == CT_DELEGATE)
      {
         log_rule("ADD");
         return(IARF_ADD);
      }

      // PAWN-specific: "state (condition) next"
      if (first->parent_type == CT_STATE)
      {
         log_rule("ADD");
         return(IARF_ADD);
      }

      /* C++ new operator: new(bar) Foo */
      if (first->parent_type == CT_NEW)
      {
         log_rule("sp_after_newop_paren");
         return(options::sp_after_newop_paren());
      }
   }

   /* "((" vs "( (" or "))" vs ") )" */
   // Issue #1342
   if (  (chunk_is_str(first, "(", 1) && chunk_is_str(second, "(", 1))
      || (chunk_is_str(first, ")", 1) && chunk_is_str(second, ")", 1)))
   {
      if (second->parent_type == CT_FUNC_CALL_USER)
      {
         log_rule("sp_func_call_user_paren_paren");
         return(options::sp_func_call_user_paren_paren());
      }
      log_rule("sp_paren_paren");
      return(options::sp_paren_paren());
   }

   // "foo(...)" vs "foo( ... )"
   if (chunk_is_token(first, CT_FPAREN_OPEN) || chunk_is_token(second, CT_FPAREN_CLOSE))
   {
      if (  (first->parent_type == CT_FUNC_CALL_USER)
         || (  (second->parent_type == CT_FUNC_CALL_USER)
            && ((chunk_is_token(first, CT_WORD)) || (chunk_is_token(first, CT_SQUARE_CLOSE)))))
      {
         log_rule("sp_func_call_user_inside_fparen");
         return(options::sp_func_call_user_inside_fparen());
      }
      if (chunk_is_token(first, CT_FPAREN_OPEN) && chunk_is_token(second, CT_FPAREN_CLOSE))
      {
         log_rule("sp_inside_fparens");
         return(options::sp_inside_fparens());
      }
      log_rule("sp_inside_fparen");
      return(options::sp_inside_fparen());
   }

   // "foo(...)" vs "foo( ... )"
   if (chunk_is_token(first, CT_TPAREN_OPEN) || chunk_is_token(second, CT_TPAREN_CLOSE))
   {
      log_rule("sp_inside_tparen");
      return(options::sp_inside_tparen());
   }

   if (chunk_is_token(first, CT_PAREN_CLOSE))
   {
      if (  (first->flags & PCF_OC_RTYPE)  // == CT_OC_RTYPE)
         && (  first->parent_type == CT_OC_MSG_DECL
            || first->parent_type == CT_OC_MSG_SPEC))
      {
         log_rule("sp_after_oc_return_type");
         return(options::sp_after_oc_return_type());
      }

      if (first->parent_type == CT_OC_MSG_SPEC || first->parent_type == CT_OC_MSG_DECL)
      {
         log_rule("sp_after_oc_type");
         return(options::sp_after_oc_type());
      }

      if (first->parent_type == CT_OC_SEL && second->type != CT_SQUARE_CLOSE)
      {
         log_rule("sp_after_oc_at_sel_parens");
         return(options::sp_after_oc_at_sel_parens());
      }
   }

   if (options::sp_inside_oc_at_sel_parens() != IARF_IGNORE)
   {
      if (  (  chunk_is_token(first, CT_PAREN_OPEN)
            && (  first->parent_type == CT_OC_SEL
               || first->parent_type == CT_OC_PROTOCOL))
         || (  chunk_is_token(second, CT_PAREN_CLOSE)
            && (  second->parent_type == CT_OC_SEL
               || second->parent_type == CT_OC_PROTOCOL)))
      {
         log_rule("sp_inside_oc_at_sel_parens");
         return(options::sp_inside_oc_at_sel_parens());
      }
   }

   if (  chunk_is_token(second, CT_PAREN_OPEN)
      && (chunk_is_token(first, CT_OC_SEL) || chunk_is_token(first, CT_OC_PROTOCOL)))
   {
      log_rule("sp_after_oc_at_sel");
      return(options::sp_after_oc_at_sel());
   }

   /*
    * C cast:   "(int)"      vs "( int )"
    * D cast:   "cast(int)"  vs "cast( int )"
    * CPP cast: "int(a + 3)" vs "int( a + 3 )"
    */
   if (chunk_is_token(first, CT_PAREN_OPEN))
   {
      if (  first->parent_type == CT_C_CAST
         || first->parent_type == CT_CPP_CAST
         || first->parent_type == CT_D_CAST)
      {
         log_rule("sp_inside_paren_cast");
         return(options::sp_inside_paren_cast());
      }
      if (first->parent_type == CT_NEW)
      {
         if (options::sp_inside_newop_paren_open() != IARF_IGNORE)
         {
            log_rule("sp_inside_newop_paren_open");
            return(options::sp_inside_newop_paren_open());
         }
         if (options::sp_inside_newop_paren() != IARF_IGNORE)
         {
            log_rule("sp_inside_newop_paren");
            return(options::sp_inside_newop_paren());
         }
      }
      log_rule("sp_inside_paren");
      return(options::sp_inside_paren());
   }

   if (chunk_is_token(second, CT_PAREN_CLOSE))
   {
      if (  second->parent_type == CT_C_CAST
         || second->parent_type == CT_CPP_CAST
         || second->parent_type == CT_D_CAST)
      {
         log_rule("sp_inside_paren_cast");
         return(options::sp_inside_paren_cast());
      }
      if (second->parent_type == CT_NEW)
      {
         if (options::sp_inside_newop_paren_close() != IARF_IGNORE)
         {
            log_rule("sp_inside_newop_paren_close");
            return(options::sp_inside_newop_paren_close());
         }
         if (options::sp_inside_newop_paren() != IARF_IGNORE)
         {
            log_rule("sp_inside_newop_paren");
            return(options::sp_inside_newop_paren());
         }
      }
      log_rule("sp_inside_paren");
      return(options::sp_inside_paren());
   }

   // "[3]" vs "[ 3 ]" or for objective-c "@[@3]" vs "@[ @3 ]"
   if (chunk_is_token(first, CT_SQUARE_OPEN) || chunk_is_token(second, CT_SQUARE_CLOSE))
   {
      if (  language_is_set(LANG_OC)
         && (  (first->parent_type == CT_OC_AT && chunk_is_token(first, CT_SQUARE_OPEN))
            || (second->parent_type == CT_OC_AT && chunk_is_token(second, CT_SQUARE_CLOSE)))
         && (options::sp_inside_square_oc_array() != IARF_IGNORE))
      {
         log_rule("sp_inside_square_oc_array");
         return(options::sp_inside_square_oc_array());
      }
      log_rule("sp_inside_square");
      return(options::sp_inside_square());
   }
   if (chunk_is_token(first, CT_SQUARE_CLOSE) && chunk_is_token(second, CT_FPAREN_OPEN))
   {
      log_rule("sp_square_fparen");
      return(options::sp_square_fparen());
   }

   // "if(...)" vs "if( ... )"
   if (  chunk_is_token(second, CT_SPAREN_CLOSE)
      && (options::sp_inside_sparen_close() != IARF_IGNORE))
   {
      log_rule("sp_inside_sparen_close");
      return(options::sp_inside_sparen_close());
   }
   if (  chunk_is_token(first, CT_SPAREN_OPEN)
      && (options::sp_inside_sparen_open() != IARF_IGNORE))
   {
      log_rule("sp_inside_sparen_open");
      return(options::sp_inside_sparen_open());
   }
   if (chunk_is_token(first, CT_SPAREN_OPEN) || chunk_is_token(second, CT_SPAREN_CLOSE))
   {
      log_rule("sp_inside_sparen");
      return(options::sp_inside_sparen());
   }

   if (chunk_is_token(first, CT_CLASS_COLON))
   {
      if (  first->parent_type == CT_OC_CLASS
         && (  !chunk_get_prev_type(first, CT_OC_INTF, first->level, scope_e::ALL)
            && !chunk_get_prev_type(first, CT_OC_IMPL, first->level, scope_e::ALL)))
      {
         if (options::sp_after_oc_colon() != IARF_IGNORE)
         {
            log_rule("sp_after_oc_colon");
            return(options::sp_after_oc_colon());
         }
      }

      if (options::sp_after_class_colon() != IARF_IGNORE)
      {
         log_rule("sp_after_class_colon");
         return(options::sp_after_class_colon());
      }
   }
   if (chunk_is_token(second, CT_CLASS_COLON))
   {
      if (  second->parent_type == CT_OC_CLASS
         && (  !chunk_get_prev_type(second, CT_OC_INTF, second->level, scope_e::ALL)
            && !chunk_get_prev_type(second, CT_OC_IMPL, second->level, scope_e::ALL)))
      {
         if (second->parent_type == CT_OC_CLASS && !chunk_get_prev_type(second, CT_OC_INTF, second->level, scope_e::ALL))
         {
            if (options::sp_before_oc_colon() != IARF_IGNORE)
            {
               log_rule("sp_before_oc_colon");
               return(options::sp_before_oc_colon());
            }
         }
      }

      if (options::sp_before_class_colon() != IARF_IGNORE)
      {
         log_rule("sp_before_class_colon");
         return(options::sp_before_class_colon());
      }
   }

   if (  (options::sp_after_constr_colon() != IARF_IGNORE)
      && chunk_is_token(first, CT_CONSTR_COLON))
   {
      min_sp = options::indent_ctor_init_leading() - 1; // default indent is 1 space

      log_rule("sp_after_constr_colon");
      return(options::sp_after_constr_colon());
   }
   if (  (options::sp_before_constr_colon() != IARF_IGNORE)
      && chunk_is_token(second, CT_CONSTR_COLON))
   {
      log_rule("sp_before_constr_colon");
      return(options::sp_before_constr_colon());
   }

   if (  (options::sp_before_case_colon() != IARF_IGNORE)
      && chunk_is_token(second, CT_CASE_COLON))
   {
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

   if (chunk_is_token(first, CT_NULLCOND) || chunk_is_token(second, CT_NULLCOND))
   {
      log_rule("sp_member");
      return(options::sp_member());
   }

   if (  chunk_is_token(first, CT_ARITH)
      || chunk_is_token(first, CT_CARET)
      || chunk_is_token(second, CT_ARITH)
      || chunk_is_token(second, CT_CARET))
   {
      if (options::sp_arith_additive() != IARF_IGNORE)
      {
         auto arith_char = (chunk_is_token(first, CT_ARITH) || chunk_is_token(first, CT_CARET))
                           ? first->str[0] : second->str[0];
         if (arith_char == '+' || arith_char == '-')
         {
            log_rule("sp_arith_additive");
            return(options::sp_arith_additive());
         }
      }

      log_rule("sp_arith");
      return(options::sp_arith());
   }
   if (chunk_is_token(first, CT_BOOL) || chunk_is_token(second, CT_BOOL))
   {
      iarf_e arg = options::sp_bool();
      if (  (options::pos_bool() != TP_IGNORE)
         && first->orig_line != second->orig_line
         && arg != IARF_REMOVE)
      {
         arg = arg | IARF_ADD;
      }
      log_rule("sp_bool");
      return(arg);
   }
   if (chunk_is_token(first, CT_COMPARE) || chunk_is_token(second, CT_COMPARE))
   {
      log_rule("sp_compare");
      return(options::sp_compare());
   }

   if (chunk_is_token(first, CT_PAREN_OPEN) && chunk_is_token(second, CT_PTR_TYPE))
   {
      log_rule("REMOVE");
      return(IARF_REMOVE);
   }

   if (  chunk_is_token(first, CT_PTR_TYPE)
      && (options::sp_ptr_star_paren() != IARF_IGNORE)
      && (chunk_is_token(second, CT_FPAREN_OPEN) || chunk_is_token(second, CT_TPAREN_OPEN)))
   {
      log_rule("sp_ptr_star_paren");
      return(options::sp_ptr_star_paren());
   }

   if (  chunk_is_token(first, CT_PTR_TYPE)
      && chunk_is_token(second, CT_PTR_TYPE)
      && (options::sp_between_ptr_star() != IARF_IGNORE))
   {
      log_rule("sp_between_ptr_star");
      return(options::sp_between_ptr_star());
   }

   if (  chunk_is_token(first, CT_PTR_TYPE)
      && (options::sp_after_ptr_star_func() != IARF_IGNORE)
      && (  first->parent_type == CT_FUNC_DEF
         || first->parent_type == CT_FUNC_PROTO
         || first->parent_type == CT_FUNC_VAR))
   {
      log_rule("sp_after_ptr_star_func");
      return(options::sp_after_ptr_star_func());
   }

   if (chunk_is_token(first, CT_PTR_TYPE) && CharTable::IsKw1(second->str[0]))
   {
      chunk_t *prev = chunk_get_prev(first);
      if (chunk_is_token(prev, CT_IN))
      {
         log_rule("sp_deref");
         return(options::sp_deref());
      }

      if (  (first->parent_type == CT_FUNC_VAR || first->parent_type == CT_FUNC_TYPE)
         && options::sp_after_ptr_block_caret() != IARF_IGNORE)
      {
         log_rule("sp_after_ptr_block_caret");
         return(options::sp_after_ptr_block_caret());
      }

      if (  chunk_is_token(second, CT_QUALIFIER)
         && (options::sp_after_ptr_star_qualifier() != IARF_IGNORE))
      {
         log_rule("sp_after_ptr_star_qualifier");
         return(options::sp_after_ptr_star_qualifier());
      }

      if (options::sp_after_ptr_star() != IARF_IGNORE)
      {
         log_rule("sp_after_ptr_star");
         return(options::sp_after_ptr_star());
      }
   }

   if (chunk_is_token(second, CT_PTR_TYPE) && first->type != CT_IN)
   {
      if (language_is_set(LANG_CS) && chunk_is_nullable(second))
      {
         min_sp = 0;
         return(IARF_REMOVE);
      }
      if (options::sp_before_ptr_star_func() != IARF_IGNORE)
      {
         // Find the next non-'*' chunk
         chunk_t *next = second;
         do
         {
            next = chunk_get_next(next);
         } while (chunk_is_token(next, CT_PTR_TYPE));

         if (chunk_is_token(next, CT_FUNC_DEF) || chunk_is_token(next, CT_FUNC_PROTO))
         {
            log_rule("sp_before_ptr_star_func");
            return(options::sp_before_ptr_star_func());
         }
      }

      if (options::sp_before_unnamed_ptr_star() != IARF_IGNORE)
      {
         chunk_t *next = chunk_get_next_nc(second);
         while (chunk_is_token(next, CT_PTR_TYPE))
         {
            next = chunk_get_next_nc(next);
         }
         if (next != nullptr && next->type != CT_WORD)
         {
            log_rule("sp_before_unnamed_ptr_star");
            return(options::sp_before_unnamed_ptr_star());
         }
      }
      if (options::sp_before_ptr_star() != IARF_IGNORE)
      {
         log_rule("sp_before_ptr_star");
         return(options::sp_before_ptr_star());
      }
   }

   if (chunk_is_token(first, CT_OPERATOR))
   {
      log_rule("sp_after_operator");
      return(options::sp_after_operator());
   }

   if (chunk_is_token(second, CT_FUNC_PROTO) || chunk_is_token(second, CT_FUNC_DEF))
   {
      if (first->type != CT_PTR_TYPE)
      {
         log_rule("sp_type_func|ADD");
         return(options::sp_type_func() | IARF_ADD);
      }
      log_rule("sp_type_func");
      return(options::sp_type_func());
   }

   // "(int)a" vs "(int) a" or "cast(int)a" vs "cast(int) a"
   if (  (first->parent_type == CT_C_CAST || first->parent_type == CT_D_CAST)
      && chunk_is_token(first, CT_PAREN_CLOSE))
   {
      log_rule("sp_after_cast");
      return(options::sp_after_cast());
   }

   if (chunk_is_token(first, CT_BRACE_CLOSE))
   {
      if (chunk_is_token(second, CT_ELSE))
      {
         log_rule("sp_brace_else");
         return(options::sp_brace_else());
      }

      if (  language_is_set(LANG_OC)
         && chunk_is_token(second, CT_CATCH)
         && (options::sp_oc_brace_catch() != IARF_IGNORE))
      {
         log_rule("sp_oc_brace_catch");
         return(options::sp_oc_brace_catch());
      }

      if (chunk_is_token(second, CT_CATCH))
      {
         log_rule("sp_brace_catch");
         return(options::sp_brace_catch());
      }

      if (chunk_is_token(second, CT_FINALLY))
      {
         log_rule("sp_brace_finally");
         return(options::sp_brace_finally());
      }
   }

   if (chunk_is_token(first, CT_BRACE_OPEN))
   {
      if (first->parent_type == CT_ENUM)
      {
         log_rule("sp_inside_braces_enum");
         return(options::sp_inside_braces_enum());
      }
      if (first->parent_type == CT_STRUCT || first->parent_type == CT_UNION)
      {
         // Fix for issue #1240  adding space in struct initializers
         chunk_t *tmp = chunk_get_prev_ncnl(first);
         if (chunk_is_token(tmp, CT_ASSIGN))
         {
            return(IARF_IGNORE);
         }
         log_rule("sp_inside_braces_struct");
         return(options::sp_inside_braces_struct());
      }
      else if (  first->parent_type == CT_OC_AT
              && options::sp_inside_braces_oc_dict() != IARF_IGNORE)
      {
         log_rule("sp_inside_braces_oc_dict");
         return(options::sp_inside_braces_oc_dict());
      }
      if (first->parent_type == CT_BRACED_INIT_LIST)
      {
         if (  options::sp_brace_brace() != IARF_IGNORE
            && chunk_is_token(second, CT_BRACE_OPEN)
            && second->parent_type == CT_BRACED_INIT_LIST)
         {
            log_rule("sp_brace_brace");
            return(options::sp_brace_brace());
         }
         if (options::sp_after_type_brace_init_lst_open() != IARF_IGNORE)
         {
            log_rule("sp_after_type_brace_init_lst_open");
            return(options::sp_after_type_brace_init_lst_open());
         }
         if (options::sp_inside_type_brace_init_lst() != IARF_IGNORE)
         {
            log_rule("sp_inside_type_brace_init_lst");
            return(options::sp_inside_type_brace_init_lst());
         }
      }
      if (!chunk_is_comment(second))
      {
         log_rule("sp_inside_braces");
         return(options::sp_inside_braces());
      }
   }


   if (  chunk_is_token(first, CT_BRACE_CLOSE)
      && (first->flags & PCF_IN_TYPEDEF)
      && (  first->parent_type == CT_ENUM
         || first->parent_type == CT_STRUCT
         || first->parent_type == CT_UNION))
   {
      log_rule("sp_brace_typedef");
      return(options::sp_brace_typedef());
   }

   if (chunk_is_token(second, CT_PAREN_OPEN) && second->parent_type == CT_TEMPLATE)
   {
      log_rule("sp_before_template_paren");
      return(options::sp_before_template_paren());
   }

   if (  !chunk_is_token(second, CT_PTR_TYPE)
      && chunk_is_token(first, CT_PAREN_CLOSE)
      && first->parent_type == CT_DECLTYPE)
   {
      if (auto arg = iarf_flags_t{ options::sp_after_decltype() })
      {
         log_rule("sp_after_decltype");
         return(arg);
      }
      log_rule("sp_after_type");
      return(options::sp_after_type());
   }

   if (  language_is_set(LANG_VALA)
      && chunk_is_token(second, CT_QUESTION))
   {
      // Issue #2090
      log_rule("sp_type_question");
      return(options::sp_type_question());
   }
   if (  !chunk_is_token(second, CT_PTR_TYPE)
      && (chunk_is_token(first, CT_QUALIFIER) || chunk_is_token(first, CT_TYPE)))
   {
      iarf_e arg = options::sp_after_type();
      log_rule("sp_after_type");
      return(arg);
   }

   if (  chunk_is_token(first, CT_MACRO_OPEN)
      || chunk_is_token(first, CT_MACRO_CLOSE)
      || chunk_is_token(first, CT_MACRO_ELSE))
   {
      if (chunk_is_token(second, CT_PAREN_OPEN))
      {
         log_rule("sp_func_call_paren");
         return(options::sp_func_call_paren());
      }
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }

   // If nothing claimed the PTR_TYPE, then return ignore
   if (chunk_is_token(first, CT_PTR_TYPE) || chunk_is_token(second, CT_PTR_TYPE))
   {
      log_rule("IGNORE");
      return(IARF_IGNORE);
   }

   if (chunk_is_token(first, CT_NOT))
   {
      log_rule("sp_not");
      return(options::sp_not());
   }
   if (chunk_is_token(first, CT_INV))
   {
      log_rule("sp_inv");
      return(options::sp_inv());
   }
   if (chunk_is_token(first, CT_ADDR))
   {
      log_rule("sp_addr");
      return(options::sp_addr());
   }
   if (chunk_is_token(first, CT_DEREF))
   {
      log_rule("sp_deref");
      return(options::sp_deref());
   }
   if (chunk_is_token(first, CT_POS) || chunk_is_token(first, CT_NEG))
   {
      log_rule("sp_sign");
      return(options::sp_sign());
   }
   if (chunk_is_token(first, CT_INCDEC_BEFORE) || chunk_is_token(second, CT_INCDEC_AFTER))
   {
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
   if (chunk_is_token(first, CT_OC_SCOPE))
   {
      log_rule("sp_after_oc_scope");
      return(options::sp_after_oc_scope());
   }
   if (chunk_is_token(first, CT_OC_DICT_COLON))
   {
      log_rule("sp_after_oc_dict_colon");
      return(options::sp_after_oc_dict_colon());
   }
   if (chunk_is_token(second, CT_OC_DICT_COLON))
   {
      log_rule("sp_before_oc_dict_colon");
      return(options::sp_before_oc_dict_colon());
   }
   if (chunk_is_token(first, CT_OC_COLON))
   {
      if (first->flags & PCF_IN_OC_MSG)
      {
         log_rule("sp_after_send_oc_colon");
         return(options::sp_after_send_oc_colon());
      }

      log_rule("sp_after_oc_colon");
      return(options::sp_after_oc_colon());
   }
   if (chunk_is_token(second, CT_OC_COLON))
   {
      if (  (first->flags & PCF_IN_OC_MSG)
         && (chunk_is_token(first, CT_OC_MSG_FUNC) || chunk_is_token(first, CT_OC_MSG_NAME)))
      {
         log_rule("sp_before_send_oc_colon");
         return(options::sp_before_send_oc_colon());
      }

      log_rule("sp_before_oc_colon");
      return(options::sp_before_oc_colon());
   }

   if (chunk_is_token(second, CT_COMMENT) && second->parent_type == CT_COMMENT_EMBED)
   {
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   if (chunk_is_token(first, CT_COMMENT))
   {
      log_rule("FORCE");
      return(IARF_FORCE);
   }

   if (chunk_is_token(first, CT_NEW) && chunk_is_token(second, CT_PAREN_OPEN))
   {
      // c# new Constraint, c++ new operator
      log_rule("sp_between_new_paren");
      return(options::sp_between_new_paren());
   }
   if (  chunk_is_token(first, CT_NEW)
      || chunk_is_token(first, CT_DELETE)
      || (chunk_is_token(first, CT_TSQUARE) && first->parent_type == CT_DELETE))
   {
      log_rule("sp_after_new");
      return(options::sp_after_new());
   }

   if (chunk_is_token(first, CT_ANNOTATION) && chunk_is_paren_open(second))
   {
      log_rule("sp_annotation_paren");
      return(options::sp_annotation_paren());
   }

   if (chunk_is_token(first, CT_OC_PROPERTY))
   {
      log_rule("sp_after_oc_property");
      return(options::sp_after_oc_property());
   }

   if (chunk_is_token(first, CT_EXTERN) && chunk_is_token(second, CT_PAREN_OPEN))
   {
      log_rule("sp_extern_paren");
      return(options::sp_extern_paren());
   }

   if (  chunk_is_token(second, CT_TYPE)
      && (  (chunk_is_token(first, CT_STRING) && first->parent_type == CT_EXTERN)
         || (chunk_is_token(first, CT_FPAREN_CLOSE) && first->parent_type == CT_ATTRIBUTE)))
   {
      log_rule("FORCE");
      return(IARF_FORCE);  /* TODO: make this configurable? */
   }

   // this table lists out all combos where a space should NOT be present
   // CT_UNKNOWN is a wildcard.
   for (auto it : no_space_table)
   {
      if (  (it.first == CT_UNKNOWN || it.first == first->type)
         && (it.second == CT_UNKNOWN || it.second == second->type))
      {
         log_rule("REMOVE from no_space_table");
         return(IARF_REMOVE);
      }
   }

   if (chunk_is_token(first, CT_NOEXCEPT))
   {
      log_rule("sp_after_noexcept");
      return(options::sp_after_noexcept());
   }

   // Issue #2138
   if (chunk_is_token(first, CT_FPAREN_CLOSE))
   {
      if (chunk_is_token(second, CT_QUALIFIER))
      {
         log_rule("sp_paren_qualifier");
         return(options::sp_paren_qualifier());
      }
      else if (chunk_is_token(second, CT_NOEXCEPT))
      {
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

   // these lines are only useful for debugging uncrustify itself
   D_LOG_FMT(LSPACE, "\n\n%s(%d): WARNING: unrecognize do_space:\n",
             __func__, __LINE__);
   D_LOG_FMT(LSPACE, "   first->orig_line  is %zu, first->orig_col  is %zu, first->text()  '%s', first->type is  %s\n",
             first->orig_line, first->orig_col, first->text(), get_token_name(first->type));
   D_LOG_FMT(LSPACE, "   second->orig_line is %zu, second->orig_col is %zu, second->text() '%s', second->type is %s\n",
             second->orig_line, second->orig_col, second->text(), get_token_name(second->type));

   log_rule("ADD as default value");
   return(IARF_ADD);
} // do_space


static iarf_e ensure_force_space(chunk_t *first, chunk_t *second, iarf_e av)
{
   if (first->flags & PCF_FORCE_SPACE)
   {
      LOG_FMT(LSPACE, " <force between '%s' and '%s'>",
              first->text(), second->text());
      return(av | IARF_ADD);
   }

   return(av);
}


static iarf_e do_space_ensured(chunk_t *first, chunk_t *second, int &min_sp)
{
   return(ensure_force_space(first, second, do_space(first, second, min_sp)));
}


void space_text(void)
{
   LOG_FUNC_ENTRY();

   chunk_t *pc = chunk_get_head();
   if (pc == nullptr)
   {
      return;
   }

   chunk_t *next;
   size_t  prev_column;
   size_t  column = pc->column;
   while (pc != nullptr)
   {
      if (chunk_is_token(pc, CT_NEWLINE))
      {
         LOG_FMT(LSPACE, "%s(%d): orig_line is %zu, orig_col is %zu, <Newline>, nl is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->nl_count);
      }
      else
      {
         LOG_FMT(LSPACE, "%s(%d): orig_line is %zu, orig_col is %zu, '%s' type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
      }
      if (  (options::use_options_overriding_for_qt_macros())
         && (  (strcmp(pc->text(), "SIGNAL") == 0)
            || (strcmp(pc->text(), "SLOT") == 0)))
      {  // guy 2015-09-22
         LOG_FMT(LSPACE, "%s(%d): orig_col is %zu, type is %s SIGNAL/SLOT found\n",
                 __func__, __LINE__, pc->orig_line, get_token_name(pc->type));
         chunk_flags_set(pc, PCF_IN_QT_MACRO); // flag the chunk for a second processing

         // save the values
         save_set_options_for_QT(pc->level);
      } // guy
        // Bug # 637
      if (options::sp_skip_vbrace_tokens())
      {
         next = chunk_get_next(pc);
         while (  chunk_is_blank(next)
               && !chunk_is_newline(next)
               && (chunk_is_token(next, CT_VBRACE_OPEN) || chunk_is_token(next, CT_VBRACE_CLOSE)))
         {
            LOG_FMT(LSPACE, "%s(%d): orig_line is %zu, orig_col is %zu, Skip %s (%zu+%zu)\n",
                    __func__, __LINE__, next->orig_line, next->orig_col, get_token_name(next->type),
                    pc->column, pc->str.size());
            next->column = pc->column + pc->str.size();
            next         = chunk_get_next(next);
         }
      }
      else
      {
         next = pc->next;
      }
      if (!next)
      {
         break;
      }
      // Issue # 481
      if ((QT_SIGNAL_SLOT_found) && (options::sp_balance_nested_parens()))
      {
         if (next->next != nullptr && next->next->type == CT_SPACE)
         {
            chunk_del(next->next); // remove the space
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
            column += pc->len();
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
         if (  (pc->len() > 0)
            && !chunk_is_str(pc, "[]", 2)
            && !chunk_is_str(pc, "{{", 2)
            && !chunk_is_str(pc, "}}", 2)
            && !chunk_is_str(pc, "()", 2)
            && !pc->str.startswith("@\""))
         {
            // Find the next non-empty chunk on this line
            chunk_t *tmp = next;
            // TODO: better use chunk_search here
            while (  tmp != nullptr
                  && (tmp->len() == 0)
                  && !chunk_is_newline(tmp))
            {
               tmp = chunk_get_next(tmp);
            }
            if (tmp != nullptr && tmp->len() > 0)
            {
               bool kw1 = CharTable::IsKw2(pc->str[pc->len() - 1]);
               bool kw2 = CharTable::IsKw1(next->str[0]);
               if (kw1 && kw2)
               {
                  // back-to-back words need a space
                  LOG_FMT(LSPACE, "%s(%d): back-to-back words need a space: pc->text() '%s', next->text() '%s'\n",
                          __func__, __LINE__, pc->text(), next->text());
                  chunk_flags_set(pc, PCF_FORCE_SPACE);
               }
               // TODO:  what is the meaning of 4
               else if (  !kw1
                       && !kw2
                       && (pc->len() < 4)
                       && (next->len() < 4))
               {
                  // We aren't dealing with keywords. concat and try punctuators
                  char buf[9];
                  memcpy(buf, pc->text(), pc->len());
                  memcpy(buf + pc->len(), next->text(), next->len());
                  buf[pc->len() + next->len()] = 0;

                  const chunk_tag_t *ct;
                  ct = find_punctuator(buf, cpd.lang_flags);
                  if (ct != nullptr && (strlen(ct->tag) != pc->len()))
                  {
                     // punctuator parsed to a different size..

                     /*
                      * C++11 allows '>>' to mean '> >' in templates:
                      *   some_func<vector<string>>();
                      */
                     if (  (  (  language_is_set(LANG_CPP)
                              && options::sp_permit_cpp11_shift())
                           || (language_is_set(LANG_JAVA | LANG_CS | LANG_VALA)))
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
                        LOG_FMT(LSPACE, "%s(%d): : pc->text() is %s, next->text() is %s\n",
                                __func__, __LINE__, pc->text(), next->text());
                        chunk_flags_set(pc, PCF_FORCE_SPACE);
                     }
                  }
               }
            }
         }

         int min_sp;
         LOG_FMT(LSPACE, "%s(%d): orig_line is %zu, orig_col is %zu, pc-text() '%s', type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
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
            if (next->orig_col >= pc->orig_col_end && pc->orig_col_end != 0)
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
            if (next->orig_col >= pc->orig_col_end && pc->orig_col_end != 0)
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

         if (  chunk_is_comment(next)
            && chunk_is_newline(chunk_get_next(next))
            && column < next->orig_col)
         {
            /*
             * do some comment adjustments if sp_before_tr_emb_cmt and
             * sp_endif_cmt did not apply.
             */
            if (  (  options::sp_before_tr_emb_cmt() == IARF_IGNORE
                  || (  next->parent_type != CT_COMMENT_END
                     && next->parent_type != CT_COMMENT_EMBED))
               && (  options::sp_endif_cmt() == IARF_IGNORE
                  || (  pc->type != CT_PP_ELSE
                     && pc->type != CT_PP_ENDIF)))
            {
               if (options::indent_relative_single_line_comments())
               {
                  // Try to keep relative spacing between tokens
                  LOG_FMT(LSPACE, "%s(%d): <relative adj>", __func__, __LINE__);
                  LOG_FMT(LSPACE, "%s(%d): pc is '%s', pc->orig_col is %zu, next->orig_col is %zu, pc->orig_col_end is %zu\n",
                          __func__, __LINE__, pc->text(),
                          pc->orig_col, next->orig_col, pc->orig_col_end);
                  column = pc->column + (next->orig_col - pc->orig_col_end);
               }
               else
               {
                  /*
                   * If there was a space, we need to force one, otherwise
                   * try to keep the comment in the same column.
                   */
                  size_t col_min = pc->column + pc->len() + ((next->orig_prev_sp > 0) ? 1 : 0);
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

   chunk_t *first = chunk_get_head();
   while (first != nullptr)
   {
      chunk_t *next = chunk_get_next(first);
      if (next == nullptr)
      {
         break;
      }

      // if there are two successive opening parenthesis
      if (chunk_is_str(first, "(", 1) && chunk_is_str(next, "(", 1))
      {
         // insert a space between them
         space_add_after(first, 1);

         // test after the closing parens   Issue #1703
         chunk_t *closing = chunk_get_next_type(first, (c_token_t)(first->type + 1), first->level);
         if (closing->orig_col == closing->prev->orig_col_end)
         {
            space_add_after(closing->prev, 1);
         }
      }
      else if (chunk_is_str(first, ")", 1) && chunk_is_str(next, ")", 1))
      {
         // insert a space between the two closing parens
         space_add_after(first, 1);

         // test after the opening parens   Issue #1703
         chunk_t *opening = chunk_get_prev_type(next, (c_token_t)(next->type - 1), next->level);
         if (opening->orig_col_end == opening->next->orig_col)
         {
            space_add_after(opening, 1);
         }
      }

      first = next;
   }
} // space_text_balance_nested_parens


size_t space_needed(chunk_t *first, chunk_t *second)
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
      return(second->orig_col > (first->orig_col + first->len()));
   }
}


size_t space_col_align(chunk_t *first, chunk_t *second)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LSPACE, "%s(%d): first->orig_line is %zu, orig_col is %zu, [%s/%s], text() '%s' <==>\n",
           __func__, __LINE__, first->orig_line, first->orig_col,
           get_token_name(first->type), get_token_name(first->parent_type),
           first->text());
   LOG_FMT(LSPACE, "%s(%d): second->orig_line is %zu, orig_col is %zu [%s/%s], text() '%s',",
           __func__, __LINE__, second->orig_line, second->orig_col,
           get_token_name(second->type), get_token_name(second->parent_type),
           second->text());
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
      LOG_FMT(LSPACE, "%s(%d):    len is %zu\n", __func__, __LINE__, first->len());
      coldiff = first->len();
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
      LOG_FMT(LSPACE, "%s(%d):    => first->text()     is '%s'\n", __func__, __LINE__, first->text());
      LOG_FMT(LSPACE, "%s(%d):    => second->text()    is '%s'\n", __func__, __LINE__, second->text());
      LOG_FMT(LSPACE, "%s(%d):    => first->orig_col   is %zu\n", __func__, __LINE__, first->orig_col);
      LOG_FMT(LSPACE, "%s(%d):    => second->orig_col  is %zu\n", __func__, __LINE__, second->orig_col);
      LOG_FMT(LSPACE, "%s(%d):    => first->len()      is %zu\n", __func__, __LINE__, first->len());
      if (  first->orig_line == second->orig_line
         && second->orig_col > (first->orig_col + first->len()))
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


void space_add_after(chunk_t *pc, size_t count)
{
   LOG_FUNC_ENTRY();

   chunk_t *next = chunk_get_next(pc);

   // don't add at the end of the file or before a newline
   if (next == nullptr || chunk_is_newline(next))
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
      if (next->len() < count)
      {
         while (next->len() < count)
         {
            next->str.append(' ');
         }
      }
      return;
   }

   chunk_t sp;

   sp.flags = pc->flags & PCF_COPY_FLAGS;
   sp.type  = CT_SPACE;
   sp.str   = "                ";       // 16 spaces
   sp.str.resize(count);
   sp.level       = pc->level;
   sp.brace_level = pc->brace_level;
   sp.pp_level    = pc->pp_level;
   sp.column      = pc->column + pc->len();
   sp.orig_line   = pc->orig_line;
   sp.orig_col    = pc->orig_col;

   chunk_add_after(&sp, pc);
} // space_add_after
