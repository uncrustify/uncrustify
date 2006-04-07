/**
 * @file c_space.c
 * Adds or removes inter-chunk spaces.
 *
 * $Id$
 */
#include "cparse_types.h"
#include "chunk_list.h"
#include "prototypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

struct no_space_table_s
{
   c_token_t first;
   c_token_t second;
};

/** this table lists out all combos where a space should NOT be present
 * CT_UNKNOWN is a wildcard.
 */
struct no_space_table_s no_space_table[] =
{
   { CT_INCDEC_BEFORE, CT_WORD },
   { CT_UNKNOWN,       CT_INCDEC_AFTER },
   { CT_UNKNOWN,       CT_ELIPSIS },
   { CT_UNKNOWN,       CT_LABEL_COLON },
   { CT_UNKNOWN,       CT_SEMICOLON },
   { CT_UNKNOWN,       CT_D_TEMPLATE },
   { CT_D_TEMPLATE,    CT_UNKNOWN },
   { CT_UNKNOWN,       CT_MEMBER },
   { CT_MEMBER,        CT_UNKNOWN },
   { CT_UNKNOWN,       CT_DC_MEMBER },
   { CT_DC_MEMBER,     CT_UNKNOWN },
   { CT_MACRO_FUNC,    CT_FPAREN_OPEN },
   { CT_PAREN_OPEN,    CT_UNKNOWN },
   { CT_UNKNOWN,       CT_PAREN_CLOSE },
   { CT_FPAREN_OPEN,   CT_UNKNOWN },
   { CT_UNKNOWN,       CT_SPAREN_CLOSE },
   { CT_SPAREN_OPEN,   CT_UNKNOWN },
   { CT_UNKNOWN,       CT_FPAREN_CLOSE },
   { CT_UNKNOWN,       CT_COMMA },
   { CT_POS,           CT_UNKNOWN },
   { CT_ADDR,          CT_UNKNOWN },
   { CT_STAR,          CT_UNKNOWN },
   { CT_DEREF,         CT_UNKNOWN },
   { CT_NOT,           CT_UNKNOWN },
   { CT_INV,           CT_UNKNOWN },
   { CT_VBRACE_CLOSE,  CT_UNKNOWN },
   { CT_VBRACE_OPEN,   CT_UNKNOWN },
   { CT_UNKNOWN,       CT_VBRACE_CLOSE },
   { CT_UNKNOWN,       CT_VBRACE_OPEN },
   { CT_PREPROC,       CT_UNKNOWN },
   { CT_NEG,           CT_UNKNOWN },
   { CT_UNKNOWN,       CT_SQUARE_OPEN },
   { CT_UNKNOWN,       CT_SQUARE_CLOSE },
   { CT_UNKNOWN,       CT_CASE_COLON },
   { CT_SQUARE_OPEN,   CT_UNKNOWN },
   { CT_PAREN_CLOSE,   CT_WORD },
   { CT_PAREN_CLOSE,   CT_FUNC_DEF },
   { CT_PAREN_CLOSE,   CT_FUNC_CALL },
   { CT_PAREN_CLOSE,   CT_ADDR },
   { CT_PAREN_CLOSE,   CT_FPAREN_OPEN },
   { CT_PTR_TYPE,      CT_WORD },
   { CT_PTR_TYPE,      CT_FUNC_DEF },
   { CT_PTR_TYPE,      CT_FUNC_CALL },
   { CT_PTR_TYPE,      CT_FUNC_PROTO },
   { CT_PTR_TYPE,      CT_PTR_TYPE },

   // conflict: (type)(val) vs #define fcn(x) (x+1)
   //   {CT_PAREN_CLOSE,        CT_PAREN_OPEN},
};


/**
 * Decides how to change inter-chunk spacing.
 * Note that the order of the if statements is VERY important.
 *
 * @param first   The first chunk
 * @param second  The second chunk
 * @return -1=remove space, 0=no change, 1=add space, 2=force one space
 */
