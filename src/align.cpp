/**
 * @file align.cpp
 * Does all the aligning stuff.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */
#include "align.h"
#include "uncrustify_types.h"
#include "chunk_list.h"
#include "ChunkStack.h"
#include "align_stack.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "unc_ctype.h"
#include "uncrustify.h"
#include "indent.h"
#include "space.h"
#include "tabulator.h"


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
 *        int  height   : 12;
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
static void align_stack(ChunkStack &cs, size_t col, bool align_single, log_sev_t sev);


/**
 * Adds an item to the align stack and adjust the nl_count and max_col.
 * Adjust max_col as needed
 *
 * @param pc         the item to add
 * @param max_col    pointer to the column variable
 * @param extra_pad  extra padding
 */
static void align_add(ChunkStack &cs, chunk_t *pc, size_t &max_col, size_t min_pad, bool squeeze);


/**
 * Scan everything at the current level until the close brace and find the
 * variable def align column.  Also aligns bit-colons, but that assumes that
 * bit-types are the same! But that should always be the case...
 */
static chunk_t *align_var_def_brace(chunk_t *pc, size_t span, size_t *nl_count);


/**
 * For a series of lines ending in a comment, align them.
 * The series ends when more than align_right_cmt_span newlines are found.
 *
 * Interesting info:
 *  - least physically allowed column
 *  - intended column
 *  - least original cmt column
 *
 * min_col is the minimum allowed column (based on prev token col/size)
 * cmt_col less than
 *
 * @param start   Start point
 * @return        pointer the last item looked at
 */
static chunk_t *align_trailing_comments(chunk_t *start);


/**
 * Shifts out all columns by a certain amount.
 *
 * @param idx  The index to start shifting
 * @param num  The number of columns to shift
 */
void ib_shift_out(size_t idx, size_t num);


/**
 * If sq_open is CT_SQUARE_OPEN and the matching close is followed by '=',
 * then return the chunk after the '='.  Otherwise, return NULL.
 */
static chunk_t *skip_c99_array(chunk_t *sq_open);


/**
 * Scans a line for stuff to align on.
 *
 * We trigger on BRACE_OPEN, FPAREN_OPEN, ASSIGN, and COMMA.
 * We want to align the NEXT item.
 */
static chunk_t *scan_ib_line(chunk_t *start, bool first_pass);


static void align_log_al(log_sev_t sev, size_t line);


/**
 * Generically aligns on '=', '{', '(' and item after ','
 * It scans the first line and picks up the location of those tags.
 * It then scans subsequent lines and adjusts the column.
 * Finally it does a second pass to align everything.
 *
 * Aligns all the '=' signs in structure assignments.
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
static void align_init_brace(chunk_t *start);


static void align_func_params(void);


static void align_params(chunk_t *start, deque<chunk_t *> &chunks);


static void align_same_func_call_params(void);


chunk_t *step_back_over_member(chunk_t *pc);


/**
 * Aligns all function prototypes in the file.
 */
static void align_func_proto(size_t span);


/**
 * Aligns all function prototypes in the file.
 */
static void align_oc_msg_spec(size_t span);


static chunk_t *align_func_param(chunk_t *start);


/**
 * Aligns simple typedefs that are contained on a single line each.
 * This should be called after the typedef target is marked as a type.
 *
 * typedef int        foo_t;
 * typedef char       bar_t;
 * typedef const char cc_t;
 */
static void align_typedefs(size_t span);


/**
 * Align '<<' (CT_ARITH?)
 */
static void align_left_shift(void);


/**
 * Aligns OC messages
 */
static void align_oc_msg_colons(void);


/**
 * Aligns an OC message
 *
 * @param so   the square open of the message
 */
static void align_oc_msg_colon(chunk_t *so);


/**
 * Aligns OC declarations on the colon
 * -(void) doSomething: (NSString*) param1
 *                with: (NSString*) param2
 */
static void align_oc_decl_colon(void);


/**
 * Aligns asm declarations on the colon
 * asm volatile (
 *    "xxx"
 *    : "x"(h),
 *      "y"(l),
 *    : "z"(h)
 *    );
 */
static void align_asm_colon(void);


static void align_stack(ChunkStack &cs, size_t col, bool align_single, log_sev_t sev)
{
   LOG_FUNC_ENTRY();

   if (cpd.settings[UO_align_on_tabstop].b)
   {
      col = align_tab_column(col);
   }

   if ((cs.Len() > 1) || (align_single && (cs.Len() == 1)))
   {
      LOG_FMT(sev, "%s: max_col=%zu\n", __func__, col);
      chunk_t *pc;
      while ((pc = cs.Pop_Back()) != NULL)
      {
         align_to_column(pc, col);
         chunk_flags_set(pc, PCF_WAS_ALIGNED);

         LOG_FMT(sev, "%s: indented [%s] on line %zu to %zu\n",
                 __func__, pc->text(), pc->orig_line, pc->column);
      }
   }
   cs.Reset();
}


static void align_add(ChunkStack &cs, chunk_t *pc, size_t &max_col, size_t min_pad, bool squeeze)
{
   LOG_FUNC_ENTRY();

   size_t  min_col;
   chunk_t *prev = chunk_get_prev(pc);
   if ((prev == NULL) || chunk_is_newline(prev))
   {
      min_col = squeeze ? 1 : pc->column;
      LOG_FMT(LALADD, "%s: pc->orig_line=%zu, pc->col=%zu max_col=%zu min_pad=%zu min_col=%zu\n",
              __func__, pc->orig_line, pc->column, max_col, min_pad, min_col);
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
      LOG_FMT(LALADD, "%s: pc->orig_line=%zu, pc->col=%zu max_col=%zu min_pad=%zu min_col=%zu multi:%s prev->col=%d prev->len()=%zu %s\n",
              __func__, pc->orig_line, pc->column, max_col, min_pad, min_col, (prev->type == CT_COMMENT_MULTI) ? "Y" : "N",
              (prev->type == CT_COMMENT_MULTI) ? prev->orig_col_end : (UINT32)prev->column, prev->len(), get_token_name(prev->type));
   }

   if (cs.Empty())
   {
      max_col = 0;
   }

   cs.Push_Back(pc);
   if (min_col > max_col)
   {
      max_col = min_col;
   }
} // align_add


