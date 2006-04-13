/**
 * @file c_align.c
 * Does all the aligning stuff.
 *
 * $Id$
 */
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "ChunkStack.h"
#include "prototypes.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <cctype>


static chunk_t *align_var_def_brace(chunk_t *pc, int span);
chunk_t *align_trailing_comments(chunk_t *start);
static void align_init_brace(chunk_t *start);

static void align_typedefs(int span);


/**
 * Aligns everything in the chunk stack to a particular column.
 * The stack is empty after this function.
 *
 * @param col           the column
 * @param align_single  align even if there is only one item on the stack
 */
static void align_stack(ChunkStack& cs, int col, bool align_single, log_sev_t sev)
{
   chunk_t *pc;

   if (cpd.settings[UO_align_on_tabstop].b)
   {
      int rem = (col - 1) % cpd.settings[UO_output_tab_size].n;
      if (rem != 0)
      {
         LOG_FMT(sev, "%s: col=%d rem=%d", __func__, col, rem);
         col += cpd.settings[UO_output_tab_size].n - rem;
      }
   }

   if ((cs.Len() > 1) || (align_single && (cs.Len() == 1)))
   {
      LOG_FMT(sev, "%s: max_col=%d\n", __func__, col);
      while ((pc = cs.Pop()) != NULL)
      {
         indent_to_column(pc, col);
         pc->flags |= PCF_WAS_ALIGNED;

         LOG_FMT(sev, "%s: indented [%s] on line %d to %d\n",
                 __func__, (pc->type == CT_NL_CONT) ? "NL_CONT" : pc->str,
                 pc->orig_line, pc->column);
      }
   }
   cs.Reset();
}


/**
 * Adds an item to the align stack and adjust the nl_count and max_col.
 * Adjust max_col as needed
 *
 * @param pc         the item to add
 * @param max_col    pointer to the column variable
 * @param extra_pad  extra padding
 */
static void align_add(ChunkStack& cs, chunk_t *pc, int *max_col, int min_pad)
{
   chunk_t *prev;
   int     min_col;

   prev = chunk_get_prev(pc);
   if ((prev == NULL) || chunk_is_newline(prev))
   {
      min_col = pc->column;
      LOG_FMT(LALADD, "%s: pc->col=%d max_col=%d min_pad=%d min_col=%d\n",
              __func__, pc->column, *max_col, min_pad, min_col);
   }
   else
   {
      min_col = prev->column + prev->len + 1 + min_pad;
      if (min_pad > 0)
      {
         min_col--;
      }
      if (min_col < pc->column)
      {
         min_col = pc->column;
      }
      LOG_FMT(LALADD, "%s: pc->col=%d max_col=%d min_pad=%d min_col=%d prev->col=%d\n",
              __func__, pc->column, *max_col, min_pad, min_col, prev->column);
   }

   if (cs.Empty())
   {
      *max_col = 0;
   }

   cs.Push(pc);
   if (min_col > *max_col)
   {
      *max_col = min_col;
   }
}


void align_all(void)
{
   if (cpd.settings[UO_align_typedef_span].n > 0)
   {
      align_typedefs(cpd.settings[UO_align_typedef_span].n);
   }

   /* Align variable definitions */
   if ((cpd.settings[UO_align_var_def_span].n > 0) ||
       (cpd.settings[UO_align_var_struct_span].n > 0))
   {
      align_var_def_brace(chunk_get_head(), cpd.settings[UO_align_var_def_span].n);
   }

   /* Align assignments */
   align_assign(chunk_get_head(), cpd.settings[UO_align_assign_span].n);

   /* Align structure initiailizers */
   if (cpd.settings[UO_align_struct_init_span].n > 0)
   {
      align_struct_initializers();
   }

   /* Align function prototypes */
   if (cpd.settings[UO_align_func_proto_span].n > 0)
   {
      align_func_proto(cpd.settings[UO_align_func_proto_span].n);
   }
}


/**
 * Aligns all function prototypes in the file.
 */
void align_func_proto(int span)
{
   chunk_t    *pc;
   int        max_col  = 0;
   int        span_cnt = 0;
   ChunkStack cs;    /* TODO: grab from a cached list */

   pc = chunk_get_head();
   while (pc != NULL)
   {
      if (chunk_is_newline(pc))
      {
         if (cs.Len() > 0)
         {
            span_cnt += pc->nl_count;
            if (span_cnt > span)
            {
               align_stack(cs, max_col, false, LALPROTO);
               max_col = 0;
            }
         }
      }
      else if (pc->type == CT_FUNC_PROTO)
      {
         align_add(cs, pc, &max_col, 0);
         span_cnt = 0;
      }
      pc = chunk_get_next(pc);
   }
   align_stack(cs, max_col, false, LALPROTO);
}


