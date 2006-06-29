/**
 * @file c_output.c
 * Does all the output & comment formatting.
 *
 * $Id$
 */

#include "uncrustify_types.h"
#include "prototypes.h"
#include "chunk_list.h"
#include <cstring>
#include <cstdlib>




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



void output_parsed(FILE *pfile)
{
   chunk_t *pc;
   int     cnt;

   fprintf(pfile, "-== Options ==-\n");
   output_options(pfile);
   fprintf(pfile, "-=====-\n");
   fprintf(pfile, "Line      Tag          Parent     Columns  Br/Lvl Flg Nl  Text");
   for (pc = chunk_get_head(); pc != NULL; pc = chunk_get_next(pc))
   {
      fprintf(pfile, "\n%3d> %13.13s[%13.13s][%2d/%2d/%2d][%d/%d][%4x][%d-%d]",
              pc->orig_line, get_token_name(pc->type),
              get_token_name(pc->parent_type),
              pc->column, pc->orig_col, pc->orig_col_end,
              pc->brace_level, pc->level, pc->flags, pc->nl_count, pc->after_tab);

      if ((pc->type != CT_NEWLINE) && (pc->len != 0))
      {
         for (cnt = 0; cnt < pc->column; cnt++)
         {
            fprintf(pfile, " ");
         }
         fprintf(pfile, "%.*s", pc->len, pc->str);
      }
   }
   fprintf(pfile, "\n-=====-\n");
   fflush(pfile);
}

void output_options(FILE *pfile)
{
   int                    idx;
   const options_name_tab *ptr;

   for (idx = 0; idx < UO_option_count; idx++)
   {
      ptr = get_option_name(idx);
      if (ptr != NULL)
      {
         if (ptr->type == AT_BOOL)
         {
            fprintf(pfile, "%3d) %32s = %s\n",
                    ptr->id, ptr->name,
                    cpd.settings[ptr->id].b ? "True" : "False");
         }
         else if (ptr->type == AT_IARF)
         {
            fprintf(pfile, "%3d) %32s = %s\n",
                    ptr->id, ptr->name,
                    (cpd.settings[ptr->id].a == AV_ADD) ? "Add" :
                    (cpd.settings[ptr->id].a == AV_REMOVE) ? "Remove" :
                    (cpd.settings[ptr->id].a == AV_FORCE) ? "Force" : "Ignore");
         }
         else  /* AT_NUM */
         {
            fprintf(pfile, "%3d) %32s = %d\n",
                    ptr->id, ptr->name, cpd.settings[ptr->id].n);
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
      else if (pc->type == CT_COMMENT_MULTI)
      {
         output_comment_multi(pc);
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
               lvlcol = 1 + (pc->brace_level * cpd.settings[UO_indent_columns].n);
               output_to_column(lvlcol, true);
            }
            allow_tabs = (cpd.settings[UO_indent_with_tabs].n == 2) ||
                         (chunk_is_comment(pc) &&
                          (cpd.settings[UO_indent_with_tabs].n != 0));

            LOG_FMT(LOUTIND, "  %d> col %d/%d - ", pc->orig_line, pc->column, cpd.column);
         }
         else
         {
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


void output_comment_multi(chunk_t *pc)
{
   int        idx     = 0;
   int        cmt_col = pc->column;
   const char *str    = pc->str;
   char       ch;
   chunk_t    *prev;
   char       line[1024];
   int        len        = 0;
   int        line_count = 0;
   int        ccol;
   int        col_diff    = 0;
   int        lead_width  = -1;
   int        first_width = 2;
   int        xtra        = 1;

   prev = chunk_get_prev(pc);
   if ((prev != NULL) && (prev->type != CT_NEWLINE))
   {
      cmt_col = pc->orig_col;
   }
   else
   {
      col_diff = pc->orig_col - pc->column;
   }

   //   fprintf(stderr, "Indenting1 line %d to col %d (orig=%d) col_diff=%d\n",
   //           pc->orig_line, cmt_col, pc->orig_col, col_diff);

   ccol = 1;
   while (idx < pc->len)
   {
      ch = str[idx++];

      /* handle the CRLF and CR endings. convert both to LF */
      if (ch == '\r')
      {
         ch = '\n';
         if (str[idx] == '\n')
         {
            idx++;
         }
      }

      /* Find the start column */
      if (len == 0)
      {
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
      line[len++] = ch;

      /* If we just hit an end of line OR we just hit end-of-comment... */
      if ((ch == '\n') || (idx >= pc->len) ||
          ((len > 2) && (line[len - 2] == '*') && (ch == '/')))
      {
         line_count++;

         /* strip trailing tabs and spaces before the newline */
         if (ch == '\n')
         {
            len--;
            while ((len > 0) && ((line[len - 1] == ' ') || (line[len - 1] == '\t')))
            {
               len--;
            }
            line[len++] = ch;
         }
         line[len] = 0;

         /* Recognized line starts: ' *', '**', '||', or nothing */
         if (line_count == 1)
         {
            while (line[first_width] == '*')
            {
               first_width++;
            }
            /* this is the first line - add unchanged */
            output_to_column(cmt_col, cpd.settings[UO_indent_with_tabs].b);
            add_text_len(line, len);
         }
         else
         {
            /* This is not the first line, so we need to indent to the right
             * column
             */

            if (lead_width < 0)
            {
               /** count the number of lead chars */
               lead_width = 0;
               while ((line[lead_width] == '*') ||
                      (line[lead_width] == '|') ||
                      (line[lead_width] == '\\') ||
                      (line[lead_width] == '#'))
               {
                  lead_width++;
               }

               xtra = 0;
               if ((lead_width <= 1) || (lead_width == (first_width - 1)))
               {
                  xtra = 1;
               }
            }

            ccol -= col_diff;
            if (ccol < cmt_col)
            {
               ccol = cmt_col;
            }

            if (line[0] == '\n')
            {
               /* Emtpy line - just a '\n' */
               if (cpd.settings[UO_cmt_star_cont].b)
               {
                  output_to_column(cmt_col, cpd.settings[UO_indent_with_tabs].b);
                  add_text(" *");
               }
               add_char('\n');
            }
            else
            {
               /* If this doesn't start with a '*' or '|' */
               if ((line[0] != '*') && (line[0] != '|') && (line[0] != '#') &&
                   (line[0] != '\\'))
               {
                  output_to_column(cmt_col, cpd.settings[UO_indent_with_tabs].b);
                  if (cpd.settings[UO_cmt_star_cont].b)
                  {
                     add_text(" * ");
                  }
                  else
                  {
                     add_text("   ");
                  }
                  output_to_column(ccol, cpd.settings[UO_indent_with_tabs].b);
               }
               else
               {
                  if ((line[0] == '*') && (line[1] == '/'))
                  {
                     xtra = (lead_width <= 1) ? 1 : 0;
                  }
                  output_to_column(cmt_col + xtra, cpd.settings[UO_indent_with_tabs].b);
               }
               add_text_len(line, len);
            }
         }
         len  = 0;
         ccol = 1;
      }
   }
}
