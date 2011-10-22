/**
 * @file align.cpp
 * Does all the aligning stuff.
 *
 * @author  Ben Gardner
 * @license GPL v2+
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
#include "unc_ctype.h"


static chunk_t *align_var_def_brace(chunk_t *pc, int span, int *nl_count);
static chunk_t *align_trailing_comments(chunk_t *start);
static void align_init_brace(chunk_t *start);
static void align_func_params();
static void align_same_func_call_params();
static void align_func_proto(int span);
static void align_oc_msg_spec(int span);
static void align_typedefs(int span);
static void align_left_shift(void);
static void align_oc_msg_colons();
static void align_oc_msg_colon(chunk_t *so);
static void align_oc_decl_colon(void);


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
      col = align_tab_column(col);
   }

   if ((cs.Len() > 1) || (align_single && (cs.Len() == 1)))
   {
      LOG_FMT(sev, "%s: max_col=%d\n", __func__, col);
      while ((pc = cs.Pop()) != NULL)
      {
         align_to_column(pc, col);
         pc->flags |= PCF_WAS_ALIGNED;

         LOG_FMT(sev, "%s: indented [%s] on line %d to %d\n",
                 __func__, pc->str.c_str(), pc->orig_line, pc->column);
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
static void align_add(ChunkStack& cs, chunk_t *pc, int& max_col, int min_pad, bool squeeze)
{
   chunk_t *prev;
   int     min_col;

   prev = chunk_get_prev(pc);
   if ((prev == NULL) || chunk_is_newline(prev))
   {
      min_col = squeeze ? 1 : pc->column;
      LOG_FMT(LALADD, "%s: pc->col=%d max_col=%d min_pad=%d min_col=%d\n",
              __func__, pc->column, max_col, min_pad, min_col);
   }
   else
   {
      if (prev->type == CT_COMMENT_MULTI)
      {
         min_col = prev->orig_col_end + min_pad;
      }
      else
      {
         min_col = prev->column + prev->len() + min_pad;
      }
      if (!squeeze)
      {
         if (min_col < pc->column)
         {
            min_col = pc->column;
         }
      }
      LOG_FMT(LALADD, "%s: pc->col=%d max_col=%d min_pad=%d min_col=%d multi:%s prev->col=%d prev->len=%d %s\n",
              __func__, pc->column, max_col, min_pad, min_col, (prev->type == CT_COMMENT_MULTI) ? "Y" : "N",
              (prev->type == CT_COMMENT_MULTI) ? prev->orig_col_end : prev->column, prev->len(), get_token_name(prev->type));
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


void quick_align_again(void)
{
   chunk_t    *pc;
   chunk_t    *tmp;
   AlignStack as;

   LOG_FMT(LALAGAIN, "%s:\n", __func__);
   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if ((pc->align.next != NULL) && (pc->flags & PCF_ALIGN_START))
      {
         as.Start(100, 0);
         as.m_right_align = pc->align.right_align;
         as.m_star_style  = (AlignStack::StarStyle)pc->align.star_style;
         as.m_amp_style   = (AlignStack::StarStyle)pc->align.amp_style;
         as.m_gap         = pc->align.gap;

         LOG_FMT(LALAGAIN, "   [%s:%d]", pc->str.c_str(), pc->orig_line);
         as.Add(pc->align.start);
         pc->flags |= PCF_WAS_ALIGNED;
         for (tmp = pc->align.next; tmp != NULL; tmp = tmp->align.next)
         {
            tmp->flags |= PCF_WAS_ALIGNED;
            as.Add(tmp->align.start);
            LOG_FMT(LALAGAIN, " => [%s:%d]", tmp->str.c_str(), tmp->orig_line);
         }
         LOG_FMT(LALAGAIN, "\n");
         as.End();
      }
   }
}


void align_all(void)
{
   if (cpd.settings[UO_align_typedef_span].n > 0)
   {
      align_typedefs(cpd.settings[UO_align_typedef_span].n);
   }

   if (cpd.settings[UO_align_left_shift].b)
   {
      align_left_shift();
   }

   if (cpd.settings[UO_align_oc_msg_colon_span].n > 0)
   {
      align_oc_msg_colons();
   }

   if (cpd.settings[UO_align_oc_decl_colon].b)
   {
      align_oc_decl_colon();
   }

   /* Align variable definitions */
   if ((cpd.settings[UO_align_var_def_span].n > 0) ||
       (cpd.settings[UO_align_var_struct_span].n > 0))
   {
      align_var_def_brace(chunk_get_head(), cpd.settings[UO_align_var_def_span].n, NULL);
   }

   /* Align assignments */
   align_assign(chunk_get_head(),
                cpd.settings[UO_align_assign_span].n,
                cpd.settings[UO_align_assign_thresh].n);

   /* Align structure initializers */
   if (cpd.settings[UO_align_struct_init_span].n > 0)
   {
      align_struct_initializers();
   }

   /* Align function prototypes */
   if ((cpd.settings[UO_align_func_proto_span].n > 0) &&
       !cpd.settings[UO_align_mix_var_proto].b)
   {
      align_func_proto(cpd.settings[UO_align_func_proto_span].n);
   }

   /* Align function prototypes */
   if (cpd.settings[UO_align_oc_msg_spec_span].n > 0)
   {
      align_oc_msg_spec(cpd.settings[UO_align_oc_msg_spec_span].n);
   }

   /* Align variable defs in function prototypes */
   if (cpd.settings[UO_align_func_params].b)
   {
      align_func_params();
   }

   if (cpd.settings[UO_align_same_func_call_params].b)
   {
      align_same_func_call_params();
   }
   /* Just in case something was aligned out of order... do it again */
   quick_align_again();
}


