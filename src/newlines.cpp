/**
 * @file newlines.cpp
 * Adds or removes newlines.
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

/*
 * Basic approach:
 * 1. Find next open brace
 * 2. Find next close brace
 * 3. Determine why the braces are there
 * a. struct/union/enum "enum [name] {"
 * c. assignment "= {"
 * b. if/while/switch/for/etc ") {"
 * d. else "} else {"
 */

//#define DEBUG_NEWLINES

/**
 * 2 parts:
 *  - if/switch/while/for after braces
 *  - else before/after braces
 *  - do after braces
 *  - do/while before braces
 */

chunk_t *newline_add_before(chunk_t *pc)
{
   chunk_t nl;

   //fprintf(stderr, "%s: %s line %d\n", __func__, pc->str, pc->orig_line);

   memset(&nl, 0, sizeof(nl));
   nl.nl_count = 1;
   nl.flags    = pc->flags & PCF_COPY_FLAGS;
   if ((pc->flags & PCF_IN_PREPROC) != 0)
   {
      nl.type = CT_NL_CONT;
      nl.str  = "\\\n";
      nl.len  = 2;
   }
   else
   {
      nl.type = CT_NEWLINE;
      nl.str  = "\n";
      nl.len  = 1;
   }

   return(chunk_add_before(&nl, pc));
}

chunk_t *newline_add_after(chunk_t *pc)
{
   chunk_t nl;
   chunk_t *next;

   next = chunk_get_next(pc);
   if ((next != NULL) && (next->type == CT_NEWLINE))
   {
      /* Already has a newline after this chunk */
      return(next);
   }

   //fprintf(stderr, "%s: %s line %d\n", __func__, pc->str, pc->orig_line);

   memset(&nl, 0, sizeof(nl));
   nl.nl_count = 1;
   nl.flags    = pc->flags & PCF_COPY_FLAGS;
   if ((pc->flags & PCF_IN_PREPROC) != 0)
   {
      nl.type = CT_NL_CONT;
      nl.str  = "\\\n";
      nl.len  = 2;
   }
   else
   {
      nl.type = CT_NEWLINE;
      nl.str  = "\n";
      nl.len  = 1;
   }

   return(chunk_add_after(&nl, pc));
}

#define newline_min_after(ref, cnt)     newline_min_after2(ref, cnt, __func__, __LINE__)

static void newline_min_after2(chunk_t *ref, INT32 count,
                               const char *func, int line)
{
   chunk_t *pc = ref;
   chunk_t *next;

   LOG_FMT(LNEWLINE, "%s: '%.*s' line %d - count %d : caller=%s:%d\n",
           __func__, ref->len, ref->str, ref->orig_line, count, func, line);

   do
   {
      pc = chunk_get_next(pc);
   } while ((pc != NULL) && !chunk_is_newline(pc));

   //LOG_FMT(LNEWLINE, "%s: on %s, line %d, col %d\n",
   //        __func__, get_token_name(pc->type), pc->orig_line, pc->orig_col);

   next = chunk_get_next(pc);
   if (chunk_is_comment(next) && (next->nl_count == 1) &&
       chunk_is_comment(chunk_get_prev(pc)))
   {
      newline_min_after(next, count);
      return;
   }
   else
   {
      if (chunk_is_newline(pc))
      {
         if (pc->nl_count < count)
         {
            pc->nl_count = count;
         }
      }
   }
}


/**
 * Adds a newline between the two chunks if there isn't one already.
 */
#define newline_add_between(start, end)     newline_add_between2(start, end, __func__, __LINE__)

static chunk_t *newline_add_between2(chunk_t *start, chunk_t *end,
                                     const char *func, int line)
{
   chunk_t *pc;

   if ((start == NULL) || (end == NULL))
   {
      return(NULL);
   }

   LOG_FMT(LNEWLINE, "%s: '%.*s' line %d and '%.*s' line %d : caller=%s:%d\n",
           __func__, start->len, start->str, start->orig_line,
           end->len, end->str, end->orig_line, func, line);

   /* Scan for a line break */
   for (pc = start; pc != end; pc = chunk_get_next(pc))
   {
      if ((pc->type == CT_NEWLINE) || (pc->type == CT_NL_CONT))
      {
         return(pc);
      }
   }

   return(newline_add_before(end));
}


/**
 * Removes any CT_NEWLINE or CT_NL_CONT between start and end.
 * Start must be before end on the chunk list.
 *
 * @param start   The starting chunk (cannot be a newline)
 * @param end     The ending chunk (cannot be a newline)
 */