/**
 * Aligns all backslah-newline combos in the file.
 * This should be done LAST.
 */
void align_backslash_newline(void)
{
   chunk_t *pc;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      if (pc->type != CT_NL_CONT)
      {
         pc = chunk_get_next_type(pc, CT_NL_CONT, -1);
         continue;
      }
      pc = align_nl_cont(pc);
   }
}


void align_right_comments(void)
{
   chunk_t *pc;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      if ((pc->flags & PCF_RIGHT_COMMENT) != 0)
      {
         pc = align_trailing_comments(pc);
      }
      else
      {
         pc = chunk_get_next(pc);
      }
   }
}


/**
 * Aligns stuff inside a multi-line "= { ... }" sequence.
 */
void align_struct_initializers(void)
{
   chunk_t *pc;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      if ((pc->type == CT_BRACE_OPEN) && (pc->parent_type == CT_ASSIGN))
      {
         align_init_brace(pc);
      }
      pc = chunk_get_next_type(pc, CT_BRACE_OPEN, -1);
   }
}


/**
 * Scans the whole file for #defines. Aligns all within X lines of each other
 */
void align_preprocessor(void)
{
   chunk_t    *pc;
   int        max_col  = 0;
   int        nl_count = 0;
   bool       mf       = false;
   bool       pmf      = false;
   ChunkStack cs;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      if ((pc->type == CT_NEWLINE) && !cs.Empty())
      {
         nl_count += pc->nl_count;
         if (nl_count > cpd.settings[UO_align_pp_define_span].n)
         {
            align_stack(cs, max_col, true, LALPP);
         }
      }

      if (pc->type != CT_PP_DEFINE)
      {
         pc = chunk_get_next_nc(pc);
         continue;
      }

      /* skip the 'define' */
      pc = chunk_get_next_nc(pc);
      if (pc == NULL)
      {
         break;
      }

      LOG_FMT(LALPP, "%s: define (%s) on line %d col %d\n",
              __func__, pc->str, pc->orig_line, pc->orig_col);

      pmf = mf;
      if (pc->type == CT_MACRO_FUNC)
      {
         /* Skip over the paren pair */
         pc = chunk_get_next_nc(pc); // point to open (
         pc = chunk_get_next_type(pc, CT_FPAREN_CLOSE, pc->level);
         mf = true;

         LOG_FMT(LALPP, "%s: jumped to (%s) on line %d col %d\n",
                 __func__, pc->str, pc->orig_line, pc->orig_col);
      }
      else
      {
         mf = false;
      }

      /* flush if changing between a macro func and regular macro */
      if ((cs.Len() > 0) && (mf != pmf))
      {
         align_stack(cs, max_col, true, LALPP);
      }

      /* step to the value past the close paren or 'define' */
      pc = chunk_get_next_nc(pc);
      if (pc == NULL)
      {
         break;
      }

      /* don't align anything if the first line ends with a newline before
       * a value is given */
      if (!chunk_is_newline(pc))
      {
         LOG_FMT(LALPP, "%s: align on '%s', line %d col %d\n",
                 __func__, pc->str, pc->orig_line, pc->orig_col);

         align_add(cs, pc, &max_col, cpd.settings[UO_align_pp_define_gap].n);
         nl_count   = 0;
         pc->flags |= PCF_DEF_ALIGNED;
      }
   }

   /* flush just in case we ended on a #define */
   align_stack(cs, max_col, true, LALPP);
}

static void align_assign_group(chunk_t *start, chunk_t *end,
                               int max_col)
{
   chunk_t *pc;
   int     my_level = start->level;

   //   fprintf(stderr, "%s: lines %d - %d to col %d\n", __func__,
   //           start->orig_line, end->orig_line, max_col);

   for (pc = start; pc != end; pc = chunk_get_next(pc))
   {
      if ((pc->level == my_level) &&
          (pc->type == CT_ASSIGN) &&
          ((pc->flags & PCF_WAS_ALIGNED) != 0))
      {
         indent_to_column(pc, max_col - pc->len);
      }
   }
   if ((end->level == my_level) &&
       (end->type == CT_ASSIGN) &&
       ((end->flags & PCF_WAS_ALIGNED) != 0))
   {
      indent_to_column(end, max_col - pc->len);
   }
}


