/**
 * @file c_combine.c
 * Labels the chunks as needed.
 *
 * $Id$
 */
#include "cparse_types.h"
#include "chunk_list.h"
#include "chunk_stack.h"
#include "prototypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

static void fix_fcn_def_params(chunk_t *pc);
static void fix_typedef(chunk_t *pc);
static void fix_enum_struct_union(chunk_t *pc);
static void fix_casts(chunk_t *pc);
static void fix_var_def(chunk_t *pc);
static void mark_function(chunk_t *pc);
static void mark_struct_union_body(chunk_t *start);
static chunk_t *mark_variable_definition(chunk_t *start);
static void mark_define_expressions(void);
static void process_returns(void);
static chunk_t *process_return(chunk_t *pc);



/**
 * Flags everything from the open paren to the close paren.
 *
 * @param po   Pointer to the open parenthesis
 */
static void flag_parens(chunk_t *po, uint16_t flags,
                        c_token_t opentype, c_token_t parenttype,
                        BOOL parent_all)
{
   chunk_t *paren_close;
   chunk_t *pc;


   //   po->flags  |= flags;
   paren_close = chunk_skip_to_match(po);
   if (paren_close != NULL)
   {
      if (po == paren_close)
      {
         //         fprintf(stderr, "Called on %s on line %d\n",
         //                 get_token_name(po->type), po->orig_line);
         return;
      }
      for (pc = chunk_get_next(po); pc != paren_close; pc = chunk_get_next(pc))
      {
         pc->flags |= flags;
         if (parent_all)
         {
            pc->parent_type = parenttype;
         }
      }
      //      paren_close->flags |= flags;

      if (opentype != CT_NONE)
      {
         po->type          = opentype;
         paren_close->type = opentype + 1;
      }

      if (parenttype != CT_NONE)
      {
         po->parent_type          = parenttype;
         paren_close->parent_type = parenttype;
      }

      //      fprintf(stderr, "%s: from line %d to line %d flg=%x\n", __func__,
      //              po->orig_line, paren_close->orig_line, flags);
   }
}


/**
 * Change CT_INCDEC_AFTER + WORD to CT_INCDEC_BEFORE
 * Change number/word + CT_ADDR to CT_ARITH
 * Change number/word + CT_STAR to CT_ARITH
 * Change number/word + CT_NEG to CT_ARITH
 * Change word + ( to a CT_FUNCTION
 * Cahnge struct/union/enum + CT_WORD => CT_TYPE
 * Force parens on return.
 *
 * TODO: This could be done earlier.
 *
 * Patterns detected:
 *   STRUCT/ENUM/UNION + WORD :: WORD => TYPE
 *   WORD + '('               :: WORD => FUNCTION
 */