argval_t do_space(chunk_t *first, chunk_t *second)
{
   int      idx;
   argval_t arg;

   if (first->type == CT_MACRO_FUNC)
   {
      return(AV_REMOVE);
   }

   if (chunk_is_comment(second))
   {
      return(AV_IGNORE);
   }

   if (second->type == CT_VBRACE_OPEN)
   {
      return(AV_ADD);
   }

   if (second->type == CT_SEMICOLON)
   {
      arg = cpd.settings[UO_sp_before_semi];
      if (first->type == CT_SPAREN_CLOSE)
      {
         arg |= cpd.settings[UO_sp_special_semi];
      }
      return(arg);
   }

   /* "return(a);" vs "return (foo_t)a + 3;" vs "return a;" vs "return;" */
   if (first->type == CT_RETURN)
   {
      if ((second->type == CT_PAREN_OPEN) &&
          (second->parent_type == CT_RETURN))
      {
         return(cpd.settings[UO_sp_return_paren]);
      }
      /* everything else requires a space */
      return(AV_FORCE);
   }

   /* "sizeof(foo_t)" vs "sizeof foo_t" */
   if (first->type == CT_SIZEOF)
   {
      if (second->type == CT_PAREN_OPEN)
      {
         return(cpd.settings[UO_sp_sizeof_paren]);
      }
      return(AV_FORCE);
   }

   /* handle '::' */
   if ((first->type == CT_DC_MEMBER) || (second->type == CT_DC_MEMBER))
   {
      return(AV_REMOVE);
   }

   /* handle '~' */
   if (first->type == CT_DESTRUCTOR)
   {
      return(AV_REMOVE);
   }

   /* "((" vs "( (" */
   if ((first->type == CT_PAREN_OPEN) && (second->type == CT_PAREN_OPEN))
   {
      return(cpd.settings[UO_sp_paren_paren]);
   }

   /* "if (" vs "if(" */
   if (second->type == CT_SPAREN_OPEN)
   {
      return(cpd.settings[UO_sp_before_sparen]);
   }

   /* "a [x]" vs "a[x]" */
   if (second->type == CT_SQUARE_OPEN)
   {
      return(cpd.settings[UO_sp_before_square]);
   }

   /* "byte[]" vs "byte []" */
   if (second->type == CT_TSQUARE)
   {
      return(cpd.settings[UO_sp_before_squares]);
   }

   /* "for (...) {...}" vs "for (...){...}" */
   if (first->type == CT_SPAREN_CLOSE)
   {
      return(cpd.settings[UO_sp_after_sparen]);
   }

   /* spaces between function and open paren */
   if (first->type == CT_FUNC_CALL)
   {
      return(cpd.settings[UO_sp_func_call_paren]);
   }
   if (first->type == CT_FUNC_DEF)
   {
      return(cpd.settings[UO_sp_func_def_paren]);
   }
   if (first->type == CT_FUNC_PROTO)
   {
      return(cpd.settings[UO_sp_func_proto_paren]);
   }

   if (first->type == CT_BRACE_OPEN)
   {
      if (first->parent_type == CT_ENUM)
      {
         return(cpd.settings[UO_sp_inside_braces_enum]);
      }
      if ((first->parent_type == CT_STRUCT) ||
          (first->parent_type == CT_UNION))
      {
         return(cpd.settings[UO_sp_inside_braces_struct]);
      }
      return(cpd.settings[UO_sp_inside_braces]);
   }

   if (second->type == CT_BRACE_CLOSE)
   {
      if (second->parent_type == CT_ENUM)
      {
         return(cpd.settings[UO_sp_inside_braces_enum]);
      }
      if ((second->parent_type == CT_STRUCT) ||
          (second->parent_type == CT_UNION))
      {
         return(cpd.settings[UO_sp_inside_braces_struct]);
      }
      return(cpd.settings[UO_sp_inside_braces]);
   }

   /* "a = { ... }" vs "a = {...}" */
   if (((first->type == CT_BRACE_OPEN) && (first->parent_type == CT_ASSIGN)) ||
       ((second->type == CT_BRACE_CLOSE) && (second->parent_type == CT_ASSIGN)))
   {
      return(cpd.settings[UO_sp_func_call_paren]);
   }

   if (first->type == CT_CAST)
   {
      return(AV_REMOVE);
   }

   if ((first->type == CT_THIS) && (second->type == CT_PAREN_OPEN))
   {
      return(AV_REMOVE);
   }

   if ((first->type == CT_DELEGATE) && (second->type == CT_PAREN_OPEN))
   {
      return(AV_REMOVE);
   }

   if (((second->type == CT_MEMBER) || (second->type == CT_DC_MEMBER)) &&
       ((first->type != CT_COMMA) && (first->type != CT_BRACE_OPEN)))
   {
      return(AV_REMOVE);
   }

   if ((first->type == CT_SUPER) && (second->type == CT_PAREN_OPEN))
   {
      return(AV_REMOVE);
   }

   if (first->type == CT_PAREN_CLOSE)
   {
      /* "(int)a" vs "(int) a" */
      if (first->parent_type == CT_CAST)
      {
         return(cpd.settings[UO_sp_after_cast]);
      }

      /* "(struct foo) {...}" vs "(struct foo){...}" */
      if (second->type == CT_BRACE_OPEN)
      {
         return(cpd.settings[UO_sp_paren_brace]);
      }

      /* D-specific: "delegate(some thing) dg */
      if (first->parent_type == CT_DELEGATE)
      {
         return(AV_ADD);
      }
   }

   /* "foo(...)" vs "foo( ... )" */
   if ((first->type == CT_FPAREN_OPEN) || (second->type == CT_FPAREN_CLOSE))
   {
      return(cpd.settings[UO_sp_inside_fparen]);
   }

   /* "(a + 3)" vs "( a + 3 )" */
   if ((first->type == CT_PAREN_OPEN) || (second->type == CT_PAREN_CLOSE))
   {
      return(cpd.settings[UO_sp_inside_paren]);
   }

   /* "[3]" vs "[ 3 ]" */
   if ((first->type == CT_SQUARE_OPEN) || (second->type == CT_SQUARE_CLOSE))
   {
      return(cpd.settings[UO_sp_inside_square]);
   }

   /* "if(...)" vs "if( ... )" */
   if ((first->type == CT_SPAREN_OPEN) || (second->type == CT_SPAREN_CLOSE))
   {
      return(cpd.settings[UO_sp_inside_sparen]);
   }

   /* "a,b" vs "a, b" */
   if (first->type == CT_COMMA)
   {
      return(cpd.settings[UO_sp_after_comma]);
   }
   if (second->type == CT_COMMA)
   {
      return(AV_REMOVE);
   }

   if ((first->type == CT_ARITH) || (second->type == CT_ARITH))
   {
      return(cpd.settings[UO_sp_arith]);
   }
   if ((first->type == CT_BOOL) || (second->type == CT_BOOL))
   {
      return(cpd.settings[UO_sp_bool]);
   }
   if ((first->type == CT_COMPARE) || (second->type == CT_COMPARE))
   {
      return(cpd.settings[UO_sp_compare]);
   }
   if ((first->type == CT_ASSIGN) || (second->type == CT_ASSIGN))
   {
      return(cpd.settings[UO_sp_assign]);
   }

   if ((first->type == CT_PAREN_OPEN) && (second->type == CT_PTR_TYPE))
   {
      return(AV_REMOVE);
   }

   if ((second->type == CT_FUNC_PROTO) || (second->type == CT_FUNC_DEF))
   {
      if (first->type != CT_PTR_TYPE)
      {
         return(cpd.settings[UO_sp_type_func] | AV_ADD);
      }
      return(cpd.settings[UO_sp_type_func]);
   }

   if (first->type == CT_BRACE_OPEN)
   {
      if (first->parent_type == CT_ENUM)
      {
         return(cpd.settings[UO_sp_inside_braces_enum]);
      }
      else if ((first->parent_type == CT_UNION) ||
               (first->parent_type == CT_STRUCT))
      {
         return(cpd.settings[UO_sp_inside_braces_struct]);
      }
      return(cpd.settings[UO_sp_inside_braces]);
   }

   if (second->type == CT_BRACE_CLOSE)
   {
      if (second->parent_type == CT_ENUM)
      {
         return(cpd.settings[UO_sp_inside_braces_enum]);
      }
      else if ((second->parent_type == CT_UNION) ||
               (second->parent_type == CT_STRUCT))
      {
         return(cpd.settings[UO_sp_inside_braces_struct]);
      }
      return(cpd.settings[UO_sp_inside_braces]);
   }

   if (first->type == CT_PAREN_CLOSE)
   {
      if (second->type == CT_PAREN_CLOSE)
      {
         return(cpd.settings[UO_sp_paren_paren]);
      }
      if (first->parent_type == CT_CAST)
      {
         return(cpd.settings[UO_sp_after_cast]);
      }
      /* Must be an indirect function call */
      if (second->type == CT_PAREN_OPEN)
      {
         return(AV_REMOVE);  /* TODO: make this configurable? */
      }
   }

   if (second->type == CT_SPAREN_OPEN)
   {
      return(cpd.settings[UO_sp_before_sparen]);
   }

   if ((second->type == CT_SPAREN_CLOSE) &&
       (first->type == CT_SEMICOLON) &&
       (second->parent_type == CT_FOR))
   {
      return(AV_ADD);
   }

   if ((first->type == CT_SPAREN_CLOSE) &&
       (second->type == CT_SEMICOLON) &&
       (first->parent_type == CT_WHILE_OF_DO))
   {
      return(AV_REMOVE); /*TODO: does this need to be configured? */
   }

   if ((first->type == CT_QUALIFIER) || (first->type == CT_TYPE))
   {
      return(AV_FORCE);
   }

   if (first->type == CT_PTR_TYPE)
   {
      return(AV_REMOVE);
   }


   for (idx = 0; idx < ARRAY_SIZE(no_space_table); idx++)
   {
      if (((no_space_table[idx].first == CT_UNKNOWN) ||
           (no_space_table[idx].first == first->type))
          &&
          ((no_space_table[idx].second == CT_UNKNOWN) ||
           (no_space_table[idx].second == second->type)))
      {
         return(AV_REMOVE);
      }
   }
   return(AV_ADD);
}


