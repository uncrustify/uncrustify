/**
 * @file output.cpp
 * Does all the output & comment formatting.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 *
 * $Id$
 */

#include "uncrustify_types.h"
#include "prototypes.h"
#include "chunk_list.h"
#include "unc_ctype.h"
#include <cstring>
#include <cstdlib>


struct cmt_reflow
{
   chunk_t    *pc;
   int        column;      /* Column of the comment start */
   int        br_column;   /* Brace column (for indenting with tabs) */
   int        word_count;  /* number of words on this line */
   bool       kw_subst;    /* do keyword substitution */
   const char *cont_text;  /* fixed text to output at the start of the line (3-chars) */
};


static chunk_t *output_comment_c(chunk_t *pc);
static chunk_t *output_comment_cpp(chunk_t *pc);
static void add_comment_text(const char *text, int len,
                             cmt_reflow& cmt, bool esc_close);


void add_char(char ch)
{
   /* convert a newline into the LF/CRLF/CR sequence */
   if (ch == '\n')
   {
      fputs(cpd.newline, cpd.fout);
      cpd.column      = 1;
      cpd.did_newline = 1;
   }
   else
   {
      fputc(ch, cpd.fout);
      if (ch == '\t')
      {
         cpd.column = next_tab_column(cpd.column);
      }
      else
      {
         cpd.column++;
      }
   }
}

void add_text(const char *text)
{
   char ch;

   while ((ch = *text) != 0)
   {
      text++;
      add_char(ch);
   }
}

void add_text_len(const char *text, int len)
{
   while (len-- > 0)
   {
      add_char(*text);
      text++;
   }
}

/**
 * Advance to a specific column
 * cpd.column is the current column
 *
 * @param column  The column to advance to
 */
void output_to_column(int column, bool allow_tabs)
{
   int nc;

   cpd.did_newline = 0;
   if (allow_tabs)
   {
      /* tab out as far as possible and then use spaces */
      while ((nc = next_tab_column(cpd.column)) <= column)
      {
         add_text("\t");
      }
   }
   /* space out the final bit */
   while (cpd.column < column)
   {
      add_text(" ");
   }
}

void output_indent(int column, int brace_col)
{
   if ((cpd.column == 1) && (cpd.settings[UO_indent_with_tabs].n != 0))
   {
      if (cpd.settings[UO_indent_with_tabs].n == 2)
      {
         brace_col = column;
      }

      /* tab out as far as possible and then use spaces */
      int nc;
      while ((nc = next_tab_column(cpd.column)) <= brace_col)
      {
         add_text("\t");
      }
   }

   /* space out the rest */
   while (cpd.column < column)
   {
      add_text(" ");
   }
}

void output_parsed(FILE *pfile)
{
   chunk_t *pc;
   int     cnt;

   output_options(pfile);
   output_defines(pfile);
   output_types(pfile);

   fprintf(pfile, "-=====-\n");
   fprintf(pfile, "Line      Tag          Parent     Columns  Br/Lvl/pp Flag Nl  Text");
   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      fprintf(pfile, "\n%3d> %13.13s[%13.13s][%2d/%2d/%2d][%d/%d/%d][%8x][%d-%d]",
              pc->orig_line, get_token_name(pc->type),
              get_token_name(pc->parent_type),
              pc->column, pc->orig_col, pc->orig_col_end,
              pc->brace_level, pc->level, pc->pp_level,
              pc->flags, pc->nl_count, pc->after_tab);

      if ((pc->type != CT_NEWLINE) && (pc->len != 0))
      {
         for (cnt = 0; cnt < pc->column; cnt++)
         {
            fprintf(pfile, " ");
         }
         if (pc->type != CT_NL_CONT)
         {
            fprintf(pfile, "%.*s", pc->len, pc->str);
         }
         else
         {
            fprintf(pfile, "\\");
         }
      }
   }
   fprintf(pfile, "\n-=====-\n");
   fflush(pfile);
}