void fix_symbols(void)
{
   chunk_t *pc;
   chunk_t *next;
   chunk_t *prev;
   chunk_t *tmp;
   chunk_t dummy;


   process_returns();

   mark_define_expressions();

   memset(&dummy, 0, sizeof(dummy));

   prev = &dummy;
   pc   = chunk_get_head();
   next = chunk_get_next_ncnl(pc);

   while ((pc != NULL) && (next != NULL))
   {
      if ((pc->type == CT_WORD) && (next->type == CT_PAREN_OPEN))
      {
         pc->type = CT_FUNCTION;
      }

      /**
       * A word before an open paren is a function call or definition.
       * CT_WORD => CT_FUNC_CALL or CT_FUNC_DEF
       */
      if (pc->type == CT_FUNCTION)
      {
         mark_function(pc);
      }

      /* Check for stuff that can only occur at the start of an statement */
      if ((pc->flags & PCF_STMT_START) != 0)
      {
         /* Mark variable definitions */
      }

      /* Check for stuff that can only occur at the start of an expression */
      if ((pc->flags & PCF_EXPR_START) != 0)
      {
         /**
          * Check a paren pair to see if it is a cast.
          * Note that SPAREN and FPAREN have already been marked.
          */
         if ((pc->type == CT_PAREN_OPEN) &&
             ((next->type == CT_WORD) ||
              (next->type == CT_TYPE) ||
              (next->type == CT_STRUCT) ||
              (next->type == CT_ENUM) ||
              (next->type == CT_UNION)) &&
             (prev->type != CT_SIZEOF) &&
             (prev->type != CT_TYPE)) /*TODO: why check for CT_TYPE? */
         {
            fix_casts(pc);
         }

         /* Change STAR, MINUS, and PLUS in the easy cases */
         if (pc->type == CT_STAR)
         {
            pc->type = CT_DEREF;
         }
         if (pc->type == CT_MINUS)
         {
            pc->type = CT_NEG;
         }
         if (pc->type == CT_PLUS)
         {
            pc->type = CT_POS;
         }
         if (pc->type == CT_INCDEC_AFTER)
         {
            pc->type = CT_INCDEC_BEFORE;
            //fprintf(stderr, "%s: %d> changed INCDEC_AFTER to INCDEC_BEFORE\n", __func__, pc->orig_line);
         }
         if (pc->type == CT_AMP)
         {
            //fprintf(stderr, "Changed AMP to ADDR on line %d\n", pc->orig_line);
            pc->type = CT_ADDR;
         }
      }

      /* Detect a variable definition that starts with struct/enum/union */
      if ((prev->type != CT_TYPEDEF) &&
          (prev->parent_type != CT_CAST) &&
          ((prev->flags & PCF_IN_FCN_DEF) == 0) &&
          ((pc->type == CT_STRUCT) ||
           (pc->type == CT_UNION) ||
           (pc->type == CT_ENUM)))
      {
         tmp = next;
         if (tmp->type == CT_TYPE)
         {
            tmp = chunk_get_next_ncnl(tmp);
         }
         if ((tmp != NULL) && (tmp->type == CT_BRACE_OPEN))
         {
            tmp = chunk_skip_to_match(tmp);
            tmp = chunk_get_next_ncnl(tmp);
         }
         if ((tmp != NULL) && (chunk_is_star(tmp) || (tmp->type == CT_WORD)))
         {
            //            fprintf(stderr, "%s:%d mark_variable_definition\n", __func__, __LINE__);
            mark_variable_definition(tmp);
         }
      }

      /**
       * Change the paren pair after a function/macrofunc.
       * CT_PAREN_OPEN => CT_FPAREN_OPEN
       */
      if (pc->type == CT_MACRO_FUNC)
      {
         flag_parens(next, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_MACRO_FUNC, FALSE);
      }

      /* Change CT_STAR to CT_PTR_TYPE or CT_ARITH or SYM_DEREF */
      if (pc->type == CT_STAR)
      {
         pc->type = CT_ARITH;
      }

      if (pc->type == CT_AMP)
      {
         pc->type = CT_ARITH;
      }

      if ((pc->type == CT_MINUS) ||
          (pc->type == CT_PLUS))
      {
         pc->type = CT_ARITH;
      }

      prev = pc;
      pc   = next;
      next = chunk_get_next_ncnl(next);
   }

   /**
    * 2nd pass - handle typedef, struct, enum, union
    * We need function params marked for those.
    */
   prev = &dummy;
   pc   = chunk_get_head();

   while (pc != NULL)
   {
      if (prev->type == CT_TYPEDEF)
      {
         fix_typedef(prev);
      }
      else
      {
         if ((pc->type == CT_ENUM) ||
             (pc->type == CT_STRUCT) ||
             (pc->type == CT_UNION))
         {
            fix_enum_struct_union(pc);
         }
      }


      /**
       * A variable definition is possible after at the start of a statement
       * that starts with: QUALIFIER, TYPE, or WORD
       */
      if (((pc->flags & PCF_STMT_START) != 0) &&
          ((pc->type == CT_QUALIFIER) ||
           (pc->type == CT_TYPE) ||
           (pc->type == CT_WORD)) &&
          (pc->parent_type != CT_ENUM)) // TODO: why this check?
      {
         fix_var_def(pc);
      }

      prev = pc;
      pc   = chunk_get_next_ncnl(pc);
   }

   /* 3rd pass - flag comments */
   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if ((pc->type == CT_COMMENT) || (pc->type == CT_COMMENT_CPP))
      {
         prev = chunk_get_prev(pc);
         next = chunk_get_next(pc);

         if (!chunk_is_newline(prev) &&
             ((next == NULL) || (next->type == CT_NEWLINE)))
         {
            pc->flags |= PCF_RIGHT_COMMENT;
         }
      }
   }
}

static void process_returns(void)
{
   chunk_t *pc;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      if (pc->type != CT_RETURN)
      {
         pc = chunk_get_next_type(pc, CT_RETURN, -1);
         continue;
      }

      pc = process_return(pc);
   }
}

/**
 * Processes a return statment, labeling the parens and marking the parent.
 * May remove or add parens around the return statement
 *
 * @param pc   Pointer to the return chunk
 */