/**
 * Aligns all assignment operators on the same level as first, starting with
 * first.
 *
 * For variable definitions, only consider the '=' for the first variable.
 * Otherwise, only look at the first '=' on the line.
 *
 * This is recursive, so I can't use the simple stack stuff (bummer)
 */
chunk_t *align_assign(chunk_t *first, int span)
{
   int     my_level;
   chunk_t *pc;
   chunk_t *start       = NULL;
   chunk_t *end         = NULL;
   int     max_col      = 0;
   int     max_col_line = 0;
   int     nl_count     = 0;
   int     equ_count    = 0;
   int     tmp;
   int     prev_equ_type = 0;
   int     myspan        = span;
   int     var_def_cnt   = 0;

   if (first == NULL)
   {
      return(NULL);
   }
   my_level = first->level;

   if (first->parent_type == CT_ENUM)
   {
      myspan = cpd.settings[UO_align_enum_equ].n;
   }

   LOG_FMT(LALASS, "%s[%d]: checking %s on line %d\n",
           __func__, my_level, first->str, first->orig_line);

   pc = first;
   while ((pc != NULL) && ((pc->level >= my_level) || (pc->level == 0)))
   {
      /* Don't check inside PAREN or SQUARE groups */
      if ((pc->type == CT_SPAREN_OPEN) ||
          (pc->type == CT_FPAREN_OPEN) ||
          (pc->type == CT_SQUARE_OPEN) ||
          (pc->type == CT_PAREN_OPEN))
      {
         tmp = pc->orig_line;
         pc  = chunk_skip_to_match(pc);
         if (pc != NULL)
         {
            nl_count += (pc->orig_line - tmp);
         }
         continue;
      }

      /* Recurse if a brace set is found */
      if ((pc->type == CT_BRACE_OPEN) ||
          (pc->type == CT_VBRACE_OPEN))
      {
         tmp = pc->orig_line;
         pc  = align_assign(chunk_get_next_ncnl(pc), span);
         if (pc != NULL)
         {
            /* do a rough count of the number of lines just spanned */
            nl_count += (pc->orig_line - tmp);
         }
         continue;
      }

      if (chunk_is_newline(pc))
      {
         nl_count += pc->nl_count;
         if ((start != NULL) && (myspan >= 0) && (nl_count > myspan))
         {
            if (start != end)
            {
               align_assign_group(start, end, max_col);
            }
            start         = NULL;
            max_col       = 0;
            prev_equ_type = 0;
         }
         var_def_cnt = 0;
         equ_count   = 0;
      }
      else if ((pc->flags & PCF_VAR_DEF) != 0)
      {
         var_def_cnt++;
      }
      else if (var_def_cnt > 1)
      {
         /* we hit the second variable def - don't look for assigns */
      }
      else if ((equ_count == 0) && (pc->type == CT_ASSIGN))
      {
         //fprintf(stderr, "%s:  ** %s level=%d line=%d col=%d prev=%d count=%d\n",
         //        __func__, pc->str, pc->level, pc->orig_line, pc->orig_col, prev_equ_type,
         //        equ_count);

         /* Don't align variable def assigns and regular assignments together */
         if (start != NULL)
         {
            if (var_def_cnt != 0)
            {
               if (prev_equ_type == 2)
               {
                  align_assign_group(start, end, max_col);
                  start   = NULL;
                  max_col = 0;
               }
               prev_equ_type = 1;
            }
            else
            {
               if (prev_equ_type == 1)
               {
                  align_assign_group(start, end, max_col);
                  start   = NULL;
                  max_col = 0;
               }
               prev_equ_type = 2;
            }
         }

         pc->flags |= PCF_WAS_ALIGNED;
         if ((pc->column + pc->len) > max_col)
         {
            max_col      = pc->column + pc->len;
            max_col_line = pc->orig_line;
         }
         equ_count++;
         nl_count = 0;

         if (start == NULL)
         {
            prev_equ_type = (var_def_cnt != 0) ? 1 : 2;
            start         = pc;
         }
         end = pc;
      }

      pc = chunk_get_next(pc);
   }

   if (start != NULL)
   {
      align_assign_group(start, end, max_col);
   }

   if (pc != NULL)
   {
      LOG_FMT(LALASS, "%s: done on %s on line %d\n", __func__, pc->str, pc->orig_line);
   }
   else
   {
      LOG_FMT(LALASS, "%s: done on NULL\n", __func__);
   }

   return(pc);
}


