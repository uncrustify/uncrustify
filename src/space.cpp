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
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "prototypes.h"
#include "char_table.h"
#include "options_for_QT.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "unc_ctype.h"
#include "uncrustify.h"


static void log_rule2(size_t line, const char *rule, chunk_t *first, chunk_t *second, bool complete);


/**
 * Decides how to change inter-chunk spacing.
 * Note that the order of the if statements is VERY important.
 *
 * @param first   The first chunk
 * @param second  The second chunk
 *
 * @return AV_IGNORE, AV_ADD, AV_REMOVE or AV_FORCE
 */
static argval_t do_space(chunk_t *first, chunk_t *second, int &min_sp, bool complete);


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
   { CT_OC_AT,          CT_UNKNOWN       },
   { CT_INCDEC_BEFORE,  CT_WORD          },
   { CT_UNKNOWN,        CT_INCDEC_AFTER  },
   { CT_UNKNOWN,        CT_LABEL_COLON   },
   { CT_UNKNOWN,        CT_PRIVATE_COLON },
   { CT_UNKNOWN,        CT_SEMICOLON     },
   { CT_UNKNOWN,        CT_D_TEMPLATE    },
   { CT_D_TEMPLATE,     CT_UNKNOWN       },
   { CT_MACRO_FUNC,     CT_FPAREN_OPEN   },
   { CT_PAREN_OPEN,     CT_UNKNOWN       },
   { CT_UNKNOWN,        CT_PAREN_CLOSE   },
   { CT_FPAREN_OPEN,    CT_UNKNOWN       },
   { CT_UNKNOWN,        CT_SPAREN_CLOSE  },
   { CT_SPAREN_OPEN,    CT_UNKNOWN       },
   { CT_UNKNOWN,        CT_FPAREN_CLOSE  },
   { CT_UNKNOWN,        CT_COMMA         },
   { CT_POS,            CT_UNKNOWN       },
   { CT_STAR,           CT_UNKNOWN       },
   { CT_VBRACE_CLOSE,   CT_UNKNOWN       },
   { CT_VBRACE_OPEN,    CT_UNKNOWN       },
   { CT_UNKNOWN,        CT_VBRACE_CLOSE  },
   { CT_UNKNOWN,        CT_VBRACE_OPEN   },
   { CT_PREPROC,        CT_UNKNOWN       },
   { CT_PREPROC_INDENT, CT_UNKNOWN       },
   { CT_NEG,            CT_UNKNOWN       },
   { CT_UNKNOWN,        CT_SQUARE_OPEN   },
   { CT_UNKNOWN,        CT_SQUARE_CLOSE  },
   { CT_SQUARE_OPEN,    CT_UNKNOWN       },
   { CT_PAREN_CLOSE,    CT_WORD          },
   { CT_PAREN_CLOSE,    CT_FUNC_DEF      },
   { CT_PAREN_CLOSE,    CT_FUNC_CALL     },
   { CT_PAREN_CLOSE,    CT_ADDR          },
   { CT_PAREN_CLOSE,    CT_FPAREN_OPEN   },
   { CT_OC_SEL_NAME,    CT_OC_SEL_NAME   },
   { CT_TYPENAME,       CT_TYPE          },
};

#define log_rule(rule)                                             \
   do { if (log_sev_on(LSPACE)) {                                  \
           log_rule2(__LINE__, (rule), first, second, complete); } \
   } while (0)


static void log_rule2(size_t line, const char *rule, chunk_t *first, chunk_t *second, bool complete)
{
   LOG_FUNC_ENTRY();
   if (second->type != CT_NEWLINE)
   {
      LOG_FMT(LSPACE, "%s(%d): Spacing: line %zu [%s/%s] '%s' <===> [%s/%s] '%s' : %s[%zu]%s",
              __func__, __LINE__, first->orig_line,
              get_token_name(first->type), get_token_name(first->parent_type),
              first->text(),
              get_token_name(second->type), get_token_name(second->parent_type),
              second->text(),
              rule, line,
              complete ? "\n" : "");
   }
}


/*
 * this function is called for every chunk in the input file.
 * Thus it is important to keep this function efficient
 */