static chunk_t *process_return(chunk_t *pc)
{
   chunk_t *next;
   chunk_t *temp;
   chunk_t *semi;
   chunk_t *cpar;
   chunk_t chunk;


   /* grab next and bail if it is a semicolon */
   next = chunk_get_next_ncnl(pc);
   if ((next == NULL) || (next->type == CT_SEMICOLON))
   {
      return(next);
   }

   if (next->type == CT_PAREN_OPEN)
   {
      /* See if the return is fully paren'd */
      cpar = chunk_get_next_type(next, CT_PAREN_CLOSE, next->level);
      semi = chunk_get_next_ncnl(cpar);
      if ((semi != NULL) && (semi->type == CT_SEMICOLON))
      {
         if (cpd.settings[UO_mod_paren_on_return] == AV_REMOVE)
         {
            LOG_FMT(LRETURN, "%s: removing parens on line %d\n",
                    __func__, pc->orig_line);

            /* lower the level of everything */
            for (temp = next; temp != cpar; temp = chunk_get_next(temp))
            {
               temp->level--;
            }

            /* delete the parens */
            chunk_del(next);
            chunk_del(cpar);

            /* back up the semicolon */
            semi->column--;
            semi->orig_col--;
            semi->orig_col_end--;
         }
         else
         {
            LOG_FMT(LRETURN, "%s: keeping parens on line %d\n",
                    __func__, pc->orig_line);

            /* mark & keep them */
            next->parent_type = CT_RETURN;
            cpar->parent_type = CT_RETURN;
         }
         return(semi);
      }
   }

   /* We don't have a fully paren'd return. Should we add some? */
   if ((cpd.settings[UO_mod_paren_on_return] & AV_ADD) == 0)
   {
      return(next);
   }

   /* find the next semicolon on the same level */
   semi = chunk_get_next_type(next, CT_SEMICOLON, next->level);
   if (semi != NULL)
   {
      /* add the parens */
      memset(&chunk, 0, sizeof(chunk));
      chunk.type        = CT_PAREN_OPEN;
      chunk.str         = "(";
      chunk.len         = 1;
      chunk.level       = pc->level;
      chunk.brace_level = pc->brace_level;
      chunk.orig_line   = pc->orig_line;
      chunk.parent_type = CT_RETURN;
      chunk.flags       = pc->flags & PCF_COPY_FLAGS;
      chunk_add_after(&chunk, pc);

      chunk.type      = CT_PAREN_CLOSE;
      chunk.str       = ")";
      chunk.orig_line = semi->orig_line;
      cpar            = chunk_add_before(&chunk, semi);

      LOG_FMT(LRETURN, "%s: added parens on line %d\n",
              __func__, pc->orig_line);

      for (temp = next; temp != cpar; temp = chunk_get_next(temp))
      {
         temp->level++;
      }
   }
   return(semi);
}


BOOL is_ucase_str(const char *str)
{
   while (isupper(*str))
   {
      str++;
   }
   return((*str == 0) ? TRUE : FALSE);
}

/**
 * Checks to see if the current paren is part of a cast.
 * We already verified that this doesn't follow function, TYPE, IF, FOR,
 * SWITCH, or WHILE and is followed by WORD, TYPE, STRUCT, ENUM, or UNION.
 *
 * @param start   Pointer to the open paren
 */
static void fix_casts(chunk_t *start)
{
   chunk_t    *pc;
   chunk_t    *first;
   chunk_t    *last = NULL;
   chunk_t    *paren_close;
   const char *verb      = "likely";
   int        count      = 0;
   int        word_count = 0;


   /* Make sure there is only WORD, TYPE, and '*' before the close paren */
   pc    = chunk_get_next_ncnl(start);
   first = pc;
   while ((pc != NULL) && (chunk_is_type(pc) ||
                           (pc->type == CT_WORD) ||
                           (pc->type == CT_QUALIFIER) ||
                           (pc->type == CT_STAR)))
   {
      last = pc;
      pc   = chunk_get_next_ncnl(pc);
      if (pc->type == CT_WORD)
      {
         word_count++;
      }
      count++;
   }

   if ((pc == NULL) || (pc->type != CT_PAREN_CLOSE))
   {
      LOG_FMT(LCASTS, "%s: not a cast on line %d\n", __func__, start->orig_line);
      return;
   }

   if (word_count > 1)
   {
      LOG_FMT(LCASTS, "%s: too many words %d on line %d\n", __func__,
              word_count, start->orig_line);
      return;
   }
   paren_close = pc;

   /* If last is a type or star, we have a cast for sure */
   if ((last->type == CT_STAR) ||
       (last->type == CT_PTR_TYPE) ||
       (last->type == CT_TYPE))
   {
      verb = "for sure";
   }
   else if (count == 1)
   {
      /**
       * We are on a potential cast of the form "(word)".
       *
       * For this to be a cast, the close paren must be followed by:
       *  - constant (number or string)
       *  - paren open
       *  - word
       *
       * It MIGHT be a cast if followed by '*' or '&'
       * It the word is all CAPS or ends in "_t", we assume it is a cast.
       */
      pc = chunk_get_next_ncnl(paren_close);
      if (chunk_is_star(pc) || chunk_is_addr(pc))
      {
         verb = "guessed";
         /* for now, call it a cast if it ends in '_t' */
         if ((last->len > 3) &&
             (last->str[last->len - 2] == '_') &&
             (last->str[last->len - 1] == 't'))
         {
            LOG_FMT(LCASTS, "%s: guessed cast (%s) on line %d -- '_t'\n",
                    __func__, last->str, start->orig_line);
         }
         else if (is_ucase_str(last->str))
         {
            LOG_FMT(LCASTS, "%s: guessed cast (%s) on line %d -- isupper()\n",
                    __func__, last->str, start->orig_line);
         }
         else
         {
            LOG_FMT(LCASTS, "%s: unlikely cast (%s) on line %d\n",
                    __func__, last->str, start->orig_line);
            return;
         }
      }
      else if ((pc->type != CT_NUMBER) &&
               (pc->type != CT_WORD) &&
               (pc->type != CT_PAREN_OPEN) &&
               (pc->type != CT_STRING))
      {
         LOG_FMT(LCASTS, "%s: not a cast on line %d - followed by %s\n",
                 __func__, start->orig_line, pc->str);
         return;
      }
   }

   start->parent_type       = CT_CAST;
   paren_close->parent_type = CT_CAST;

   LOG_FMT(LCASTS, "%s: %s cast on line %d: (", __func__, verb, start->orig_line);

   for (pc = first; pc != paren_close; pc = chunk_get_next_ncnl(pc))
   {
      pc->parent_type = CT_CAST;
      if (pc->type == CT_WORD)
      {
         pc->type = CT_TYPE;
      }
      if (pc->type == CT_STAR)
      {
         pc->type = CT_PTR_TYPE;
      }
      LOG_FMT(LCASTS, " %s", pc->str);
   }
   LOG_FMT(LCASTS, " )\n");

   /* Mark the next item as an expression start */
   pc = chunk_get_next_ncnl(paren_close);
   if (pc != NULL)
   {
      pc->flags |= PCF_EXPR_START;
   }
}