/**
 * Aligns the variable definitions in a range.
 * If I had a stack class, I could just use one for variables and one for bits
 *
 * @param first      First item to look at
 * @param last       Last item to look at
 * @param var_column column for the variable
 * @param bit_column column for the bit colon
 */
static void indent_var_def_brace(chunk_t *first, chunk_t *last,
                                 int var_column, int bit_column)
{
   chunk_t *pc;
   chunk_t *end = chunk_get_next_ncnl(last);
   chunk_t *last_star;
   chunk_t *prev;
   int     align_mask = PCF_IN_FCN_DEF | PCF_VAR_1ST;

   if (!cpd.settings[UO_align_var_def_inline].b)
   {
      align_mask |= PCF_VAR_INLINE;
   }

   /* Shift out to the next tabstop */
   if (cpd.settings[UO_align_on_tabstop].b)
   {
      int new_column = align_tab_column(var_column);
      bit_column += (new_column - var_column);
      var_column  = new_column;
   }

   //fprintf(stderr, "%s: first=%s line=%d col=%d level=%d  "
   //        "last=%s line=%d col=%d level=%d  var=%d bit=%d\n",
   //        __func__,
   //        first->str, first->orig_line, first->orig_col, first->level,
   //        last->str, last->orig_line, last->orig_col, last->level,
   //        var_column, bit_column);

   /* now indent */
   for (pc = first; pc != end; pc = chunk_get_next_ncnl(pc))
   {
      if (pc->level == first->level)
      {
         if ((pc->flags & align_mask) == PCF_VAR_1ST)
         {
            int my_col = var_column + 1;
            prev = pc;
            do
            {
               my_col--;
               last_star = prev;
               prev      = chunk_get_prev_ncnl(prev);
            } while ((prev->type == CT_PTR_TYPE) || (prev->type == CT_PAREN_OPEN));

            if (cpd.settings[UO_align_var_def_star].b)
            {
               my_col = var_column;
            }

            //fprintf(stderr, "%s: indenting=%s to col=%d\n", __func__, last_star->str, my_col);

            reindent_line(last_star, my_col);
         }
         else if (cpd.settings[UO_align_var_def_colon].b &&
                  (pc->type == CT_BIT_COLON) &&
                  ((pc->flags & PCF_WAS_ALIGNED) != 0))
         {
            indent_to_column(pc, bit_column);
         }
      }
   }
}


/**
 * Counts how many '*' pointers are in a row, going backwards
 *
 * @param pc   Pointer to the last '*' in the series
 * @return     The count, including the current one
 */
int count_prev_ptr_type(chunk_t *pc)
{
   int count = 0;

   while ((pc != NULL) && (pc->type == CT_PTR_TYPE))
   {
      count++;
      pc = chunk_get_prev_nc(pc);
   }
   return(count);
}


/**
 * Scan everything at the current level until the close brace and find the
 * variable def align column.  Also aligns bit-colons, but that assumes that
 * bit-types are the same! (ie, same definition indent)
 *
 * Can't use the stack stuff here, because there is only one stack and
 * this stuff may be recursive.
 * If only I had a simple stack class...
 */