#define newline_del_between(start, end) \
   newline_del_between2(start, end, __func__, __LINE__)

static void newline_del_between2(chunk_t *start, chunk_t *end,
                                 const char *func, int line)
{
   chunk_t *next;
   chunk_t *prev;
   chunk_t *pc = start;

   LOG_FMT(LNEWLINE, "%s: '%.*s' line %d and '%.*s' line %d : caller=%s:%d\n",
           __func__, start->len, start->str, start->orig_line,
           end->len, end->str, end->orig_line, func, line);

   do
   {
      next = chunk_get_next(pc);
      if (chunk_is_newline(pc))
      {
         prev = chunk_get_prev(pc);
         if ((prev->type != CT_COMMENT_CPP) &&
             (next->type != CT_COMMENT_CPP))
         {
            chunk_del(pc);
         }
         else if (chunk_is_newline(prev) ||
                  chunk_is_newline(next))
         {
            chunk_del(pc);
         }
         else
         {
            if (pc->nl_count > 1)
            {
               pc->nl_count = 1;
            }
         }
      }
      pc = next;
   } while (pc != end);

   if (chunk_is_str(end, "{", 1) &&
       (chunk_is_str(start, ")", 1) ||
        (start->type == CT_DO) ||
        (start->type == CT_ELSE)))
   {
      if (chunk_get_prev_nl(end) != start)
      {
         chunk_move_after(end, start);
      }
   }
}


/**
 * Add or remove a newline between the closing paren and opening brace.
 * Also uncuddles anything on the closing brace. (may get fixed later)
 *
 * "if (...) { \n" or "if (...) \n { \n"
 */
static void newlines_if_for_while_switch(chunk_t *start, argval_t nl_opt)
{
   chunk_t *pc;
   chunk_t *close_paren;
   chunk_t *brace_open;

   if ((nl_opt == AV_IGNORE) ||
       (((start->flags & PCF_IN_PREPROC) != 0) &&
        !cpd.settings[UO_nl_define_macro].b))
   {
      return;
   }

   pc = chunk_get_next_ncnl(start);
   if ((pc != NULL) && (pc->type == CT_SPAREN_OPEN))
   {
      close_paren = chunk_get_next_type(pc, CT_SPAREN_CLOSE, pc->level);
      brace_open  = chunk_get_next_ncnl(close_paren);

      if ((brace_open != NULL) && (brace_open->type == CT_BRACE_OPEN))
      {
         if (nl_opt & AV_ADD)
         {
            newline_add_between(close_paren, brace_open);
         }
         else if (nl_opt & AV_REMOVE)
         {
            newline_del_between(close_paren, brace_open);
         }

         newline_add_between(brace_open, chunk_get_next_ncnl(brace_open));

         /* Make sure nothing is cuddled with the closing brace */
         pc = chunk_get_next_type(brace_open, CT_BRACE_CLOSE, brace_open->level);
         newline_add_between(pc, chunk_get_next_nblank(pc));
      }
   }
}


/**
 * Adds or removes a newline between the keyword and the open brace.
 * If there is something after the '{' on the same line, then
 * the newline is removed unconditionally.
 *
 * "struct [name] {" or "struct [name] \n {"
 */
static void newlines_struct_enum_union(chunk_t *start, argval_t nl_opt)
{
   chunk_t *pc;
   chunk_t *next;

   if ((nl_opt == AV_IGNORE) ||
       (((start->flags & PCF_IN_PREPROC) != 0) &&
        !cpd.settings[UO_nl_define_macro].b))
   {
      return;
   }

   //fprintf(stderr, "%s(%s, %d)\n", __func__, start->str, nl_opt);

   /* step past any junk between the keyword and the open brace
    * Quit if we hit a semicolon, which is not expected.
    */
   int level = start->level;
   pc = start;
   while (((pc = chunk_get_next_ncnl(pc)) != NULL) && (pc->level >= level))
   {
      if ((pc->level == level) &&
          ((pc->type == CT_BRACE_OPEN) || chunk_is_semicolon(pc)))
      {
         break;
      }
      start = pc;
   }

   /* If we hit a brace open, then we need to toy with the newlines */
   if ((pc != NULL) && (pc->type == CT_BRACE_OPEN))
   {
      /* Skip over embedded C comments */
      next = chunk_get_next(pc);
      while ((next != NULL) && (next->type == CT_COMMENT))
      {
         next = chunk_get_next(next);
      }
      if (!chunk_is_comment(next) && !chunk_is_newline(next))
      {
         nl_opt = AV_IGNORE;
      }

      if ((nl_opt & AV_ADD) != 0)
      {
         newline_add_between(start, pc);
      }
      else if ((nl_opt & AV_REMOVE) != 0)
      {
         newline_del_between(start, pc);
      }
   }
}