void quick_align_again(void)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LALAGAIN, "%s:\n", __func__);
   for (chunk_t *pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if ((pc->align.next != NULL) && (pc->flags & PCF_ALIGN_START))
      {
         AlignStack as;
         as.Start(100, 0);
         as.m_right_align = pc->align.right_align;
         as.m_star_style  = (AlignStack::StarStyle)pc->align.star_style;
         as.m_amp_style   = (AlignStack::StarStyle)pc->align.amp_style;
         as.m_gap         = pc->align.gap;

         LOG_FMT(LALAGAIN, "   [%s:%zu]", pc->text(), pc->orig_line);
         as.Add(pc->align.start);
         chunk_flags_set(pc, PCF_WAS_ALIGNED);
         for (chunk_t *tmp = pc->align.next; tmp != NULL; tmp = tmp->align.next)
         {
            chunk_flags_set(tmp, PCF_WAS_ALIGNED);
            as.Add(tmp->align.start);
            LOG_FMT(LALAGAIN, " => [%s:%zu]", tmp->text(), tmp->orig_line);
         }
         LOG_FMT(LALAGAIN, "\n");
         as.End();
      }
   }
}


void quick_indent_again(void)
{
   LOG_FUNC_ENTRY();

   for (chunk_t *pc = chunk_get_head(); pc; pc = chunk_get_next(pc))
   {
      if (pc->indent.ref)
      {
         chunk_t *tmp = chunk_get_prev(pc);
         if (chunk_is_newline(tmp))
         {
            size_t col = pc->indent.ref->column + pc->indent.delta;

            indent_to_column(pc, col);
            LOG_FMT(LINDENTAG, "%s: [%zu] indent [%s] to %zu based on [%s] @ %zu:%zu\n",
                    __func__, pc->orig_line, pc->text(), col,
                    pc->indent.ref->text(),
                    pc->indent.ref->orig_line, pc->indent.ref->column);
         }
      }
   }
}