static chunk_t *align_var_def_brace(chunk_t *start, int span)
{
   chunk_t *pc;
   chunk_t *next;
   chunk_t *prev;
   chunk_t *first_match = NULL;
   chunk_t *last_match  = NULL;
   int     max_col      = 0;
   int     max_bit_col  = 0;
   int     align_mask   = PCF_IN_FCN_DEF | PCF_VAR_1ST;
   bool    did_one;
   int     max_line     = 0;
   int     max_bit_line = 0;
   int     nl_count     = 0;
   int     myspan       = span;
   int     tmpcol;
   int     minvarcol = 0;

   if (start == NULL)
   {
      return(NULL);
   }

   /* Override the span, if this is a struct/union */
   if ((start->parent_type == CT_STRUCT) ||
       (start->parent_type == CT_UNION))
   {
      myspan = cpd.settings[UO_align_var_struct_span].n;
   }

   /* can't be any variable definitions in a "= {" block */
   if (start->parent_type == CT_ASSIGN)
   {
      pc = chunk_get_next_type(start, CT_BRACE_CLOSE, start->level);
      return(chunk_get_next_ncnl(pc));
   }

   if (!cpd.settings[UO_align_var_def_inline].b)
   {
      align_mask |= PCF_VAR_INLINE;
   }

   //fprintf(stderr, "%s: start=%s line=%d col=%d level=%d\n",
   //        __func__, start->str, start->orig_line, start->orig_col, start->level);

   did_one = false;
   prev    = start;
   pc      = chunk_get_next_ncnl(start);
   while ((pc != NULL) && ((pc->level >= start->level) || (pc->level == 0)))
   {
      /* process nested braces */
      if (pc->type == CT_BRACE_OPEN)
      {
         pc = align_var_def_brace(pc, span);
         continue;
      }

      /* Done with this brace set? */
      if (pc->type == CT_BRACE_CLOSE)
      {
         pc = chunk_get_next_ncnl(pc);
         break;
      }

      if (!did_one)
      {
         minvarcol = pc->column;
      }
      else
      {
         minvarcol += pc->len + 1;
      }

      if (chunk_is_newline(pc))
      {
         did_one = false;
         if (first_match != NULL)
         {
            nl_count += pc->nl_count;
            if (nl_count > myspan)
            {
               if (first_match != last_match)
               {
                  indent_var_def_brace(first_match, last_match,
                                       max_col, max_bit_col);
               }
               first_match = NULL;
               nl_count    = 0;
               max_col     = 0;
               max_bit_col = 0;
            }
         }
      }

      /* don't align stuff inside of a function call */
      if ((pc->flags & PCF_IN_FCN_CALL) != 0)
      {
         pc = chunk_get_next_nc(pc);
         continue;
      }

      /* If this is a variable def, update the max_col */
      if (((pc->flags & align_mask) == PCF_VAR_1ST) &&
          (pc->level == (start->level + 1)))
      {
         nl_count = 0;
         if (first_match == NULL)
         {
            first_match = pc;
         }

         /* TODO: need to add up the column on the line */
         tmpcol = pc->column; // + 1; // + prev->len + 1;
         //fprintf(stderr, "%s: [%d] %s - tmpcol=%d mincol=%d\n", __func__, pc->orig_line, pc->str, tmpcol, minvarcol);
         if (!did_one)
         {
            last_match = pc;

            if (cpd.settings[UO_align_var_def_star].b)
            {
               tmpcol -= count_prev_ptr_type(prev);
            }

            if (tmpcol > max_col)
            {
               max_col  = tmpcol;
               max_line = pc->orig_line;
            }

            next = chunk_get_next_nc(pc);
            if (next->type == CT_BIT_COLON)
            {
               next->flags |= PCF_WAS_ALIGNED;
               if (next->column > max_bit_col)
               {
                  max_bit_col  = next->column;
                  max_bit_line = next->orig_line;
               }
            }
         }
         did_one = true;
      }
      prev = pc;
      pc   = chunk_get_next_nc(pc);
   }

   if (first_match != NULL)
   {
      if (first_match != last_match)
      {
         indent_var_def_brace(first_match, last_match,
                              max_col, max_bit_col);
      }
   }

   //   if (pc != NULL)
   //   {
   //      fprintf(stderr, "%s: return=%s line=%d col=%d level=%d max_col=%d\n",
   //              __func__, pc->str, pc->orig_line, pc->orig_col, pc->level, max_col);
   //   }
   //   else
   //   {
   //      fprintf(stderr, "%s: return=<NULL>\n", __func__);
   //   }
   return(pc);
}


/**
 * For a series of lines ending in backslash-newline, align them.
 * The series ends when a newline or multi-line C comment is encountered.
 *
 * @param start   Start point
 * @return        pointer the last item looked at (NULL/newline/comment)
 */
chunk_t *align_nl_cont(chunk_t *start)
{
   int        max_col = 0;
   chunk_t    *pc     = start;
   chunk_t    *tmp;
   ChunkStack cs;

   LOG_FMT(LALNLC, "%s: start on [%s] on line %d\n", __func__,
           get_token_name(start->type), start->orig_line);

   /* Find the max column */
   while ((pc != NULL) &&
          (pc->type != CT_NEWLINE) &&
          (pc->type != CT_COMMENT_MULTI))
   {
      if (pc->type == CT_NL_CONT)
      {
         align_add(cs, pc, &max_col, 0);
      }
      pc = chunk_get_next(pc);
   }

   /* shift out to the next tabstop */
   max_col = align_tab_column(max_col);

   /* NL_CONT is always the last thing on a line */
   while ((tmp = cs.Pop()) != NULL)
   {
      tmp->column = max_col;
   }

   return(pc);
}


