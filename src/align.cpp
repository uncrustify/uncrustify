/**
 * @file c_align.c
 * Does all the aligning stuff.
 *
 * $Id$
 */
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "ChunkStack.h"
#include "align_stack.h"
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

/*
 *   Here are the items aligned:
 *
 *   - enum value assignments
 *     enum {
 *        cat  = 1,
 *        fred = 2,
 *     };
 *
 *   - struct/union variable & bit definitions
 *     struct foo {
 *        char cat;
 *        int  id       : 5;
 *        int  name_len : 6;
 *        int  heigth   : 12;
 *     };
 *
 *   - variable definitions & assignments in normal code
 *     const char *cat = "feline";
 *     int        id   = 4;
 *     a   = 5;
 *     bat = 14;
 *
 *   - simple array initializers
 *     int a[] = {
 *        1, 2, 3, 4, 5,
 *        6, 7, 8, 9, 10
 *     };
 *
 *   - c99 array initializers
 *     const char *name[] = {
 *        [FRED]  = "fred",
 *        [JOE]   = "joe",
 *        [PETER] = "peter",
 *     };
 *     struct foo b[] = {
 *        { .id = 1,   .name = "text 1" },
 *        { .id = 567, .name = "text 2" },
 *     };
 *     struct foo_t bars[] =
 *     {
 *        [0] = { .name = "bar",
 *                .age  = 21 },
 *        [1] = { .name = "barley",
 *                .age  = 55 },
 *     };
 *
 *   - compact array initializers
 *     struct foo b[] = {
 *        { 3, "dog" },      { 6, "spider" },
 *        { 8, "elephant" }, { 3, "cat" },
 *     };
 *
 *   - multiline array initializers (2nd line indented, not aligned)
 *     struct foo b[] = {
 *        { AD_NOT_ALLOWED, "Sorry, you failed to guess the password.",
 *          "Try again?", "Yes", "No" },
 *        { AD_SW_ERROR,    "A software error has occured.", "Bye!", NULL, NULL },
 *     };
 *
 *   - Trailing comments
 *
 *   - Back-slash newline groups
 *
 *   - Function prototypes
 *     int  foo();
 *     void bar();
 *
 *   - Preprocessors
 *     #define FOO_VAL        15
 *     #define MAX_TIMEOUT    60
 *     #define FOO(x)         ((x) * 65)
 *
 *   - typedefs
 *     typedef uint8_t     BYTE;
 *     typedef int32_t     INT32;
 *     typedef uint32_t    UINT32;
 */

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

         if (pc->type == CT_NL_CONT)
         {
            LOG_FMT(sev, "%s: indented [NL_CONT] on line %d to %d\n",
                    __func__, pc->orig_line, pc->column);
         }
         else
         {
            LOG_FMT(sev, "%s: indented [%.*s] on line %d to %d\n",
                    __func__, pc->len, pc->str, pc->orig_line, pc->column);
         }
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
static void align_add(ChunkStack& cs, chunk_t *pc, int& max_col, int min_pad)
{
   chunk_t *prev;
   int     min_col;

   prev = chunk_get_prev(pc);
   if ((prev == NULL) || chunk_is_newline(prev))
   {
      min_col = pc->column;
      LOG_FMT(LALADD, "%s: pc->col=%d max_col=%d min_pad=%d min_col=%d\n",
              __func__, pc->column, max_col, min_pad, min_col);
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
              __func__, pc->column, max_col, min_pad, min_col, prev->column);
   }

   if (cs.Empty())
   {
      max_col = 0;
   }

   cs.Push(pc);
   if (min_col > max_col)
   {
      max_col = min_col;
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
   align_assign(chunk_get_head(),
                cpd.settings[UO_align_assign_span].n,
                cpd.settings[UO_align_assign_thresh].n);

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
         align_add(cs, pc, max_col, 0);
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
   AlignStack as;    // value macros
   AlignStack asf;   // function macros
   AlignStack *cur_as = &as;

   as.Start(cpd.settings[UO_align_pp_define_span].n);
   as.m_gap = cpd.settings[UO_align_pp_define_gap].n;

   asf.Start(cpd.settings[UO_align_pp_define_span].n);
   asf.m_gap = cpd.settings[UO_align_pp_define_gap].n;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      /* Note: not counting back-slash newline combos */
      if (pc->type == CT_NEWLINE)
      {
         as.NewLines(pc->nl_count);
         asf.NewLines(pc->nl_count);
      }

      /* If we aren't on a 'define', then skip to the next non-comment */
      if (pc->type != CT_PP_DEFINE)
      {
         pc = chunk_get_next_nc(pc);
         continue;
      }

      /* step past the 'define' */
      pc = chunk_get_next_nc(pc);
      if (pc == NULL)
      {
         break;
      }

      LOG_FMT(LALPP, "%s: define (%.*s) on line %d col %d\n",
              __func__, pc->len, pc->str, pc->orig_line, pc->orig_col);

      cur_as = &as;
      if (pc->type == CT_MACRO_FUNC)
      {
         cur_as = &asf;

         /* Skip to the close paren */
         pc = chunk_get_next_nc(pc); // point to open (
         pc = chunk_get_next_type(pc, CT_FPAREN_CLOSE, pc->level);

         LOG_FMT(LALPP, "%s: jumped to (%.*s) on line %d col %d\n",
                 __func__, pc->len, pc->str, pc->orig_line, pc->orig_col);
      }

      /* step to the value past the close paren or the macro name */
      pc = chunk_get_next(pc);
      if (pc == NULL)
      {
         break;
      }

      /* don't align anything if the first line ends with a newline before
       * a value is given */
      if (!chunk_is_newline(pc))
      {
         LOG_FMT(LALPP, "%s: align on '%.*s', line %d col %d\n",
                 __func__, pc->len, pc->str, pc->orig_line, pc->orig_col);

         cur_as->Add(pc);
      }
   }

   as.End();
   asf.End();
}