/**
 * We are on an enum/struct/union tag that does NOT follow a typedef tag.
 * If there is a {...} and words before the ';', then they are variables.
 *
 * tag { ... } [*] word [, [*]word] ;
 * tag [word/type] { ... } [*] word [, [*]word] ;
 * tag [word/type] [word]; -- this gets caught later.
 * fcn(tag [word/type] [word])
 * a = (tag [word/type] [*])&b;
 */
static void fix_enum_struct_union(chunk_t *pc)
{
   chunk_t *next;
   int     flags        = PCF_VAR_1ST_DEF;
   int     in_fcn_paren = pc->flags & PCF_IN_FCN_DEF;

   /* Make sure this wasn't a cast */
   if (pc->parent_type == CT_CAST)
   {
      return;
   }

   /* the next item is either a type or open brace */
   next = chunk_get_next_ncnl(pc);
   if (next->type == CT_TYPE)
   {
      next = chunk_get_next_ncnl(next);
   }
   if (next->type == CT_BRACE_OPEN)
   {
      if ((pc->type == CT_UNION) || (pc->type == CT_STRUCT))
      {
         mark_struct_union_body(next);
      }

      /* Skip to the closing brace */
      next->parent_type = pc->type;
      next              = chunk_get_next_type(next, CT_BRACE_CLOSE, pc->level);
      flags            |= PCF_VAR_INLINE;
      if (next != NULL)
      {
         next->parent_type = pc->type;
      }
      next = chunk_get_next_ncnl(next);
   }

   if (next->type == CT_PAREN_CLOSE)
   {
      return;
   }

   /* We are either pointing to a ';' or a variable */
   while ((next != NULL) && (next->type != CT_SEMICOLON) &&
          (next->type != CT_ASSIGN) &&
          ((in_fcn_paren ^ (next->flags & PCF_IN_FCN_DEF)) == 0))
   {
      //      fprintf(stderr, "%s: checking %s on line %d col %d\n",
      //              __func__, next->str, next->orig_line, next->orig_col);

      if (next->type == CT_WORD)
      {
         next->flags |= flags;
         flags       &= ~PCF_VAR_1ST;/* clear the first flag for the next items */

         //         fprintf(stderr, "%s: var %s on line %d\n", __func__,
         //                 next->str, next->orig_line);
      }

      if (next->type == CT_STAR)
      {
         next->type = CT_PTR_TYPE;
      }

      /* If we hit a comma in a function param, we are done */
      if (((next->type == CT_COMMA) ||
           (next->type == CT_FPAREN_CLOSE)) &&
          ((next->flags & (PCF_IN_FCN_DEF | PCF_IN_FCN_CALL)) != 0))
      {
         return;
      }

      next = chunk_get_next_ncnl(next);
   }
}

/**
 * We are on a typedef.
 * If the next word is not enum/union/struct, then the last word before the
 * next ',' or ';' is a type.
 *
 * typedef [type...] [*] type [, [*]type] ;
 * typedef <enum/struct/union> [type] [*] type [, [*]type] ;
 * typedef <enum/struct/union> [type] { ... } [*] type [, [*]type] ;
 */