void output_options(FILE *pfile)
{
   int idx;
   const option_map_value *ptr;

   fprintf(pfile, "-== Options ==-\n");
   for (idx = 0; idx < UO_option_count; idx++)
   {
      ptr = get_option_name(idx);
      if (ptr != NULL)
      {
         if (ptr->type == AT_STRING)
         {
            fprintf(pfile, "%3d) %32s = \"%s\"\n",
                    ptr->id, ptr->name,
                    op_val_to_string(ptr->type, cpd.settings[ptr->id]).c_str());
         }
         else
         {
            fprintf(pfile, "%3d) %32s = %s\n",
                    ptr->id, ptr->name,
                    op_val_to_string(ptr->type, cpd.settings[ptr->id]).c_str());
         }
      }
   }
}

/**
 * This renders the chunk list to a file.
 */
void output_text(FILE *pfile)
{
   chunk_t *pc;
   chunk_t *prev;
   int     cnt;
   int     lvlcol;
   bool    allow_tabs;

   cpd.fout = pfile;

   cpd.column = 1;
   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      if (pc->type == CT_NEWLINE)
      {
         for (cnt = 0; cnt < pc->nl_count; cnt++)
         {
            add_char('\n');
         }
         cpd.did_newline = 1;
         cpd.column      = 1;
         LOG_FMT(LOUTIND, " xx\n");
      }
      else if (pc->type == CT_NL_CONT)
      {
         output_to_column(pc->column, (cpd.settings[UO_indent_with_tabs].n == 2));
         add_char('\\');
         add_char('\n');
         cpd.did_newline = 1;
         cpd.column      = 1;
         LOG_FMT(LOUTIND, " \\xx\n");
      }
      else if (pc->type == CT_COMMENT_MULTI)
      {
         output_comment_multi(pc);
      }
      else if (pc->type == CT_COMMENT_CPP)
      {
         pc = output_comment_cpp(pc);
      }
      else if (pc->type == CT_COMMENT)
      {
         pc = output_comment_c(pc);
      }
      else if (pc->len == 0)
      {
         /* don't do anything for non-visible stuff */
         LOG_FMT(LOUTIND, " <%d> -", pc->column);
      }
      else
      {
         /* indent to the 'level' first */
         if (cpd.did_newline)
         {
            if (cpd.settings[UO_indent_with_tabs].n == 1)
            {
               /* FIXME: it would be better to properly set column_indent in
                * indent_text(), but this hack for '}' and ':' seems to work. */
               if ((pc->type == CT_BRACE_CLOSE) ||
                   chunk_is_str(pc, ":", 1))
               {
                  lvlcol = pc->column;
               }
               else
               {
                  lvlcol = pc->column_indent;
                  if (lvlcol > pc->column)
                  {
                     lvlcol = pc->column;
                  }
               }

               if (lvlcol > 1)
               {
                  output_to_column(lvlcol, true);
               }
            }
            allow_tabs = (cpd.settings[UO_indent_with_tabs].n == 2) ||
                         (chunk_is_comment(pc) &&
                          (cpd.settings[UO_indent_with_tabs].n != 0));

            LOG_FMT(LOUTIND, "  %d> col %d/%d - ", pc->orig_line, pc->column, cpd.column);
         }
         else
         {
            /**
             * Reformatting multi-line comments can screw up the column.
             * Make sure we don't mess up the spacing on this line.
             * This has to be done here because comments are not formatted
             * until the output phase.
             */
            if (pc->column < cpd.column)
            {
               reindent_line(pc, cpd.column);
            }

            /* not the first item on a line */
            if (cpd.settings[UO_align_keep_tabs].b)
            {
               allow_tabs = pc->after_tab;
            }
            else
            {
               prev       = chunk_get_prev(pc);
               allow_tabs = (cpd.settings[UO_align_with_tabs].b &&
                             ((pc->flags & PCF_WAS_ALIGNED) != 0) &&
                             (((pc->column - 1) % cpd.settings[UO_output_tab_size].n) == 0) &&
                             ((prev->column + prev->len + 1) != pc->column));
            }
            LOG_FMT(LOUTIND, " %d -", pc->column);
         }

         output_to_column(pc->column, allow_tabs);
         add_text_len(pc->str, pc->len);
         cpd.did_newline = chunk_is_newline(pc);
      }
   }
}