/**
 * Aligns all function prototypes in the file.
 */
static void align_oc_msg_spec(int span)
{
   chunk_t    *pc;
   AlignStack as;

   LOG_FMT(LALIGN, "%s\n", __func__);
   as.Start(span, 0);

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if (chunk_is_newline(pc))
      {
         as.NewLines(pc->nl_count);
      }
      else if (pc->type == CT_OC_MSG_SPEC)
      {
         as.Add(pc);
      }
   }
   as.End();
}


/**
 * Aligns all backslash-newline combos in the file.
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
   chunk_t *prev;
   bool    skip;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if ((pc->type == CT_COMMENT) ||
          (pc->type == CT_COMMENT_CPP) ||
          (pc->type == CT_COMMENT_MULTI))
      {
         skip = false;
         if (pc->parent_type == CT_COMMENT_END)
         {
            prev = chunk_get_prev(pc);
            if (pc->orig_col < (prev->orig_col_end + cpd.settings[UO_align_right_cmt_gap].n))
            {
               LOG_FMT(LALTC, "NOT changing END comment on line %d (%d <= %d + %d)\n",
                       pc->orig_line,
                       pc->orig_col, prev->orig_col_end, cpd.settings[UO_align_right_cmt_gap].n);
               skip = true;
            }
            if (!skip)
            {
               LOG_FMT(LALTC, "Changing END comment on line %d into a RIGHT-comment\n",
                       pc->orig_line);
               pc->flags |= PCF_RIGHT_COMMENT;
            }
         }

         /* Change certain WHOLE comments into RIGHT-alignable comments */
         if (pc->parent_type == CT_COMMENT_WHOLE)
         {
            int max_col = pc->column_indent + cpd.settings[UO_input_tab_size].n;

            /* If the comment is further right than the brace level... */
            if (pc->column >= max_col)
            {
               LOG_FMT(LALTC, "Changing WHOLE comment on line %d into a RIGHT-comment (col=%d col_ind=%d max_col=%d)\n",
                       pc->orig_line, pc->column, pc->column_indent, max_col);

               pc->flags |= PCF_RIGHT_COMMENT;
            }
         }
      }
   }

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
   chunk_t *prev;

   pc = chunk_get_head();
   while (pc != NULL)
   {
      prev = chunk_get_prev_ncnl(pc);
      if ((prev != NULL) && (prev->type == CT_ASSIGN) &&
          ((pc->type == CT_BRACE_OPEN) ||
           ((cpd.lang_flags & LANG_D) && (pc->type == CT_SQUARE_OPEN))))
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

      LOG_FMT(LALPP, "%s: define (%s) on line %d col %d\n",
              __func__, pc->str.c_str(), pc->orig_line, pc->orig_col);

      cur_as = &as;
      if (pc->type == CT_MACRO_FUNC)
      {
         if (!cpd.settings[UO_align_pp_define_together].b)
         {
            cur_as = &asf;
         }

         /* Skip to the close paren */
         pc = chunk_get_next_nc(pc); // point to open (
         pc = chunk_get_next_type(pc, CT_FPAREN_CLOSE, pc->level);

         LOG_FMT(LALPP, "%s: jumped to (%s) on line %d col %d\n",
                 __func__, pc->str.c_str(), pc->orig_line, pc->orig_col);
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
         LOG_FMT(LALPP, "%s: align on '%s', line %d col %d\n",
                 __func__, pc->str.c_str(), pc->orig_line, pc->orig_col);

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
   int     var_def_cnt = 0;
   int     equ_count   = 0;

   if (first == NULL)
   {
      return(NULL);
   }
   my_level = first->level;

   if (span <= 0)
   {
      return(chunk_get_next(first));
   }

   LOG_FMT(LALASS, "%s[%d]: checking %s on line %d - span=%d thresh=%d\n",
           __func__, my_level, first->str.c_str(), first->orig_line,
           span, thresh);

   AlignStack as;    // regular assigns
   AlignStack vdas;  // variable def assigns

   /* If we are aligning on a tabstop, we shouldn't right-align */
   as.Start(span, thresh);
   as.m_right_align = !cpd.settings[UO_align_on_tabstop].b;
   vdas.Start(span, thresh);
   vdas.m_right_align = as.m_right_align;

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
         int myspan;
         int mythresh;

         tmp = pc->orig_line;

         if (pc->parent_type == CT_ENUM)
         {
            myspan   = cpd.settings[UO_align_enum_equ_span].n;
            mythresh = cpd.settings[UO_align_enum_equ_thresh].n;
         }
         else
         {
            myspan   = cpd.settings[UO_align_assign_span].n;
            mythresh = cpd.settings[UO_align_assign_thresh].n;
         }

         pc = align_assign(chunk_get_next_ncnl(pc), myspan, mythresh);
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
      LOG_FMT(LALASS, "%s: done on %s on line %d\n",
              __func__, pc->str.c_str(), pc->orig_line);
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


static chunk_t *align_func_param(chunk_t *start)
{
   AlignStack as;
   chunk_t    *pc = start;

   as.Start(2, 0);

   as.m_star_style = (AlignStack::StarStyle)cpd.settings[UO_align_var_def_star_style].n;
   as.m_amp_style  = (AlignStack::StarStyle)cpd.settings[UO_align_var_def_amp_style].n;

   bool did_this_line = false;
   int  comma_count   = 0;
   int  chunk_count   = 0;

   while ((pc = chunk_get_next(pc)) != NULL)
   {
      chunk_count++;
      if (chunk_is_newline(pc))
      {
         did_this_line = false;
         comma_count   = 0;
         chunk_count   = 0;
      }
      else if (pc->level <= start->level)
      {
         break;
      }
      else if (!did_this_line && (pc->flags & PCF_VAR_DEF))
      {
         if (chunk_count > 1)
         {
            as.Add(pc);
         }
         did_this_line = true;
      }
      else if (comma_count > 0)
      {
         if (!chunk_is_comment(pc))
         {
            comma_count = 2;
            break;
         }
      }
      else if (pc->type == CT_COMMA)
      {
         comma_count++;
      }
   }

   if (comma_count <= 1)
   {
      as.End();
   }

   return(pc);
}


static void align_func_params()
{
   chunk_t *pc;

   pc = chunk_get_head();
   while ((pc = chunk_get_next(pc)) != NULL)
   {
      if ((pc->type != CT_FPAREN_OPEN) ||
          ((pc->parent_type != CT_FUNC_PROTO) &&
           (pc->parent_type != CT_FUNC_DEF) &&
           (pc->parent_type != CT_FUNC_CLASS) &&
           (pc->parent_type != CT_TYPEDEF)))
      {
         continue;
      }

      /* We're on a open paren of a prototype */
      pc = align_func_param(pc);
   }
}


static void align_params(chunk_t *start, deque<chunk_t *>& chunks)
{
   chunk_t *pc       = start;
   bool    hit_comma = true;

   chunks.clear();

   pc = chunk_get_next_type(start, CT_FPAREN_OPEN, start->level);

   while ((pc = chunk_get_next(pc)) != NULL)
   {
      if (chunk_is_newline(pc) ||
          (pc->type == CT_SEMICOLON) ||
          ((pc->type == CT_FPAREN_CLOSE) && (pc->level == start->level)))
      {
         break;
      }

      if (pc->level == (start->level + 1))
      {
         if (hit_comma)
         {
            chunks.push_back(pc);
            hit_comma = false;
         }
         else if (pc->type == CT_COMMA)
         {
            hit_comma = true;
         }
      }
   }
}


static void align_same_func_call_params()
{
   chunk_t           *pc;
   chunk_t           *align_root = NULL;
   chunk_t           *align_cur  = NULL;
   int               align_len   = 0;
   deque<chunk_t *>  chunks;
   deque<AlignStack> as;
   AlignStack        fcn_as;
   int               idx;
   const char        *add_str;

   fcn_as.Start(3);

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if (pc->type != CT_FUNC_CALL)
      {
         if (chunk_is_newline(pc))
         {
            for (idx = 0; idx <= (int)as.size(); idx++)
            {
               as[idx].NewLines(pc->nl_count);
            }
            fcn_as.NewLines(pc->nl_count);
         }
         continue;
      }

      /* Only align function calls that are right after a newline */
      if (!chunk_is_newline(chunk_get_prev(pc)))
      {
         continue;
      }

      add_str = NULL;
      if (align_root != NULL)
      {
         if (pc->str.equals(align_root->str))
         {
            fcn_as.Add(pc);
            align_cur->align.next = pc;
            align_cur             = pc;
            align_len++;
            add_str = "  Add";
         }
         else
         {
            LOG_FMT(LASFCP, "  ++ Ended with %d fcns\n", align_len);

            /* Flush it all! */
            fcn_as.Flush();
            for (idx = 0; idx < (int)as.size(); idx++)
            {
               as[idx].Flush();
            }
            align_root = NULL;
         }
      }

      if (align_root == NULL)
      {
         fcn_as.Add(pc);
         align_root = pc;
         align_cur  = pc;
         align_len  = 1;
         add_str    = "Start";
      }

      if (add_str != NULL)
      {
         LOG_FMT(LASFCP, "%s '%s' on line %d -",
                 add_str, pc->str.c_str(), pc->orig_line);
         align_params(pc, chunks);
         LOG_FMT(LASFCP, " %d items:", (int)chunks.size());

         for (idx = 0; idx < (int)chunks.size(); idx++)
         {
            LOG_FMT(LASFCP, " [%s]", chunks[idx]->str.c_str());
            if (idx >= (int)as.size())
            {
               as.resize(idx + 1);
               as[idx].Start(3);
               if (!cpd.settings[UO_align_number_left].b)
               {
                  if ((chunks[idx]->type == CT_NUMBER_FP) ||
                      (chunks[idx]->type == CT_NUMBER) ||
                      (chunks[idx]->type == CT_POS) ||
                      (chunks[idx]->type == CT_NEG))
                  {
                     as[idx].m_right_align = !cpd.settings[UO_align_on_tabstop].b;
                  }
               }
            }
            as[idx].Add(chunks[idx]);
         }
         LOG_FMT(LASFCP, "\n");
      }
   }

   if (align_len > 1)
   {
      LOG_FMT(LASFCP, "  ++ Ended with %d fcns\n", align_len);
      fcn_as.End();
      for (idx = 0; idx < (int)as.size(); idx++)
      {
         as[idx].End();
      }
   }
}


chunk_t *step_back_over_member(chunk_t *pc)
{
   chunk_t *tmp;

   /* Skip over any class stuff: bool CFoo::bar() */
   while (((tmp = chunk_get_prev_ncnl(pc)) != NULL) &&
          (tmp->type == CT_DC_MEMBER))
   {
      /* TODO: verify that we are pointing at something sane? */
      pc = chunk_get_prev_ncnl(tmp);
   }
   return(pc);
}


/**
 * Aligns all function prototypes in the file.
 */
static void align_func_proto(int span)
{
   chunk_t    *pc;
   chunk_t    *toadd;
   bool       look_bro = false;
   AlignStack as;
   AlignStack as_br;

   LOG_FMT(LALIGN, "%s\n", __func__);
   as.Start(span, 0);
   as.m_gap = cpd.settings[UO_align_func_proto_gap].n;
   as.m_star_style = (AlignStack::StarStyle)cpd.settings[UO_align_var_def_star_style].n;
   as.m_amp_style  = (AlignStack::StarStyle)cpd.settings[UO_align_var_def_amp_style].n;

   as_br.Start(span, 0);
   as_br.m_gap = cpd.settings[UO_align_single_line_brace_gap].n;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if (chunk_is_newline(pc))
      {
         look_bro = false;
         as.NewLines(pc->nl_count);
         as_br.NewLines(pc->nl_count);
      }
      else if ((pc->type == CT_FUNC_PROTO) ||
               ((pc->type == CT_FUNC_DEF) &&
                cpd.settings[UO_align_single_line_func].b))
      {
         if ((pc->parent_type == CT_OPERATOR) &&
             cpd.settings[UO_align_on_operator].b)
         {
            toadd = chunk_get_prev_ncnl(pc);
         }
         else
         {
            toadd = pc;
         }
         as.Add(step_back_over_member(toadd));
         look_bro = (pc->type == CT_FUNC_DEF) &&
                    cpd.settings[UO_align_single_line_brace].b;
      }
      else if (look_bro &&
               (pc->type == CT_BRACE_OPEN) &&
               (pc->flags & PCF_ONE_LINER))
      {
         as_br.Add(pc);
         look_bro = false;
      }
   }
   as.End();
   as_br.End();
}