/**
 * For a series of lines ending in a comment, align them.
 * The series ends when more than align_right_cmt_span newlines are found.
 *
 * @param start   Start point
 * @return        pointer the last item looked at
 */
chunk_t *align_trailing_comments(chunk_t *start)
{
   int        max_col  = 0;
   chunk_t    *pc      = start;
   int        nl_count = 0;
   ChunkStack cs;

   /* Find the max column */
   while ((pc != NULL) && (nl_count < cpd.settings[UO_align_right_cmt_span].n))
   {
      if ((pc->flags & PCF_RIGHT_COMMENT) != 0)
      {
         align_add(cs, pc, &max_col, 0);
         nl_count = 0;
      }
      if (chunk_is_newline(pc))
      {
         nl_count += pc->nl_count;
      }
      pc = chunk_get_next(pc);
   }

   align_stack(cs, max_col, false, LALTC);

   return(chunk_get_next(pc));
}


// /**
//  * Aligns the equal signs in a braced initializer.
//  *
//  * @param start   Points to the open brace chunk
//  * @return        The chunk after the close brace
//  */
// static chunk_t *align_initializer(chunk_t *start)
// {
//    int            my_level;
//    int            max_col       = 0;
//    int            val_count     = 0;
//    int            max_val_count = 0;
//    chunk_t *pc;
//    chunk_t *close_brace;
//    bool           hit_equal;
//
//    //fprintf(stderr, "%s: line %d\n", __func__, start->orig_line);
//
//    /* We are just looking for comma seperated statments. Align on '='. */
//    close_brace = chunk_get_next_type(start, CT_BRACE_CLOSE, start->level);
//
//    pc        = chunk_get_next(start);
//    my_level  = pc->level;
//    hit_equal = false;
//    val_count = 0;
//    while ((pc != NULL) && (pc != close_brace))
//    {
//       /* Check for nested initializers */
//       if (pc->type == CT_BRACE_OPEN)
//       {
//          pc = align_initializer(pc);
//       }
//       else
//       {
//          if (pc->type == CT_ASSIGN)
//          {
//             if (!hit_equal)
//             {
//                //fprintf(stderr, "%s: equal line %d, col %d\n",
//                //        __func__, pc->orig_line, pc->column);
//                hit_equal  = true;
//                pc->flags |= PCF_WAS_ALIGNED;
//                if (pc->column > max_col)
//                {
//                   max_col = pc->column;
//                }
//             }
//          }
//          else if (pc->type == CT_COMMA)
//          {
//             hit_equal = false;
//             val_count++;
//          }
//          else if (pc->type == CT_NEWLINE)
//          {
//             if (val_count > max_val_count)
//             {
//                max_val_count = val_count;
//             }
//             val_count = 0;
//          }
//          else
//          {
//             /* nada */
//          }
//          pc = chunk_get_next(pc);
//       }
//    }
//
//    /* check the alignment on the last line */
//    if (val_count > max_val_count)
//    {
//       max_val_count = val_count;
//    }
//
//    //   fprintf(stderr, "%s: max_val_count %d, max_col %d\n",
//    //           __func__, max_val_count, max_col);
//
//    /* If there were more than 1 value per line, we don't align anything */
//    if (max_val_count != 1)
//    {
//       return(chunk_get_next(close_brace));
//    }
//
//    /* TODO: align on next tabstop, set min, max indent, etc */
//    //   if ((max_col % cpd.tabstop) != 1)
//    //   {
//    //      max_col = next_tab_column(max_col);
//    //   }
//
//    /* Apply the max column */
//    pc = start;
//    while (pc != close_brace)
//    {
//       if ((pc->type == CT_ASSIGN) &&
//           (pc->level == my_level) &&
//           ((pc->flags & PCF_WAS_ALIGNED) != 0))
//       {
//          indent_column(pc, max_col);
//       }
//       pc = chunk_get_next(pc);
//    }
//    return(chunk_get_next(close_brace));
// }


/**
 * Shifts out all columns by a certain amount.
 *
 * @param idx  The index to start shifting
 * @param num  The number of columns to shift
 */
void ib_shift_out(int idx, int num)
{
   while (idx < cpd.al_cnt)
   {
      cpd.al[idx].col += num;
      idx++;
   }
}


/**
 * Scans a line for stuff to align on.
 *
 * We trigger on BRACE_OPEN, FPAREN_OPEN, ASSIGN, and COMMA.
 * We want to align the NEXT item.
 */