/**
 * Cuddles or un-cuddles a chunk with a previous close brace
 *
 * "} while" vs "} \n while"
 * "} else" vs "} \n else"
 *
 * @param start   The chunk - should be CT_ELSE or CT_WHILE_OF_DO
 */
static void newlines_cuddle_uncuddle(chunk_t *start, argval_t nl_opt)
{
   chunk_t *br_close;

   if (((start->flags & PCF_IN_PREPROC) != 0) &&
       !cpd.settings[UO_nl_define_macro].b)
   {
      return;
   }

   br_close = chunk_get_prev_ncnl(start);
   if ((br_close != NULL) && (br_close->type == CT_BRACE_CLOSE))
   {
      /* remove before add - not exclusive! */
      if ((nl_opt & AV_REMOVE) != 0)
      {
         newline_del_between(br_close, start);
      }
      if ((nl_opt & AV_ADD) != 0)
      {
         newline_add_between(br_close, start);
      }
   }
}


/**
 * Adds/removes a newline between else and '{'.
 * "else {" or "else \n {"
 */
static void newlines_do_else(chunk_t *start, argval_t nl_opt)
{
   chunk_t *next;

   if ((nl_opt == AV_IGNORE) ||
       (((start->flags & PCF_IN_PREPROC) != 0) &&
        !cpd.settings[UO_nl_define_macro].b))
   {
      return;
   }

   next = chunk_get_next_ncnl(start);
   if ((next != NULL) && (next->type == CT_BRACE_OPEN))
   {
      if ((nl_opt & AV_ADD) != 0)
      {
         newline_add_between(start, next);
      }
      else if ((nl_opt & AV_REMOVE) != 0)
      {
         newline_del_between(start, next);
      }
   }
}

/**
 * We are at the open brace for a function body.
 * put a newline after the block of variable definitions
 */
static void newline_fnc_var_def(chunk_t *br_open, int nl_count)
{
   chunk_t *prev = NULL;
   chunk_t *pc;

   //printf("%s: line %d\n", __func__, br_open->orig_line);

   pc = chunk_get_next_ncnl(br_open);
   while (pc != NULL)
   {
      //printf("%s: [%s] line %d\n", __func__, pc->str, pc->orig_line);

      if (chunk_is_type(pc) || (pc->type == CT_QUALIFIER) || (pc->type == CT_DC_MEMBER))
      {
         //printf("%s: type [%s] line %d\n", __func__, pc->str, pc->orig_line);
         pc = chunk_get_next_ncnl(pc);
         continue;
      }

      if ((pc->type == CT_WORD) &&
          ((pc->flags & PCF_VAR_1ST) != 0))
      {
         pc   = chunk_get_next_type(pc, CT_SEMICOLON, pc->level);
         prev = pc;
         pc   = chunk_get_next_ncnl(pc);
         continue;
      }

      break;
   }

   /* prev is either NULL or points to a semicolon */
   if (prev != NULL)
   {
      //fprintf(stderr, "%s: inserting a newline after [%s] on line %d col%d\n",
      //        __func__, get_token_name(prev->type), prev->orig_line, prev->orig_col);
      newline_min_after(prev, 1 + cpd.settings[UO_nl_func_var_def_blk].n);
   }
}


/**
 * Handles the brace_on_func_line setting and decides if the closing brace
 * of a pair should be right after a newline.
 * The only case where the closing brace shouldn't be the first thing on a line
 * is where the opening brace has junk after it.
 *
 * General rule for break before close brace:
 * If the brace is part of a function (call or definition) OR if the only
 * thing after the opening brace is comments, the there must be a newline
 * before the close brace.
 *
 * Example of no newline before close
 * struct mystring { int  len;
 *                   char str[]; };
 * while (*(++ptr) != 0) { }
 *
 * Examples of newline before close
 * void foo() {
 * }
 *
 */