/**
 * Given a multi-line comemnt block that starts in column X, figure out how
 * much subsequent lines should be indented.
 *
 * The answer is either 0 or 1.
 *
 * The decision is based on:
 *  - the first line length
 *  - the second line leader length
 *  - the last line length
 *
 * If the first and last line are the same length and don't contain any alnum
 * chars and (the first line len > 2 or the second leader is the same as the
 * first line length), then the indent is 0.
 *
 * If the leader on the second line is 1 wide or missing, then the indent is 1.
 *
 * Otherwise, the indent is 0.
 *
 * @param str       The comment string
 * @param len       Length of the comment
 * @param start_col Starting column
 * @return 0 or 1
 */
static int calculate_comment_body_indent(const char *str, int len, int start_col)
{
   int idx       = 0;
   int first_len = 0;
   int last_len  = 0;
   int width     = 0;

   if (!cpd.settings[UO_cmt_indent_multi].b)
   {
      return(0);
   }

   /* find the last line length */
   for (idx = len - 1; idx > 0; idx--)
   {
      if ((str[idx] == '\n') || (str[idx] == '\r'))
      {
         idx++;
         while ((idx < len) && ((str[idx] == ' ') || (str[idx] == '\t')))
         {
            idx++;
         }
         last_len = len - idx;
         break;
      }
   }

   /* find the first line length */
   for (idx = 0; idx < len; idx++)
   {
      if ((str[idx] == '\n') || (str[idx] == '\r'))
      {
         first_len = idx;
         while ((str[first_len - 1] == ' ') || (str[first_len - 1] == '\t'))
         {
            first_len--;
         }

         /* handle DOS endings */
         if ((str[idx] == '\r') && (str[idx + 1] == '\n'))
         {
            idx++;
         }
         idx++;
         break;
      }
   }

   /* Scan the second line */
   width = 0;
   for ( /* nada */; idx < len; idx++)
   {
      if ((str[idx] == ' ') || (str[idx] == '\t'))
      {
         if (width > 0)
         {
            break;
         }
         continue;
      }
      if ((str[idx] == '\n') || (str[idx] == '\r'))
      {
         /* Done with second line */
         break;
      }

      /* Count the leading chars */
      if ((str[idx] == '*') ||
          (str[idx] == '|') ||
          (str[idx] == '\\') ||
          (str[idx] == '#') ||
          (str[idx] == '+'))
      {
         width++;
      }
      else
      {
         break;
      }
   }

   //LOG_FMT(LSYS, "%s: first=%d last=%d width=%d\n", __func__, first_len, last_len, width);

   /*TODO: make the first_len minimum (4) configurable? */
   if ((first_len == last_len) && ((first_len > 4) || (first_len == width)))
   {
      return(0);
   }

   return((width == 2) ? 0 : 1);
}

static_inline void add_spaces_before_star()
{
   int count = cpd.settings[UO_cmt_sp_before_star_cont].n;

   while (count-- > 0)
   {
      add_char(' ');
   }
}

static_inline void add_spaces_after_star()
{
   if (cpd.settings[UO_cmt_star_cont].b)
   {
      int count = cpd.settings[UO_cmt_sp_after_star_cont].n;
      while (count-- > 0)
      {
         add_char(' ');
      }
   }
}

static chunk_t *get_next_function(chunk_t *pc)
{
   while ((pc = chunk_get_next(pc)) != NULL)
   {
      if ((pc->type == CT_FUNC_DEF) ||
          (pc->type == CT_FUNC_PROTO))
      {
         return(pc);
      }
   }
   return(NULL);
}

static chunk_t *get_next_class(chunk_t *pc)
{
   while ((pc = chunk_get_next(pc)) != NULL)
   {
      if (pc->type == CT_CLASS)
      {
         return(chunk_get_next(pc));
      }
   }
   return(NULL);
}

/**
 * Adds the javadoc-style @param and @return stuff, based on the params and
 * return value for pc.
 * If the arg list is '()' or '(void)', then no @params are added.
 * Likewise, if the reutrn value is 'void', then no @return is added.
 */
