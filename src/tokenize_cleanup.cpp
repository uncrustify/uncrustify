/**
 * @file tokenize_cleanup.cpp
 * Looks at simple sequences to refine the chunk types.
 * Examples:
 *  - change '[' + ']' into '[]'/
 *  - detect "version = 10;" vs "version (xxx) {"
 *
 * @author  Ben Gardner
 * @license GPL v2+
 *
 * $Id$
 */

#include "uncrustify_types.h"
#include "prototypes.h"
#include "chunk_list.h"
#include "char_table.h"
#include <cstring>

static void check_template(chunk_t *start);


void tokenize_cleanup(void)
{
   chunk_t *pc   = chunk_get_head();
   chunk_t *prev = NULL;
   chunk_t *next;
   chunk_t *tmp;
   bool    in_type_cast = false;

   pc   = chunk_get_head();
   next = chunk_get_next_ncnl(pc);
   while ((pc != NULL) && (next != NULL))
   {
      /* Change '[' + ']' into '[]' */
      if ((pc->type == CT_SQUARE_OPEN) && (next->type == CT_SQUARE_CLOSE))
      {
         pc->type = CT_TSQUARE;
         pc->str  = "[]";
         pc->len  = 2;
         chunk_del(next);
         next = chunk_get_next_ncnl(pc);
      }

      if ((pc->type == CT_DOT) && ((cpd.lang_flags & LANG_ALLC) != 0))
      {
         pc->type = CT_MEMBER;
      }

      /* Determine the version stuff (D only) */
      if (pc->type == CT_VERSION)
      {
         if (next->type == CT_PAREN_OPEN)
         {
            pc->type = CT_IF;
         }
         else
         {
            if (next->type != CT_ASSIGN)
            {
               LOG_FMT(LERR, "%s:%d %s: version: Unexpected token %s\n",
                       cpd.filename, pc->orig_line, __func__, get_token_name(next->type));
               cpd.error_count++;
            }
            pc->type = CT_WORD;
         }
      }

      /**
       * Change CT_WORD after CT_ENUM, CT_UNION, or CT_STRUCT to CT_TYPE
       * Change CT_WORD before CT_WORD to CT_TYPE
       */
      if (next->type == CT_WORD)
      {
         if ((pc->type == CT_ENUM) ||
             (pc->type == CT_UNION) ||
             (pc->type == CT_STRUCT))
         {
            next->type = CT_TYPE;
         }
         if (pc->type == CT_WORD)
         {
            pc->type = CT_TYPE;
         }
      }

      /**
       * Change CT_STAR to CT_PTR_TYPE if preceeded by CT_TYPE,
       * CT_QUALIFIER, or CT_PTR_TYPE.
       */
      if ((next->type == CT_STAR) &&
          ((pc->type == CT_TYPE) ||
           (pc->type == CT_QUALIFIER) ||
           (pc->type == CT_PTR_TYPE)))
      {
         next->type = CT_PTR_TYPE;
      }

      if ((pc->type == CT_TYPE_CAST) &&
          (next->type == CT_ANGLE_OPEN))
      {
         next->parent_type = CT_TYPE_CAST;
         in_type_cast      = true;
      }

      /**
       * Change angle open/close to CT_COMPARE, if not a template thingy
       */
      if ((pc->type == CT_ANGLE_OPEN) && (pc->parent_type == CT_NONE))
      {
         check_template(pc);
      }
      if ((pc->type == CT_ANGLE_CLOSE) && (pc->parent_type != CT_TEMPLATE))
      {
         if (in_type_cast)
         {
            in_type_cast    = false;
            pc->parent_type = CT_TYPE_CAST;
         }
         else
         {
            pc->type = CT_COMPARE;
         }
      }

      if ((cpd.lang_flags & LANG_D) != 0)
      {
         /* Check for the D string concat symbol '~' */
         if ((pc->type == CT_INV) &&
             ((prev->type == CT_STRING) ||
              (next->type == CT_STRING)))
         {
            pc->type = CT_CONCAT;
         }

         /* Check for the D template symbol '!' */
         if ((pc->type == CT_NOT) &&
             (prev->type == CT_WORD) &&
             (next->type == CT_PAREN_OPEN))
         {
            pc->type = CT_D_TEMPLATE;
         }
      }

      /* Change get/set to CT_WORD if not followed by a brace open */
      if ((pc->type == CT_GETSET) && (next->type != CT_BRACE_OPEN))
      {
         pc->type = CT_WORD;
      }

      if ((pc->type == CT_ENUM) ||
          (pc->type == CT_STRUCT) ||
          (pc->type == CT_UNION))
      {
         if (get_char_table(*next->str) & CT_KW1)
         {
            next->type = CT_TYPE;
         }
      }

      /* Change item after operator (>=, ==, etc) to a FUNC_OPERATOR */
      if (pc->type == CT_OPERATOR)
      {
         /* Handle special case of [] and () operators */
         if ((next->type == CT_PAREN_OPEN) ||
             (next->type == CT_SQUARE_OPEN))
         {
            tmp = chunk_get_next(next);
            if ((tmp != NULL) && (tmp->type == (next->type + 1)))
            {
               next->str         = (next->type == CT_PAREN_OPEN) ? "()" : "[]";
               next->len         = 2;
               next->type        = CT_FUNCTION;
               next->parent_type = CT_OPERATOR;
               chunk_del(tmp);
            }
         }
         else
         {
            next->type        = CT_FUNCTION;
            next->parent_type = CT_OPERATOR;
         }
         if (chunk_is_addr(prev))
         {
            prev->type = CT_BYREF;
         }
      }

      /* Change private, public, protected into either a qualifier or label */
      if (pc->type == CT_PRIVATE)
      {
         if (next->type == CT_COLON)
         {
            next->type = CT_PRIVATE_COLON;
            if ((tmp = chunk_get_next_ncnl(next)) != NULL)
            {
               tmp->flags |= PCF_STMT_START | PCF_EXPR_START;
            }
         }
         else
         {
            pc->type = CT_QUALIFIER;
         }
      }

      /* Look for <newline> 'EXEC' 'SQL' */
      if (chunk_is_str(pc, "EXEC", 4) && chunk_is_str(next, "SQL", 3))
      {
         tmp = chunk_get_prev(pc);
         if (chunk_is_newline(tmp))
         {
            tmp = chunk_get_next(next);
            if (chunk_is_str(tmp, "BEGIN", 5))
            {
               pc->type = CT_SQL_BEGIN;
            }
            else if (chunk_is_str(tmp, "END", 3))
            {
               pc->type = CT_SQL_END;
            }
            else
            {
               pc->type = CT_SQL_EXEC;
            }

            /* Change words into CT_SQL_WORD until CT_SEMICOLON */
            while (tmp != NULL)
            {
               if (tmp->type == CT_SEMICOLON)
               {
                  break;
               }
               if ((tmp->len > 0) && isalpha(*tmp->str))
               {
                  tmp->type = CT_SQL_WORD;
               }
               tmp = chunk_get_next_ncnl(tmp);
            }
         }
      }

      /* Detect Objective C class name */
      if ((pc->type == CT_OC_IMPL) || (pc->type == CT_OC_INTF))
      {
         next->type = CT_CLASS;
         tmp = chunk_get_next_ncnl(next);
         if (tmp != NULL)
         {
            tmp->flags |= PCF_STMT_START | PCF_EXPR_START;
         }
      }

      /* TODO: determine other stuff here */

      prev = pc;
      pc   = next;
      next = chunk_get_next_ncnl(pc);
   }
}