/**
 * Scan everything at the current level until the close brace and find the
 * variable def align column.  Also aligns bit-colons, but that assumes that
 * bit-types are the same! But that should always be the case...
 */
static chunk_t *align_var_def_brace(chunk_t *start, int span, int *p_nl_count)
{
   chunk_t    *pc;
   chunk_t    *next;
   chunk_t    *prev;
   UINT64     align_mask = PCF_IN_FCN_DEF | PCF_VAR_1ST;
   int        myspan     = span;
   int        mythresh   = 0;
   int        mygap      = 0;
   AlignStack as;    /* var/proto/def */
   AlignStack as_bc; /* bit-colon */
   AlignStack as_at; /* attribute */
   AlignStack as_br; /* one-liner brace open */
   bool       fp_active   = cpd.settings[UO_align_mix_var_proto].b;
   bool       fp_look_bro = false;


   if (start == NULL)
   {
      return(NULL);
   }

   /* Override the span, if this is a struct/union */
   if ((start->parent_type == CT_STRUCT) ||
       (start->parent_type == CT_UNION))
   {
      myspan   = cpd.settings[UO_align_var_struct_span].n;
      mythresh = cpd.settings[UO_align_var_struct_thresh].n;
      mygap    = cpd.settings[UO_align_var_struct_gap].n;
   }
   else
   {
      mythresh = cpd.settings[UO_align_var_def_thresh].n;
      mygap    = cpd.settings[UO_align_var_def_gap].n;
   }

   /* can't be any variable definitions in a "= {" block */
   prev = chunk_get_prev_ncnl(start);
   if ((prev != NULL) && (prev->type == CT_ASSIGN))
   {
      LOG_FMT(LAVDB, "%s: start=%s [%s] on line %d (abort due to assign)\n", __func__,
              start->str.c_str(), get_token_name(start->type), start->orig_line);

      pc = chunk_get_next_type(start, CT_BRACE_CLOSE, start->level);
      return(chunk_get_next_ncnl(pc));
   }

   LOG_FMT(LAVDB, "%s: start=%s [%s] on line %d\n", __func__,
           start->str.c_str(), get_token_name(start->type), start->orig_line);

   if (!cpd.settings[UO_align_var_def_inline].b)
   {
      align_mask |= PCF_VAR_INLINE;
   }

   /* Set up the var/proto/def aligner */
   as.Start(myspan, mythresh);
   as.m_gap        = mygap;
   as.m_star_style = (AlignStack::StarStyle)cpd.settings[UO_align_var_def_star_style].n;
   as.m_amp_style  = (AlignStack::StarStyle)cpd.settings[UO_align_var_def_amp_style].n;

   /* Set up the bit colon aligner */
   as_bc.Start(myspan, 0);
   as_bc.m_gap = cpd.settings[UO_align_var_def_colon_gap].n;

   as_at.Start(myspan, 0);

   /* Set up the brace open aligner */
   as_br.Start(myspan, mythresh);
   as_br.m_gap = cpd.settings[UO_align_single_line_brace_gap].n;

   bool did_this_line = false;
   pc = chunk_get_next(start);
   while ((pc != NULL) && ((pc->level >= start->level) || (pc->level == 0)))
   {
      if (chunk_is_comment(pc))
      {
         if (pc->nl_count > 0)
         {
            as.NewLines(pc->nl_count);
            as_bc.NewLines(pc->nl_count);
            as_at.NewLines(pc->nl_count);
            as_br.NewLines(pc->nl_count);
         }
         pc = chunk_get_next(pc);
         continue;
      }

      if (fp_active)
      {
         if ((pc->type == CT_FUNC_PROTO) ||
             ((pc->type == CT_FUNC_DEF) &&
              cpd.settings[UO_align_single_line_func].b))
         {
            LOG_FMT(LAVDB, "    add=[%s] line=%d col=%d level=%d\n",
                    pc->str.c_str(), pc->orig_line, pc->orig_col, pc->level);

            as.Add(pc);
            fp_look_bro = (pc->type == CT_FUNC_DEF) &&
                          cpd.settings[UO_align_single_line_brace].b;
         }
         else if (fp_look_bro &&
                  (pc->type == CT_BRACE_OPEN) &&
                  (pc->flags & PCF_ONE_LINER))
         {
            as_br.Add(pc);
            fp_look_bro = false;
         }
      }

      /* process nested braces */
      if (pc->type == CT_BRACE_OPEN)
      {
         int sub_nl_count = 0;

         pc = align_var_def_brace(pc, span, &sub_nl_count);
         if (sub_nl_count > 0)
         {
            fp_look_bro   = false;
            did_this_line = false;
            as.NewLines(sub_nl_count);
            as_bc.NewLines(sub_nl_count);
            as_at.NewLines(sub_nl_count);
            as_br.NewLines(sub_nl_count);
            if (p_nl_count != NULL)
            {
               *p_nl_count += sub_nl_count;
            }
         }
         continue;
      }

      /* Done with this brace set? */
      if (pc->type == CT_BRACE_CLOSE)
      {
         pc = chunk_get_next(pc);
         break;
      }

      if (chunk_is_newline(pc))
      {
         fp_look_bro   = false;
         did_this_line = false;
         as.NewLines(pc->nl_count);
         as_bc.NewLines(pc->nl_count);
         as_at.NewLines(pc->nl_count);
         as_br.NewLines(pc->nl_count);
         if (p_nl_count != NULL)
         {
            *p_nl_count += pc->nl_count;
         }
      }

      /* don't align stuff inside parens/squares/angles */
      if (pc->level > pc->brace_level)
      {
         pc = chunk_get_next(pc);
         continue;
      }

      /* If this is a variable def, update the max_col */
      if ((pc->type != CT_FUNC_CLASS) &&
          ((pc->flags & align_mask) == PCF_VAR_1ST) &&
          ((pc->level == (start->level + 1)) ||
           (pc->level == 0)))
      {
         if (!did_this_line)
         {
            LOG_FMT(LAVDB, "    add=[%s] line=%d col=%d level=%d\n",
                    pc->str.c_str(), pc->orig_line, pc->orig_col, pc->level);

            as.Add(step_back_over_member(pc));

            if (cpd.settings[UO_align_var_def_colon].b)
            {
               next = chunk_get_next_nc(pc);
               if (next->type == CT_BIT_COLON)
               {
                  as_bc.Add(next);
               }
            }
            if (cpd.settings[UO_align_var_def_attribute].b)
            {
               next = pc;
               while ((next = chunk_get_next_nc(next)) != NULL)
               {
                  if (next->type == CT_ATTRIBUTE)
                  {
                     as_at.Add(next);
                     break;
                  }
                  if ((next->type == CT_SEMICOLON) || chunk_is_newline(next))
                  {
                     break;
                  }
               }
            }
         }
         did_this_line = true;
      }
      else if (pc->type == CT_BIT_COLON)
      {
         if (!did_this_line)
         {
            as_bc.Add(pc);
            did_this_line = true;
         }
      }
      pc = chunk_get_next(pc);
   }

   as.End();
   as_bc.End();
   as_at.End();
   as_br.End();

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
         align_add(cs, pc, max_col, 1, true);
      }
      pc = chunk_get_next(pc);
   }

   /* NL_CONT is always the last thing on a line */
   while ((tmp = cs.Pop()) != NULL)
   {
      tmp->flags |= PCF_WAS_ALIGNED;
      tmp->column = max_col;
   }

   return(pc);
}