static chunk_t *scan_ib_line(chunk_t *start, bool first)
{
   chunk_t *pc;
   chunk_t *next;
   int     idx              = 0;
   bool    last_was_comment = false;

   if (start->type == CT_SQUARE_OPEN)
   {
      start = chunk_get_next_type(start, CT_ASSIGN, start->level);
      start = chunk_get_next_ncnl(start);
   }
   pc = start;

   while ((pc != NULL) && !chunk_is_newline(pc) &&
          !((pc->type == CT_BRACE_CLOSE) &&
            (pc->parent_type == CT_ASSIGN)))
   {
      if (first)
      {
         if ((pc->type == CT_COMMA) ||
             (pc->type == CT_BRACE_OPEN) ||
             (pc->type == CT_FPAREN_OPEN) ||
             (pc->type == CT_ASSIGN))
         {
            /* The next item is of interest */
            next = chunk_get_next(pc);
            if ((next != NULL) && !chunk_is_newline(next))
            {
               cpd.al[cpd.al_cnt].type = pc->type;
               cpd.al[cpd.al_cnt].col  = next->column;
               cpd.al[cpd.al_cnt].len  = next->len;
               cpd.al_cnt++;
               next->flags     |= PCF_WAS_ALIGNED;
               last_was_comment = chunk_is_comment(next);
            }
         }
      }
      else
      {
         if (cpd.al[idx].type == pc->type)
         {
            next = chunk_get_next(pc);
            if ((next != NULL) && !chunk_is_newline(next))
            {
               LOG_FMT(LSIB, "Match [%d] %s col %d/%d line %d ", idx,
                       get_token_name(next->type), next->column, next->orig_col, next->orig_line);

               next->flags |= PCF_WAS_ALIGNED;
               if (next->column > cpd.al[idx].col)
               {
                  LOG_FMT(LSIB, " [ next->col(%d) > col %d ] ",
                          next->column, cpd.al[idx].col);

                  ib_shift_out(idx, next->column - cpd.al[idx].col);
                  cpd.al[idx].col = next->column;
               }
               if (next->len > cpd.al[idx].len)
               {
                  LOG_FMT(LSIB, " [ next->len(%d) > len %d ] ",
                          next->len, cpd.al[idx].len);

                  ib_shift_out(idx + 1, next->len - cpd.al[idx].len);
                  cpd.al[idx].len = next->len;
               }
               LOG_FMT(LSIB, " - now col %d, len %d\n", cpd.al[idx].col, cpd.al[idx].len);
            }
            idx++;
         }
      }
      pc = chunk_get_next_nc(pc);
   }

   if (first && last_was_comment && (cpd.al[cpd.al_cnt - 1].type == CT_COMMA))
   {
      cpd.al_cnt--;
   }
   return(pc);
}

static void align_log_al(log_sev_t sev, int line)
{
   int idx;

   if (log_sev_on(sev))
   {
      log_fmt(sev, "%s: line %d, %d)", __func__, line, cpd.al_cnt);
      for (idx = 0; idx < cpd.al_cnt; idx++)
      {
         log_fmt(sev, " %d/%d=%s", cpd.al[idx].col, cpd.al[idx].len,
                 get_token_name(cpd.al[idx].type));
      }
      log_fmt(sev, "\n");
   }
}

/**
 * Generically aligns on '=', '{', '(' and item after ','
 * It scans the first line and picks up the location of those tags.
 * It then scans subsequent lines and adjusts the column.
 * Finally it does a second pass to align everything.
 *
 * Aligns all the '=' signs in stucture assignments.
 * a = {
 *    .a    = 1;
 *    .type = fast;
 * };
 *
 * And aligns on '{', numbers, strings, words.
 * colors[] = {
 *    {"red",   {255, 0,   0}}, {"blue",   {  0, 255, 0}},
 *    {"green", {  0, 0, 255}}, {"purple", {255, 255, 0}},
 * };
 *
 * For the C99 indexed array assignment, the leading []= is skipped (no aligning)
 * struct foo_t bars[] =
 * {
 *    [0] = { .name = "bar",
 *            .age  = 21 },
 *    [1] = { .name = "barley",
 *            .age  = 55 },
 * };
 *
 * @param start   Points to the open brace chunk
 */