/**
 * If there is nothing but CT_WORD and CT_MEMBER, then it's probably a
 * template thingy.  Otherwise, it's likely a comparison.
 */
static void check_template(chunk_t *start)
{
   chunk_t *pc;
   chunk_t *end;
   chunk_t *prev;

   LOG_FMT(LTEMPL, "%s: Line %d, col %d:", __func__, start->orig_line, start->orig_col);

   prev = chunk_get_prev_ncnl(start);

   if ((prev != NULL) && (prev->type == CT_TEMPLATE))
   {
      LOG_FMT(LTEMPL, " CT_TEMPLATE:");

      int level = 1;
      for (pc = chunk_get_next_ncnl(start); pc != NULL; pc = chunk_get_next_ncnl(pc))
      {
         LOG_FMT(LTEMPL, " [%s,%d]", get_token_name(pc->type), level);

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
      /* Scan backwards until we hit a semicolon, start of file, or brace open/close */
      do
      {
         prev = chunk_get_prev_ncnl(prev);
      } while ((prev != NULL) && ((prev->type == CT_WORD) ||
                                  (prev->type == CT_TYPE) ||
                                  (prev->type == CT_DC_MEMBER)));

      if (prev != NULL)
      {
         if (chunk_is_str(prev, "(", 1) ||
             chunk_is_str(prev, "[", 1) ||
             (prev->type == CT_ASSIGN) ||
             (prev->type == CT_BOOL))
         {
            LOG_FMT(LTEMPL, " - after %s - Not a template\n", get_token_name(prev->type));
            start->type = CT_COMPARE;
            return;
         }
         LOG_FMT(LTEMPL, " - prev %s -", get_token_name(prev->type));
      }

      int level = 1;
      for (pc = chunk_get_next_ncnl(start); pc != NULL; pc = chunk_get_next_ncnl(pc))
      {
         LOG_FMT(LTEMPL, " [%s,%d]", get_token_name(pc->type), level);

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
         else if ((pc->type != CT_WORD) &&
                  (pc->type != CT_TYPE) &&
                  (pc->type != CT_MEMBER) &&
                  (pc->type != CT_COMMA) &&
                  (pc->type != CT_STAR) &&
                  (pc->type != CT_CLASS) &&
                  (pc->type != CT_DC_MEMBER))
         {
            break;
         }
      }
      end = pc;
   }

   if (end != NULL)
   {
      if (end->type == CT_ANGLE_CLOSE)
      {
         LOG_FMT(LTEMPL, " - Template Detected\n");

         for (pc = start; pc != end; pc = chunk_get_next_ncnl(pc))
         {
            pc->parent_type = CT_TEMPLATE;
            make_type(pc);
         }
         end->parent_type = CT_TEMPLATE;
      }
      else
      {
         LOG_FMT(LTEMPL, " - Not a template: end = %s\n", get_token_name(end->type));

         start->type = CT_COMPARE;
      }
   }
}