/**
 * Aligns all assignment operators on the same level as first, starting with
 * first.
 *
 * For variable definitions, only consider the '=' for the first variable.
 * Otherwise, only look at the first '=' on the line.
 */
chunk_t *align_assign(chunk_t *first, int span, int thresh)
{
   int     my_level;
   chunk_t *pc;
   int     tmp;
   int     myspan      = span;
   int     mythresh    = thresh;
   int     var_def_cnt = 0;
   int     equ_count   = 0;

   if (first == NULL)
   {
      return(NULL);
   }
   my_level = first->level;

   if (first->parent_type == CT_ENUM)
   {
      myspan = cpd.settings[UO_align_enum_equ].n;
      //mythresh = cpd..settings[UO_align_enum_equ_thresh].n;
   }

   if (myspan <= 0)
   {
      return(chunk_get_next(first));
   }

   LOG_FMT(LALASS, "%s[%d]: checking %.*s on line %d\n",
           __func__, my_level, first->len, first->str, first->orig_line);

   AlignStack as;    // regular assigns
   AlignStack vdas;  // variable def assigns

   as.Start(myspan, mythresh);
   as.m_right_align = true;
   vdas.Start(myspan, mythresh);
   vdas.m_right_align = true;

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
            as.NewLines(pc->orig_line - tmp);
            vdas.NewLines(pc->orig_line - tmp);
         }
         continue;
      }

      /* Recurse if a brace set is found */
      if ((pc->type == CT_BRACE_OPEN) ||
          (pc->type == CT_VBRACE_OPEN))
      {
         tmp = pc->orig_line;
         pc  = align_assign(chunk_get_next_ncnl(pc), span, thresh);
         if (pc != NULL)
         {
            /* do a rough count of the number of lines just spanned */
            as.NewLines(pc->orig_line - tmp);
            vdas.NewLines(pc->orig_line - tmp);
         }
         continue;
      }

      if (chunk_is_newline(pc))
      {
         as.NewLines(pc->nl_count);
         vdas.NewLines(pc->nl_count);

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

         equ_count++;
         if (var_def_cnt != 0)
         {
            vdas.Add(pc);
         }
         else
         {
            as.Add(pc);
         }
      }

      pc = chunk_get_next(pc);
   }

   as.End();
   vdas.End();

   if (pc != NULL)
   {
      LOG_FMT(LALASS, "%s: done on %.*s on line %d\n",
              __func__, pc->len, pc->str, pc->orig_line);
   }
   else
   {
      LOG_FMT(LALASS, "%s: done on NULL\n", __func__);
   }

   return(pc);
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
 * bit-types are the same! But that should always be the case...
 */