static void fix_typedef(chunk_t *start)
{
   chunk_t   *next;
   c_token_t tag;

   /* the next item should be enum/struct/union or a type */
   next = chunk_get_next_ncnl(start);
   if ((next->type == CT_ENUM) ||
       (next->type == CT_STRUCT) ||
       (next->type == CT_UNION))
   {
      tag = next->type;
      /* the next item should be either a word or { */
      next = chunk_get_next_ncnl(next);
      if ((next->type == CT_WORD) || (next->type == CT_TYPE))
      {
         next = chunk_get_next_ncnl(next);
      }
      if (next->type == CT_BRACE_OPEN)
      {
         next->parent_type = tag;
         /* Skip to the closing brace */
         next = chunk_get_next_type(next, CT_BRACE_CLOSE, next->level);
         if (next != NULL)
         {
            next->parent_type = tag;
         }
      }

      /* now step to the first type part */
      next = chunk_get_next_ncnl(next);
   }
   else
   {
      /* now step to the first type part */
      next = chunk_get_next_ncnl(start);
   }

   /* Change everything up the semi into a type */
   while ((next != NULL) && (next->type != CT_SEMICOLON))
   {
      if (next->type == CT_STAR)
      {
         next->type = CT_PTR_TYPE;
      }
      if (next->type == CT_WORD)
      {
         next->type = CT_TYPE;
      }
      //      fprintf(stderr, "%s: type %s on line %d\n", __func__, next->str, next->orig_line);
      next = chunk_get_next_ncnl(next);
   }
}


/**
 * Examines the whole file and changes CT_COLON to
 * CT_Q_COLON, CT_LABEL_COLON, or CT_CASE_COLON.
 * It also changes the CT_WORD before CT_LABEL_COLON into CT_LABEL.
 */
void combine_labels(void)
{
   chunk_t *cur;
   chunk_t *prev;
   chunk_t *next;
   int     question_count = 0;
   BOOL    hit_case       = FALSE;

   prev = chunk_get_head();
   cur  = chunk_get_next_nc(prev);
   next = chunk_get_next_nc(cur);

   /* unlikely that the file will start with a label... */
   while (next != NULL)
   {
      if (next->type == CT_QUESTION)
      {
         question_count++;
      }
      else if (next->type == CT_CASE)
      {
         hit_case = TRUE;
      }
      else if (next->type == CT_COLON)
      {
         if (question_count > 0)
         {
            next->type = CT_Q_COLON;
            question_count--;
         }
         else if (hit_case)
         {
            hit_case   = FALSE;
            next->type = CT_CASE_COLON;
         }
         else
         {
            if (cur->type == CT_WORD)
            {
               if (chunk_is_newline(prev))
               {
                  cur->type  = CT_LABEL;
                  next->type = CT_LABEL_COLON;
               }
               else
               {
                  next->type = CT_BIT_COLON;
               }
            }
            else if (next->level > next->brace_level)
            {
               /* ignore it, as it is inside a paren */
            }
            else if (cur->type == CT_TYPE)
            {
               /* ignore it - anonymous bit field? */
            }
            else
            {
               LOG_FMT(LWARN, "%s: unexpected colon on line %d, col %d parent=%s l=%d bl=%d\n",
                       __func__, next->orig_line, next->orig_col,
                       get_token_name(next->parent_type),
                       next->level, next->brace_level);
            }
         }
      }
      prev = cur;
      cur  = next;
      next = chunk_get_next_nc(cur);
   }
}


static void mark_variable_stack(log_sev_t sev)
{
   chunk_t *var_name;
   chunk_t *word_type;

   /* throw out the last word and mark the rest */
   var_name = cs_pop(&cpd.cs);
   if (var_name != NULL)
   {
      LOG_FMT(LFCNP, "%s: parameter on line %d :",
              __func__, var_name->orig_line);

      while ((word_type = cs_pop(&cpd.cs)) != NULL)
      {
         LOG_FMT(LFCNP, " <%s>", word_type->str);
         word_type->type = CT_TYPE;
      }

      LOG_FMT(LFCNP, " [%s]\n", var_name->str);
      var_name->flags |= PCF_VAR_DEF;
   }
}

/**
 * Simply change any STAR to PTR_TYPE and WORD to TYPE
 */