void align_all(void)
{
   LOG_FUNC_ENTRY();
   if (cpd.settings[UO_align_typedef_span].u > 0)
   {
      align_typedefs(cpd.settings[UO_align_typedef_span].u);
   }

   if (cpd.settings[UO_align_left_shift].b)
   {
      align_left_shift();
   }

   if (cpd.settings[UO_align_oc_msg_colon_span].u > 0)
   {
      align_oc_msg_colons();
   }

   /* Align variable definitions */
   if ((cpd.settings[UO_align_var_def_span].u > 0) ||
       (cpd.settings[UO_align_var_struct_span].u > 0) ||
       (cpd.settings[UO_align_var_class_span].u > 0))
   {
      align_var_def_brace(chunk_get_head(), cpd.settings[UO_align_var_def_span].u, NULL);
   }

   /* Align assignments */
   align_assign(chunk_get_head(),
                cpd.settings[UO_align_assign_span].u,
                cpd.settings[UO_align_assign_thresh].u);

   /* Align structure initializers */
   if (cpd.settings[UO_align_struct_init_span].u > 0)
   {
      align_struct_initializers();
   }

   /* Align function prototypes */
   if ((cpd.settings[UO_align_func_proto_span].u > 0) &&
       !cpd.settings[UO_align_mix_var_proto].b)
   {
      align_func_proto(cpd.settings[UO_align_func_proto_span].u);
   }

   /* Align function prototypes */
   if (cpd.settings[UO_align_oc_msg_spec_span].u > 0)
   {
      align_oc_msg_spec(cpd.settings[UO_align_oc_msg_spec_span].u);
   }

   /* Align OC colons */
   if (cpd.settings[UO_align_oc_decl_colon].b)
   {
      align_oc_decl_colon();
   }

   if (cpd.settings[UO_align_asm_colon].b)
   {
      align_asm_colon();
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
} // align_all


static void align_oc_msg_spec(size_t span)
{
   LOG_FUNC_ENTRY();
   LOG_FMT(LALIGN, "%s\n", __func__);

   AlignStack as;
   as.Start(span, 0);

   for (chunk_t *pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
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


void align_backslash_newline(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = chunk_get_head();
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
   LOG_FUNC_ENTRY();

   for (chunk_t *pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if ((pc->type == CT_COMMENT) ||
          (pc->type == CT_COMMENT_CPP) ||
          (pc->type == CT_COMMENT_MULTI))
      {
         if (pc->parent_type == CT_COMMENT_END)
         {
            bool    skip  = false;
            chunk_t *prev = chunk_get_prev(pc);
            if (pc->orig_col < (prev->orig_col_end + cpd.settings[UO_align_right_cmt_gap].n))
            {
               // note the use of -5 here (-1 would probably have worked as well) to force
               // comments which are stuck to the previous token (gap=0) into alignment with the
               // others. Not the major feature, but a nice find. (min_val/max_val in
               // options.cpp isn't validated against, it seems; well, I don't mind! :-) )
               LOG_FMT(LALTC, "NOT changing END comment on line %zu (%zu <= %d + %d)\n",
                       pc->orig_line,
                       pc->orig_col, prev->orig_col_end, cpd.settings[UO_align_right_cmt_gap].n);
               skip = true;
            }
            if (!skip)
            {
               LOG_FMT(LALTC, "Changing END comment on line %zu into a RIGHT-comment\n",
                       pc->orig_line);
               chunk_flags_set(pc, PCF_RIGHT_COMMENT);
            }
         }

         /* Change certain WHOLE comments into RIGHT-alignable comments */
         if (pc->parent_type == CT_COMMENT_WHOLE)
         {
            size_t max_col = pc->column_indent + cpd.settings[UO_input_tab_size].u;

            /* If the comment is further right than the brace level... */
            if (pc->column >= max_col)
            {
               LOG_FMT(LALTC, "Changing WHOLE comment on line %zu into a RIGHT-comment (col=%zu col_ind=%zu max_col=%zu)\n",
                       pc->orig_line, pc->column, pc->column_indent, max_col);

               chunk_flags_set(pc, PCF_RIGHT_COMMENT);
            }
         }
      }
   }

   chunk_t *pc = chunk_get_head();
   while (pc != NULL)
   {
      if (pc->flags & PCF_RIGHT_COMMENT)
      {
         pc = align_trailing_comments(pc);
      }
      else
      {
         pc = chunk_get_next(pc);
      }
   }
} // align_right_comments


void align_struct_initializers(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = chunk_get_head();
   while (pc != NULL)
   {
      chunk_t *prev = chunk_get_prev_ncnl(pc);
      if ((prev != NULL) && (prev->type == CT_ASSIGN) &&
          ((pc->type == CT_BRACE_OPEN) ||
           ((cpd.lang_flags & LANG_D) && (pc->type == CT_SQUARE_OPEN))))
      {
         align_init_brace(pc);
      }
      pc = chunk_get_next_type(pc, CT_BRACE_OPEN, -1);
   }
}


void align_preprocessor(void)
{
   LOG_FUNC_ENTRY();

   AlignStack as;    // value macros
   as.Start(cpd.settings[UO_align_pp_define_span].u);
   as.m_gap = cpd.settings[UO_align_pp_define_gap].u;
   AlignStack *cur_as = &as;

   AlignStack asf;   // function macros
   asf.Start(cpd.settings[UO_align_pp_define_span].u);
   asf.m_gap = cpd.settings[UO_align_pp_define_gap].u;

   chunk_t *pc = chunk_get_head();
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

      LOG_FMT(LALPP, "%s: define (%s) on line %zu col %zu\n",
              __func__, pc->text(), pc->orig_line, pc->orig_col);

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

         LOG_FMT(LALPP, "%s: jumped to (%s) on line %zu col %zu\n",
                 __func__, pc->text(), pc->orig_line, pc->orig_col);
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
         LOG_FMT(LALPP, "%s: align on '%s', line %zu col %zu\n",
                 __func__, pc->text(), pc->orig_line, pc->orig_col);

         cur_as->Add(pc);
      }
   }

   as.End();
   asf.End();
} // align_preprocessor


chunk_t *align_assign(chunk_t *first, size_t span, size_t thresh)
{
   LOG_FUNC_ENTRY();

   if (first == NULL)
   {
      return(NULL);
   }
   size_t my_level = first->level;

   if (span == 0)
   {
      return(chunk_get_next(first));
   }

   LOG_FMT(LALASS, "%s[%zu]: checking %s on line %zu - span=%zu thresh=%zu\n",
           __func__, my_level, first->text(), first->orig_line, span, thresh);

   /* If we are aligning on a tabstop, we shouldn't right-align */
   AlignStack as;    // regular assigns
   as.Start(span, thresh);
   as.m_right_align = !cpd.settings[UO_align_on_tabstop].b;

   AlignStack vdas;  // variable def assigns
   vdas.Start(span, thresh);
   vdas.m_right_align = as.m_right_align;

   size_t  var_def_cnt = 0;
   size_t  equ_count   = 0;
   size_t  tmp;
   chunk_t *pc = first;
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
         size_t myspan;
         size_t mythresh;

         tmp = pc->orig_line;

         if (pc->parent_type == CT_ENUM)
         {
            myspan   = cpd.settings[UO_align_enum_equ_span].u;
            mythresh = cpd.settings[UO_align_enum_equ_thresh].u;
         }
         else
         {
            myspan   = cpd.settings[UO_align_assign_span].u;
            mythresh = cpd.settings[UO_align_assign_thresh].u;
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
      else if (pc->flags & PCF_VAR_DEF)
      {
         var_def_cnt++;
      }
      else if (var_def_cnt > 1)
      {
         /* we hit the second variable def - don't look for assigns, don't align */
         vdas.Reset();
      }
      else if ((equ_count == 0) && (pc->type == CT_ASSIGN))
      {
         //fprintf(stderr, "%s:  ** %s level=%d line=%zu col=%d prev=%d count=%d\n",
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
      LOG_FMT(LALASS, "%s: done on %s on line %zu\n",
              __func__, pc->text(), pc->orig_line);
   }
   else
   {
      LOG_FMT(LALASS, "%s: done on NULL\n", __func__);
   }

   return(pc);
} // align_assign


static chunk_t *align_func_param(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   AlignStack as;
   as.Start(2, 0);
   as.m_star_style = (AlignStack::StarStyle)cpd.settings[UO_align_var_def_star_style].u;
   as.m_amp_style  = (AlignStack::StarStyle)cpd.settings[UO_align_var_def_amp_style].u;

   bool    did_this_line = false;
   size_t  comma_count   = 0;
   size_t  chunk_count   = 0;

   chunk_t *pc = start;
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
} // align_func_param


static void align_func_params(void)
{
   LOG_FUNC_ENTRY();
   chunk_t *pc = chunk_get_head();
   while ((pc = chunk_get_next(pc)) != NULL)
   {
      if ((pc->type != CT_FPAREN_OPEN) ||
          ((pc->parent_type != CT_FUNC_PROTO) &&
           (pc->parent_type != CT_FUNC_DEF) &&
           (pc->parent_type != CT_FUNC_CLASS_PROTO) &&
           (pc->parent_type != CT_FUNC_CLASS_DEF) &&
           (pc->parent_type != CT_TYPEDEF)))
      {
         continue;
      }

      /* We're on a open paren of a prototype */
      pc = align_func_param(pc);
   }
}


static void align_params(chunk_t *start, deque<chunk_t *> &chunks)
{
   LOG_FUNC_ENTRY();

   chunks.clear();

   bool    hit_comma = true;
   chunk_t *pc       = chunk_get_next_type(start, CT_FPAREN_OPEN, start->level);
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


static void align_same_func_call_params(void)
{
   LOG_FUNC_ENTRY();
   chunk_t           *pc;
   chunk_t           *align_root = NULL;
   chunk_t           *align_cur  = NULL;
   size_t            align_len   = 0;
   chunk_t           *align_fcn;
   unc_text          align_fcn_name;
   unc_text          align_root_name;
   deque<chunk_t *>  chunks;
   deque<AlignStack> as;
   AlignStack        fcn_as;
   const char        *add_str;

   fcn_as.Start(3);

   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if (pc->type != CT_FUNC_CALL)
      {
         if (chunk_is_newline(pc))
         {
            for (size_t idx = 0; idx < as.size(); idx++)
            {
               as[idx].NewLines(pc->nl_count);
            }
            fcn_as.NewLines(pc->nl_count);
         }
         else
         {
            /* if we drop below the brace level that started it, we are done */
            if (align_root && (align_root->brace_level > pc->brace_level))
            {
               LOG_FMT(LASFCP, "  ++ (drop) Ended with %zu fcns\n", align_len);

               /* Flush it all! */
               fcn_as.Flush();
               for (size_t idx = 0; idx < as.size(); idx++)
               {
                  as[idx].Flush();
               }
               align_root = NULL;
            }
         }
         continue;
      }

      /* Only align function calls that are right after a newline */
      chunk_t *prev = chunk_get_prev(pc);
      while (chunk_is_token(prev, CT_MEMBER) || chunk_is_token(prev, CT_DC_MEMBER))
      {
         chunk_t *tprev = chunk_get_prev(prev);
         if (!chunk_is_token(tprev, CT_TYPE))
         {
            prev = tprev;
            break;
         }
         prev = chunk_get_prev(tprev);
      }
      if (!chunk_is_newline(prev))
      {
         continue;
      }
      prev      = chunk_get_next(prev);
      align_fcn = prev;
      align_fcn_name.clear();
      LOG_FMT(LASFCP, "(%d) align_fnc_name [%s]\n", __LINE__, align_fcn_name.c_str());
      while (prev != pc)
      {
         LOG_FMT(LASFCP, "(%d) align_fnc_name [%s]\n", __LINE__, align_fcn_name.c_str());
         align_fcn_name += prev->str;
         LOG_FMT(LASFCP, "(%d) align_fnc_name [%s]\n", __LINE__, align_fcn_name.c_str());
         prev = chunk_get_next(prev);
      }
      LOG_FMT(LASFCP, "(%d) align_fnc_name [%s]\n", __LINE__, align_fcn_name.c_str());
      align_fcn_name += pc->str;
      LOG_FMT(LASFCP, "(%d) align_fnc_name [%s]\n", __LINE__, align_fcn_name.c_str());
      LOG_FMT(LASFCP, "Func Call @ %zu:%zu [%s]\n",
              align_fcn->orig_line,
              align_fcn->orig_col,
              align_fcn_name.c_str());

      add_str = NULL;
      if (align_root != NULL)
      {
         /* can only align functions on the same brace level */
         if ((align_root->brace_level == pc->brace_level) &&
             align_fcn_name.equals(align_root_name))
         {
            fcn_as.Add(pc);
            align_cur->align.next = pc;
            align_cur             = pc;
            align_len++;
            add_str = "  Add";
         }
         else
         {
            LOG_FMT(LASFCP, "  ++ Ended with %zu fcns\n", align_len);

            /* Flush it all! */
            fcn_as.Flush();
            for (size_t idx = 0; idx < as.size(); idx++)
            {
               as[idx].Flush();
            }
            align_root = NULL;
         }
      }

      if (align_root == NULL)
      {
         fcn_as.Add(pc);
         align_root      = align_fcn;
         align_root_name = align_fcn_name;
         align_cur       = pc;
         align_len       = 1;
         add_str         = "Start";
      }

      if (add_str != NULL)
      {
         LOG_FMT(LASFCP, "%s '%s' on line %zu -",
                 add_str, align_fcn_name.c_str(), pc->orig_line);
         align_params(pc, chunks);
         LOG_FMT(LASFCP, " %d items:", (int)chunks.size());

         for (size_t idx = 0; idx < chunks.size(); idx++)
         {
            LOG_FMT(LASFCP, " [%s]", chunks[idx]->text());
            if (idx >= as.size())
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
      LOG_FMT(LASFCP, "  ++ Ended with %zu fcns\n", align_len);
      fcn_as.End();
      for (size_t idx = 0; idx < as.size(); idx++)
      {
         as[idx].End();
      }
   }
} // align_same_func_call_params


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


static void align_func_proto(size_t span)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LALIGN, "%s\n", __func__);

   AlignStack as;
   as.Start(span, 0);
   as.m_gap        = cpd.settings[UO_align_func_proto_gap].u;
   as.m_star_style = (AlignStack::StarStyle)cpd.settings[UO_align_var_def_star_style].u;
   as.m_amp_style  = (AlignStack::StarStyle)cpd.settings[UO_align_var_def_amp_style].u;

   AlignStack as_br;
   as_br.Start(span, 0);
   as_br.m_gap = cpd.settings[UO_align_single_line_brace_gap].u;

   bool    look_bro = false;
   chunk_t *toadd;

   for (chunk_t *pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
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
} // align_func_proto


static chunk_t *align_var_def_brace(chunk_t *start, size_t span, size_t *p_nl_count)
{
   LOG_FUNC_ENTRY();

   if (start == NULL)
   {
      return(NULL);
   }

   chunk_t *next;
   size_t  myspan   = span;
   size_t  mythresh = 0;
   size_t  mygap    = 0;

   /* Override the span, if this is a struct/union */
   if ((start->parent_type == CT_STRUCT) ||
       (start->parent_type == CT_UNION))
   {
      myspan   = cpd.settings[UO_align_var_struct_span].u;
      mythresh = cpd.settings[UO_align_var_struct_thresh].u;
      mygap    = cpd.settings[UO_align_var_struct_gap].u;
   }
   else if (start->parent_type == CT_CLASS)
   {
      myspan   = cpd.settings[UO_align_var_class_span].u;
      mythresh = cpd.settings[UO_align_var_class_thresh].u;
      mygap    = cpd.settings[UO_align_var_class_gap].u;
   }
   else
   {
      mythresh = cpd.settings[UO_align_var_def_thresh].u;
      mygap    = cpd.settings[UO_align_var_def_gap].u;
   }

   /* can't be any variable definitions in a "= {" block */
   chunk_t *prev = chunk_get_prev_ncnl(start);
   if ((prev != NULL) && (prev->type == CT_ASSIGN))
   {
      LOG_FMT(LAVDB, "%s: start=%s [%s] on line %zu (abort due to assign)\n", __func__,
              start->text(), get_token_name(start->type), start->orig_line);

      chunk_t *pc = chunk_get_next_type(start, CT_BRACE_CLOSE, start->level);
      return(chunk_get_next_ncnl(pc));
   }

   LOG_FMT(LAVDB, "%s: start=%s [%s] on line %zu\n", __func__,
           start->text(), get_token_name(start->type), start->orig_line);

   UINT64 align_mask = PCF_IN_FCN_DEF | PCF_VAR_1ST;
   if (!cpd.settings[UO_align_var_def_inline].b)
   {
      align_mask |= PCF_VAR_INLINE;
   }

   /* Set up the var/proto/def aligner */
   AlignStack as;
   as.Start(myspan, mythresh);
   as.m_gap        = mygap;
   as.m_star_style = (AlignStack::StarStyle)cpd.settings[UO_align_var_def_star_style].u;
   as.m_amp_style  = (AlignStack::StarStyle)cpd.settings[UO_align_var_def_amp_style].u;

   /* Set up the bit colon aligner */
   AlignStack as_bc;
   as_bc.Start(myspan, 0);
   as_bc.m_gap = cpd.settings[UO_align_var_def_colon_gap].n;

   AlignStack as_at; /* attribute */
   as_at.Start(myspan, 0);

   /* Set up the brace open aligner */
   AlignStack as_br;
   as_br.Start(myspan, mythresh);
   as_br.m_gap = cpd.settings[UO_align_single_line_brace_gap].u;

   bool    fp_look_bro   = false;
   bool    did_this_line = false;
   bool    fp_active     = cpd.settings[UO_align_mix_var_proto].b;
   chunk_t *pc           = chunk_get_next(start);
   while ((pc != NULL) && ((pc->level >= start->level) || (pc->level == 0)))
   {
      LOG_FMT(LGUY, "%s: pc->text()=%s, pc->orig_line=%zu, pc->orig_col=%zu\n",
              __func__, pc->text(), pc->orig_line, pc->orig_col);
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

      if (fp_active && !(pc->flags & PCF_IN_CLASS_BASE))
      {
         if ((pc->type == CT_FUNC_PROTO) ||
             ((pc->type == CT_FUNC_DEF) &&
              cpd.settings[UO_align_single_line_func].b))
         {
            LOG_FMT(LAVDB, "    add=[%s] line=%zu col=%zu level=%zu\n",
                    pc->text(), pc->orig_line, pc->orig_col, pc->level);

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
         size_t sub_nl_count = 0;

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
      if (!(pc->flags & PCF_IN_CLASS_BASE) &&
          (pc->type != CT_FUNC_CLASS_DEF) &&
          (pc->type != CT_FUNC_CLASS_PROTO) &&
          ((pc->flags & align_mask) == PCF_VAR_1ST) &&
          ((pc->level == (start->level + 1)) ||
           (pc->level == 0)) &&
          pc->prev && (pc->prev->type != CT_MEMBER))
      {
         if (!did_this_line)
         {
            if ((start->parent_type == CT_STRUCT) &&
                (as.m_star_style == AlignStack::SS_INCLUDE))
            {
               // we must look after the previous token
               chunk_t *prev_local = pc->prev;
               while ((prev_local->type == CT_PTR_TYPE) ||
                      (prev_local->type == CT_ADDR))
               {
                  LOG_FMT(LAVDB, "    prev_local=%s, prev_local->type=%s\n",
                          prev_local->text(), get_token_name(prev_local->type));
                  prev_local = prev_local->prev;
               }
               pc = prev_local->next;
            }
            LOG_FMT(LAVDB, "    add=[%s] line=%zu col=%zu level=%zu\n",
                    pc->text(), pc->orig_line, pc->orig_col, pc->level);

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
} // align_var_def_brace


chunk_t *align_nl_cont(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LALNLC, "%s: start on [%s] on line %zu\n", __func__,
           get_token_name(start->type), start->orig_line);

   /* Find the max column */
   ChunkStack cs;
   size_t     max_col = 0;
   chunk_t    *pc     = start;
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
   chunk_t *tmp;
   while ((tmp = cs.Pop_Back()) != NULL)
   {
      chunk_flags_set(tmp, PCF_WAS_ALIGNED);
      tmp->column = max_col;
   }

   return(pc);
}


static CmtAlignType_t get_comment_align_type(chunk_t *cmt)
{
   chunk_t        *prev;
   CmtAlignType_t cmt_type = CAT_REGULAR;

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


chunk_t *align_trailing_comments(chunk_t *start)
{
   LOG_FUNC_ENTRY();
   size_t         min_col  = 0;
   size_t         min_orig = 0;
   chunk_t        *pc      = start;
   size_t         nl_count = 0;
   ChunkStack     cs;
   size_t         col;
   size_t         intended_col = cpd.settings[UO_align_right_cmt_at_col].u;
   CmtAlignType_t cmt_type_cur;
   CmtAlignType_t cmt_type_start = get_comment_align_type(pc);

   LOG_FMT(LALADD, "%s: start on line=%zu\n",
           __func__, pc->orig_line);

   /* Find the max column */
   while ((pc != NULL) && (nl_count < cpd.settings[UO_align_right_cmt_span].u))
   {
      if ((pc->flags & PCF_RIGHT_COMMENT) && (pc->column > 1))
      {
         cmt_type_cur = get_comment_align_type(pc);

         if (cmt_type_cur == cmt_type_start)
         {
            col = 1 + (pc->brace_level * cpd.settings[UO_indent_columns].u);
            LOG_FMT(LALADD, "%s: line=%zu col=%zu min_col=%zu pc->col=%zu pc->len=%zu %s\n",
                    __func__, pc->orig_line, col, min_col, pc->column, pc->len(),
                    get_token_name(pc->type));
            if ((min_orig == 0) || (min_orig > pc->column))
            {
               min_orig = pc->column;
            }
            if (pc->column < col)
            {
               pc->column = col;
            }
            align_add(cs, pc, min_col, 1, true); // (intended_col < col));
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
   LOG_FMT(LALADD, "%s:  -- min_orig=%zu intended_col=%zu min_allowed=%zu ==> col=%zu\n",
           __func__, min_orig, intended_col, min_col, col);
   if ((cpd.frag_cols > 0) && (cpd.frag_cols <= col))
   {
      col -= cpd.frag_cols;
   }
   align_stack(cs, col, (intended_col != 0), LALTC);

   return(chunk_get_next(pc));
} // align_trailing_comments


void ib_shift_out(size_t idx, size_t num)
{
   while (idx < cpd.al_cnt)
   {
      cpd.al[idx].col += num;
      idx++;
   }
}


static chunk_t *skip_c99_array(chunk_t *sq_open)
{
   if (chunk_is_token(sq_open, CT_SQUARE_OPEN))
   {
      chunk_t *tmp = chunk_get_next_nc(chunk_skip_to_match(sq_open));

      if (chunk_is_token(tmp, CT_ASSIGN))
      {
         return(chunk_get_next_nc(tmp));
      }
   }
   return(NULL);
}


static chunk_t *scan_ib_line(chunk_t *start, bool first_pass)
{
   UNUSED(first_pass);
   LOG_FUNC_ENTRY();
   chunk_t *prev_match = NULL;
   size_t  idx         = 0;

   /* Skip past C99 "[xx] =" stuff */
   chunk_t *tmp = skip_c99_array(start);
   if (tmp)
   {
      set_chunk_parent(start, CT_TSQUARE);
      start            = tmp;
      cpd.al_c99_array = true;
   }
   chunk_t *pc = start;

   if (pc != NULL)
   {
      LOG_FMT(LSIB, "%s: start=%s col %zu/%zu line %zu\n",
              __func__, get_token_name(pc->type), pc->column, pc->orig_col, pc->orig_line);
   }

   while ((pc != NULL) && !chunk_is_newline(pc) &&
          (pc->level >= start->level))
   {
      //LOG_FMT(LSIB, "%s:     '%s'   col %d/%d line %zu\n", __func__,
      //        pc->text(), pc->column, pc->orig_col, pc->orig_line);

      chunk_t *next = chunk_get_next(pc);
      if ((next == NULL) || chunk_is_comment(next))
      {
         /* do nothing */
      }
      else if ((pc->type == CT_ASSIGN) ||
               (pc->type == CT_BRACE_OPEN) ||
               (pc->type == CT_BRACE_CLOSE) ||
               (pc->type == CT_COMMA))
      {
         size_t token_width = space_col_align(pc, next);

         /*TODO: need to handle missing structure defs? ie NULL vs { ... } ?? */

         /* Is this a new entry? */
         if (idx >= cpd.al_cnt)
         {
            LOG_FMT(LSIB, " - New   [%zu] %.2zu/%zu - %10.10s\n",
                    idx, pc->column, token_width, get_token_name(pc->type));
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
               LOG_FMT(LSIB, " - Match [%zu] %.2zu/%zu - %10.10s",
                       idx, pc->column, token_width, get_token_name(pc->type));

               /* Shift out based on column */
               if (prev_match == NULL)
               {
                  if (pc->column > cpd.al[idx].col)
                  {
                     LOG_FMT(LSIB, " [ pc->col(%zu) > col(%zu) ] ",
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
               LOG_FMT(LSIB, " - now col %zu, len %zu\n", cpd.al[idx].col, cpd.al[idx].len);
               idx++;
            }
         }
         prev_match = pc;
      }
      pc = chunk_get_next_nc(pc);
   }
   return(pc);
} // scan_ib_line


static void align_log_al(log_sev_t sev, size_t line)
{
   if (log_sev_on(sev))
   {
      log_fmt(sev, "%s: line %zu, %zu)",
              __func__, line, cpd.al_cnt);
      for (size_t idx = 0; idx < cpd.al_cnt; idx++)
      {
         log_fmt(sev, " %zu/%zu=%s", cpd.al[idx].col, cpd.al[idx].len,
                 get_token_name(cpd.al[idx].type));
      }
      log_fmt(sev, "\n");
   }
}


static void align_init_brace(chunk_t *start)
{
   LOG_FUNC_ENTRY();

   chunk_t *num_token = NULL;

   cpd.al_cnt       = 0;
   cpd.al_c99_array = false;

   LOG_FMT(LALBR, "%s: start @ %zu:%zu\n", __func__, start->orig_line, start->orig_col);

   chunk_t *pc = chunk_get_next_ncnl(start);
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

   pc = chunk_get_next(start);
   size_t idx = 0;
   do
   {
      chunk_t *tmp;
      if ((idx == 0) && ((tmp = skip_c99_array(pc)) != NULL))
      {
         pc = tmp;
         if (pc)
         {
            LOG_FMT(LALBR, " -%zu- skipped '[] =' to %s\n",
                    pc->orig_line, get_token_name(pc->type));
         }
         continue;
      }

      chunk_t *next = pc;
      if (idx < cpd.al_cnt)
      {
         LOG_FMT(LALBR, " (%zu) check %s vs %s -- ",
                 idx, get_token_name(pc->type), get_token_name(cpd.al[idx].type));
         if (pc->type == cpd.al[idx].type)
         {
            if ((idx == 0) && cpd.al_c99_array)
            {
               chunk_t *prev = chunk_get_prev(pc);
               if (chunk_is_newline(prev))
               {
                  chunk_flags_set(pc, PCF_DONT_INDENT);
               }
            }
            LOG_FMT(LALBR, " [%s] to col %zu\n", pc->text(), cpd.al[idx].col);

            if (num_token != NULL)
            {
               int col_diff = pc->column - num_token->column;

               reindent_line(num_token, cpd.al[idx].col - col_diff);
               //LOG_FMT(LSYS, "-= %zu =- NUM indent [%s] col=%d diff=%d\n",
               //        num_token->orig_line,
               //        num_token->text(), cpd.al[idx - 1].col, col_diff);

               chunk_flags_set(num_token, PCF_WAS_ALIGNED);
               num_token = NULL;
            }

            /* Comma's need to 'fall back' to the previous token */
            if (pc->type == CT_COMMA)
            {
               next = chunk_get_next(pc);
               if ((next != NULL) && !chunk_is_newline(next))
               {
                  //LOG_FMT(LSYS, "-= %zu =- indent [%s] col=%d len=%d\n",
                  //        next->orig_line,
                  //        next->text(), cpd.al[idx].col, cpd.al[idx].len);

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
                  else if (idx < (cpd.al_cnt - 1))
                  {
                     reindent_line(next, cpd.al[idx].col + cpd.al[idx].len);
                     chunk_flags_set(next, PCF_WAS_ALIGNED);
                  }
               }
            }
            else
            {
               /* first item on the line */
               reindent_line(pc, cpd.al[idx].col);
               chunk_flags_set(pc, PCF_WAS_ALIGNED);

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
} // align_init_brace


static void align_typedefs(size_t span)
{
   LOG_FUNC_ENTRY();

   AlignStack as;
   as.Start(span);
   as.m_gap        = cpd.settings[UO_align_typedef_gap].u;
   as.m_star_style = (AlignStack::StarStyle)cpd.settings[UO_align_typedef_star_style].u;
   as.m_amp_style  = (AlignStack::StarStyle)cpd.settings[UO_align_typedef_amp_style].u;

   chunk_t *c_typedef = NULL;
   chunk_t *pc        = chunk_get_head();
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
            LOG_FMT(LALTD, "%s: typedef @ %zu:%zu, tag '%s' @ %zu:%zu\n",
                    __func__, c_typedef->orig_line, c_typedef->orig_col,
                    pc->text(), pc->orig_line, pc->orig_col);
            c_typedef = NULL;
         }
      }
      else
      {
         if (pc->type == CT_TYPEDEF)
         {
            c_typedef = pc;
         }
      }

      pc = chunk_get_next(pc);
   }

   as.End();
} // align_typedefs


static void align_left_shift(void)
{
   LOG_FUNC_ENTRY();

   chunk_t    *start = NULL;
   AlignStack as;
   as.Start(255);

   chunk_t *pc = chunk_get_head();
   while (pc != NULL)
   {
      if ((start != NULL) &&
          ((pc->flags & PCF_IN_PREPROC) != (start->flags & PCF_IN_PREPROC)))
      {
         /* a change in preproc status restarts the aligning */
         as.Flush();
         start = NULL;
      }
      else if (chunk_is_newline(pc))
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
      else if (!(pc->flags & PCF_IN_ENUM) && chunk_is_str(pc, "<<", 2))
      {
         if (pc->parent_type == CT_OPERATOR)
         {
            /* Ignore operator<< */
         }
         else if (as.m_aligned.Empty())
         {
            /* check if the first one is actually on a blank line and then
             * indent it. Eg:
             *
             *      cout
             *          << "something";
             */
            chunk_t *prev = chunk_get_prev(pc);
            if (prev && chunk_is_newline(prev))
            {
               indent_to_column(pc, pc->column_indent + cpd.settings[UO_indent_columns].u);
               pc->column_indent = pc->column;
               pc->flags        |= PCF_DONT_INDENT;
            }

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
      else if (!as.m_aligned.Empty())
      {
         /* check if the given statement is on a line of its own, immediately following <<
          * and then it. Eg:
          *
          *      cout <<
          *          "something";
          */
         chunk_t *prev = chunk_get_prev(pc);
         if (prev && chunk_is_newline(prev))
         {
            indent_to_column(pc, pc->column_indent + cpd.settings[UO_indent_columns].u);
            pc->column_indent = pc->column;
            pc->flags        |= PCF_DONT_INDENT;
         }
      }

      pc = chunk_get_next(pc);
   }
   as.End();
} // align_left_shift


static void align_oc_msg_colon(chunk_t *so)
{
   LOG_FUNC_ENTRY();

   AlignStack nas;   /* for the parameter tag */
   nas.Reset();
   nas.m_right_align = !cpd.settings[UO_align_on_tabstop].b;

   AlignStack cas;   /* for the colons */
   size_t     span = cpd.settings[UO_align_oc_msg_colon_span].u;
   cas.Start(span);

   size_t  level = so->level;
   chunk_t *pc   = chunk_get_next_ncnl(so, CNAV_PREPROC);

   bool    did_line  = false;
   bool    has_colon = false;
   size_t  lcnt      = 0; /* line count with no colon for span */

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
      else if (!did_line && (lcnt < span + 1) && (pc->type == CT_OC_COLON))
      {
         has_colon = true;
         cas.Add(pc);
         chunk_t *tmp = chunk_get_prev(pc);
         if ((tmp != NULL) &&
             ((tmp->type == CT_OC_MSG_FUNC) ||
              (tmp->type == CT_OC_MSG_NAME)))
         {
            nas.Add(tmp);
            chunk_flags_set(tmp, PCF_DONT_INDENT);
         }
         did_line = true;
      }
      pc = chunk_get_next(pc, CNAV_PREPROC);
   }

   nas.m_skip_first = !cpd.settings[UO_align_oc_msg_colon_first].b;
   cas.m_skip_first = !cpd.settings[UO_align_oc_msg_colon_first].b;

   /* find the longest args that isn't the first one */
   size_t  first_len = 0;
   size_t  mlen      = 0;
   chunk_t *longest  = NULL;

   size_t  len = nas.m_aligned.Len();
   for (size_t idx = 0; idx < len; idx++)
   {
      chunk_t *tmp = nas.m_aligned.GetChunk(idx);

      size_t  tlen = tmp->str.size();
      if (tlen > mlen)
      {
         mlen = tlen;
         if (idx != 0)
         {
            longest = tmp;
         }
      }
      if (idx == 0)
      {
         first_len = tlen + 1;
      }
   }

   /* add spaces before the longest arg */
   len = cpd.settings[UO_indent_oc_msg_colon].u;
   size_t len_diff    = mlen - first_len;
   size_t indent_size = cpd.settings[UO_indent_columns].u;
   /* Align with first colon if possible by removing spaces */
   if (longest &&
       cpd.settings[UO_indent_oc_msg_prioritize_first_colon].b &&
       (len_diff > 0) &&
       ((longest->column - len_diff) > (longest->brace_level * indent_size)))
   {
      longest->column -= len_diff;
   }
   else if (longest && (len > 0))
   {
      chunk_t chunk;

      chunk.type        = CT_SPACE;
      chunk.orig_line   = longest->orig_line;
      chunk.parent_type = CT_NONE;
      chunk.level       = longest->level;
      chunk.brace_level = longest->brace_level;
      chunk.flags       = longest->flags & PCF_COPY_FLAGS;

      /* start at one since we already indent for the '[' */
      for (size_t idx = 1; idx < len; idx++)
      {
         chunk.str.append(' ');
      }

      chunk_add_before(&chunk, longest);
   }
   nas.End();
   cas.End();
} // align_oc_msg_colon


static void align_oc_msg_colons(void)
{
   LOG_FUNC_ENTRY();

   for (chunk_t *pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if ((pc->type == CT_SQUARE_OPEN) && (pc->parent_type == CT_OC_MSG))
      {
         align_oc_msg_colon(pc);
      }
   }
}


static void align_oc_decl_colon(void)
{
   LOG_FUNC_ENTRY();

   bool       did_line;
   AlignStack cas;   /* for the colons */
   AlignStack nas;   /* for the parameter label */
   cas.Start(4);
   nas.Start(4);
   nas.m_right_align = !cpd.settings[UO_align_on_tabstop].b;

   chunk_t *pc = chunk_get_head();
   while (pc != NULL)
   {
      if (pc->type != CT_OC_SCOPE)
      {
         pc = chunk_get_next(pc);
         continue;
      }

      nas.Reset();
      cas.Reset();

      size_t level = pc->level;
      pc = chunk_get_next_ncnl(pc, CNAV_PREPROC);

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
            did_line = false;
         }
         else if (!did_line && (pc->type == CT_OC_COLON))
         {
            cas.Add(pc);

            chunk_t *tmp  = chunk_get_prev(pc, CNAV_PREPROC);
            chunk_t *tmp2 = chunk_get_prev_ncnl(tmp, CNAV_PREPROC);

            /* Check for an un-labeled parameter */
            if ((tmp != NULL) &&
                (tmp2 != NULL)
                &&
                ((tmp->type == CT_WORD) ||
                 (tmp->type == CT_TYPE) ||
                 (tmp->type == CT_OC_MSG_DECL) ||
                 (tmp->type == CT_OC_MSG_SPEC))
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
} // align_oc_decl_colon


static void align_asm_colon(void)
{
   LOG_FUNC_ENTRY();

   bool       did_nl;
   AlignStack cas;   /* for the colons */
   cas.Start(4);

   chunk_t *pc = chunk_get_head();
   while (pc != NULL)
   {
      if (pc->type != CT_ASM_COLON)
      {
         pc = chunk_get_next(pc);
         continue;
      }

      cas.Reset();

      pc = chunk_get_next_ncnl(pc, CNAV_PREPROC);
      size_t level = pc ? pc->level : 0;
      did_nl = true;
      while (pc && (pc->level >= level))
      {
         if (chunk_is_newline(pc))
         {
            cas.NewLines(pc->nl_count);
            did_nl = true;
         }
         else if (pc->type == CT_ASM_COLON)
         {
            cas.Flush();
            did_nl = true;
         }
         else if (did_nl)
         {
            did_nl = false;
            cas.Add(pc);
         }
         pc = chunk_get_next_nc(pc, CNAV_PREPROC);
      }
      cas.End();
   }
} // align_asm_colon