static void add_comment_javaparam(chunk_t *pc, cmt_reflow& cmt)
{
   chunk_t *fpo;
   chunk_t *fpc;
   chunk_t *tmp;
   chunk_t *prev;
   bool    has_param = true;
   bool    need_nl   = false;
   int     col       = cpd.column;

   fpo = chunk_get_next_type(pc, CT_FPAREN_OPEN, pc->level);
   if (fpo == NULL)
   {
      return;
   }
   fpc = chunk_get_next_type(fpo, CT_FPAREN_CLOSE, pc->level);
   if (fpc == NULL)
   {
      return;
   }

   /* Check for 'foo()' and 'foo(void)' */
   if (chunk_get_next_ncnl(fpo) == fpc)
   {
      has_param = false;
   }
   else
   {
      tmp = chunk_get_next_ncnl(fpo);
      if ((tmp == chunk_get_prev_ncnl(fpc)) &&
          chunk_is_str(tmp, "void", 4))
      {
         has_param = false;
      }
   }

   if (has_param)
   {
      tmp  = fpo;
      prev = NULL;
      while ((tmp = chunk_get_next(tmp)) != NULL)
      {
         if ((tmp->type == CT_COMMA) || (tmp == fpc))
         {
            if (need_nl)
            {
               add_comment_text("\n", 1, cmt, false);
               output_to_column(col, false);
            }
            need_nl = true;
            add_text("@param");
            if (prev != NULL)
            {
               add_text(" ");
               add_text_len(prev->str, prev->len);
               add_text(" TODO");
            }
            prev = NULL;
            if (tmp == fpc)
            {
               break;
            }
         }
         if (tmp->type == CT_WORD)
         {
            prev = tmp;
         }
      }
   }

   /* Do the return stuff */
   tmp = chunk_get_prev_ncnl(pc);
   if ((tmp != NULL) && !chunk_is_str(tmp, "void", 4))
   {
      if (need_nl)
      {
         add_comment_text("\n", 1, cmt, false);
         output_to_column(col, false);
      }
      add_text("@return TODO");
   }
}

/**
 * text starts with '$('. see if this matches a keyword and add text based
 * on that keyword.
 * @return the number of characters eaten from the text
 */
static int add_comment_kw(const char *text, int len, cmt_reflow& cmt)
{
   if ((len >= 11) && (memcmp(text, "$(filename)", 11) == 0))
   {
      add_text(path_basename(cpd.filename));
      return(11);
   }
   if ((len >= 8) && (memcmp(text, "$(class)", 8) == 0))
   {
      chunk_t *tmp = get_next_class(cmt.pc);
      if (tmp != NULL)
      {
         add_text_len(tmp->str, tmp->len);
         return(8);
      }
   }

   /* If we can't find the function, we are done */
   chunk_t *fcn = get_next_function(cmt.pc);
   if (fcn == NULL)
   {
      return(0);
   }

   if ((len >= 11) && (memcmp(text, "$(function)", 11) == 0))
   {
      add_text_len(fcn->str, fcn->len);
      return(11);
   }
   if ((len >= 12) && (memcmp(text, "$(javaparam)", 12) == 0))
   {
      add_comment_javaparam(fcn, cmt);
      return(12);
   }
   if ((len >= 9) && (memcmp(text, "$(fclass)", 9) == 0))
   {
      chunk_t *tmp = chunk_get_prev_ncnl(fcn);
      if ((tmp != NULL) && ((tmp->type == CT_DC_MEMBER) ||
                            (tmp->type == CT_MEMBER)))
      {
         tmp = chunk_get_prev_ncnl(tmp);
         add_text_len(tmp->str, tmp->len);
         return(9);
      }
   }
   return(0);
}

/**
 * Outputs a comment. The initial opening '//' may be included in the text.
 * Subsequent openings (if combining comments), should not be included.
 * The closing (for C/D comments) should not be included.
 *
 * TODO:
 * If reflowing text, the comment should be added one word (or line) at a time.
 * A newline should only be sent if a blank line is encountered or if the next
 * line is indented beyond the current line (optional?).
 * If the last char on a line is a ':' or '.', then the next line won't be
 * combined.
 */