static void align_init_brace(chunk_t *start)
{
   int     idx;
   chunk_t *pc;
   chunk_t *next;

   cpd.al_cnt = 0;

   pc = chunk_get_next_ncnl(start);
   pc = scan_ib_line(pc, true);
   if ((pc == NULL) || ((pc->type == CT_BRACE_CLOSE) &&
                        (pc->parent_type == CT_ASSIGN)))
   {
      /* single line - nothing to do */
      return;
   }

   do
   {
      pc = scan_ib_line(pc, false);

      /* debug dump the current frame */
      align_log_al(LALBR, pc->orig_line);

      while (chunk_is_newline(pc))
      {
         pc = chunk_get_next(pc);
      }
   } while ((pc != NULL) && !((pc->type == CT_BRACE_CLOSE) &&
                              (pc->parent_type == CT_ASSIGN)));

   /* debug dump the current frame */
   align_log_al(LALBR, start->orig_line);

   if (cpd.settings[UO_align_on_tabstop].b && (cpd.al_cnt >= 1) &&
       (cpd.al[0].type == CT_ASSIGN))
   {
      int rem = (cpd.al[0].col - 1) % cpd.settings[UO_output_tab_size].n;
      if (rem != 0)
      {
         LOG_FMT(LALBR, "%s: col=%d rem=%d", __func__, cpd.al[0].col, rem);
         cpd.al[0].col += cpd.settings[UO_output_tab_size].n - rem;
      }
   }

   pc  = start;
   idx = 0;
   do
   {
      next = chunk_get_next(pc);
      if (idx < cpd.al_cnt)
      {
         if ((next != NULL) && (pc->type == cpd.al[idx].type) &&
             ((next->flags & PCF_WAS_ALIGNED) != 0))
         {
            LOG_FMT(LALBR, " -%d- [%s] to col %d\n", next->orig_line,
                    next->str, cpd.al[idx].col);

            next->flags |= PCF_WAS_ALIGNED;

            if ((pc->type != CT_ASSIGN) &&
                (next->type == CT_NUMBER) && cpd.settings[UO_align_number_left].b)
            {
               reindent_line(next, cpd.al[idx].col + (cpd.al[idx].len - next->len));
            }
            else
            {
               /* first item on the line */
               reindent_line(next, cpd.al[idx].col);
            }
            idx++;
         }
      }
      if (chunk_is_newline(pc))
      {
         idx = 0;
      }
      pc = next;
   } while ((pc != NULL) && !((pc->type == CT_BRACE_CLOSE) &&
                              (pc->parent_type == CT_ASSIGN)));
}


static void align_typedefs(int span)
{
   chunk_t    *pc;
   chunk_t    *next;
   chunk_t    *c_type    = NULL;
   chunk_t    *c_typedef = NULL;
   int        max_col    = 0;
   int        span_ctr   = 0;
   ChunkStack cs;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      if (c_typedef != NULL)
      {
         if (chunk_is_newline(pc))
         {
            c_typedef = NULL;
            LOG_FMT(LALTD, "%s: newline in a typedef on line %d\n", __func__, pc->orig_line);
         }
         else if (pc->type == CT_TYPE)
         {
            c_type = pc;
         }
         else if (pc->type == CT_SEMICOLON)
         {
            if ((c_type != NULL) && (c_typedef->orig_line == c_type->orig_line))
            {
               align_add(cs, c_type, &max_col, cpd.settings[UO_align_typedef_gap].n);
               span_ctr = 0;

               LOG_FMT(LALTD, "%s: max_col=%d cs_len=%d\n",
                       __func__, max_col, cs.Len());
            }
            c_type    = NULL;
            c_typedef = NULL;
         }
         else
         {
            /* don't care */
         }
      }
      else if (pc->type == CT_TYPEDEF)
      {
         next = chunk_get_next_ncnl(pc);
         if ((next != NULL) && (next->type == CT_PAREN_OPEN))
         {
            LOG_FMT(LALTD, "%s: line %d, col %d - skip function type\n",
                    __func__, pc->orig_line, pc->orig_col);
         }
         else
         {
            LOG_FMT(LALTD, "%s: line %d, col %d\n", __func__, pc->orig_line, pc->orig_col);
            c_typedef = pc;
         }
      }
      else
      {
         /* don't care */
      }

      /* Check to see if the span is exceeded */
      if (cs.Len() > 0)
      {
         if (chunk_is_newline(pc))
         {
            c_typedef = NULL;
            span_ctr += pc->nl_count;

            if (span_ctr > span)
            {
               align_stack(cs, max_col, false, LALTD);
            }
         }
      }

      pc = chunk_get_next(pc);
   }

   align_stack(cs, max_col, false, LALTD);
}

