/**
 * @file c_space.c
 * Adds or removes inter-chunk spaces.
 *
 * $Id$
 */
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "prototypes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cctype>

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
   { CT_INCDEC_BEFORE, CT_WORD         },
   { CT_UNKNOWN,       CT_INCDEC_AFTER },
   { CT_UNKNOWN,       CT_ELIPSIS      },
   { CT_UNKNOWN,       CT_LABEL_COLON  },
   { CT_UNKNOWN,       CT_SEMICOLON    },
   { CT_UNKNOWN,       CT_D_TEMPLATE   },
   { CT_D_TEMPLATE,    CT_UNKNOWN      },
   { CT_UNKNOWN,       CT_MEMBER       },
   { CT_MEMBER,        CT_UNKNOWN      },
   { CT_UNKNOWN,       CT_DC_MEMBER    },
   { CT_DC_MEMBER,     CT_UNKNOWN      },
   { CT_MACRO_FUNC,    CT_FPAREN_OPEN  },
   { CT_PAREN_OPEN,    CT_UNKNOWN      },
   { CT_UNKNOWN,       CT_PAREN_CLOSE  },
   { CT_FPAREN_OPEN,   CT_UNKNOWN      },
   { CT_UNKNOWN,       CT_SPAREN_CLOSE },
   { CT_SPAREN_OPEN,   CT_UNKNOWN      },
   { CT_UNKNOWN,       CT_FPAREN_CLOSE },
   { CT_UNKNOWN,       CT_COMMA        },
   { CT_POS,           CT_UNKNOWN      },
   { CT_ADDR,          CT_UNKNOWN      },
   { CT_STAR,          CT_UNKNOWN      },
   { CT_DEREF,         CT_UNKNOWN      },
   { CT_NOT,           CT_UNKNOWN      },
   { CT_INV,           CT_UNKNOWN      },
   { CT_VBRACE_CLOSE,  CT_UNKNOWN      },
   { CT_VBRACE_OPEN,   CT_UNKNOWN      },
   { CT_UNKNOWN,       CT_VBRACE_CLOSE },
   { CT_UNKNOWN,       CT_VBRACE_OPEN  },
   { CT_PREPROC,       CT_UNKNOWN      },
   { CT_NEG,           CT_UNKNOWN      },
   { CT_UNKNOWN,       CT_SQUARE_OPEN  },
   { CT_UNKNOWN,       CT_SQUARE_CLOSE },
   { CT_UNKNOWN,       CT_CASE_COLON   },
   { CT_SQUARE_OPEN,   CT_UNKNOWN      },
   { CT_PAREN_CLOSE,   CT_WORD         },
   { CT_PAREN_CLOSE,   CT_FUNC_DEF     },
   { CT_PAREN_CLOSE,   CT_FUNC_CALL    },
   { CT_PAREN_CLOSE,   CT_ADDR         },
   { CT_PAREN_CLOSE,   CT_FPAREN_OPEN  },
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

   if (first->type == CT_PREPROC)
   {
      /* Remove spaces, unless we are ignoring. See indent_preproc() */
      if (cpd.settings[UO_pp_space].a == AV_IGNORE)
      {
         return(AV_IGNORE);
      }
      return(AV_REMOVE);
   }

   if (second->type == CT_SEMICOLON)
   {
      arg = cpd.settings[UO_sp_before_semi].a;
      if (first->type == CT_SPAREN_CLOSE)
      {
         arg = (argval_t)(arg | cpd.settings[UO_sp_special_semi].a);
      }
      return(arg);
   }

   if (((first->type == CT_NEG) || (first->type == CT_POS) || (first->type == CT_ARITH)) &&
       ((second->type == CT_NEG) || (second->type == CT_POS) || (second->type == CT_ARITH)))
   {
      return(AV_ADD);
   }

   /* "return(a);" vs "return (foo_t)a + 3;" vs "return a;" vs "return;" */
   if (first->type == CT_RETURN)
   {
      if ((second->type == CT_PAREN_OPEN) &&
          (second->parent_type == CT_RETURN))
      {
         return(cpd.settings[UO_sp_return_paren].a);
      }
      /* everything else requires a space */
      return(AV_FORCE);
   }

   /* "sizeof(foo_t)" vs "sizeof foo_t" */
   if (first->type == CT_SIZEOF)
   {
      if (second->type == CT_PAREN_OPEN)
      {
         return(cpd.settings[UO_sp_sizeof_paren].a);
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
      return(cpd.settings[UO_sp_paren_paren].a);
   }

   /* "if (" vs "if(" */
   if (second->type == CT_SPAREN_OPEN)
   {
      return(cpd.settings[UO_sp_before_sparen].a);
   }

   /* "a [x]" vs "a[x]" */
   if (second->type == CT_SQUARE_OPEN)
   {
      return(cpd.settings[UO_sp_before_square].a);
   }

   /* "byte[]" vs "byte []" */
   if (second->type == CT_TSQUARE)
   {
      return(cpd.settings[UO_sp_before_squares].a);
   }

   /* spacing around template < > stuff */
   if ((first->type == CT_ANGLE_OPEN) ||
       (second->type == CT_ANGLE_CLOSE))
   {
      return(cpd.settings[UO_sp_inside_angle].a);
   }
   if (second->type == CT_ANGLE_OPEN)
   {
      return(cpd.settings[UO_sp_before_angle].a);
   }
   if (first->type == CT_ANGLE_CLOSE)
   {
      return(cpd.settings[UO_sp_after_angle].a);
   }

   /* "for (...) {...}" vs "for (...){...}" */
   if (first->type == CT_SPAREN_CLOSE)
   {
      return(cpd.settings[UO_sp_after_sparen].a);
   }

   /* spaces between function and open paren */
   if (first->type == CT_FUNC_CALL)
   {
      return(cpd.settings[UO_sp_func_call_paren].a);
   }
   if (first->type == CT_FUNC_DEF)
   {
      return(cpd.settings[UO_sp_func_def_paren].a);
   }
   if (first->type == CT_FUNC_PROTO)
   {
      return(cpd.settings[UO_sp_func_proto_paren].a);
   }
   if (first->type == CT_FUNC_CLASS)
   {
      return(cpd.settings[UO_sp_func_class_paren].a);
   }

   if (first->type == CT_BRACE_OPEN)
   {
      if (first->parent_type == CT_ENUM)
      {
         return(cpd.settings[UO_sp_inside_braces_enum].a);
      }
      if ((first->parent_type == CT_STRUCT) ||
          (first->parent_type == CT_UNION))
      {
         return(cpd.settings[UO_sp_inside_braces_struct].a);
      }
      return(cpd.settings[UO_sp_inside_braces].a);
   }

   if (second->type == CT_BRACE_CLOSE)
   {
      if (second->parent_type == CT_ENUM)
      {
         return(cpd.settings[UO_sp_inside_braces_enum].a);
      }
      if ((second->parent_type == CT_STRUCT) ||
          (second->parent_type == CT_UNION))
      {
         return(cpd.settings[UO_sp_inside_braces_struct].a);
      }
      return(cpd.settings[UO_sp_inside_braces].a);
   }

   /* "a = { ... }" vs "a = {...}" */
   if (((first->type == CT_BRACE_OPEN) && (first->parent_type == CT_ASSIGN)) ||
       ((second->type == CT_BRACE_CLOSE) && (second->parent_type == CT_ASSIGN)))
   {
      return(cpd.settings[UO_sp_func_call_paren].a);
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
         return(cpd.settings[UO_sp_after_cast].a);
      }

      /* "(struct foo) {...}" vs "(struct foo){...}" */
      if (second->type == CT_BRACE_OPEN)
      {
         return(cpd.settings[UO_sp_paren_brace].a);
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
      if ((first->type == CT_FPAREN_OPEN) && (second->type == CT_FPAREN_CLOSE))
      {
         return(cpd.settings[UO_sp_inside_fparens].a);
      }
      return(cpd.settings[UO_sp_inside_fparen].a);
   }

   /* "(a + 3)" vs "( a + 3 )" */
   if ((first->type == CT_PAREN_OPEN) || (second->type == CT_PAREN_CLOSE))
   {
      return(cpd.settings[UO_sp_inside_paren].a);
   }

   /* "[3]" vs "[ 3 ]" */
   if ((first->type == CT_SQUARE_OPEN) || (second->type == CT_SQUARE_CLOSE))
   {
      return(cpd.settings[UO_sp_inside_square].a);
   }

   /* "if(...)" vs "if( ... )" */
   if ((first->type == CT_SPAREN_OPEN) || (second->type == CT_SPAREN_CLOSE))
   {
      return(cpd.settings[UO_sp_inside_sparen].a);
   }

   /* "a,b" vs "a, b" */
   if (first->type == CT_COMMA)
   {
      return(cpd.settings[UO_sp_after_comma].a);
   }
   if (second->type == CT_COMMA)
   {
      return(AV_REMOVE);
   }

   if ((first->type == CT_ARITH) || (second->type == CT_ARITH))
   {
      return(cpd.settings[UO_sp_arith].a);
   }
   if ((first->type == CT_BOOL) || (second->type == CT_BOOL))
   {
      arg = cpd.settings[UO_sp_bool].a;
      if ((cpd.settings[UO_nl_bool_pos].n != 0) &&
          (first->orig_line != second->orig_line) &&
          (arg != AV_REMOVE))
      {
         arg = (argval_t)(arg | AV_ADD);
      }
      return(arg);
   }
   if ((first->type == CT_COMPARE) || (second->type == CT_COMPARE))
   {
      return(cpd.settings[UO_sp_compare].a);
   }
   if ((first->type == CT_ASSIGN) || (second->type == CT_ASSIGN))
   {
      return(cpd.settings[UO_sp_assign].a);
   }

   if ((first->type == CT_PAREN_OPEN) && (second->type == CT_PTR_TYPE))
   {
      return(AV_REMOVE);
   }

   if ((second->type == CT_FUNC_PROTO) || (second->type == CT_FUNC_DEF))
   {
      if (first->type != CT_PTR_TYPE)
      {
         return((argval_t)(cpd.settings[UO_sp_type_func].a | AV_ADD));
      }
      return(cpd.settings[UO_sp_type_func].a);
   }

   if (first->type == CT_BRACE_OPEN)
   {
      if (first->parent_type == CT_ENUM)
      {
         return(cpd.settings[UO_sp_inside_braces_enum].a);
      }
      else if ((first->parent_type == CT_UNION) ||
               (first->parent_type == CT_STRUCT))
      {
         return(cpd.settings[UO_sp_inside_braces_struct].a);
      }
      return(cpd.settings[UO_sp_inside_braces].a);
   }

   if (second->type == CT_BRACE_CLOSE)
   {
      if (second->parent_type == CT_ENUM)
      {
         return(cpd.settings[UO_sp_inside_braces_enum].a);
      }
      else if ((second->parent_type == CT_UNION) ||
               (second->parent_type == CT_STRUCT))
      {
         return(cpd.settings[UO_sp_inside_braces_struct].a);
      }
      return(cpd.settings[UO_sp_inside_braces].a);
   }

   if (first->type == CT_PAREN_CLOSE)
   {
      if (second->type == CT_PAREN_CLOSE)
      {
         return(cpd.settings[UO_sp_paren_paren].a);
      }
      if (first->parent_type == CT_CAST)
      {
         return(cpd.settings[UO_sp_after_cast].a);
      }
      /* Must be an indirect function call */
      if (second->type == CT_PAREN_OPEN)
      {
         return(AV_REMOVE);  /* TODO: make this configurable? */
      }
   }

   if (second->type == CT_SPAREN_OPEN)
   {
      return(cpd.settings[UO_sp_before_sparen].a);
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

   if ((first->type == CT_TYPE) && (second->type == CT_BYREF))
   {
      return(cpd.settings[UO_sp_before_byref].a);
   }

   if ((first->type == CT_QUALIFIER) || (first->type == CT_TYPE))
   {
      return(AV_FORCE);
   }

   if (first->type == CT_PTR_TYPE)
   {
      return(AV_REMOVE);
   }


   for (idx = 0; idx < (int)ARRAY_SIZE(no_space_table); idx++)
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

/**
 * Calculates the column difference between two chunks.
 * The rules are bent a bit here, as AV_IGNORE and AV_ADD become AV_FORCE.
 * So the column difference is either first->len or first->len + 1.
 *
 * @param first   The first chunk
 * @param second  The second chunk
 * @return        the column difference between the two chunks
 */
int space_col_align(chunk_t *first, chunk_t *second)
{
   int      coldiff;
   argval_t av;

   av = do_space(first, second);

   coldiff = first->len;
   if (av != AV_REMOVE)
   {
      coldiff++;
   }
   return(coldiff);
}