static argval_t do_space(chunk_t *first, chunk_t *second, int &min_sp, bool complete = true)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LSPACE, "%s(%d): orig_line is %zu, orig_col is %zu, type is %s, %s\n",
           __func__, __LINE__, first->orig_line, first->orig_col, get_token_name(first->type), first->text());

   min_sp = 1;
   if (first->type == CT_IGNORED || second->type == CT_IGNORED)
   {
      log_rule("IGNORED");
      return(AV_REMOVE);
   }
   if (first->type == CT_PP_IGNORE && second->type == CT_PP_IGNORE)
   {
      // Leave spacing alone between PP_IGNORE tokens as we don't want the default behavior (which is ADD).
      log_rule("PP_IGNORE");
      return(AV_IGNORE);
   }
   if (first->type == CT_PP || second->type == CT_PP)
   {
      log_rule("sp_pp_concat");
      return(cpd.settings[UO_sp_pp_concat].a);
   }
   if (first->type == CT_POUND)
   {
      log_rule("sp_pp_stringify");
      return(cpd.settings[UO_sp_pp_stringify].a);
   }
   if (  second->type == CT_POUND
      && (second->flags & PCF_IN_PREPROC)
      && first->parent_type != CT_MACRO_FUNC)
   {
      log_rule("sp_before_pp_stringify");
      return(cpd.settings[UO_sp_before_pp_stringify].a);
   }

   if (first->type == CT_SPACE || second->type == CT_SPACE)
   {
      log_rule("REMOVE");
      return(AV_REMOVE);
   }

   if (second->type == CT_NEWLINE || second->type == CT_VBRACE_OPEN)
   {
      log_rule("REMOVE");
      return(AV_REMOVE);
   }
   if (first->type == CT_VBRACE_OPEN && second->type != CT_NL_CONT)
   {
      log_rule("FORCE");
      return(AV_FORCE);
   }
   if (first->type == CT_VBRACE_CLOSE && second->type != CT_NL_CONT)
   {
      log_rule("REMOVE");
      return(AV_REMOVE);
   }
   if (second->type == CT_VSEMICOLON)
   {
      log_rule("REMOVE");
      return(AV_REMOVE);
   }
   if (first->type == CT_MACRO_FUNC)
   {
      log_rule("REMOVE");
      return(AV_REMOVE);
   }
   if (second->type == CT_NL_CONT)
   {
      log_rule("sp_before_nl_cont");
      return(cpd.settings[UO_sp_before_nl_cont].a);
   }

   if (first->type == CT_D_ARRAY_COLON || second->type == CT_D_ARRAY_COLON)
   {
      log_rule("sp_d_array_colon");
      return(cpd.settings[UO_sp_d_array_colon].a);
   }

   if (  first->type == CT_CASE
      && ((CharTable::IsKw1(second->str[0]) || second->type == CT_NUMBER)))
   {
      log_rule("sp_case_label");
      return(argval_t(cpd.settings[UO_sp_case_label].a | AV_ADD));
   }

   if (first->type == CT_FOR_COLON)
   {
      log_rule("sp_after_for_colon");
      return(cpd.settings[UO_sp_after_for_colon].a);
   }
   if (second->type == CT_FOR_COLON)
   {
      log_rule("sp_before_for_colon");
      return(cpd.settings[UO_sp_before_for_colon].a);
   }

   if (first->type == CT_QUESTION && second->type == CT_COND_COLON)
   {
      if (cpd.settings[UO_sp_cond_ternary_short].a != AV_IGNORE)
      {
         return(cpd.settings[UO_sp_cond_ternary_short].a);
      }
   }

   if (first->type == CT_QUESTION || second->type == CT_QUESTION)
   {
      if (  second->type == CT_QUESTION
         && (cpd.settings[UO_sp_cond_question_before].a != AV_IGNORE))
      {
         log_rule("sp_cond_question_before");
         return(cpd.settings[UO_sp_cond_question_before].a);
      }
      if (  first->type == CT_QUESTION
         && (cpd.settings[UO_sp_cond_question_after].a != AV_IGNORE))
      {
         log_rule("sp_cond_question_after");
         return(cpd.settings[UO_sp_cond_question_after].a);
      }
      if (cpd.settings[UO_sp_cond_question].a != AV_IGNORE)
      {
         log_rule("sp_cond_question");
         return(cpd.settings[UO_sp_cond_question].a);
      }
   }

   if (first->type == CT_COND_COLON || second->type == CT_COND_COLON)
   {
      if (  second->type == CT_COND_COLON
         && (cpd.settings[UO_sp_cond_colon_before].a != AV_IGNORE))
      {
         log_rule("sp_cond_colon_before");
         return(cpd.settings[UO_sp_cond_colon_before].a);
      }
      if (  first->type == CT_COND_COLON
         && (cpd.settings[UO_sp_cond_colon_after].a != AV_IGNORE))
      {
         log_rule("sp_cond_colon_after");
         return(cpd.settings[UO_sp_cond_colon_after].a);
      }
      if (cpd.settings[UO_sp_cond_colon].a != AV_IGNORE)
      {
         log_rule("sp_cond_colon");
         return(cpd.settings[UO_sp_cond_colon].a);
      }
   }

   if (first->type == CT_RANGE || second->type == CT_RANGE)
   {
      return(cpd.settings[UO_sp_range].a);
   }

   if (first->type == CT_COLON && first->parent_type == CT_SQL_EXEC)
   {
      log_rule("REMOVE");
      return(AV_REMOVE);
   }

   // Macro stuff can only return IGNORE, ADD, or FORCE
   if (first->type == CT_MACRO)
   {
      log_rule("sp_macro");
      argval_t arg = cpd.settings[UO_sp_macro].a;
      return(static_cast<argval_t>(arg | ((arg != AV_IGNORE) ? AV_ADD : AV_IGNORE)));
   }

   if (first->type == CT_FPAREN_CLOSE && first->parent_type == CT_MACRO_FUNC)
   {
      log_rule("sp_macro_func");
      argval_t arg = cpd.settings[UO_sp_macro_func].a;
      return(static_cast<argval_t>(arg | ((arg != AV_IGNORE) ? AV_ADD : AV_IGNORE)));
   }

   if (first->type == CT_PREPROC)
   {
      // Remove spaces, unless we are ignoring. See indent_preproc()
      if (cpd.settings[UO_pp_space].a == AV_IGNORE)
      {
         log_rule("IGNORE");
         return(AV_IGNORE);
      }
      log_rule("REMOVE");
      return(AV_REMOVE);
   }

   if (second->type == CT_SEMICOLON)
   {
      if (second->parent_type == CT_FOR)
      {
         if (  (cpd.settings[UO_sp_before_semi_for_empty].a != AV_IGNORE)
            && (  first->type == CT_SPAREN_OPEN
               || first->type == CT_SEMICOLON))
         {
            log_rule("sp_before_semi_for_empty");
            return(cpd.settings[UO_sp_before_semi_for_empty].a);
         }
         if (cpd.settings[UO_sp_before_semi_for].a != AV_IGNORE)
         {
            log_rule("sp_before_semi_for");
            return(cpd.settings[UO_sp_before_semi_for].a);
         }
      }

      argval_t arg = cpd.settings[UO_sp_before_semi].a;
      if (  first->type == CT_SPAREN_CLOSE
         && first->parent_type != CT_WHILE_OF_DO)
      {
         log_rule("sp_before_semi|sp_special_semi");
         arg = static_cast<argval_t>(arg | cpd.settings[UO_sp_special_semi].a);
      }
      else
      {
         log_rule("sp_before_semi");
      }
      return(arg);
   }

   if (  second->type == CT_COMMENT
      && (first->type == CT_PP_ELSE || first->type == CT_PP_ENDIF))
   {
      if (cpd.settings[UO_sp_endif_cmt].a != AV_IGNORE)
      {
         set_chunk_type(second, CT_COMMENT_ENDIF);
         log_rule("sp_endif_cmt");
         return(cpd.settings[UO_sp_endif_cmt].a);
      }
   }

   if (  (cpd.settings[UO_sp_before_tr_emb_cmt].a != AV_IGNORE)
      && (  second->parent_type == CT_COMMENT_END
         || second->parent_type == CT_COMMENT_EMBED))
   {
      log_rule("sp_before_tr_emb_cmt");
      min_sp = cpd.settings[UO_sp_num_before_tr_emb_cmt].u;
      return(cpd.settings[UO_sp_before_tr_emb_cmt].a);
   }

   if (second->parent_type == CT_COMMENT_END)
   {
      switch (second->orig_prev_sp)
      {
      case 0:
         log_rule("orig_prev_sp-REMOVE");
         return(AV_REMOVE);

      case 1:
         log_rule("orig_prev_sp-FORCE");
         return(AV_FORCE);

      default:
         log_rule("orig_prev_sp-ADD");
         return(AV_ADD);
      }
   }

   // "for (;;)" vs "for (;; )" and "for (a;b;c)" vs "for (a; b; c)"
   if (first->type == CT_SEMICOLON)
   {
      if (first->parent_type == CT_FOR)
      {
         if (  (cpd.settings[UO_sp_after_semi_for_empty].a != AV_IGNORE)
            && second->type == CT_SPAREN_CLOSE)
         {
            log_rule("sp_after_semi_for_empty");
            return(cpd.settings[UO_sp_after_semi_for_empty].a);
         }
         if (cpd.settings[UO_sp_after_semi_for].a != AV_IGNORE)
         {
            log_rule("sp_after_semi_for");
            return(cpd.settings[UO_sp_after_semi_for].a);
         }
      }
      else if (!chunk_is_comment(second) && second->type != CT_BRACE_CLOSE) // issue #197
      {
         log_rule("sp_after_semi");
         return(cpd.settings[UO_sp_after_semi].a);
      }
      // Let the comment spacing rules handle this
   }

   // puts a space in the rare '+-' or '-+'
   if (  (  first->type == CT_NEG
         || first->type == CT_POS
         || first->type == CT_ARITH)
      && (  second->type == CT_NEG
         || second->type == CT_POS
         || second->type == CT_ARITH))
   {
      log_rule("ADD");
      return(AV_ADD);
   }

   // "return(a);" vs "return (foo_t)a + 3;" vs "return a;" vs "return;"
   if (first->type == CT_RETURN)
   {
      if (second->type == CT_PAREN_OPEN && second->parent_type == CT_RETURN)
      {
         log_rule("sp_return_paren");
         return(cpd.settings[UO_sp_return_paren].a);
      }
      // everything else requires a space
      log_rule("FORCE");
      return(AV_FORCE);
   }

   // "sizeof(foo_t)" vs "sizeof foo_t"
   if (first->type == CT_SIZEOF)
   {
      if (second->type == CT_PAREN_OPEN)
      {
         log_rule("sp_sizeof_paren");
         return(cpd.settings[UO_sp_sizeof_paren].a);
      }
      log_rule("FORCE");
      return(AV_FORCE);
   }

   // handle '::'
   if (first->type == CT_DC_MEMBER)
   {
      log_rule("sp_after_dc");
      return(cpd.settings[UO_sp_after_dc].a);
   }
   // Issue #889
   // mapped_file_source abc((int) ::CW2A(sTemp));
   if (  first->type == CT_PAREN_CLOSE
      && second->type == CT_DC_MEMBER
      && second->next != nullptr
      && second->next->type == CT_FUNC_CALL)
   {
      log_rule("REMOVE_889_A");
      return(AV_REMOVE);
   }
   if (second->type == CT_DC_MEMBER)
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
      case CT_PRIVATE:
      case CT_QUALIFIER:
      case CT_RETURN:
      case CT_SIZEOF:
      case CT_STRUCT:
      case CT_THROW:
      case CT_TYPEDEF:
      case CT_TYPENAME:
      case CT_UNION:
      case CT_USING:
         log_rule("FORCE");
         return(AV_FORCE);

      default:
         break;
      }

      // Issue #1005
      /* '::' at the start of an identifier is not member access, but global scope operator.
       * Detect if previous chunk is a type and previous-previous is "friend"
       */
      if (  first->type == CT_TYPE
         && first->prev != nullptr
         && first->prev->type == CT_FRIEND)
      {
         log_rule("FORCE");
         return(AV_FORCE);
      }

      if (  first->type == CT_WORD
         || first->type == CT_TYPE
         || first->type == CT_PAREN_CLOSE
         || CharTable::IsKw1(first->str[0]))
      {
         log_rule("sp_before_dc");
         return(cpd.settings[UO_sp_before_dc].a);
      }
   }

   // "a,b" vs "a, b"
   if (first->type == CT_COMMA)
   {
      if (first->parent_type == CT_TYPE)
      {
         // C# multidimensional array type: ',,' vs ', ,' or ',]' vs ', ]'
         if (second->type == CT_COMMA)
         {
            log_rule("sp_between_mdatype_commas");
            return(cpd.settings[UO_sp_between_mdatype_commas].a);
         }

         log_rule("sp_after_mdatype_commas");
         return(cpd.settings[UO_sp_after_mdatype_commas].a);
      }
      log_rule("sp_after_comma");
      return(cpd.settings[UO_sp_after_comma].a);
   }
   // test if we are within a SIGNAL/SLOT call
   if (QT_SIGNAL_SLOT_found)
   {
      if (  first->type == CT_FPAREN_CLOSE
         && (second->type == CT_FPAREN_CLOSE || second->type == CT_COMMA))
      {
         if (second->level == QT_SIGNAL_SLOT_level)
         {
            restoreValues = true;
         }
      }
   }
   if (second->type == CT_COMMA)
   {
      if (first->type == CT_SQUARE_OPEN && first->parent_type == CT_TYPE)
      {
         log_rule("sp_before_mdatype_commas");
         return(cpd.settings[UO_sp_before_mdatype_commas].a);
      }
      if (  first->type == CT_PAREN_OPEN
         && (cpd.settings[UO_sp_paren_comma].a != AV_IGNORE))
      {
         log_rule("sp_paren_comma");
         return(cpd.settings[UO_sp_paren_comma].a);
      }
      log_rule("sp_before_comma");
      return(cpd.settings[UO_sp_before_comma].a);
   }

   if (second->type == CT_ELLIPSIS)
   {
      // non-punc followed by a ellipsis
      if (  ((first->flags & PCF_PUNCTUATOR) == 0)
         && (cpd.settings[UO_sp_before_ellipsis].a != AV_IGNORE))
      {
         log_rule("sp_before_ellipsis");
         return(cpd.settings[UO_sp_before_ellipsis].a);
      }

      if (first->type == CT_TAG_COLON)
      {
         log_rule("FORCE");
         return(AV_FORCE);
      }
   }
   if (first->type == CT_ELLIPSIS && CharTable::IsKw1(second->str[0]))
   {
      log_rule("FORCE");
      return(AV_FORCE);
   }
   if (first->type == CT_TAG_COLON)
   {
      log_rule("sp_after_tag");
      return(cpd.settings[UO_sp_after_tag].a);
   }
   if (second->type == CT_TAG_COLON)
   {
      log_rule("REMOVE");
      return(AV_REMOVE);
   }

   // handle '~'
   if (first->type == CT_DESTRUCTOR)
   {
      log_rule("REMOVE");
      return(AV_REMOVE);
   }

   if (  first->type == CT_CATCH
      && second->type == CT_SPAREN_OPEN
      && (cpd.settings[UO_sp_catch_paren].a != AV_IGNORE))
   {
      log_rule("sp_catch_paren");
      return(cpd.settings[UO_sp_catch_paren].a);
   }

   if (  first->type == CT_D_VERSION_IF
      && second->type == CT_SPAREN_OPEN
      && (cpd.settings[UO_sp_version_paren].a != AV_IGNORE))
   {
      log_rule("sp_version_paren");
      return(cpd.settings[UO_sp_version_paren].a);
   }

   if (  first->type == CT_D_SCOPE_IF
      && second->type == CT_SPAREN_OPEN
      && (cpd.settings[UO_sp_scope_paren].a != AV_IGNORE))
   {
      log_rule("sp_scope_paren");
      return(cpd.settings[UO_sp_scope_paren].a);
   }

   // "if (" vs "if("
   if (second->type == CT_SPAREN_OPEN)
   {
      log_rule("sp_before_sparen");
      return(cpd.settings[UO_sp_before_sparen].a);
   }

   if (first->type == CT_LAMBDA || second->type == CT_LAMBDA)
   {
      log_rule("sp_assign (lambda)");
      return(cpd.settings[UO_sp_assign].a);
   }

   // Handle the special lambda case for C++11:
   //    [=](Something arg){.....}
   if (  (cpd.settings[UO_sp_cpp_lambda_assign].a != AV_IGNORE)
      && (  (  first->type == CT_SQUARE_OPEN
            && first->parent_type == CT_CPP_LAMBDA
            && second->type == CT_ASSIGN)
         || (  first->type == CT_ASSIGN
            && second->type == CT_SQUARE_CLOSE
            && second->parent_type == CT_CPP_LAMBDA)))
   {
      log_rule("UO_sp_cpp_lambda_assign");
      return(cpd.settings[UO_sp_cpp_lambda_assign].a);
   }

   // Handle the special lambda case for C++11:
   //    [](Something arg){.....}
   if (  (cpd.settings[UO_sp_cpp_lambda_paren].a != AV_IGNORE)
      && first->type == CT_SQUARE_CLOSE
      && first->parent_type == CT_CPP_LAMBDA
      && second->type == CT_FPAREN_OPEN)
   {
      log_rule("UO_sp_cpp_lambda_paren");
      return(cpd.settings[UO_sp_cpp_lambda_paren].a);
   }

   if (first->type == CT_ENUM && second->type == CT_FPAREN_OPEN)
   {
      if (cpd.settings[UO_sp_enum_paren].a != AV_IGNORE)
      {
         log_rule("sp_enum_paren");
         return(cpd.settings[UO_sp_enum_paren].a);
      }
   }

   if (second->type == CT_ASSIGN)
   {
      if (second->flags & PCF_IN_ENUM)
      {
         if (cpd.settings[UO_sp_enum_before_assign].a != AV_IGNORE)
         {
            log_rule("sp_enum_before_assign");
            return(cpd.settings[UO_sp_enum_before_assign].a);
         }
         log_rule("sp_enum_assign");
         return(cpd.settings[UO_sp_enum_assign].a);
      }
      if (  (cpd.settings[UO_sp_assign_default].a != AV_IGNORE)
         && second->parent_type == CT_FUNC_PROTO)
      {
         log_rule("sp_assign_default");
         return(cpd.settings[UO_sp_assign_default].a);
      }
      if (cpd.settings[UO_sp_before_assign].a != AV_IGNORE)
      {
         log_rule("sp_before_assign");
         return(cpd.settings[UO_sp_before_assign].a);
      }
      log_rule("sp_assign");
      return(cpd.settings[UO_sp_assign].a);
   }

   if (first->type == CT_ASSIGN)
   {
      if (first->flags & PCF_IN_ENUM)
      {
         if (cpd.settings[UO_sp_enum_after_assign].a != AV_IGNORE)
         {
            log_rule("sp_enum_after_assign");
            return(cpd.settings[UO_sp_enum_after_assign].a);
         }
         log_rule("sp_enum_assign");
         return(cpd.settings[UO_sp_enum_assign].a);
      }
      if (  (cpd.settings[UO_sp_assign_default].a != AV_IGNORE)
         && first->parent_type == CT_FUNC_PROTO)
      {
         log_rule("sp_assign_default");
         return(cpd.settings[UO_sp_assign_default].a);
      }
      if (cpd.settings[UO_sp_after_assign].a != AV_IGNORE)
      {
         log_rule("sp_after_assign");
         return(cpd.settings[UO_sp_after_assign].a);
      }
      log_rule("sp_assign");
      return(cpd.settings[UO_sp_assign].a);
   }

   if (first->type == CT_BIT_COLON)
   {
      if (first->flags & PCF_IN_ENUM)
      {
         log_rule("sp_enum_colon");
         return(cpd.settings[UO_sp_enum_colon].a);
      }
   }

   if (second->type == CT_BIT_COLON)
   {
      if (second->flags & PCF_IN_ENUM)
      {
         log_rule("sp_enum_colon");
         return(cpd.settings[UO_sp_enum_colon].a);
      }
   }

   if (second->type == CT_OC_BLOCK_CARET)
   {
      log_rule("sp_before_oc_block_caret");
      return(cpd.settings[UO_sp_before_oc_block_caret].a);
   }
   if (first->type == CT_OC_BLOCK_CARET)
   {
      log_rule("sp_after_oc_block_caret");
      return(cpd.settings[UO_sp_after_oc_block_caret].a);
   }
   if (second->type == CT_OC_MSG_FUNC)
   {
      log_rule("sp_after_oc_msg_receiver");
      return(cpd.settings[UO_sp_after_oc_msg_receiver].a);
   }

   // "a [x]" vs "a[x]"
   if (second->type == CT_SQUARE_OPEN && second->parent_type != CT_OC_MSG)
   {
      log_rule("sp_before_square");
      return(cpd.settings[UO_sp_before_square].a);
   }

   // "byte[]" vs "byte []"
   if (second->type == CT_TSQUARE)
   {
      log_rule("sp_before_squares");
      return(cpd.settings[UO_sp_before_squares].a);
   }

   if (  (cpd.settings[UO_sp_angle_shift].a != AV_IGNORE)
      && first->type == CT_ANGLE_CLOSE
      && second->type == CT_ANGLE_CLOSE)
   {
      log_rule("sp_angle_shift");
      return(cpd.settings[UO_sp_angle_shift].a);
   }

   // spacing around template < > stuff
   if (first->type == CT_ANGLE_OPEN || second->type == CT_ANGLE_CLOSE)
   {
      log_rule("sp_inside_angle");
      return(cpd.settings[UO_sp_inside_angle].a);
   }
   if (second->type == CT_ANGLE_OPEN)
   {
      if (  first->type == CT_TEMPLATE
         && (cpd.settings[UO_sp_template_angle].a != AV_IGNORE))
      {
         log_rule("sp_template_angle");
         return(cpd.settings[UO_sp_template_angle].a);
      }
      log_rule("sp_before_angle");
      return(cpd.settings[UO_sp_before_angle].a);
   }
   if (first->type == CT_ANGLE_CLOSE)
   {
      if (second->type == CT_WORD || CharTable::IsKw1(second->str[0]))
      {
         if (cpd.settings[UO_sp_angle_word].a != AV_IGNORE)
         {
            log_rule("sp_angle_word");
            return(cpd.settings[UO_sp_angle_word].a);
         }
      }
      if (second->type == CT_FPAREN_OPEN || second->type == CT_PAREN_OPEN)
      {
         chunk_t *next = chunk_get_next_ncnl(second);
         if (next && next->type == CT_FPAREN_CLOSE)
         {
            log_rule("sp_angle_paren_empty");
            return(cpd.settings[UO_sp_angle_paren_empty].a);
         }

         log_rule("sp_angle_paren");
         return(cpd.settings[UO_sp_angle_paren].a);
      }
      if (second->type == CT_DC_MEMBER)
      {
         log_rule("sp_before_dc");
         return(cpd.settings[UO_sp_before_dc].a);
      }
      if (second->type != CT_BYREF && second->type != CT_PTR_TYPE)
      {
         log_rule("sp_after_angle");
         return(cpd.settings[UO_sp_after_angle].a);
      }
   }

   if (  first->type == CT_BYREF
      && (cpd.settings[UO_sp_after_byref_func].a != AV_IGNORE)
      && (  first->parent_type == CT_FUNC_DEF
         || first->parent_type == CT_FUNC_PROTO))
   {
      log_rule("sp_after_byref_func");
      return(cpd.settings[UO_sp_after_byref_func].a);
   }

   if (first->type == CT_BYREF && CharTable::IsKw1(second->str[0]))
   {
      log_rule("sp_after_byref");
      return(cpd.settings[UO_sp_after_byref].a);
   }

   if (second->type == CT_BYREF)
   {
      if (cpd.settings[UO_sp_before_byref_func].a != AV_IGNORE)
      {
         chunk_t *next = chunk_get_next(second);
         if (  next != nullptr
            && (next->type == CT_FUNC_DEF || next->type == CT_FUNC_PROTO))
         {
            return(cpd.settings[UO_sp_before_byref_func].a);
         }
      }

      if (cpd.settings[UO_sp_before_unnamed_byref].a != AV_IGNORE)
      {
         chunk_t *next = chunk_get_next_nc(second);
         if (next != nullptr && next->type != CT_WORD)
         {
            log_rule("sp_before_unnamed_byref");
            return(cpd.settings[UO_sp_before_unnamed_byref].a);
         }
      }
      log_rule("sp_before_byref");
      return(cpd.settings[UO_sp_before_byref].a);
   }

   if (first->type == CT_SPAREN_CLOSE)
   {
      if (  second->type == CT_BRACE_OPEN
         && (cpd.settings[UO_sp_sparen_brace].a != AV_IGNORE))
      {
         log_rule("sp_sparen_brace");
         return(cpd.settings[UO_sp_sparen_brace].a);
      }
      if (  !chunk_is_comment(second)
         && (cpd.settings[UO_sp_after_sparen].a != AV_IGNORE))
      {
         log_rule("sp_after_sparen");
         return(cpd.settings[UO_sp_after_sparen].a);
      }
   }

   if (  second->type == CT_FPAREN_OPEN
      && first->parent_type == CT_OPERATOR
      && (cpd.settings[UO_sp_after_operator_sym].a != AV_IGNORE))
   {
      if (  (cpd.settings[UO_sp_after_operator_sym_empty].a != AV_IGNORE)
         && second->type == CT_FPAREN_OPEN)
      {
         chunk_t *next = chunk_get_next_ncnl(second);
         if (next && next->type == CT_FPAREN_CLOSE)
         {
            log_rule("sp_after_operator_sym_empty");
            return(cpd.settings[UO_sp_after_operator_sym_empty].a);
         }
      }

      log_rule("sp_after_operator_sym");
      return(cpd.settings[UO_sp_after_operator_sym].a);
   }

   // spaces between function and open paren
   if (first->type == CT_FUNC_CALL || first->type == CT_FUNC_CTOR_VAR)
   {
      if (  (cpd.settings[UO_sp_func_call_paren_empty].a != AV_IGNORE)
         && second->type == CT_FPAREN_OPEN)
      {
         chunk_t *next = chunk_get_next_ncnl(second);
         if (next && next->type == CT_FPAREN_CLOSE)
         {
            log_rule("sp_func_call_paren_empty");
            return(cpd.settings[UO_sp_func_call_paren_empty].a);
         }
      }
      log_rule("sp_func_call_paren");
      return(cpd.settings[UO_sp_func_call_paren].a);
   }
   if (first->type == CT_FUNC_CALL_USER)
   {
      log_rule("sp_func_call_user_paren");
      return(cpd.settings[UO_sp_func_call_user_paren].a);
   }
   if (first->type == CT_ATTRIBUTE)
   {
      log_rule("sp_attribute_paren");
      return(cpd.settings[UO_sp_attribute_paren].a);
   }
   if (first->type == CT_FUNC_DEF)
   {
      if (  (cpd.settings[UO_sp_func_def_paren_empty].a != AV_IGNORE)
         && second->type == CT_FPAREN_OPEN)
      {
         chunk_t *next = chunk_get_next_ncnl(second);
         if (next && next->type == CT_FPAREN_CLOSE)
         {
            log_rule("sp_func_def_paren_empty");
            return(cpd.settings[UO_sp_func_def_paren_empty].a);
         }
      }
      log_rule("sp_func_def_paren");
      return(cpd.settings[UO_sp_func_def_paren].a);
   }
   if (first->type == CT_CPP_CAST || first->type == CT_TYPE_WRAP)
   {
      log_rule("sp_cpp_cast_paren");
      return(cpd.settings[UO_sp_cpp_cast_paren].a);
   }

   if (first->type == CT_PAREN_CLOSE && second->type == CT_WHEN)
   {
      log_rule("FORCE");
      return(AV_FORCE); // TODO: make this configurable?
   }

   if (  first->type == CT_PAREN_CLOSE
      && (second->type == CT_PAREN_OPEN || second->type == CT_FPAREN_OPEN))
   {
      // "(int)a" vs "(int) a" or "cast(int)a" vs "cast(int) a"
      if (first->parent_type == CT_C_CAST || first->parent_type == CT_D_CAST)
      {
         log_rule("sp_after_cast");
         return(cpd.settings[UO_sp_after_cast].a);
      }

      // Must be an indirect/chained function call?
      log_rule("REMOVE");
      return(AV_REMOVE);  // TODO: make this configurable?
   }

   // handle the space between parens in fcn type 'void (*f)(void)'
   if (first->type == CT_TPAREN_CLOSE)
   {
      log_rule("sp_after_tparen_close");
      return(cpd.settings[UO_sp_after_tparen_close].a);
   }

   // ")(" vs ") ("
   if (  (  chunk_is_str(first, ")", 1)
         && chunk_is_str(second, "(", 1))
      || (chunk_is_paren_close(first) && chunk_is_paren_open(second)))
   {
      log_rule("sp_cparen_oparen");
      return(cpd.settings[UO_sp_cparen_oparen].a);
   }

   if (  first->type == CT_FUNC_PROTO
      || (  second->type == CT_FPAREN_OPEN
         && second->parent_type == CT_FUNC_PROTO))
   {
      if (  (cpd.settings[UO_sp_func_proto_paren_empty].a != AV_IGNORE)
         && second->type == CT_FPAREN_OPEN)
      {
         chunk_t *next = chunk_get_next_ncnl(second);
         if (next && next->type == CT_FPAREN_CLOSE)
         {
            log_rule("sp_func_proto_paren_empty");
            return(cpd.settings[UO_sp_func_proto_paren_empty].a);
         }
      }
      log_rule("sp_func_proto_paren");
      return(cpd.settings[UO_sp_func_proto_paren].a);
   }
   if (first->type == CT_FUNC_CLASS_DEF || first->type == CT_FUNC_CLASS_PROTO)
   {
      if (  (cpd.settings[UO_sp_func_class_paren_empty].a != AV_IGNORE)
         && second->type == CT_FPAREN_OPEN)
      {
         chunk_t *next = chunk_get_next_ncnl(second);
         if (next && next->type == CT_FPAREN_CLOSE)
         {
            log_rule("sp_func_class_paren_empty");
            return(cpd.settings[UO_sp_func_class_paren_empty].a);
         }
      }
      log_rule("sp_func_class_paren");
      return(cpd.settings[UO_sp_func_class_paren].a);
   }
   if (first->type == CT_CLASS && !(first->flags & PCF_IN_OC_MSG))
   {
      log_rule("FORCE");
      return(AV_FORCE);
   }

   if (first->type == CT_BRACE_OPEN && second->type == CT_BRACE_CLOSE)
   {
      log_rule("sp_inside_braces_empty");
      return(cpd.settings[UO_sp_inside_braces_empty].a);
   }

   if (second->type == CT_BRACE_OPEN && second->parent_type == CT_TYPE)
   {
      // 'int{9}' vs 'int {9}'
      return(cpd.settings[UO_sp_type_brace_init_lst].a);
   }

   if (second->type == CT_BRACE_CLOSE)
   {
      if (second->parent_type == CT_ENUM)
      {
         log_rule("sp_inside_braces_enum");
         return(cpd.settings[UO_sp_inside_braces_enum].a);
      }
      if (second->parent_type == CT_STRUCT || second->parent_type == CT_UNION)
      {
         log_rule("sp_inside_braces_struct");
         return(cpd.settings[UO_sp_inside_braces_struct].a);
      }
      if (second->parent_type == CT_TYPE)
      {
         if (cpd.settings[UO_sp_before_type_brace_init_lst_close].a != AV_IGNORE)
         {
            log_rule("sp_before_init_braces_close");
            return(cpd.settings[UO_sp_before_type_brace_init_lst_close].a);
         }
         if (cpd.settings[UO_sp_inside_type_brace_init_lst].a != AV_IGNORE)
         {
            log_rule("sp_inside_init_braces");
            return(cpd.settings[UO_sp_inside_type_brace_init_lst].a);
         }
      }

      log_rule("sp_inside_braces");
      return(cpd.settings[UO_sp_inside_braces].a);
   }

   if (first->type == CT_D_CAST)
   {
      log_rule("REMOVE");
      return(AV_REMOVE);
   }

   if (first->type == CT_PP_DEFINED && second->type == CT_PAREN_OPEN)
   {
      log_rule("sp_defined_paren");
      return(cpd.settings[UO_sp_defined_paren].a);
   }

   if (first->type == CT_THROW)
   {
      if (second->type == CT_PAREN_OPEN)
      {
         log_rule("sp_throw_paren");
         return(cpd.settings[UO_sp_throw_paren].a);
      }
      log_rule("sp_after_throw");
      return(cpd.settings[UO_sp_after_throw].a);
   }

   if (first->type == CT_THIS && second->type == CT_PAREN_OPEN)
   {
      log_rule("sp_this_paren");
      return(cpd.settings[UO_sp_this_paren].a);
   }

   if (first->type == CT_STATE && second->type == CT_PAREN_OPEN)
   {
      log_rule("ADD");
      return(AV_ADD);
   }

   if (first->type == CT_DELEGATE && second->type == CT_PAREN_OPEN)
   {
      log_rule("REMOVE");
      return(AV_REMOVE);
   }

   if (first->type == CT_MEMBER || second->type == CT_MEMBER)
   {
      log_rule("sp_member");
      return(cpd.settings[UO_sp_member].a);
   }

   if (first->type == CT_C99_MEMBER)
   {
      // always remove space(s) after then '.' of a C99-member
      log_rule("REMOVE");
      return(AV_REMOVE);
   }

   if (first->type == CT_SUPER && second->type == CT_PAREN_OPEN)
   {
      log_rule("sp_super_paren");
      return(cpd.settings[UO_sp_super_paren].a);
   }

   if (first->type == CT_FPAREN_CLOSE && second->type == CT_BRACE_OPEN)
   {
      if (second->parent_type == CT_DOUBLE_BRACE)
      {
         log_rule("sp_fparen_dbrace");
         return(cpd.settings[UO_sp_fparen_dbrace].a);
      }
      log_rule("sp_fparen_brace");
      return(cpd.settings[UO_sp_fparen_brace].a);
   }

   if (first->type == CT_D_TEMPLATE || second->type == CT_D_TEMPLATE)
   {
      log_rule("REMOVE");
      return(AV_REMOVE);
   }

   if (first->type == CT_ELSE && second->type == CT_BRACE_OPEN)
   {
      log_rule("sp_else_brace");
      return(cpd.settings[UO_sp_else_brace].a);
   }

   if (first->type == CT_ELSE && second->type == CT_ELSEIF)
   {
      log_rule("FORCE");
      return(AV_FORCE);
   }

   if (first->type == CT_CATCH && second->type == CT_BRACE_OPEN)
   {
      log_rule("sp_catch_brace");
      return(cpd.settings[UO_sp_catch_brace].a);
   }

   if (first->type == CT_FINALLY && second->type == CT_BRACE_OPEN)
   {
      log_rule("sp_finally_brace");
      return(cpd.settings[UO_sp_finally_brace].a);
   }

   if (first->type == CT_TRY && second->type == CT_BRACE_OPEN)
   {
      log_rule("sp_try_brace");
      return(cpd.settings[UO_sp_try_brace].a);
   }

   if (first->type == CT_GETSET && second->type == CT_BRACE_OPEN)
   {
      log_rule("sp_getset_brace");
      return(cpd.settings[UO_sp_getset_brace].a);
   }

   //if ((first->type == CT_WORD || first->type == CT_WORD) &&   Coverity CID 76001 Same on both sides, 2016-03-16
   //    second->type == CT_BRACE_OPEN)
   if (first->type == CT_WORD && second->type == CT_BRACE_OPEN)
   {
      if (first->parent_type == CT_NAMESPACE)
      {
         log_rule("sp_word_brace_ns");
         return(cpd.settings[UO_sp_word_brace_ns].a);
      }
      if (first->parent_type == CT_NONE && second->parent_type == CT_NONE)
      {
         log_rule("sp_word_brace");
         return(cpd.settings[UO_sp_word_brace].a);
      }
   }

   if (second->type == CT_PAREN_OPEN && second->parent_type == CT_INVARIANT)
   {
      log_rule("sp_invariant_paren");
      return(cpd.settings[UO_sp_invariant_paren].a);
   }

   if (first->type == CT_PAREN_CLOSE)
   {
      if (first->parent_type == CT_D_TEMPLATE)
      {
         log_rule("FORCE");
         return(AV_FORCE);
      }

      if (first->parent_type == CT_INVARIANT)
      {
         log_rule("sp_after_invariant_paren");
         return(cpd.settings[UO_sp_after_invariant_paren].a);
      }

      // Arith after a cast comes first
      if (second->type == CT_ARITH || second->type == CT_CARET)
      {
         log_rule("sp_arith");
         return(cpd.settings[UO_sp_arith].a);
      }

      // "(struct foo) {...}" vs "(struct foo){...}"
      if (second->type == CT_BRACE_OPEN)
      {
         log_rule("sp_paren_brace");
         return(cpd.settings[UO_sp_paren_brace].a);
      }

      // D-specific: "delegate(some thing) dg
      if (first->parent_type == CT_DELEGATE)
      {
         log_rule("ADD");
         return(AV_ADD);
      }

      // PAWN-specific: "state (condition) next"
      if (first->parent_type == CT_STATE)
      {
         log_rule("ADD");
         return(AV_ADD);
      }

      /* C++ new operator: new(bar) Foo */
      if (first->parent_type == CT_NEW)
      {
         log_rule("sp_after_newop_paren");
         return(cpd.settings[UO_sp_after_newop_paren].a);
      }
   }

   // "foo(...)" vs "foo( ... )"
   if (first->type == CT_FPAREN_OPEN || second->type == CT_FPAREN_CLOSE)
   {
      if (first->type == CT_FPAREN_OPEN && second->type == CT_FPAREN_CLOSE)
      {
         log_rule("sp_inside_fparens");
         return(cpd.settings[UO_sp_inside_fparens].a);
      }
      log_rule("sp_inside_fparen");
      return(cpd.settings[UO_sp_inside_fparen].a);
   }

   // "foo(...)" vs "foo( ... )"
   if (first->type == CT_TPAREN_OPEN || second->type == CT_TPAREN_CLOSE)
   {
      log_rule("sp_inside_tparen");
      return(cpd.settings[UO_sp_inside_tparen].a);
   }

   if (first->type == CT_PAREN_CLOSE)
   {
      if (  (first->flags & PCF_OC_RTYPE)  // == CT_OC_RTYPE)
         && (  first->parent_type == CT_OC_MSG_DECL
            || first->parent_type == CT_OC_MSG_SPEC))
      {
         log_rule("sp_after_oc_return_type");
         return(cpd.settings[UO_sp_after_oc_return_type].a);
      }

      if (first->parent_type == CT_OC_MSG_SPEC || first->parent_type == CT_OC_MSG_DECL)
      {
         log_rule("sp_after_oc_type");
         return(cpd.settings[UO_sp_after_oc_type].a);
      }

      if (first->parent_type == CT_OC_SEL && second->type != CT_SQUARE_CLOSE)
      {
         log_rule("sp_after_oc_at_sel_parens");
         return(cpd.settings[UO_sp_after_oc_at_sel_parens].a);
      }
   }

   if (cpd.settings[UO_sp_inside_oc_at_sel_parens].a != AV_IGNORE)
   {
      if (  (  first->type == CT_PAREN_OPEN
            && (  first->parent_type == CT_OC_SEL
               || first->parent_type == CT_OC_PROTOCOL))
         || (  second->type == CT_PAREN_CLOSE
            && (  second->parent_type == CT_OC_SEL
               || second->parent_type == CT_OC_PROTOCOL)))
      {
         log_rule("sp_inside_oc_at_sel_parens");
         return(cpd.settings[UO_sp_inside_oc_at_sel_parens].a);
      }
   }

   if (  second->type == CT_PAREN_OPEN
      && (first->type == CT_OC_SEL || first->type == CT_OC_PROTOCOL))
   {
      log_rule("sp_after_oc_at_sel");
      return(cpd.settings[UO_sp_after_oc_at_sel].a);
   }

   /*
    * C cast:   "(int)"      vs "( int )"
    * D cast:   "cast(int)"  vs "cast( int )"
    * CPP cast: "int(a + 3)" vs "int( a + 3 )"
    */
   if (first->type == CT_PAREN_OPEN)
   {
      if (  first->parent_type == CT_C_CAST
         || first->parent_type == CT_CPP_CAST
         || first->parent_type == CT_D_CAST)
      {
         log_rule("sp_inside_paren_cast");
         return(cpd.settings[UO_sp_inside_paren_cast].a);
      }
      if (first->parent_type == CT_NEW)
      {
         if (cpd.settings[UO_sp_inside_newop_paren_open].a != AV_IGNORE)
         {
            log_rule("sp_inside_newop_paren_open");
            return(cpd.settings[UO_sp_inside_newop_paren_open].a);
         }
         if (cpd.settings[UO_sp_inside_newop_paren].a != AV_IGNORE)
         {
            log_rule("sp_inside_newop_paren");
            return(cpd.settings[UO_sp_inside_newop_paren].a);
         }
      }
      log_rule("sp_inside_paren");
      return(cpd.settings[UO_sp_inside_paren].a);
   }

   if (second->type == CT_PAREN_CLOSE)
   {
      if (  second->parent_type == CT_C_CAST
         || second->parent_type == CT_CPP_CAST
         || second->parent_type == CT_D_CAST)
      {
         log_rule("sp_inside_paren_cast");
         return(cpd.settings[UO_sp_inside_paren_cast].a);
      }
      if (second->parent_type == CT_NEW)
      {
         if (cpd.settings[UO_sp_inside_newop_paren_close].a != AV_IGNORE)
         {
            log_rule("sp_inside_newop_paren_close");
            return(cpd.settings[UO_sp_inside_newop_paren_close].a);
         }
         if (cpd.settings[UO_sp_inside_newop_paren].a != AV_IGNORE)
         {
            log_rule("sp_inside_newop_paren");
            return(cpd.settings[UO_sp_inside_newop_paren].a);
         }
      }
      log_rule("sp_inside_paren");
      return(cpd.settings[UO_sp_inside_paren].a);
   }

   // "[3]" vs "[ 3 ]"
   if (first->type == CT_SQUARE_OPEN || second->type == CT_SQUARE_CLOSE)
   {
      log_rule("sp_inside_square");
      return(cpd.settings[UO_sp_inside_square].a);
   }
   if (first->type == CT_SQUARE_CLOSE && second->type == CT_FPAREN_OPEN)
   {
      log_rule("sp_square_fparen");
      return(cpd.settings[UO_sp_square_fparen].a);
   }

   // "if(...)" vs "if( ... )"
   if (  second->type == CT_SPAREN_CLOSE
      && (cpd.settings[UO_sp_inside_sparen_close].a != AV_IGNORE))
   {
      log_rule("sp_inside_sparen_close");
      return(cpd.settings[UO_sp_inside_sparen_close].a);
   }
   if (  first->type == CT_SPAREN_OPEN
      && (cpd.settings[UO_sp_inside_sparen_open].a != AV_IGNORE))
   {
      log_rule("sp_inside_sparen_open");
      return(cpd.settings[UO_sp_inside_sparen_open].a);
   }
   if (first->type == CT_SPAREN_OPEN || second->type == CT_SPAREN_CLOSE)
   {
      log_rule("sp_inside_sparen");
      return(cpd.settings[UO_sp_inside_sparen].a);
   }

   if (  (cpd.settings[UO_sp_after_class_colon].a != AV_IGNORE)
      && first->type == CT_CLASS_COLON)
   {
      log_rule("sp_after_class_colon");
      return(cpd.settings[UO_sp_after_class_colon].a);
   }
   if (  (cpd.settings[UO_sp_before_class_colon].a != AV_IGNORE)
      && second->type == CT_CLASS_COLON)
   {
      log_rule("sp_before_class_colon");
      return(cpd.settings[UO_sp_before_class_colon].a);
   }

   if (  (cpd.settings[UO_sp_after_constr_colon].a != AV_IGNORE)
      && first->type == CT_CONSTR_COLON)
   {
      min_sp = cpd.settings[UO_indent_ctor_init_leading].u - 1; // default indent is 1 space

      log_rule("sp_after_constr_colon");
      return(cpd.settings[UO_sp_after_constr_colon].a);
   }
   if (  (cpd.settings[UO_sp_before_constr_colon].a != AV_IGNORE)
      && second->type == CT_CONSTR_COLON)
   {
      log_rule("sp_before_constr_colon");
      return(cpd.settings[UO_sp_before_constr_colon].a);
   }

   if (  (cpd.settings[UO_sp_before_case_colon].a != AV_IGNORE)
      && second->type == CT_CASE_COLON)
   {
      log_rule("sp_before_case_colon");
      return(cpd.settings[UO_sp_before_case_colon].a);
   }

   if (first->type == CT_DOT)
   {
      log_rule("REMOVE");
      return(AV_REMOVE);
   }
   if (second->type == CT_DOT)
   {
      log_rule("ADD");
      return(AV_ADD);
   }

   if (first->type == CT_NULLCOND || second->type == CT_NULLCOND)
   {
      log_rule("sp_member");
      return(cpd.settings[UO_sp_member].a);
   }

   if (  first->type == CT_ARITH
      || first->type == CT_CARET
      || second->type == CT_ARITH
      || second->type == CT_CARET)
   {
      log_rule("sp_arith");
      return(cpd.settings[UO_sp_arith].a);
   }
   if (first->type == CT_BOOL || second->type == CT_BOOL)
   {
      argval_t arg = cpd.settings[UO_sp_bool].a;
      if (  (cpd.settings[UO_pos_bool].tp != TP_IGNORE)
         && first->orig_line != second->orig_line
         && arg != AV_REMOVE)
      {
         arg = static_cast<argval_t>(arg | AV_ADD);
      }
      log_rule("sp_bool");
      return(arg);
   }
   if (first->type == CT_COMPARE || second->type == CT_COMPARE)
   {
      log_rule("sp_compare");
      return(cpd.settings[UO_sp_compare].a);
   }

   if (first->type == CT_PAREN_OPEN && second->type == CT_PTR_TYPE)
   {
      log_rule("REMOVE");
      return(AV_REMOVE);
   }

   if (  first->type == CT_PTR_TYPE
      && (cpd.settings[UO_sp_ptr_star_paren].a != AV_IGNORE)
      && (second->type == CT_FPAREN_OPEN || second->type == CT_TPAREN_OPEN))
   {
      log_rule("sp_ptr_star_paren");
      return(cpd.settings[UO_sp_ptr_star_paren].a);
   }

   if (  first->type == CT_PTR_TYPE
      && second->type == CT_PTR_TYPE
      && (cpd.settings[UO_sp_between_ptr_star].a != AV_IGNORE))
   {
      log_rule("sp_between_ptr_star");
      return(cpd.settings[UO_sp_between_ptr_star].a);
   }

   if (  first->type == CT_PTR_TYPE
      && (cpd.settings[UO_sp_after_ptr_star_func].a != AV_IGNORE)
      && (  first->parent_type == CT_FUNC_DEF
         || first->parent_type == CT_FUNC_PROTO
         || first->parent_type == CT_FUNC_VAR))
   {
      log_rule("sp_after_ptr_star_func");
      return(cpd.settings[UO_sp_after_ptr_star_func].a);
   }

   if (first->type == CT_PTR_TYPE && CharTable::IsKw1(second->str[0]))
   {
      chunk_t *prev = chunk_get_prev(first);
      if (prev != nullptr && prev->type == CT_IN)
      {
         log_rule("sp_deref");
         return(cpd.settings[UO_sp_deref].a);
      }

      if (  second->type == CT_QUALIFIER
         && (cpd.settings[UO_sp_after_ptr_star_qualifier].a != AV_IGNORE))
      {
         log_rule("sp_after_ptr_star_qualifier");
         return(cpd.settings[UO_sp_after_ptr_star_qualifier].a);
      }

      if (cpd.settings[UO_sp_after_ptr_star].a != AV_IGNORE)
      {
         log_rule("sp_after_ptr_star");
         return(cpd.settings[UO_sp_after_ptr_star].a);
      }
   }

   if (second->type == CT_PTR_TYPE && first->type != CT_IN)
   {
      if (cpd.settings[UO_sp_before_ptr_star_func].a != AV_IGNORE)
      {
         // Find the next non-'*' chunk
         chunk_t *next = second;
         do
         {
            next = chunk_get_next(next);
         } while (next != nullptr && next->type == CT_PTR_TYPE);

         if (  next != nullptr
            && (next->type == CT_FUNC_DEF || next->type == CT_FUNC_PROTO))
         {
            return(cpd.settings[UO_sp_before_ptr_star_func].a);
         }
      }

      if (cpd.settings[UO_sp_before_unnamed_ptr_star].a != AV_IGNORE)
      {
         chunk_t *next = chunk_get_next_nc(second);
         while (next != nullptr && next->type == CT_PTR_TYPE)
         {
            next = chunk_get_next_nc(next);
         }
         if (next != nullptr && next->type != CT_WORD)
         {
            log_rule("sp_before_unnamed_ptr_star");
            return(cpd.settings[UO_sp_before_unnamed_ptr_star].a);
         }
      }
      if (cpd.settings[UO_sp_before_ptr_star].a != AV_IGNORE)
      {
         log_rule("sp_before_ptr_star");
         return(cpd.settings[UO_sp_before_ptr_star].a);
      }
   }

   if (first->type == CT_OPERATOR)
   {
      log_rule("sp_after_operator");
      return(cpd.settings[UO_sp_after_operator].a);
   }

   if (second->type == CT_FUNC_PROTO || second->type == CT_FUNC_DEF)
   {
      if (first->type != CT_PTR_TYPE)
      {
         log_rule("sp_type_func|ADD");
         return(static_cast<argval_t>(cpd.settings[UO_sp_type_func].a | AV_ADD));
      }
      log_rule("sp_type_func");
      return(cpd.settings[UO_sp_type_func].a);
   }

   // "(int)a" vs "(int) a" or "cast(int)a" vs "cast(int) a"
   if (first->parent_type == CT_C_CAST || first->parent_type == CT_D_CAST)
   {
      log_rule("sp_after_cast");
      return(cpd.settings[UO_sp_after_cast].a);
   }

   if (first->type == CT_BRACE_CLOSE)
   {
      if (second->type == CT_ELSE)
      {
         log_rule("sp_brace_else");
         return(cpd.settings[UO_sp_brace_else].a);
      }

      if (second->type == CT_CATCH)
      {
         log_rule("sp_brace_catch");
         return(cpd.settings[UO_sp_brace_catch].a);
      }

      if (second->type == CT_FINALLY)
      {
         log_rule("sp_brace_finally");
         return(cpd.settings[UO_sp_brace_finally].a);
      }
   }

   if (first->type == CT_BRACE_OPEN)
   {
      if (first->parent_type == CT_ENUM)
      {
         log_rule("sp_inside_braces_enum");
         return(cpd.settings[UO_sp_inside_braces_enum].a);
      }

      if (first->parent_type == CT_UNION || first->parent_type == CT_STRUCT)
      {
         log_rule("sp_inside_braces_struct");
         return(cpd.settings[UO_sp_inside_braces_struct].a);
      }

      if (first->parent_type == CT_TYPE)
      {
         if (cpd.settings[UO_sp_after_type_brace_init_lst_open].a != AV_IGNORE)
         {
            log_rule("sp_after_init_braces_open");
            return(cpd.settings[UO_sp_after_type_brace_init_lst_open].a);
         }

         log_rule("sp_inside_braces_struct");
         return(cpd.settings[UO_sp_inside_type_brace_init_lst].a);
      }

      if (!chunk_is_comment(second))
      {
         log_rule("sp_inside_braces");
         return(cpd.settings[UO_sp_inside_braces].a);
      }
   }


   if (  first->type == CT_BRACE_CLOSE
      && (first->flags & PCF_IN_TYPEDEF)
      && (  first->parent_type == CT_ENUM
         || first->parent_type == CT_STRUCT
         || first->parent_type == CT_UNION))
   {
      log_rule("sp_brace_typedef");
      return(cpd.settings[UO_sp_brace_typedef].a);
   }

   if (second->type == CT_SPAREN_OPEN)
   {
      log_rule("sp_before_sparen");
      return(cpd.settings[UO_sp_before_sparen].a);
   }

   if (second->type == CT_PAREN_OPEN && second->parent_type == CT_TEMPLATE)
   {
      log_rule("UO_sp_before_template_paren");
      return(cpd.settings[UO_sp_before_template_paren].a);
   }

   if (  second->type != CT_PTR_TYPE
      && (first->type == CT_QUALIFIER || first->type == CT_TYPE))
   {
      argval_t arg = cpd.settings[UO_sp_after_type].a;
      log_rule("sp_after_type");
      return((arg != AV_REMOVE) ? arg : AV_FORCE);
   }

   if (  first->type == CT_MACRO_OPEN
      || first->type == CT_MACRO_CLOSE
      || first->type == CT_MACRO_ELSE)
   {
      if (second->type == CT_PAREN_OPEN)
      {
         log_rule("sp_func_call_paren");
         return(cpd.settings[UO_sp_func_call_paren].a);
      }
      log_rule("IGNORE");
      return(AV_IGNORE);
   }

   // If nothing claimed the PTR_TYPE, then return ignore
   if (first->type == CT_PTR_TYPE || second->type == CT_PTR_TYPE)
   {
      log_rule("IGNORE");
      return(AV_IGNORE);
   }

   if (first->type == CT_NOT)
   {
      log_rule("sp_not");
      return(cpd.settings[UO_sp_not].a);
   }
   if (first->type == CT_INV)
   {
      log_rule("sp_inv");
      return(cpd.settings[UO_sp_inv].a);
   }
   if (first->type == CT_ADDR)
   {
      log_rule("sp_addr");
      return(cpd.settings[UO_sp_addr].a);
   }
   if (first->type == CT_DEREF)
   {
      log_rule("sp_deref");
      return(cpd.settings[UO_sp_deref].a);
   }
   if (first->type == CT_POS || first->type == CT_NEG)
   {
      log_rule("sp_sign");
      return(cpd.settings[UO_sp_sign].a);
   }
   if (first->type == CT_INCDEC_BEFORE || second->type == CT_INCDEC_AFTER)
   {
      log_rule("sp_incdec");
      return(cpd.settings[UO_sp_incdec].a);
   }
   if (second->type == CT_CS_SQ_COLON)
   {
      log_rule("REMOVE");
      return(AV_REMOVE);
   }
   if (first->type == CT_CS_SQ_COLON)
   {
      log_rule("FORCE");
      return(AV_FORCE);
   }
   if (first->type == CT_OC_SCOPE)
   {
      log_rule("sp_after_oc_scope");
      return(cpd.settings[UO_sp_after_oc_scope].a);
   }
   if (first->type == CT_OC_DICT_COLON)
   {
      log_rule("sp_after_oc_dict_colon");
      return(cpd.settings[UO_sp_after_oc_dict_colon].a);
   }
   if (second->type == CT_OC_DICT_COLON)
   {
      log_rule("sp_before_oc_dict_colon");
      return(cpd.settings[UO_sp_before_oc_dict_colon].a);
   }
   if (first->type == CT_OC_COLON)
   {
      if (first->flags & PCF_IN_OC_MSG)
      {
         log_rule("sp_after_send_oc_colon");
         return(cpd.settings[UO_sp_after_send_oc_colon].a);
      }

      log_rule("sp_after_oc_colon");
      return(cpd.settings[UO_sp_after_oc_colon].a);
   }
   if (second->type == CT_OC_COLON)
   {
      if (  (first->flags & PCF_IN_OC_MSG)
         && (first->type == CT_OC_MSG_FUNC || first->type == CT_OC_MSG_NAME))
      {
         log_rule("sp_before_send_oc_colon");
         return(cpd.settings[UO_sp_before_send_oc_colon].a);
      }

      log_rule("sp_before_oc_colon");
      return(cpd.settings[UO_sp_before_oc_colon].a);
   }

   if (second->type == CT_COMMENT && second->parent_type == CT_COMMENT_EMBED)
   {
      log_rule("FORCE");
      return(AV_FORCE);
   }

   if (chunk_is_comment(second))
   {
      log_rule("IGNORE");
      return(AV_IGNORE);
   }

   if (first->type == CT_COMMENT)
   {
      log_rule("FORCE");
      return(AV_FORCE);
   }

   if (first->type == CT_NEW && second->type == CT_PAREN_OPEN)
   {
      // c# new Constraint, c++ new operator
      log_rule("sp_between_new_paren");
      return(cpd.settings[UO_sp_between_new_paren].a);
   }
   if (  first->type == CT_NEW
      || first->type == CT_DELETE
      || (first->type == CT_TSQUARE && first->parent_type == CT_DELETE))
   {
      log_rule("sp_after_new");
      return(cpd.settings[UO_sp_after_new].a);
   }

   if (first->type == CT_ANNOTATION && chunk_is_paren_open(second))
   {
      log_rule("sp_annotation_paren");
      return(cpd.settings[UO_sp_annotation_paren].a);
   }

   if (first->type == CT_OC_PROPERTY)
   {
      log_rule("sp_after_oc_property");
      return(cpd.settings[UO_sp_after_oc_property].a);
   }

   if (first->type == CT_EXTERN && second->type == CT_PAREN_OPEN)
   {
      log_rule("sp_extern_paren");
      return(cpd.settings[UO_sp_extern_paren].a);
   }

   /* "((" vs "( (" or "))" vs ") )" */
   if (  (chunk_is_str(first, "(", 1) && chunk_is_str(second, "(", 1))
      || (chunk_is_str(first, ")", 1) && chunk_is_str(second, ")", 1)))
   {
      log_rule("sp_paren_paren");
      return(cpd.settings[UO_sp_paren_paren].a);
   }

   // this table lists out all combos where a space should NOT be present
   // CT_UNKNOWN is a wildcard.
   for (auto it : no_space_table)
   {
      if (  (it.first == CT_UNKNOWN || it.first == first->type)
         && (it.second == CT_UNKNOWN || it.second == second->type))
      {
         log_rule("REMOVE from no_space_table");
         return(AV_REMOVE);
      }
   }

   // Issue #889
   // mapped_file_source abc((int) A::CW2A(sTemp));
   if (  first->type == CT_PAREN_CLOSE
      && second->type == CT_TYPE
      && second->next != nullptr
      && second->next->type == CT_DC_MEMBER
      && second->next->next != nullptr
      && second->next->next->type == CT_FUNC_CALL)
   {
      log_rule("REMOVE_889_B");
      return(AV_REMOVE);
   }