enum CmtAlignType
{
   CAT_REGULAR,
   CAT_BRACE,
   CAT_ENDIF,
};

static CmtAlignType get_comment_align_type(chunk_t *cmt)
{
   chunk_t      *prev;
   CmtAlignType cmt_type = CAT_REGULAR;

   if (!cpd.settings[UO_align_right_cmt_mix].b &&
       ((prev = chunk_get_prev(cmt)) != NULL))
   {
      if ((prev->type == CT_PP_ENDIF) ||
          (prev->type == CT_PP_ELSE) ||
          (prev->type == CT_ELSE) ||
          (prev->type == CT_BRACE_CLOSE))
      {
         /* REVISIT: someone may want this configurable */
         if ((cmt->column - (prev->column + prev->len())) < 3)
         {
            cmt_type = (prev->type == CT_PP_ENDIF) ? CAT_ENDIF : CAT_BRACE;
         }
      }
   }
   return(cmt_type);
}


/**
 * For a series of lines ending in a comment, align them.
 * The series ends when more than align_right_cmt_span newlines are found.
 *
 * Interesting info:
 *  - least pysically allowed column
 *  - intended column
 *  - least original cmt column
 *
 * min_col is the minimum allowed column (based on prev token col/size)
 * cmt_col less than
 *
 * @param start   Start point
 * @return        pointer the last item looked at
 */