/**
 * Marches through the whole file and checks to see how many spaces should be
 * between two chunks
 */
void space_text(void)
{
   chunk_t *pc;
   chunk_t *next;
   int     column = 1;
   int     delta;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      next = chunk_get_next(pc);
      if (next == NULL)
      {
         break;
      }
      /* If the current chunk contains a newline, do not change the column
       * of the next item */
      if ((pc->type == CT_NEWLINE) ||
          (pc->type == CT_NL_CONT) ||
          (pc->type == CT_COMMENT_MULTI))
      {
         column = next->column;
      }
      //      /* The the next guy is a comment, make sure there is a space */
      //      else if ((next->type == CT_COMMENT) ||
      //               (next->type == CT_COMMENT_CPP) ||
      //               (next->type == CT_COMMENT_MULTI))
      //      {
      //         column += pc->len + 1;
      //         if (next->column < column)
      //         {
      //            next->column = column;
      //         }
      //      }
      else
      {
         /* Set to the minimum allowed column */
         column += pc->len;

         switch (do_space(pc, next))
         {
         case AV_FORCE:
            /* add exactly one space */
            column++;
            break;

         case AV_ADD:
            delta = 1;
            if ((next->orig_col >= pc->orig_col_end) && (pc->orig_col_end != 0))
            {
               /* Keep the same relative spacing, minimum 1 */
               delta = next->orig_col - pc->orig_col_end;
               if (delta < 1)
               {
                  delta = 1;
               }
            }
            column += delta;
            break;

         case AV_REMOVE:
            /* the symbols will be back-to-back "a+3" */
            break;

         default:
            /* Keep the same relative spacing, if possible */
            if ((next->orig_col >= pc->orig_col_end) && (pc->orig_col_end != 0))
            {
               column += next->orig_col - pc->orig_col_end;
            }
            break;
         }
         next->column = column;
      }

      pc = next;
   }
}