#ifdef DEBUG
   // these lines are only useful for debugging uncrustify itself
   LOG_FMT(LSPACE, "\n\n%s(%d): WARNING: unrecognize do_space: first: %zu:%zu %s %s and second: %zu:%zu %s %s\n\n\n",
           __func__, __LINE__, first->orig_line, first->orig_col, first->text(), get_token_name(first->type),
           second->orig_line, second->orig_col, second->text(), get_token_name(second->type));
#endif
   log_rule("ADD as default value");
   return(AV_ADD);
} // do_space


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
      LOG_FMT(LSPACE, "%s(%d): orig_col is %zu, orig_col is %zu, %s type is %s\n",
              __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
      if (  (cpd.settings[UO_use_options_overriding_for_qt_macros].b)
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
      if (cpd.settings[UO_sp_skip_vbrace_tokens].b)
      {
         next = chunk_get_next(pc);
         while (  chunk_is_blank(next)
               && !chunk_is_newline(next)
               && (next->type == CT_VBRACE_OPEN || next->type == CT_VBRACE_CLOSE))
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
      if ((QT_SIGNAL_SLOT_found) && (cpd.settings[UO_sp_balance_nested_parens].b))
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
      if (  pc->type == CT_NEWLINE
         || pc->type == CT_NL_CONT
         || pc->type == CT_COMMENT_MULTI)
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
                     if (  (  (  (cpd.lang_flags & LANG_CPP)
                              && cpd.settings[UO_sp_permit_cpp11_shift].b)
                           || (  (cpd.lang_flags & LANG_JAVA)
                              || (cpd.lang_flags & LANG_CS)))
                        && pc->type == CT_ANGLE_CLOSE
                        && next->type == CT_ANGLE_CLOSE)
                     {
                        // allow '>' and '>' to become '>>'
                     }
                     else if (strcmp(ct->tag, "[]") == 0)
                     {
                        // this is OK
                     }
                     else
                     {
                        chunk_flags_set(pc, PCF_FORCE_SPACE);
                     }
                  }
               }
            }
         }

         int min_sp;
         LOG_FMT(LSPACE, "%s(%d): orig_line is %zu, orig_col is %zu, %s type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type));
         argval_t av = do_space(pc, next, min_sp, false);
         if (pc->flags & PCF_FORCE_SPACE)
         {
            int av_int = av;
            LOG_FMT(LSPACE, " <force between '%s' and '%s'>",
                    pc->text(), next->text());
            av_int |= AV_ADD;
            av      = static_cast<argval_t>(av_int);
         }
         min_sp = max(1, min_sp);
         switch (av)
         {
         case AV_FORCE:
            column += min_sp;  // add exactly the specified number of spaces
            break;

         case AV_ADD:
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

         case AV_REMOVE:
         // the symbols will be back-to-back "a+3"
         case AV_NOT_DEFINED:
            break;

         case AV_IGNORE:
            // Keep the same relative spacing, if possible
            if (next->orig_col >= pc->orig_col_end && pc->orig_col_end != 0)
            {
               column += next->orig_col - pc->orig_col_end;
            }
            break;
         }

         if (  chunk_is_comment(next)
            && chunk_is_newline(chunk_get_next(next))
            && column < next->orig_col)
         {
            /*
             * do some comment adjustments if sp_before_tr_emb_cmt and
             * sp_endif_cmt did not apply.
             */
            if (  (  cpd.settings[UO_sp_before_tr_emb_cmt].a == AV_IGNORE
                  || (  next->parent_type != CT_COMMENT_END
                     && next->parent_type != CT_COMMENT_EMBED))
               && (  cpd.settings[UO_sp_endif_cmt].a == AV_IGNORE
                  || (  pc->type != CT_PP_ELSE
                     && pc->type != CT_PP_ENDIF)))
            {
               if (cpd.settings[UO_indent_relative_single_line_comments].b)
               {
                  // Try to keep relative spacing between tokens
                  LOG_FMT(LSPACE, " <relative adj>");
                  column = pc->column + 1 + (next->orig_col - pc->orig_col_end);
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
                  LOG_FMT(LSPACE, " <relative set>");
               }
            }
         }
         next->column = column;

         LOG_FMT(LSPACE, " = %s @ %zu => %zu\n",
                 (av == AV_IGNORE) ? "IGNORE" :
                 (av == AV_ADD) ? "ADD" :
                 (av == AV_REMOVE) ? "REMOVE" : "FORCE",
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

         /*
          * find the closing paren that matches the 'first' open paren and force
          * a space before it
          */
         chunk_t *cur  = next;
         chunk_t *prev = cur;
         while ((cur = chunk_get_next(cur)) != nullptr) // find the closing parenthesis
         {                                              // that matches the
            if (cur->level == first->level)             // first open parenthesis
            {
               space_add_after(prev, 1);                // and force a space before it
               break;
            }
            prev = cur;
         }
      }
      else if (chunk_is_str(first, ")", 1) && chunk_is_str(next, ")", 1))
      {
         // insert a space between the two closing parens
         space_add_after(first, 1);

         // issue # 752
         // the next lines are never used in the tests.
         // TODO: why that?
         ///* find the opening paren that matches the 'next' close paren and force
         // * a space after it */
         //cur = first;
         //while ((cur = chunk_get_prev(cur)) != nullptr)
         //{
         //   if (cur->level == next->level)
         //   {
         //      //space_add_after(cur, 1);
         //      break;
         //   }
         //}
      }

      first = next;
   }
} // space_text_balance_nested_parens