static void fix_fcn_def_params(chunk_t *pc)
{
   LOG_FMT(LFCNP, "%s: %s on line %d\n", __func__, pc->str, pc->orig_line);

   cs_reset(&cpd.cs);

   while ((pc != NULL) && (pc->type != CT_FPAREN_CLOSE))
   {
      if (chunk_is_star(pc))
      {
         pc->type = CT_PTR_TYPE;
      }
      else if (pc->type == CT_WORD)
      {
         cs_push(&cpd.cs, pc);
      }
      else if (pc->type == CT_COMMA)
      {
         mark_variable_stack(LFCNP);
      }
      //if ((pc->type == CT_COMMA) && (prev != NULL))
      //{
      //   if (!chunk_is_star(prev))
      //   {
      //      LOG_FMT(LFCNP, "%s: marked %s on line %d\n", __func__,
      //              prev->str, prev->orig_line);
      //      prev->flags |= PCF_VAR_DEF;
      //   }
      //}
      //prev = pc;
      pc = chunk_get_next_ncnl(pc);
   }
   mark_variable_stack(LFCNP);
}


void make_type(chunk_t *pc)
{
   if (pc->type == CT_WORD)
   {
      pc->type = CT_TYPE;
   }
   else if (chunk_is_star(pc))
   {
      pc->type = CT_PTR_TYPE;
   }
}

//#define DEBUG_FIX_VAR_DEF

/**
 * We are on the start of a sequence that could be a var def
 *  - FPAREN_OPEN (parent == CT_FOR)
 *  - BRACE_OPEN
 *  - SEMICOLON
 *
 */
static void fix_var_def(chunk_t *start)
{
   chunk_t *pc = start;
   chunk_t *before_end;
   chunk_t *end;
   int     type_count = 0;

   LOG_FMT(LFVD, "%s: top[%d]", __func__, pc->orig_line);

   /* Scan for words and types and stars oh my! */
   before_end = pc;
   while ((pc->type == CT_TYPE) ||
          (pc->type == CT_WORD) ||
          (pc->type == CT_QUALIFIER) ||
          chunk_is_star(pc))
   {
      LOG_FMT(LFVD, " %s[%s]", pc->str, get_token_name(pc->type));
      type_count++;
      before_end = pc;
      pc         = chunk_get_next_ncnl(pc);
   }
   end = pc;

   LOG_FMT(LFVD, "\n");

   /* A single word can only be a type if followed by a function */
   if ((type_count == 1) && (end->type != CT_FUNC_DEF))
   {
      return;
   }

   /* Everything before a function def is a type */
   if (end->type == CT_FUNC_DEF)
   {
      for (pc = start; pc != end; pc = chunk_get_next_ncnl(pc))
      {
         make_type(pc);
      }
      return;
   }


   LOG_FMT(LFVD, "%s:%d TYPE : ", __func__, start->orig_line);
   for (pc = start; pc != before_end; pc = chunk_get_next_ncnl(pc))
   {
      make_type(pc);
      LOG_FMT(LFVD, " %s[%s]", pc->str, get_token_name(pc->type));
   }
   LOG_FMT(LFVD, "\n");

   /**
    * OK we have two or more items, mark types up to the end.
    */
   //   fprintf(stderr, "%s:%d mark_variable_definition\n", __func__, __LINE__);
   mark_variable_definition(before_end);
}


/**
 * Skips everything until a comma or semicolon at the same level.
 * Returns the semicolon, comma, or close brace/paren or NULL.
 */
static chunk_t *skip_expression(chunk_t *start)
{
   chunk_t *pc = start;

   while ((pc != NULL) && (pc->level >= start->level))
   {
      if ((pc->level == start->level) &&
          ((pc->type == CT_SEMICOLON) || (pc->type == CT_COMMA)))
      {
         return(pc);
      }
      pc = chunk_get_next_ncnl(pc);
   }
   return(pc);
}


/**
 * We are on the first word of a variable definition.
 * Mark all the variable names with PCF_VAR_1ST and PCF_VAR_DEF as appropriate.
 * Also mark any '*' encountered as a CT_PTR_TYPE.
 * Skip over []. Go until a ';' is hit.
 *
 * Example input:
 * int   a = 3, b, c = 2;              ## called with 'a'
 * foo_t f = {1, 2, 3}, g = {5, 6, 7}; ## called with 'f'
 * struct {...} *a, *b;                ## called with 'a' or '*'
 */
static chunk_t *mark_variable_definition(chunk_t *start)
{
   chunk_t *pc   = start;
   int     flags = PCF_VAR_1ST_DEF;

   if (start == NULL)
   {
      return(NULL);
   }

   //    fprintf(stderr, "%s:%d on var '%s'[%s] in col %d\n",
   //            __func__, start->orig_line, start->str,
   //            get_token_name(start->type), start->orig_col);

   pc = start;
   while ((pc != NULL) && (pc->type != CT_SEMICOLON) &&
          (pc->level >= start->level))
   {
      if (pc->type == CT_WORD)
      {
         pc->flags |= flags;
         flags     &= ~PCF_VAR_1ST;

         //         fprintf(stderr, "%s:%d marked '%s'[%s] in col %d\n",
         //                 __func__, pc->orig_line, pc->str,
         //                 get_token_name(pc->type), pc->orig_col);
      }
      else if (chunk_is_star(pc))
      {
         pc->type = CT_PTR_TYPE;

         //         fprintf(stderr, "%s:%d marked '%s'[%s] in col %d\n",
         //                 __func__, pc->orig_line, pc->str,
         //                 get_token_name(pc->type), pc->orig_col);
      }
      else if ((pc->type == CT_SQUARE_OPEN) || (pc->type == CT_ASSIGN))
      {
         pc = skip_expression(pc);
         continue;
      }
      pc = chunk_get_next_ncnl(pc);
   }
   return(pc);
}