static void newlines_brace_pair(chunk_t *br_open)
{
   chunk_t  *prev;
   chunk_t  *next;
   chunk_t  *pc;
   argval_t val = AV_IGNORE;
   bool     nl_close_brace = false;

   if (((br_open->flags & PCF_IN_PREPROC) != 0) &&
       !cpd.settings[UO_nl_define_macro].b)
   {
      return;
   }

   next = chunk_get_next_nc(br_open);

   /** Insert a newline between the '=' and open brace, if needed */
   if (br_open->parent_type == CT_ASSIGN)
   {
      /* Only mess with it if the open brace is followed by a newline */
      if (chunk_is_newline(next))
      {
         prev = chunk_get_prev_ncnl(br_open);
         if ((cpd.settings[UO_nl_assign_brace].a & AV_ADD) != 0)
         {
            newline_add_between(prev, br_open);
         }
         else if ((cpd.settings[UO_nl_assign_brace].a & AV_REMOVE) != 0)
         {
            newline_del_between(prev, br_open);
         }
      }
   }

   /* Eat any extra newlines after the brace open */
   if (cpd.settings[UO_eat_blanks_after_open_brace].b)
   {
      if (chunk_is_newline(next))
      {
         if (next->nl_count > 1)
         {
            next->nl_count = 1;
         }
      }
   }

   /* Handle the cases where the brace is part of a function call or definition */
   if ((br_open->parent_type == CT_FUNC_DEF) ||
       (br_open->parent_type == CT_FUNC_CALL))
   {
      /* Need to force a newline before the close brace */
      nl_close_brace = true;

      /* handle newlines after the open brace */
      pc = chunk_get_next_ncnl(br_open);
      newline_add_between(br_open, pc);

      val = (br_open->parent_type == CT_FUNC_DEF) ?
            cpd.settings[UO_nl_fdef_brace].a :
            cpd.settings[UO_nl_fcall_brace].a;

      if (val != AV_IGNORE)
      {
         /* Grab the chunk before the open brace */
         prev = chunk_get_prev_ncnl(br_open);

         if (val & AV_ADD)
         {
            newline_add_between(prev, br_open);
         }
         else if (val & AV_REMOVE)
         {
            newline_del_between(prev, br_open);
         }
      }

      if (cpd.settings[UO_nl_func_var_def_blk].n > 0)
      {
         newline_fnc_var_def(br_open, cpd.settings[UO_nl_func_var_def_blk].n);
      }
   }

   /* Grab the matching brace close */
   chunk_t  *br_close;
   br_close = chunk_get_next_type(br_open, CT_BRACE_CLOSE, br_open->level);
   if (br_close == NULL)
   {
      return;
   }

   if (!nl_close_brace)
   {
      /**
       * If the open brace hits a CT_NEWLINE, CT_NL_CONT, CT_COMMENT_MULTI, or
       * CT_COMMENT_CPP without hitting anything other than CT_COMMENT, then
       * there should be a newline before the close brace.
       */
      pc = chunk_get_next(br_open);
      while ((pc != NULL) && (pc->type == CT_COMMENT))
      {
         pc = chunk_get_next(pc);
      }
      if (chunk_is_newline(pc) || chunk_is_comment(pc))
      {
         nl_close_brace = true;
      }
   }

   prev = chunk_get_prev_nblank(br_close);
   if (nl_close_brace)
   {
      newline_add_between(prev, br_close);
   }
   else
   {
      newline_del_between(prev, br_close);
   }

   if (cpd.settings[UO_eat_blanks_before_close_brace].b)
   {
      pc = chunk_get_prev_nc(br_close);
      if (chunk_is_newline(pc))
      {
         if (pc->nl_count > 1)
         {
            pc->nl_count = 1;
         }
      }
   }
}

/**
 * Put a empty line between the 'case' statement and the previous case colon
 * or semicolon.
 * Does not work with PAWN (?)
 */
static void newline_case(chunk_t *start)
{
   chunk_t *pc;
   chunk_t *prev = start;

   //   printf("%s case (%s) on line %d col %d\n",
   //          __func__, c_chunk_names[start->type],
   //          start->orig_line, start->orig_col);

   /* Scan backwards until a '{' or ';' or ':'. Abort if a multi-newline is found */
   do
   {
      prev = chunk_get_prev_nc(prev);
      if ((prev != NULL) && chunk_is_newline(prev) && (prev->nl_count > 1))
      {
         return;
      }
   } while ((prev != NULL) &&
            (prev->type != CT_BRACE_OPEN) &&
            (prev->type != CT_BRACE_CLOSE) &&
            (prev->type != CT_SEMICOLON) &&
            (prev->type != CT_CASE_COLON));

   if (prev == NULL)
   {
      return;
   }

   pc = newline_add_between(prev, start);
   if (pc == NULL)
   {
      return;
   }

   /* Only add an extra line after a semicolon or brace close */
   if ((prev->type == CT_SEMICOLON) ||
       (prev->type == CT_BRACE_CLOSE))
   {
      if (pc->nl_count < 2)
      {
         //         fprintf(stderr, "%s: newline before line %d\n",
         //                 __func__, start->orig_line);
         pc->nl_count = 2;
      }
   }
}