static void add_comment_text(const char *text, int len,
                             cmt_reflow& cmt, bool esc_close)
{
   bool was_star   = false;
   bool was_dollar = false;
   bool in_word    = false;

   for (int idx = 0; idx < len; idx++)
   {
      if (!was_dollar && cmt.kw_subst &&
          (text[idx] == '$') && (len > (idx + 3)) && (text[idx + 1] == '('))
      {
         idx += add_comment_kw(&text[idx], len - idx, cmt);
         if (idx >= len)
         {
            break;
         }
      }

      /* Split the comment */
      if ((text[idx] == '\n') ||
          ((text[idx] == ' ') &&
           (cpd.settings[UO_cmt_width].n > 0) &&
           (cpd.column > cpd.settings[UO_cmt_width].n)))
      {
         in_word = false;
         add_char('\n');
         output_indent(cmt.column, cmt.br_column);
         add_text(cmt.cont_text);
      }
      else
      {
         /* Escape a C closure in a CPP comment */
         if (esc_close && was_star && (text[idx] == '/'))
         {
            add_char(' ');
         }
         if (!in_word && !unc_isspace(text[idx]))
         {
            cmt.word_count++;
         }
         in_word = !unc_isspace(text[idx]);
         add_char(text[idx]);
         was_star   = (text[idx] == '*');
         was_dollar = (text[idx] == '$');
      }
   }
}

static void output_cmt_start(cmt_reflow& cmt, chunk_t *pc)
{
   cmt.word_count = 0;
   cmt.pc         = pc;
   cmt.column     = pc->column;
   cmt.br_column  = pc->column_indent;

   if ((pc->parent_type == CT_COMMENT_START) ||
       (pc->parent_type == CT_COMMENT_WHOLE))
   {
      if (!cpd.settings[UO_indent_col1_comment].b &&
          (pc->orig_col == 1))
      {
         cmt.column    = 1;
         cmt.br_column = 1;
      }
   }
   else if (pc->parent_type == CT_COMMENT_END)
   {
      /* Make sure we have at least one space past the last token */
      chunk_t *prev = chunk_get_prev(pc);
      if (prev != NULL)
      {
         int col_min = prev->column + prev->len + 1;
         if (cmt.column < col_min)
         {
            cmt.column = col_min;
         }
      }
   }

   /* Bump out to the column */
   output_indent(cmt.column, cmt.br_column);

   cmt.kw_subst = (pc->flags & PCF_INSERTED) != 0;
}

/**
 * Checks to see if the current comment can be combined with the next comment.
 * The two can be combined if:
 *  1. They are the same type
 *  2. There is exactly one newline between then
 *  3. They are indented to the same level
 */
static bool can_combine_comment(chunk_t *pc, cmt_reflow& cmt)
{
   /* We can't combine if there is something other than a newline next */
   if (pc->parent_type == CT_COMMENT_START)
   {
      return(false);
   }

   /* next is a newline for sure, make sure it is a single newline */
   chunk_t *next = chunk_get_next(pc);
   if ((next != NULL) && (next->nl_count == 1))
   {
      /* Make sure the comment is the same type at the same column */
      next = chunk_get_next(next);
      if ((next != NULL) &&
          (next->type == pc->type) &&
          (((next->column == 1) && (pc->column == 1)) ||
           ((next->column == cmt.br_column) && (pc->column == cmt.br_column)) ||
           ((next->column > cmt.br_column) && (pc->parent_type == CT_COMMENT_END))))
      {
         return(true);
      }
   }
   return(false);
}

/**
 * Outputs the C comment at pc.
 * C comment combining is done here
 *
 * @return the last chunk output'd
 */
static chunk_t *output_comment_c(chunk_t *first)
{
   cmt_reflow cmt;

   output_cmt_start(cmt, first);

   cmt.cont_text = cpd.settings[UO_cmt_star_cont].b ? " *" : "  ";

   /* See if we can combine this comment with the next comment */
   if (!cpd.settings[UO_cmt_c_group].b ||
       !can_combine_comment(first, cmt))
   {
      /* Just add the single comment */
      add_comment_text(first->str, first->len, cmt, false);
      return(first);
   }

   add_text_len("/*", 2);
   if (cpd.settings[UO_cmt_c_nl_start].b)
   {
      add_comment_text("\n", 1, cmt, false);
   }
   chunk_t *pc = first;
   while (can_combine_comment(pc, cmt))
   {
      add_comment_text(pc->str + 2, pc->len - 4, cmt, false);
      add_comment_text("\n", 1, cmt, false);
      pc = chunk_get_next(chunk_get_next(pc));
   }
   add_comment_text(pc->str + 2, pc->len - 4, cmt, false);
   if (cpd.settings[UO_cmt_c_nl_end].b)
   {
      cmt.cont_text = " ";
      add_comment_text("\n", 1, cmt, false);
   }
   add_comment_text("*/", 2, cmt, false);
   return(pc);
}