/**
 * We are on a function word. we need to:
 *  - find out if this is a call or prototype or implementation
 *  - mark return type
 *  - mark parameter types
 *  - mark brace pair
 */
static void mark_function(chunk_t *pc)
{
   chunk_t *prev;
   chunk_t *next;
   chunk_t *tmp;
   chunk_t *paren_close;
   chunk_t *var = NULL;

   prev = chunk_get_prev_ncnlnp(pc);
   next = chunk_get_next_ncnlnp(pc);

   LOG_FMT(LFCN, "%s: %s[%s]\n", __func__, pc->str, get_token_name(pc->type));

   /**
    * Scan to see if this is a function variable def:
    * const struct bar * (*func)(param_list)
    * int (*foo)(void);
    * bar_t (word)(...);  <- flagged as a function call
    *
    * These need to be identified BEFORE checking for casts.
    *
    */
   /* point to the next item after the '(' */
   tmp = chunk_get_next_ncnlnp(next);
   /* Skip any leading '*' characters */
   while (chunk_is_star(tmp))
   {
      tmp = chunk_get_next_ncnlnp(tmp);
   }
   if ((tmp != NULL) && (tmp->type == CT_WORD))
   {
      var = tmp;
      tmp = chunk_get_next_ncnlnp(tmp);
      if ((tmp != NULL) && (tmp->type == CT_PAREN_CLOSE))
      {
         tmp = chunk_get_next_ncnl(tmp);
         if ((tmp != NULL) && (tmp->type == CT_PAREN_OPEN))
         {
            LOG_FMT(LFCN, "Detected func var %s on line %d col %d\n",
                    var->str, var->orig_line, var->orig_col);
            var->flags |= PCF_VAR_1ST_DEF;

            /* Mark parameters */
            flag_parens(tmp, PCF_IN_FCN_DEF, CT_FPAREN_OPEN, CT_NONE, FALSE);
            next = chunk_get_next_ncnlnp(tmp);
            if (next->level > tmp->level)
            {
               fix_fcn_def_params(next);
            }
            return;
         }
      }
   }

   /* Assume it is a function call */
   pc->type = CT_FUNC_CALL;
   while ((prev != NULL) &&
          ((prev->type == CT_TYPE) ||
           (prev->type == CT_WORD) ||
           chunk_is_star(prev)))
   {
      LOG_FMT(LFCN, "FCN_DEF due to %s[%s] ", prev->str, get_token_name(prev->type));

      pc->type = CT_FUNC_DEF;
      make_type(prev);
      prev = chunk_get_prev_ncnlnp(prev);
   }
   LOG_FMT(LFCN, "\n");

   if (pc->type != CT_FUNC_DEF)
   {
      flag_parens(next, PCF_IN_FCN_CALL, CT_FPAREN_OPEN, CT_NONE, FALSE);
   }
   else
   {
      flag_parens(next, PCF_IN_FCN_DEF, CT_FPAREN_OPEN, CT_NONE, FALSE);

      /* See if this is a prototype or implementation */
      paren_close = chunk_get_next_type(pc, CT_FPAREN_CLOSE, pc->level);
      tmp         = chunk_get_next_ncnl(paren_close);
      if ((tmp != NULL) && (tmp->type == CT_SEMICOLON))
      {
         pc->type = CT_FUNC_PROTO;
      }

      /* Mark parameters */
      tmp = chunk_get_next_ncnl(next);
      if (tmp->level > next->level)
      {
         fix_fcn_def_params(tmp);
      }

      /* Find the brace pair */
      if (pc->type == CT_FUNC_DEF)
      {
         BOOL on_first = TRUE;
         tmp           = chunk_get_next_ncnl(paren_close);
         while ((tmp != NULL) && (tmp->type != CT_BRACE_OPEN))
         {
            tmp->parent_type = CT_FUNC_DEF;
            if (tmp->type == CT_SEMICOLON)
            {
               on_first = TRUE;
            }
            else
            {
               tmp->flags |= PCF_OLD_FCN_PARAMS;
               on_first    = FALSE;
            }
            tmp = chunk_get_next_ncnl(tmp);
         }
         if ((tmp != NULL) && (tmp->type == CT_BRACE_OPEN))
         {
            tmp->parent_type = CT_FUNC_DEF;
            tmp              = chunk_skip_to_match(tmp);
            if (tmp != NULL)
            {
               tmp->parent_type = CT_FUNC_DEF;
            }
         }
      }
   }
}