static void newline_case_colon(chunk_t *start)
{
   chunk_t *pc = start;

   /* Scan forwards until a non-comment is found */
   do
   {
      pc = chunk_get_next(pc);
   } while (chunk_is_comment(pc));

   if ((pc != NULL) && !chunk_is_newline(pc))
   {
      newline_add_before(pc);
   }
}


/**
 * Put a empty line after a return statement, unless it is followed by a
 * close brace.
 *
 * May not work with PAWN
 */
static void newline_return(chunk_t *start)
{
   chunk_t *pc;
   chunk_t *semi;
   chunk_t *after;

   semi  = chunk_get_next_type(start, CT_SEMICOLON, start->level);
   after = chunk_get_next_nblank(semi);

   /* If we hit a brace or an 'else', then a newline isn't needed */
   if ((after == NULL) ||
       (after->type == CT_BRACE_CLOSE) ||
       (after->type == CT_ELSE))
   {
      return;
   }

   for (pc = chunk_get_next(semi); pc != after; pc = chunk_get_next(pc))
   {
      if (pc->type == CT_NEWLINE)
      {
         if (pc->nl_count < 2)
         {
            pc->nl_count = 2;
         }
         return;
      }
   }
}


/**
 * Does a simple Ignore, Add, Remove, or Force after the given chunk
 *
 * @param pc   The chunk
 * @param av   The IARF value
 */
static void newline_iarf(chunk_t *pc, argval_t av)
{
   chunk_t *next;

   if ((av & AV_REMOVE) != 0)
   {
      next = chunk_get_next_nnl(pc);
      if (next != NULL)
      {
         newline_del_between(pc, next);
      }
   }

   if ((av & AV_ADD) != 0)
   {
      next = chunk_get_next_nnl(pc);
      if (next != NULL)
      {
         newline_add_between(pc, next);
      }
   }
}


/**
 * Formats a function declaration
 * Start points to the open paren
 */
static void newline_func_def(chunk_t *start)
{
   chunk_t *pc;
   chunk_t *prev = NULL;

   /* Handle break newlines type and function */
   if (cpd.settings[UO_nl_func_type_name].a != AV_IGNORE)
   {
      prev = chunk_get_prev_ncnl(start);
      prev = chunk_get_prev_ncnl(prev);
      if (prev != NULL)
      {
         newline_iarf(prev, cpd.settings[UO_nl_func_type_name].a);
      }
   }

   newline_iarf(start, cpd.settings[UO_nl_func_decl_start].a);

   /* Now scan for commas */
   for (pc = chunk_get_next_ncnl(start);
        (pc != NULL) && (pc->level > start->level);
        pc = chunk_get_next_ncnl(pc))
   {
      prev = pc;
      if ((pc->type == CT_COMMA) && (pc->level == (start->level + 1)))
      {
         newline_iarf(pc, cpd.settings[UO_nl_func_decl_args].a);
      }
   }

   /* and fix up the close paren */
   if ((prev != NULL) && (pc != NULL) && (pc->type == CT_FPAREN_CLOSE))
   {
      newline_iarf(prev, cpd.settings[UO_nl_func_decl_end].a);
   }
}


/**
 * Step through all chunks.
 */