/**
 * Outputs the CPP comment at pc.
 * CPP comment combining is done here
 *
 * @return the last chunk output'd
 */
static chunk_t *output_comment_cpp(chunk_t *first)
{
   cmt_reflow cmt;

   output_cmt_start(cmt, first);

   /* CPP comments can't be grouped unless they are converted to C comments */
   if (!cpd.settings[UO_cmt_cpp_to_c].b)
   {
      cmt.cont_text = "// ";
      add_comment_text(first->str, first->len, cmt, false);
      return(first);
   }

   /* We are going to convert the CPP comments to C comments */
   cmt.cont_text = cpd.settings[UO_cmt_star_cont].b ? " * " : "   ";

   /* See if we can combine this comment with the next comment */
   if (!cpd.settings[UO_cmt_cpp_group].b ||
       !can_combine_comment(first, cmt))
   {
      /* nothing to group: just output a single line */
      add_text_len("/*", 2);
      if (!unc_isspace(first->str[2]))
      {
         add_char(' ');
      }
      add_comment_text(first->str + 2, first->len - 2, cmt, true);
      add_text_len(" */", 3);
      return(first);
   }

   add_text_len("/*", 2);
   if (cpd.settings[UO_cmt_cpp_nl_start].b)
   {
      add_comment_text("\n", 1, cmt, false);
   }
   else
   {
      add_text_len(" ", 1);
   }
   chunk_t *pc = first;

   int offs;
   while (can_combine_comment(pc, cmt))
   {
      offs = unc_isspace(pc->str[2]) ? 1 : 0;
      add_comment_text(pc->str + 2 + offs, pc->len - (2 + offs), cmt, true);
      add_comment_text("\n", 1, cmt, false);
      pc = chunk_get_next(chunk_get_next(pc));
   }
   offs = unc_isspace(pc->str[2]) ? 1 : 0;
   add_comment_text(pc->str + 2 + offs, pc->len - (2 + offs), cmt, true);
   if (cpd.settings[UO_cmt_cpp_nl_end].b)
   {
      cmt.cont_text = "";
      add_comment_text("\n", 1, cmt, false);
   }
   add_comment_text(" */", 3, cmt, false);
   return(pc);
}

/**
 * A multiline comment -- woopeee!
 * The only trick here is that we have to trim out whitespace characters
 * to get the comment to line up.
 */