size_t space_needed(chunk_t *first, chunk_t *second)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LSPACE, "%s(%d)\n", __func__, __LINE__);

   int min_sp;
   switch (do_space(first, second, min_sp))
   {
   case AV_ADD:
   case AV_FORCE:
      return(max(1, min_sp));

   case AV_REMOVE:
      return(0);

   case AV_IGNORE:
   default:
      return(second->orig_col > (first->orig_col + first->len()));
   }
}


size_t space_col_align(chunk_t *first, chunk_t *second)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LSPACE, "%s(%d): orig_line is %zu, orig_col is %zu, [%s/%s] '%s' <==> line is %zu, col is %zu [%s/%s] '%s'",
           __func__, __LINE__, first->orig_line, first->orig_col,
           get_token_name(first->type), get_token_name(first->parent_type),
           first->text(),
           second->orig_line, second->orig_col,
           get_token_name(second->type), get_token_name(second->parent_type),
           second->text());
   log_func_stack_inline(LSPACE);

   int      min_sp;
   argval_t av = do_space(first, second, min_sp);

   LOG_FMT(LSPACE, "%s(%d): av is %d, ", __func__, __LINE__, av);
   size_t coldiff;
   if (first->nl_count)
   {
      LOG_FMT(LSPACE, "nl_count is %zu, orig_col_end is %zu", first->nl_count, first->orig_col_end);
      coldiff = first->orig_col_end - 1;
   }
   else
   {
      LOG_FMT(LSPACE, "len is %zu", first->len());
      coldiff = first->len();
   }

   switch (av)
   {
   case AV_ADD:
   case AV_FORCE:
      coldiff++;
      break;

   case AV_REMOVE:
      break;

   case AV_IGNORE:
      if (second->orig_col > (first->orig_col + first->len()))
      {
         coldiff++;
      }
      break;

   case AV_NOT_DEFINED:
      break;
   }
   LOG_FMT(LSPACE, " => %zu\n", coldiff);
   return(coldiff);
} // space_col_align


void space_add_after(chunk_t *pc, size_t count)
{
   LOG_FUNC_ENTRY();
   //if (count <= 0)
   //{
   //   return;
   //}

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
   if (next->type == CT_SPACE)
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

   chunk_add_after(&sp, pc);
} // space_add_after