void newlines_cleanup_braces(void)
{
   chunk_t *pc;
   chunk_t *next;
   argval_t arg;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->type == CT_IF)
      {
         newlines_if_for_while_switch(pc, cpd.settings[UO_nl_if_brace].a);
      }
      else if (pc->type == CT_ELSEIF)
      {
         arg = cpd.settings[UO_nl_elseif_brace].a;
         newlines_if_for_while_switch(
            pc, (arg != AV_IGNORE) ? arg : cpd.settings[UO_nl_if_brace].a);
      }
      else if (pc->type == CT_FOR)
      {
         newlines_if_for_while_switch(pc, cpd.settings[UO_nl_for_brace].a);
      }
      else if (pc->type == CT_WHILE)
      {
         newlines_if_for_while_switch(pc, cpd.settings[UO_nl_while_brace].a);
      }
      else if (pc->type == CT_SWITCH)
      {
         newlines_if_for_while_switch(pc, cpd.settings[UO_nl_switch_brace].a);
      }
      else if (pc->type == CT_DO)
      {
         newlines_do_else(pc, cpd.settings[UO_nl_do_brace].a);
      }
      else if (pc->type == CT_ELSE)
      {
         newlines_cuddle_uncuddle(pc, cpd.settings[UO_nl_brace_else].a);
         newlines_do_else(pc, cpd.settings[UO_nl_else_brace].a);
      }
      else if (pc->type == CT_WHILE_OF_DO)
      {
         newlines_cuddle_uncuddle(pc, cpd.settings[UO_nl_brace_while].a);
      }
      else if (pc->type == CT_BRACE_OPEN)
      {
         newlines_brace_pair(pc);
      }
      else if (pc->type == CT_STRUCT)
      {
         newlines_struct_enum_union(pc, cpd.settings[UO_nl_struct_brace].a);
      }
      else if (pc->type == CT_UNION)
      {
         newlines_struct_enum_union(pc, cpd.settings[UO_nl_union_brace].a);
      }
      else if (pc->type == CT_ENUM)
      {
         newlines_struct_enum_union(pc, cpd.settings[UO_nl_enum_brace].a);
      }
      else if (pc->type == CT_CASE)
      {
         /* Note: 'default' also maps to CT_CASE */
         if (cpd.settings[UO_nl_before_case].b)
         {
            newline_case(pc);
         }
      }
      else if (pc->type == CT_CASE_COLON)
      {
         if (cpd.settings[UO_nl_after_case].b)
         {
            newline_case_colon(pc);
         }
      }
      else if (pc->type == CT_SPAREN_CLOSE)
      {
         next = chunk_get_next_ncnl(pc);
         if ((next != NULL) && (next->type == CT_BRACE_OPEN))
         {
            if ((cpd.settings[UO_nl_fcall_brace].a & AV_ADD) != 0)
            {
               /*TODO: insert a brace? */
            }
            else if ((cpd.settings[UO_nl_fcall_brace].a & AV_REMOVE) != 0)
            {
               /*TODO: insert a brace? */
            }
         }
      }
      else if (pc->type == CT_RETURN)
      {
         if (cpd.settings[UO_nl_after_return].b)
         {
            newline_return(pc);
         }
      }
      else if (pc->type == CT_FPAREN_OPEN)
      {
         if (((pc->parent_type == CT_FUNC_DEF) ||
              (pc->parent_type == CT_FUNC_PROTO))
             &&
             ((cpd.settings[UO_nl_func_decl_start].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_decl_args].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_decl_end].a != AV_IGNORE) ||
              (cpd.settings[UO_nl_func_type_name].a != AV_IGNORE)))
         {
            newline_func_def(pc);
         }
      }
      else if (pc->type == CT_CLASS)
      {
         newlines_struct_enum_union(pc, cpd.settings[UO_nl_class_brace].a);
      }
      else if (pc->type == CT_NAMESPACE)
      {
         newlines_struct_enum_union(pc, cpd.settings[UO_nl_namespace_brace].a);
      }
      else
      {
         /* ignore it */
      }
   }
}

void newlines_squeeze_ifdef(void)
{
   chunk_t *pc;
   chunk_t *ppr;
   chunk_t *pnl;
   chunk_t *nnl;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if ((pc->type == CT_PREPROC) && (pc->level > 0))
      {
         ppr = chunk_get_next(pc);
         //          fprintf(stderr, "%s: %s online %d\n",
         //                  __func__, get_token_name(ppr->type), ppr->orig_line);

         if ((ppr->type == CT_PP_IF) ||
             (ppr->type == CT_PP_ELSE) ||
             (ppr->type == CT_PP_ENDIF))
         {
            pnl = NULL;
            nnl = chunk_get_next_nl(ppr);
            if (ppr->type == CT_PP_ENDIF)
            {
               pnl = chunk_get_prev_nl(pc);
            }

            if (nnl != NULL)
            {
               //                fprintf(stderr, "%s: next nl on line %d, count=%d\n",
               //                        __func__, nnl->orig_line, nnl->nl_count);

               if (pnl != NULL)
               {
                  //                  fprintf(stderr, "%s: prev nl on line %d, count=%d\n",
                  //                          __func__, pnl->orig_line, pnl->nl_count);
                  if (pnl->nl_count > 1)
                  {
                     nnl->nl_count += pnl->nl_count - 1;
                     pnl->nl_count  = 1;
                  }
               }
               else
               {
                  if (nnl->nl_count > 1)
                  {
                     nnl->nl_count = 1;
                  }
               }
            }
         }
      }
   }
}