chunk_t *align_trailing_comments(chunk_t *start)
{
   int          min_col  = 0;
   int          min_orig = -1;
   chunk_t      *pc      = start;
   int          nl_count = 0;
   ChunkStack   cs;
   CmtAlignType cmt_type_start, cmt_type_cur;
   int          col;
   int          intended_col = cpd.settings[UO_align_right_cmt_at_col].n;

   cmt_type_start = get_comment_align_type(pc);

   LOG_FMT(LALADD, "%s: start on line=%d\n", __func__, pc->orig_line);

   /* Find the max column */
   while ((pc != NULL) && (nl_count < cpd.settings[UO_align_right_cmt_span].n))
   {
      if (((pc->flags & PCF_RIGHT_COMMENT) != 0) && (pc->column > 1))
      {
         cmt_type_cur = get_comment_align_type(pc);

         if (cmt_type_cur == cmt_type_start)
         {
            col = 1 + (pc->brace_level * cpd.settings[UO_indent_columns].n);
            LOG_FMT(LALADD, "%s: line=%d col=%d max_col=%d pc->col=%d pc->len=%d %s\n",
                    __func__, pc->orig_line, col, min_col, pc->column, pc->len(),
                    get_token_name(pc->type));
            if ((min_orig < 0) || (min_orig > pc->column))
            {
               min_orig = pc->column;
            }
            if (pc->column < col)
            {
               pc->column = col;
            }
            align_add(cs, pc, min_col, 1, true);//(intended_col < col));
            nl_count = 0;
         }
      }
      if (chunk_is_newline(pc))
      {
         nl_count += pc->nl_count;
      }
      pc = chunk_get_next(pc);
   }

   /* Start with the minimum original column */
   col = min_orig;
   /* fall back to the intended column */
   if ((intended_col > 0) && (col > intended_col))
   {
      col = intended_col;
   }
   /* if less than allowed, bump it out */
   if (col < min_col)
   {
      col = min_col;
   }
   /* bump out to the intended column */
   if (col < intended_col)
   {
      col = intended_col;
   }
   LOG_FMT(LALADD, "%s:  -- min_orig=%d intend=%d min_allowed=%d ==> col=%d\n", __func__,
           min_orig, intended_col, min_col, col);
   align_stack(cs, col, (intended_col != 0), LALTC);

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
   int     idx = 0;

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
      //LOG_FMT(LSIB, "%s:     '%s'   col %d/%d line %d\n", __func__,
      //        pc->str.c_str(), pc->column, pc->orig_col, pc->orig_line);

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
               else if (idx > 0)
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
      pc = chunk_get_next_nc(pc);
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
      cpd.al[0].col = align_tab_column(cpd.al[0].col);
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
            LOG_FMT(LALBR, " [%s] to col %d\n", pc->str.c_str(), cpd.al[idx].col);

            if (num_token != NULL)
            {
               int col_diff = pc->column - num_token->column;

               reindent_line(num_token, cpd.al[idx].col - col_diff);
               //LOG_FMT(LSYS, "-= %d =- NUM indent [%s] col=%d diff=%d\n",
               //        num_token->orig_line,
               //        num_token->str.c_str(), cpd.al[idx - 1].col, col_diff);

               num_token->flags |= PCF_WAS_ALIGNED;
               num_token         = NULL;
            }

            /* Comma's need to 'fall back' to the previous token */
            if (pc->type == CT_COMMA)
            {
               next = chunk_get_next(pc);
               if ((next != NULL) && !chunk_is_newline(next))
               {
                  //LOG_FMT(LSYS, "-= %d =- indent [%s] col=%d len=%d\n",
                  //        next->orig_line,
                  //        next->str.c_str(), cpd.al[idx].col, cpd.al[idx].len);

                  if ((idx < (cpd.al_cnt - 1)) &&
                      cpd.settings[UO_align_number_left].b &&
                      ((next->type == CT_NUMBER_FP) ||
                       (next->type == CT_NUMBER) ||
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
                      ((next->type == CT_NUMBER_FP) ||
                       (next->type == CT_NUMBER) ||
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
   chunk_t    *c_typedef = NULL;
   AlignStack as;

   as.Start(span);
   as.m_gap        = cpd.settings[UO_align_typedef_gap].n;
   as.m_star_style = (AlignStack::StarStyle)cpd.settings[UO_align_typedef_star_style].n;
   as.m_amp_style  = (AlignStack::StarStyle)cpd.settings[UO_align_typedef_amp_style].n;

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
         if (pc->flags & PCF_ANCHOR)
         {
            as.Add(pc);
            c_typedef = NULL;
         }
      }
      else
      {
         if (pc->type == CT_TYPEDEF)
         {
            LOG_FMT(LALTD, "%s: line %d, col %d\n",
                    __func__, pc->orig_line, pc->orig_col);
            c_typedef = pc;
         }
      }

      pc = chunk_get_next(pc);
   }

   as.End();
}


/**
 * Align '<<' (CT_ARITH?)
 */
static void align_left_shift(void)
{
   chunk_t    *pc;
   chunk_t    *start = NULL;
   AlignStack as;

   as.Start(2);

   pc = chunk_get_head();
   while (pc != NULL)
   {
      if (chunk_is_newline(pc))
      {
         as.NewLines(pc->nl_count);
      }
      else if ((start != NULL) && (pc->level < start->level))
      {
         /* A drop in level restarts the aligning */
         as.Flush();
         start = NULL;
      }
      else if ((start != NULL) && (pc->level > start->level))
      {
         /* Ignore any deeper levels when aligning */
      }
      else if (pc->type == CT_SEMICOLON)
      {
         /* A semicolon at the same level flushes */
         as.Flush();
         start = NULL;
      }
      else if (chunk_is_str(pc, "<<", 2))
      {
         if (as.m_aligned.Empty())
         {
            /* first one can be anywhere */
            as.Add(pc);
            start = pc;
         }
         else if (chunk_is_newline(chunk_get_prev(pc)))
         {
            /* subsequent ones must be after a newline */
            as.Add(pc);
         }
      }

      pc = chunk_get_next(pc);
   }
   as.End();
}


/**
 * Aligns an OC message
 *
 * @param so   the square open of the message
 * @param span the span value
 */
static void align_oc_msg_colon(chunk_t *so)
{
   int        span = cpd.settings[UO_align_oc_msg_colon_span].n;
   chunk_t    *pc;
   chunk_t    *tmp;
   AlignStack cas;   /* for the colons */
   AlignStack nas;   /* for the parameter tag */
   int        level;
   bool       did_line;
   int        lcnt;  /* line count with no colon for span */
   bool       has_colon;


   nas.Reset();
   nas.m_right_align = !cpd.settings[UO_align_on_tabstop].b;

   cas.Start(span);

   level = so->level;
   pc    = chunk_get_next_ncnl(so, CNAV_PREPROC);

   did_line  = false;
   has_colon = false;
   lcnt      = 0;

   while ((pc != NULL) && (pc->level > level))
   {
      if (pc->level > (level + 1))
      {
         /* do nothing */
      }
      else if (chunk_is_newline(pc))
      {
         if (!has_colon)
         {
            ++lcnt;
         }
         did_line  = false;
         has_colon = !has_colon;
      }
      else if (!did_line && (lcnt - 1 < span) && (pc->type == CT_OC_COLON))
      {
         has_colon = true;
         cas.Add(pc);
         tmp = chunk_get_prev(pc);
         if ((tmp != NULL) &&
             ((tmp->type == CT_OC_MSG_FUNC) ||
              (tmp->type == CT_OC_MSG_NAME)))
         {
            nas.Add(tmp);
         }
         did_line = true;
      }
      pc = chunk_get_next(pc, CNAV_PREPROC);
   }
   nas.End();
   cas.End();
}


/**
 * Aligns OC messages
 */
static void align_oc_msg_colons()
{
   chunk_t *pc;

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if ((pc->type == CT_SQUARE_OPEN) && (pc->parent_type == CT_OC_MSG))
      {
         align_oc_msg_colon(pc);
      }
   }
}


/**
 * Aligns OC declarations on the colon
 * -(void) doSomething: (NSString*) param1
 *                with: (NSString*) param2
 */
static void align_oc_decl_colon(void)
{
   chunk_t    *pc = chunk_get_head();
   chunk_t    *tmp;
   chunk_t    *tmp2;
   AlignStack cas;   /* for the colons */
   AlignStack nas;   /* for the parameter label */
   int        level;
   bool       did_line;

   cas.Start(4);
   nas.Start(4);
   nas.m_right_align = !cpd.settings[UO_align_on_tabstop].b;

   while (pc != NULL)
   {
      if (pc->type != CT_OC_SCOPE)
      {
         pc = chunk_get_next(pc);
         continue;
      }

      nas.Reset();
      cas.Reset();

      level = pc->level;
      pc    = chunk_get_next_ncnl(pc, CNAV_PREPROC);

      did_line = false;

      while ((pc != NULL) && (pc->level >= level))
      {
         /* The decl ends with an open brace or semicolon */
         if ((pc->type == CT_BRACE_OPEN) || chunk_is_semicolon(pc))
         {
            break;
         }

         if (chunk_is_newline(pc))
         {
            nas.NewLines(pc->nl_count);
            cas.NewLines(pc->nl_count);
            did_line  = false;
         }
         else if (!did_line && (pc->type == CT_OC_COLON))
         {
            cas.Add(pc);

            tmp = chunk_get_prev(pc, CNAV_PREPROC);
            tmp2 = chunk_get_prev_ncnl(tmp, CNAV_PREPROC);

            /* Check for an un-labeled parameter */
            if ((tmp != NULL) &&
                (tmp2 != NULL)
                &&
                ((tmp->type == CT_WORD) ||
                 (tmp->type == CT_TYPE) ||
                 (tmp->type == CT_OC_MSG_DECL))
                &&
                ((tmp2->type == CT_WORD) ||
                 (tmp2->type == CT_TYPE) ||
                 (tmp2->type == CT_PAREN_CLOSE)))
            {
               nas.Add(tmp);
            }
            did_line = true;
         }
         pc = chunk_get_next(pc, CNAV_PREPROC);
      }
      nas.End();
      cas.End();
   }
}