/**
 * Examines the stuff between braces { }.
 * There should only be variable definitions.
 */
static void mark_struct_union_body(chunk_t *start)
{
   chunk_t *pc = start;
   chunk_t *first;
   chunk_t *last;

   //    fprintf(stderr, "%s: line %d %s\n",
   //            __func__, start->orig_line, get_token_name(start->type));

   while ((pc != NULL) &&
          (pc->level >= start->level) &&
          (pc->type != CT_BRACE_CLOSE))
   {
      if (pc->type == CT_SEMICOLON)
      {
         pc = chunk_get_next_ncnlnp(pc);
         continue;
      }
      //      fprintf(stderr, "%s: line %d '%s' [%s]\n",
      //              __func__, pc->orig_line, pc->str,
      //              get_token_name(pc->type));

      if ((pc->type == CT_STRUCT) || (pc->type == CT_UNION))
      {
         pc = chunk_get_next_ncnlnp(pc);
         if ((pc != NULL) && (pc->type != CT_BRACE_OPEN))
         {
            pc = chunk_get_next_ncnlnp(pc);
         }
         if ((pc != NULL) && (pc->type == CT_BRACE_OPEN))
         {
            mark_struct_union_body(pc);
            pc = chunk_skip_to_match(pc);
            pc = chunk_get_next_ncnlnp(pc);
         }
         if (pc != NULL)
         {
            pc = mark_variable_definition(pc);
         }
      }
      else
      {
         last  = NULL;
         first = pc;
         while ((pc != NULL) && ((pc->type == CT_TYPE) ||
                                 (pc->type == CT_WORD) ||
                                 chunk_is_star(pc)))
         {
            last = pc;
            pc   = chunk_get_next_ncnlnp(pc);
         }
         if (last != NULL)
         {
            for (pc = first; pc != last; pc = chunk_get_next_ncnlnp(pc))
            {
               make_type(pc);
            }
            pc = mark_variable_definition(last);
         }
         else
         {
            pc = chunk_get_next_ncnlnp(pc);
         }
      }
   }
}



/**
 * Sets the parent for comments.
 */
void mark_comments(void)
{
   chunk_t *cur;
   chunk_t *next;
   BOOL    prev_nl = TRUE;
   BOOL    next_nl;

   cur = chunk_get_head();

   while (cur != NULL)
   {
      next    = chunk_get_next(cur);
      next_nl = (next == NULL) || chunk_is_newline(next);

      if (chunk_is_comment(cur))
      {
         if (next_nl && prev_nl)
         {
            cur->parent_type = CT_COMMENT_WHOLE;
         }
         else if (next_nl)
         {
            cur->parent_type = CT_COMMENT_END;
         }
         else if (prev_nl)
         {
            cur->parent_type = CT_COMMENT_START;
         }
         else
         {
            cur->parent_type = CT_COMMENT_EMBED;
         }
      }

      prev_nl = chunk_is_newline(cur);
      cur     = next;
   }
}


static void mark_define_expressions(void)
{
   chunk_t *pc;
   chunk_t *prev;
   BOOL    in_define = FALSE;
   BOOL    first     = TRUE;

   pc   = chunk_get_head();
   prev = pc;

   while (pc != NULL)
   {
      if (!in_define)
      {
         if (pc->type == CT_PP_DEFINE)
         {
            in_define = TRUE;
            first     = TRUE;
         }
      }
      else
      {
         if (((pc->flags & PCF_IN_PREPROC) == 0) || (pc->type == CT_PREPROC))
         {
            in_define = FALSE;
         }
         else
         {
            if ((pc->type != CT_MACRO) &&
                (first ||
                 (prev->type == CT_PAREN_OPEN) ||
                 (prev->type == CT_ARITH) ||
                 (prev->type == CT_ASSIGN) ||
                 (prev->type == CT_COMPARE) ||
                 (prev->type == CT_RETURN) ||
                 (prev->type == CT_GOTO) ||
                 (prev->type == CT_PAREN_OPEN) ||
                 (prev->type == CT_FPAREN_OPEN) ||
                 (prev->type == CT_SPAREN_OPEN) ||
                 (prev->type == CT_BRACE_OPEN) ||
                 (prev->type == CT_SEMICOLON) ||
                 (prev->type == CT_COMMA) ||
                 (prev->type == CT_COLON) ||
                 (prev->type == CT_QUESTION)))
            {
               pc->flags |= PCF_EXPR_START;
               first      = FALSE;
            }
         }
      }

      prev = pc;
      pc   = chunk_get_next(pc);
   }
}