void output_comment_multi(chunk_t *pc)
{
   int        cmt_col = pc->column;
   const char *cmt_str;
   int        remaining;
   char       ch;
   chunk_t    *prev;
   char       line[1024];
   int        line_len;
   int        line_count = 0;
   int        ccol;
   int        col_diff = 0;
   int        xtra     = 1;
   char       lead[5];
   bool       nl_end = false;

   cmt_reflow cmt;

   output_cmt_start(cmt, pc);
   cmt.cont_text = !cpd.settings[UO_cmt_indent_multi].b ? "" :
                   cpd.settings[UO_cmt_star_cont].b ? " * " : "   ";

   prev = chunk_get_prev(pc);
   if ((prev != NULL) && (prev->type != CT_NEWLINE))
   {
      cmt_col = pc->orig_col;
   }
   else
   {
      col_diff = pc->orig_col - pc->column;
   }

   xtra = calculate_comment_body_indent(pc->str, pc->len, pc->column);

   // fprintf(stderr, "Indenting1 line %d to col %d (orig=%d) col_diff=%d xtra=%d\n",
   //         pc->orig_line, cmt_col, pc->orig_col, col_diff, xtra);

   ccol      = 1;
   remaining = pc->len;
   cmt_str   = pc->str;
   line_len  = 0;
   while (remaining > 0)
   {
      ch = *cmt_str;
      cmt_str++;
      remaining--;

      /* handle the CRLF and CR endings. convert both to LF */
      if (ch == '\r')
      {
         ch = '\n';
         if (*cmt_str == '\n')
         {
            cmt_str++;
            remaining--;
         }
      }

      /* Find the start column */
      if (line_len == 0)
      {
         nl_end = false;
         if (ch == ' ')
         {
            ccol++;
            continue;
         }
         else if (ch == '\t')
         {
            ccol = calc_next_tab_column(ccol, cpd.settings[UO_input_tab_size].n);
            continue;
         }
         else
         {
            //fprintf(stderr, "%d] Text starts in col %d\n", line_count, ccol);
         }
      }

      line[line_len++] = ch;

      /* If we just hit an end of line OR we just hit end-of-comment... */
      if ((ch == '\n') || (remaining == 0))
      {
         line_count++;

         /* strip trailing tabs and spaces before the newline */
         if (ch == '\n')
         {
            line_len--;
            while ((line_len > 0) &&
                   ((line[line_len - 1] == ' ') ||
                    (line[line_len - 1] == '\t')))
            {
               line_len--;
            }
            if ((line[line_len - 1] == '\\') && (line[line_len - 2] != '*'))
            {
               /* Kill off the backslash-newline */
               line_len--;
               while ((line_len > 0) &&
                      ((line[line_len - 1] == ' ') ||
                       (line[line_len - 1] == '\t')))
               {
                  line_len--;
               }
               line[line_len++] = ' ';
               line[line_len++] = '\\';
            }
            nl_end = true;
         }
         line[line_len] = 0;

         if (line_count == 1)
         {
            /* this is the first line - add unchanged */

            /*TODO: need to support indent_with_tabs mode 1 */
            output_to_column(cmt_col, cpd.settings[UO_indent_with_tabs].n != 0);
            add_comment_text(line, line_len, cmt, false);
            if (nl_end)
            {
               add_text_len("\n", 1);
            }
         }
         else
         {
            /* This is not the first line, so we need to indent to the
             * correct column.
             */
            ccol -= col_diff;
            if (ccol < cmt_col)
            {
               ccol = cmt_col;
            }

            if (line[0] == 0)
            {
               /* Emtpy line - just a '\n' */
               if (cpd.settings[UO_cmt_star_cont].b)
               {
                  output_to_column(cmt_col, cpd.settings[UO_indent_with_tabs].n != 0);
                  add_spaces_before_star();
                  add_text((xtra == 1) ? " *" : "*");
               }
               add_char('\n');
            }
            else
            {
               add_spaces_before_star();

               /* If this doesn't start with a '*' or '|' */
               if (cpd.settings[UO_cmt_indent_multi].b &&
                   (line[0] != '*') && (line[0] != '|') && (line[0] != '#') &&
                   (line[0] != '\\') && (line[0] != '+'))
               {
                  output_to_column(cmt_col, cpd.settings[UO_indent_with_tabs].n != 0);
                  if (cpd.settings[UO_cmt_star_cont].b)
                  {
                     cmt.cont_text = (xtra == 1) ? " * " : "*  ";
                     add_spaces_after_star();
                  }
                  else
                  {
                     cmt.cont_text = "   ";
                  }
                  add_text(cmt.cont_text);
                  output_to_column(ccol, cpd.settings[UO_indent_with_tabs].n != 0);
               }
               else
               {
                  output_to_column(cmt_col + xtra, cpd.settings[UO_indent_with_tabs].n != 0);
                  int idx  = 0;
                  int sidx = 0;
                  if (xtra > 0)
                  {
                     lead[idx++] = ' ';
                  }
                  while ((idx < 3) && !unc_isspace(line[sidx]))
                  {
                     lead[idx++] = line[sidx++];
                  }
                  lead[idx]     = 0;
                  cmt.cont_text = lead;
               }

               add_comment_text(line, line_len, cmt, false);
               if (nl_end)
               {
                  add_text_len("\n", 1);
               }
            }
         }
         line_len = 0;
         ccol     = 1;
      }
   }
}