void newlines_eat_start_end(void)
{
   chunk_t *pc;

   /* Process newlines at the start of the file */
   if (((cpd.settings[UO_nl_start_of_file].a & AV_REMOVE) != 0) ||
       (((cpd.settings[UO_nl_start_of_file].a & AV_ADD) != 0) &&
        (cpd.settings[UO_nl_start_of_file_min].n > 0)))
   {
      pc = chunk_get_head();
      if (pc != NULL)
      {
         if (pc->type == CT_NEWLINE)
         {
            if (cpd.settings[UO_nl_start_of_file].a == AV_REMOVE)
            {
               chunk_del(pc);
            }
            else if ((cpd.settings[UO_nl_start_of_file].a == AV_FORCE) ||
                     (pc->nl_count < cpd.settings[UO_nl_start_of_file_min].n))
            {
               pc->nl_count = cpd.settings[UO_nl_start_of_file_min].n;
            }
         }
         else if (((cpd.settings[UO_nl_start_of_file].a & AV_ADD) != 0) &&
                  (cpd.settings[UO_nl_start_of_file_min].n > 0))
         {
            chunk_t chunk;
            memset(&chunk, 0, sizeof(chunk));
            chunk.orig_line = pc->orig_line;
            chunk.type      = CT_NEWLINE;
            chunk.nl_count  = cpd.settings[UO_nl_start_of_file_min].n;
            chunk_add_before(&chunk, pc);
         }
      }
   }

   /* Process newlines at the end of the file */
   if (((cpd.settings[UO_nl_end_of_file].a & AV_REMOVE) != 0) ||
       (((cpd.settings[UO_nl_end_of_file].a & AV_ADD) != 0) &&
        (cpd.settings[UO_nl_end_of_file_min].n > 0)))
   {
      pc = chunk_get_tail();
      if (pc != NULL)
      {
         if (pc->type == CT_NEWLINE)
         {
            if (cpd.settings[UO_nl_end_of_file].a == AV_REMOVE)
            {
               chunk_del(pc);
            }
            else if ((cpd.settings[UO_nl_end_of_file].a == AV_FORCE) ||
                     (pc->nl_count < cpd.settings[UO_nl_end_of_file_min].n))
            {
               pc->nl_count = cpd.settings[UO_nl_end_of_file_min].n;
            }
         }
         else if (((cpd.settings[UO_nl_end_of_file].a & AV_ADD) != 0) &&
                  (cpd.settings[UO_nl_end_of_file_min].n > 0))
         {
            chunk_t chunk;
            memset(&chunk, 0, sizeof(chunk));
            chunk.orig_line = pc->orig_line;
            chunk.type      = CT_NEWLINE;
            chunk.nl_count  = cpd.settings[UO_nl_end_of_file_min].n;
            chunk_add(&chunk);
         }
      }
   }
}


/**
 * Searches for CT_BOOL (|| or && or ^^) operators and moves them, if needed.
 * Will not move CT_BOOL tokens that are on their own line or have other than
 * exactly 1 newline before (UO_pos_bool == TRAIL) or
 * after (UO_pos_bool == LEAD).
 */
void newlines_bool_pos(void)
{
   chunk_t    *pc;
   chunk_t    *next;
   chunk_t    *prev;
   tokenpos_e mode = cpd.settings[UO_pos_bool].tp;

   if (mode == TP_IGNORE)
   {
      return;
   }

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->type == CT_BOOL)
      {
         prev = chunk_get_prev(pc);
         next = chunk_get_next(pc);

         /* if both are newlines or neither are newlines, skip this chunk */
         if (chunk_is_newline(prev) == chunk_is_newline(next))
         {
            continue;
         }

         /*NOTE: may end up processing a chunk twice if changed */
         if (mode == TP_TRAIL)
         {
            if (chunk_is_newline(prev) && (prev->nl_count == 1))
            {
               chunk_swap(pc, prev);
            }
         }
         else  /* (mode == TP_LEAD) */
         {
            if (chunk_is_newline(next) && (next->nl_count == 1))
            {
               chunk_swap(pc, next);
            }
         }
      }
   }
}


/**
 * Searches for CT_CLASS_COLON and moves them, if needed.
 * Also breaks up the args
 */