static chunk_t *align_var_def_brace(chunk_t *start, int span)
{
   chunk_t    *pc;
   chunk_t    *next;
   int        align_mask = PCF_IN_FCN_DEF | PCF_VAR_1ST;
   int        myspan     = span;
   int        mythresh   = 0;
   AlignStack as;


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
   else
   {
      mythresh = cpd.settings[UO_align_var_def_thresh].n;
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

   as.Start(myspan, mythresh);

   if (cpd.settings[UO_align_var_def_star].b)
   {
      as.m_star_style = AlignStack::SS_INCLUDE;
   }
   else
   {
      as.m_star_style = AlignStack::SS_DANGLE;
   }

   bool did_this_line = false;
   pc = chunk_get_next_ncnl(start);
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

      if (chunk_is_newline(pc))
      {
         did_this_line = false;
         as.NewLines(pc->nl_count);
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
         if (!did_this_line)
         {
            as.Add(pc);

            next = chunk_get_next_nc(pc);
            if (next->type == CT_BIT_COLON)
            {
               as.AddTrailer(next);
            }
         }
         did_this_line = true;
      }
      pc = chunk_get_next_nc(pc);
   }

   as.End();

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
         align_add(cs, pc, max_col, 0);
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
         align_add(cs, pc, max_col, 0);
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
static chunk_t *scan_ib_line(chunk_t *start, bool first_pass)
{
   chunk_t *pc;
   chunk_t *next;
   chunk_t *prev_match = NULL;
   int     token_width;
   int     idx              = 0;
   bool    last_was_comment = false;

   /* Skip past C99 "[xx] =" stuff */
   if (start->type == CT_SQUARE_OPEN)
   {
      start->parent_type = CT_TSQUARE;
      start            = chunk_get_next_type(start, CT_ASSIGN, start->level);
      start            = chunk_get_next_ncnl(start);
      cpd.al_c99_array = true;
   }
   pc = start;

   if (pc != NULL)
   {
      LOG_FMT(LSIB, "%s: start=%s col %d/%d line %d\n", __func__,
              get_token_name(pc->type), pc->column, pc->orig_col, pc->orig_line);
   }

   while ((pc != NULL) && !chunk_is_newline(pc) &&
          (pc->level >= start->level))
   {
      //LOG_FMT(LSIB, "%s:     '%.*s'   col %d/%d line %d\n", __func__,
      //        pc->len, pc->str, pc->column, pc->orig_col, pc->orig_line);

      next = chunk_get_next(pc);
      if ((next == NULL) || chunk_is_comment(next))
      {
         /* do nothing */
      }
      else if ((pc->type == CT_ASSIGN) ||
               (pc->type == CT_BRACE_OPEN) ||
               (pc->type == CT_BRACE_CLOSE) ||
               (pc->type == CT_COMMA))
      {
         token_width = space_col_align(pc, next);

         /*TODO: need to handle missing structure defs? ie NULL vs { ... } ?? */

         /* Is this a new entry? */
         if (idx >= cpd.al_cnt)
         {
            LOG_FMT(LSIB, " - New   [%d] %.2d/%d - %10.10s\n", idx,
                    pc->column, token_width, get_token_name(pc->type));

            cpd.al[cpd.al_cnt].type = pc->type;
            cpd.al[cpd.al_cnt].col  = pc->column;
            cpd.al[cpd.al_cnt].len  = token_width;
            cpd.al_cnt++;
            idx++;
            last_was_comment = false;
         }
         else
         {
            /* expect to match stuff */
            if (cpd.al[idx].type == pc->type)
            {
               LOG_FMT(LSIB, " - Match [%d] %.2d/%d - %10.10s", idx,
                       pc->column, token_width, get_token_name(pc->type));

               /* Shift out based on column */
               if (prev_match == NULL)
               {
                  if (pc->column > cpd.al[idx].col)
                  {
                     LOG_FMT(LSIB, " [ pc->col(%d) > col(%d) ] ",
                             pc->column, cpd.al[idx].col);

                     ib_shift_out(idx, pc->column - cpd.al[idx].col);
                     cpd.al[idx].col = pc->column;
                  }
               }
               else
               {
                  int min_col_diff = pc->column - prev_match->column;
                  int cur_col_diff = cpd.al[idx].col - cpd.al[idx - 1].col;
                  if (cur_col_diff < min_col_diff)
                  {
                     LOG_FMT(LSIB, " [ min_col_diff(%d) > cur_col_diff(%d) ] ",
                             min_col_diff, cur_col_diff);
                     ib_shift_out(idx, min_col_diff - cur_col_diff);
                  }
               }
               LOG_FMT(LSIB, " - now col %d, len %d\n", cpd.al[idx].col, cpd.al[idx].len);
               idx++;
            }
         }
         prev_match = pc;
      }
      last_was_comment = chunk_is_comment(pc);
      pc = chunk_get_next_nc(pc);
   }

   //if (last_was_comment && (cpd.al[cpd.al_cnt - 1].type == CT_COMMA))
   //{
   //   cpd.al_cnt--;
   //}
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
 * NOTE: this assumes that spacing is at the minimum correct spacing (ie force)
 *       if it isn't, some extra spaces will be inserted.
 *
 * @param start   Points to the open brace chunk
 */
static void align_init_brace(chunk_t *start)
{
   int     idx;
   chunk_t *pc;
   chunk_t *next;
   chunk_t *prev;
   chunk_t *num_token = NULL;

   cpd.al_cnt       = 0;
   cpd.al_c99_array = false;

   LOG_FMT(LALBR, "%s: line %d, col %d\n", __func__, start->orig_line, start->orig_col);

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
   } while ((pc != NULL) && (pc->level > start->level));

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

   pc  = chunk_get_next(start);
   idx = 0;
   do
   {
      if ((idx == 0) && (pc->type == CT_SQUARE_OPEN))
      {
         pc = chunk_get_next_type(pc, CT_ASSIGN, pc->level);
         pc = chunk_get_next(pc);
         if (pc != NULL)
         {
            LOG_FMT(LALBR, " -%d- skipped '[] =' to %s\n",
                    pc->orig_line, get_token_name(pc->type));
         }
         continue;
      }

      next = pc;
      if (idx < cpd.al_cnt)
      {
         LOG_FMT(LALBR, " (%d) check %s vs %s -- ",
                 idx, get_token_name(pc->type), get_token_name(cpd.al[idx].type));
         if (pc->type == cpd.al[idx].type)
         {
            if ((idx == 0) && cpd.al_c99_array)
            {
               prev = chunk_get_prev(pc);
               if (chunk_is_newline(prev))
               {
                  pc->flags |= PCF_DONT_INDENT;
               }
            }
            LOG_FMT(LALBR, " [%.*s] to col %d\n", pc->len, pc->str, cpd.al[idx].col);

            if (num_token != NULL)
            {
               int col_diff = pc->column - num_token->column;

               reindent_line(num_token, cpd.al[idx].col - col_diff);
               //LOG_FMT(LSYS, "-= %d =- NUM indent [%.*s] col=%d diff=%d\n",
               //        num_token->orig_line,
               //        num_token->len, num_token->str, cpd.al[idx - 1].col, col_diff);

               num_token->flags |= PCF_WAS_ALIGNED;
               num_token         = NULL;
            }

            /* Comma's need to 'fall back' to the previous token */
            if (pc->type == CT_COMMA)
            {
               next = chunk_get_next(pc);
               if ((next != NULL) && !chunk_is_newline(next))
               {
                  //LOG_FMT(LSYS, "-= %d =- indent [%.*s] col=%d len=%d\n",
                  //        next->orig_line,
                  //        next->len, next->str, cpd.al[idx].col, cpd.al[idx].len);

                  if ((idx < (cpd.al_cnt - 1)) &&
                      cpd.settings[UO_align_number_left].b &&
                      ((next->type == CT_NUMBER) ||
                       (next->type == CT_POS) ||
                       (next->type == CT_NEG)))
                  {
                     /* Need to wait until the next match to indent numbers */
                     num_token = next;
                  }
                  else
                  {
                     reindent_line(next, cpd.al[idx].col + cpd.al[idx].len);
                     next->flags |= PCF_WAS_ALIGNED;
                  }
               }
            }
            else
            {
               /* first item on the line */
               reindent_line(pc, cpd.al[idx].col);
               pc->flags |= PCF_WAS_ALIGNED;

               /* see if we need to right-align a number */
               if ((idx < (cpd.al_cnt - 1)) &&
                   cpd.settings[UO_align_number_left].b)
               {
                  next = chunk_get_next(pc);
                  if ((next != NULL) && !chunk_is_newline(next) &&
                      ((next->type == CT_NUMBER) ||
                       (next->type == CT_POS) ||
                       (next->type == CT_NEG)))
                  {
                     /* Need to wait until the next match to indent numbers */
                     num_token = next;
                  }
               }
            }
            idx++;
         }
         else
         {
            LOG_FMT(LALBR, " no match\n");
         }
      }
      if (chunk_is_newline(pc) || chunk_is_newline(next))
      {
         idx = 0;
      }
      pc = chunk_get_next(pc);
   } while ((pc != NULL) && (pc->level > start->level));
}


/**
 * Aligns simple typedefs that are contained on a single line each.
 * This should be called after the typedef target is marked as a type.
 *
 * Won't align function typedefs.
 *
 * typedef int        foo_t;
 * typedef char       bar_t;
 * typedef const char cc_t;
 */
static void align_typedefs(int span)
{
   chunk_t    *pc;
   chunk_t    *next;
   chunk_t    *c_type    = NULL;
   chunk_t    *c_typedef = NULL;
   AlignStack as;

   as.Start(span);
   as.m_gap        = cpd.settings[UO_align_typedef_gap].n;
   as.m_star_style = (AlignStack::StarStyle)cpd.settings[UO_align_typedef_star_style].n;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      if (chunk_is_newline(pc))
      {
         as.NewLines(pc->nl_count);
         c_typedef = NULL;
      }
      else if (c_typedef != NULL)
      {
         /* Already hit a typedef on this line */
         if (pc->type == CT_TYPE)
         {
            c_type = pc;
         }
         else if (chunk_is_semicolon(pc))
         {
            if ((c_type != NULL) && (c_typedef->orig_line == c_type->orig_line))
            {
               if (chunk_get_prev_ncnl(pc) == c_type)
               {
                  as.Add(c_type);
               }
            }
            c_type    = NULL;
            c_typedef = NULL;
         }
         else
         {
            /* don't care */
         }
      }
      else
      {
         if (pc->type == CT_TYPEDEF)
         {
            next = chunk_get_next_ncnl(pc);
            if ((next != NULL) && (next->type == CT_PAREN_OPEN))
            {
               LOG_FMT(LALTD, "%s: line %d, col %d - skip function type\n",
                       __func__, pc->orig_line, pc->orig_col);
            }
            else
            {
               LOG_FMT(LALTD, "%s: line %d, col %d\n",
                       __func__, pc->orig_line, pc->orig_col);
               c_typedef = pc;
               c_type    = NULL;
            }
         }
      }

      pc = chunk_get_next(pc);
   }

   as.End();
}