void newlines_class_colon_pos(void)
{
   chunk_t    *pc;
   chunk_t    *next;
   chunk_t    *nextnext;
   chunk_t    *prev;
   tokenpos_e mode = cpd.settings[UO_pos_class_colon].tp;
   bool       in_class_init = false;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next_ncnl(pc))
   {
      if (!in_class_init && (pc->type != CT_CLASS_COLON))
      {
         continue;
      }

      if (pc->type == CT_CLASS_COLON)
      {
         in_class_init = true;
         prev = chunk_get_prev_nc(pc);
         next = chunk_get_next_nc(pc);

         if (!chunk_is_newline(prev) && !chunk_is_newline(next) &&
             ((cpd.settings[UO_nl_class_init_args].a & AV_ADD) != 0))
         {
            newline_add_after(pc);
            prev = chunk_get_prev_nc(pc);
            next = chunk_get_next_nc(pc);
         }

         if (mode == TP_TRAIL)
         {
            if (chunk_is_newline(prev) && (prev->nl_count == 1))
            {
               chunk_swap(pc, prev);
            }
         }
         else if (mode == TP_LEAD)
         {
            if (chunk_is_newline(next) && (next->nl_count == 1))
            {
               chunk_swap(pc, next);
            }
         }
      }
      else
      {
         if ((pc->type == CT_BRACE_OPEN) || (pc->type == CT_SEMICOLON))
         {
            in_class_init = false;
            continue;
         }

         if (pc->type == CT_COMMA)
         {
            if ((cpd.settings[UO_nl_class_init_args].a & AV_ADD) != 0)
            {
               newline_add_after(pc);
            }
            else if ((cpd.settings[UO_nl_class_init_args].a & AV_REMOVE) != 0)
            {
               next = chunk_get_next(pc);
               nextnext = chunk_get_next_ncnl(pc);
               if ((next != NULL) && (nextnext != NULL) &&
                   (next->type == CT_NEWLINE) &&
                   ((nextnext->type == CT_BRACE_OPEN) ||
                    (nextnext->type == CT_SEMICOLON)))
               {
                  chunk_del(next);
               }
            }
         }
      }
   }
}


/**
 * Scans for newline tokens and limits the nl_count.
 * A newline token has a minimum nl_count of 1.
 * Note that a blank line is actually 2 newlines, unless the newline is the
 * first chunk.  But we don't handle the first chunk.
 * So, most comparisons have +1 below.
 */
void do_blank_lines(void)
{
   chunk_t *pc;
   chunk_t *next;
   chunk_t *prev;

   /* Don't process the first token, as we don't care if it is a newline */
   pc = chunk_get_head();

   while ((pc = chunk_get_next(pc)) != NULL)
   {
      if (pc->type != CT_NEWLINE)
      {
         continue;
      }

      next = chunk_get_next(pc);
      prev = chunk_get_prev(pc);

      /* Limit consecutive newlines */
      if ((cpd.settings[UO_nl_max].n > 0) &&
          (pc->nl_count > (cpd.settings[UO_nl_max].n)))
      {
         pc->nl_count = cpd.settings[UO_nl_max].n;
      }

      /** Control blanks before multi-line comments */
      if ((cpd.settings[UO_nl_before_block_comment].n > pc->nl_count) &&
          (next != NULL) &&
          (next->type == CT_COMMENT_MULTI))
      {
         /* Don't add blanks after a open brace */
         if ((prev == NULL) || (prev->type != CT_BRACE_OPEN))
         {
            pc->nl_count = cpd.settings[UO_nl_before_block_comment].n;
         }
      }

      /* Add blanks after function bodies */
      if ((cpd.settings[UO_nl_after_func_body].n > pc->nl_count) &&
          (prev != NULL) &&
          (prev->type == CT_BRACE_CLOSE) &&
          (prev->parent_type == CT_FUNC_DEF))
      {
         pc->nl_count = cpd.settings[UO_nl_after_func_body].n;
      }

      /* Add blanks after function prototypes */
      if ((prev != NULL) &&
          (prev->type == CT_SEMICOLON) &&
          (prev->parent_type == CT_FUNC_PROTO))
      {
         if (cpd.settings[UO_nl_after_func_proto].n > pc->nl_count)
         {
            pc->nl_count = cpd.settings[UO_nl_after_func_proto].n;
         }
         if ((cpd.settings[UO_nl_after_func_proto_group].n > pc->nl_count) &&
             (next != NULL) &&
             (next->parent_type != CT_FUNC_PROTO))
         {
            pc->nl_count = cpd.settings[UO_nl_after_func_proto_group].n;
         }
      }
   }
}

void newlines_cleanup_dup(void)
{
   chunk_t    *pc;
   chunk_t    *next;

   pc = chunk_get_head();
   next = pc;

   while (pc != NULL)
   {
      next = chunk_get_next(next);
      if ((next != NULL) &&
          (pc->type == CT_NEWLINE) &&
          (next->type == CT_NEWLINE))
      {
         chunk_del(pc);
      }
      pc = next;
   }
}
