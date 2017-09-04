/**
 * @file output.cpp
 * Does all the output & comment formatting.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */
#include "output.h"
#include "uncrustify_types.h"
#include "prototypes.h"
#include "chunk_list.h"
#include "unc_ctype.h"
#include "uncrustify.h"
#include "indent.h"
#include "braces.h"
#include "unicode.h"
#include "helper_for_print.h"
#include <cstdlib>


struct cmt_reflow
{
   chunk_t  *pc;
   size_t   column;        //! Column of the comment start
   size_t   brace_col;     //! Brace column (for indenting with tabs)
   size_t   base_col;      //! Base column (for indenting with tabs)
   size_t   word_count;    //! number of words on this line
   size_t   xtra_indent;   //! extra indent of non-first lines (0 or 1)
   unc_text cont_text;     //! fixed text to output at the start of a line (0 to 3 chars)
   bool     reflow;        //! reflow the current line
};


/**
 * A multiline comment
 * The only trick here is that we have to trim out whitespace characters
 * to get the comment to line up.
 */
static void output_comment_multi(chunk_t *pc);


static bool kw_fcn_filename(chunk_t *cmt, unc_text &out_txt);


static bool kw_fcn_class(chunk_t *cmt, unc_text &out_txt);


static bool kw_fcn_message(chunk_t *cmt, unc_text &out_txt);


static bool kw_fcn_category(chunk_t *cmt, unc_text &out_txt);


static bool kw_fcn_scope(chunk_t *cmt, unc_text &out_txt);


static bool kw_fcn_function(chunk_t *cmt, unc_text &out_txt);


/**
 * Adds the javadoc-style @param and @return stuff, based on the params and
 * return value for pc.
 * If the arg list is '()' or '(void)', then no @params are added.
 * Likewise, if the return value is 'void', then no @return is added.
 */
static bool kw_fcn_javaparam(chunk_t *cmt, unc_text &out_txt);


static bool kw_fcn_fclass(chunk_t *cmt, unc_text &out_txt);


/**
 * Output a multiline comment without any reformatting other than shifting
 * it left or right to get the column right.
 * Trim trailing whitespace and do keyword substitution.
 */
static void output_comment_multi_simple(chunk_t *pc, bool kw_subst);


/**
 * This renders the #if condition to a string buffer.
 *
 * @param[out] dst    unc_text buffer to be filled
 * @param[in]  ifdef  if conditional as chunk list
 */
static void generate_if_conditional_as_text(unc_text &dst, chunk_t *ifdef);


/**
 * Do keyword substitution on a comment.
 * NOTE: it is assumed that a comment will contain at most one of each type
 * of keyword.
 */
static void do_kw_subst(chunk_t *pc);


//! All output text is sent here, one char at a time.
static void add_char(UINT32 ch);


static void add_text(const char *ascii_text);


static void add_text(const unc_text &text, bool is_ignored);


/**
 * Count the number of characters to the end of the next chunk of text.
 * If it exceeds the limit, return true.
 */
static bool next_word_exceeds_limit(const unc_text &text, size_t idx);


/**
 * Output a comment to the column using indent_with_tabs and
 * indent_cmt_with_tabs as the rules.
 * base_col is the indent of the first line of the comment.
 * On the first line, column == base_col.
 * On subsequent lines, column >= base_col.
 *
 * @param brace_col  the brace-level indent of the comment
 * @param base_col   the indent of the start of the comment (multiline)
 * @param column     the column that we should end up in
 */
static void cmt_output_indent(size_t brace_col, size_t base_col, size_t column);


/**
 * Checks for and updates the lead chars.
 *
 * @param line the comment line
 *
 * @return 0: not present, >0: number of chars that are part of the lead
 */
static size_t cmt_parse_lead(const unc_text &line, bool is_last);


/**
 * Scans a multiline comment to determine the following:
 *  - the extra indent of the non-first line (0 or 1)
 *  - the continuation text ('' or '* ')
 *
 * The decision is based on:
 *  - cmt_indent_multi
 *  - cmt_star_cont
 *  - cmt_multi_first_len_minimum
 *  - the first line length
 *  - the second line leader length
 *  - the last line length (without leading space/tab)
 *
 * If the first and last line are the same length and don't contain any alnum
 * chars and (the first line len > 2 or the second leader is the same as the
 * first line length), then the indent is 0.
 *
 * If the leader on the second line is 1 wide or missing, then the indent is 1.
 *
 * Otherwise, the indent is 0.
 *
 * @param str        The comment string
 * @param len        Length of the comment
 * @param start_col  Starting column
 *
 * @return cmt.xtra_indent is set to 0 or 1
 */
static void calculate_comment_body_indent(cmt_reflow &cmt, const unc_text &str);


static int next_up(const unc_text &text, size_t idx, unc_text &tag);


/**
 * Outputs the C comment at pc.
 * C comment combining is done here
 *
 * @return the last chunk output'd
 */
static chunk_t *output_comment_c(chunk_t *pc);


/**
 * Outputs the CPP comment at pc.
 * CPP comment combining is done here
 *
 * @return the last chunk output'd
 */
static chunk_t *output_comment_cpp(chunk_t *pc);


static void cmt_trim_whitespace(unc_text &line, bool in_preproc);


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
static void add_comment_text(const unc_text &text, cmt_reflow &cmt, bool esc_close);


static void output_cmt_start(cmt_reflow &cmt, chunk_t *pc);


/**
 * Checks to see if the current comment can be combined with the next comment.
 * The two can be combined if:
 *  1. They are the same type
 *  2. There is exactly one newline between then
 *  3. They are indented to the same level
 */
static bool can_combine_comment(chunk_t *pc, cmt_reflow &cmt);


#define LOG_CONTTEXT() \
   LOG_FMT(LCONTTEXT, "%s:%d set cont_text to '%s'\n", __func__, __LINE__, cmt.cont_text.c_str())


static void add_char(UINT32 ch)
{
   // If we did a '\r' and it isn't followed by a '\n', then output a newline
   if ((cpd.last_char == '\r') && (ch != '\n'))
   {
      write_string(cpd.newline);
      cpd.column      = 1;
      cpd.did_newline = 1;
      cpd.spaces      = 0;
   }

   // convert a newline into the LF/CRLF/CR sequence
   if (ch == '\n')
   {
      write_string(cpd.newline);
      cpd.column      = 1;
      cpd.did_newline = 1;
      cpd.spaces      = 0;
   }
   else if (ch == '\r') // do not output the CARRIAGERETURN
   {
      // do not output '\r'
      cpd.column      = 1;
      cpd.did_newline = 1;
      cpd.spaces      = 0;
   }
   else if ((ch == '\t') && cpd.output_tab_as_space)
   {
      size_t endcol = next_tab_column(cpd.column);
      while (cpd.column < endcol)
      {
         add_char(' ');
      }
      return;
   }
   else
   {
      // explicitly disallow a tab after a space
      if (ch == '\t' && cpd.last_char == ' ')
      {
         size_t endcol = next_tab_column(cpd.column);
         while (cpd.column < endcol)
         {
            add_char(' ');
         }
         return;
      }

      if ((ch == ' ') && !cpd.output_trailspace)
      {
         cpd.spaces++;
         cpd.column++;
      }
      else
      {
         while (cpd.spaces > 0)
         {
            write_char(' ');
            cpd.spaces--;
         }
         write_char(ch);
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
   cpd.last_char = ch;
} // add_char


static void add_text(const char *ascii_text)
{
   char ch;

   while ((ch = *ascii_text) != 0)
   {
      ascii_text++;
      add_char(ch);
   }
}


static void add_text(const unc_text &text, bool is_ignored = false)
{
   for (size_t idx = 0; idx < text.size(); idx++)
   {
      int ch = text[idx];
      if (is_ignored)
      {
         write_char(ch);
      }
      else
      {
         add_char(ch);
      }
   }
}


static bool next_word_exceeds_limit(const unc_text &text, size_t idx)
{
   size_t length = 0;

   // Count any whitespace
   while ((idx < text.size()) && unc_isspace(text[idx]))
   {
      idx++;
      length++;
   }

   // Count non-whitespace
   while ((idx < text.size()) && !unc_isspace(text[idx]))
   {
      idx++;
      length++;
   }
   return((cpd.column + length - 1) > cpd.settings[UO_cmt_width].u);
}


/**
 * Advance to a specific column
 * cpd.column is the current column
 *
 * @param column  The column to advance to
 */
static void output_to_column(size_t column, bool allow_tabs)
{
   cpd.did_newline = 0;
   if (allow_tabs)
   {
      // tab out as far as possible and then use spaces
      size_t next_column = next_tab_column(cpd.column);
      while (next_column <= column)
      {
         add_text("\t");
         next_column = next_tab_column(cpd.column);
      }
   }
   // space out the final bit
   while (cpd.column < column)
   {
      add_text(" ");
   }
}


static void cmt_output_indent(size_t brace_col, size_t base_col, size_t column)
{
   size_t iwt = cpd.settings[UO_indent_cmt_with_tabs].b ? 2 :
                (cpd.settings[UO_indent_with_tabs].u ? 1 : 0);

   size_t tab_col = (iwt == 0) ? 0 : ((iwt == 1) ? brace_col : base_col);

   // LOG_FMT(LSYS, "%s(brace=%d base=%d col=%d iwt=%d) tab=%d cur=%d\n",
   //        __func__, brace_col, base_col, column, iwt, tab_col, cpd.column);

   cpd.did_newline = 0;
   if (  iwt == 2
      || (cpd.column == 1 && iwt == 1))
   {
      // tab out as far as possible and then use spaces
      while (next_tab_column(cpd.column) <= tab_col)
      {
         add_text("\t");
      }
   }

   // space out the rest
   while (cpd.column < column)
   {
      add_text(" ");
   }
} // cmt_output_indent


void output_parsed(FILE *pfile)
{
   // save_option_file(pfile, false);
   save_option_file_kernel(pfile, false, true);

   fprintf(pfile, "# -=====-\n");
   fprintf(pfile, "# Line              Tag           Parent          Columns Br/Lvl/pp     Flag   Nl  Text");
   for (chunk_t *pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      char *outputMessage;
      outputMessage = make_message("\n# %3zu> %16.16s[%16.16s][%3zu/%3zu/%3zu/%3d][%zu/%zu/%zu][%10" PRIx64 "][%zu-%d]",
                                   pc->orig_line, get_token_name(pc->type),
                                   get_token_name(pc->parent_type),
                                   pc->column, pc->orig_col, pc->orig_col_end, pc->orig_prev_sp,
                                   pc->brace_level, pc->level, pc->pp_level,
                                   pc->flags, pc->nl_count, pc->after_tab);
      fprintf(pfile, "%s", outputMessage);
      free(outputMessage);

      if (pc->type != CT_NEWLINE && (pc->len() != 0))
      {
         for (size_t cnt = 0; cnt < pc->column; cnt++)
         {
            fprintf(pfile, " ");
         }
         if (pc->type != CT_NL_CONT)
         {
            fprintf(pfile, "%s", pc->text());
         }
         else
         {
            fprintf(pfile, "\\");
         }
      }
   }
   fprintf(pfile, "\n# -=====-\n");
   fflush(pfile);
} // output_parsed


void output_text(FILE *pfile)
{
   cpd.fout        = pfile;
   cpd.did_newline = 1;
   cpd.column      = 1;

   if (cpd.bom)
   {
      write_bom();
   }

   chunk_t *pc;
   if (cpd.frag_cols > 0)
   {
      size_t indent = cpd.frag_cols - 1;

      // loop over the whole chunk list
      for (pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
      {
         pc->column        += indent;
         pc->column_indent += indent;
      }
      cpd.frag_cols = 0;
   }

   // loop over the whole chunk list
   for (pc = chunk_get_head(); pc != nullptr; pc = chunk_get_next(pc))
   {
      LOG_FMT(LOUTIND, "%s(%d): text() is %s, type is %s, orig_col is %zu, column is %zu\n",
              __func__, __LINE__, pc->text(), get_token_name(pc->type), pc->orig_col, pc->column);
      cpd.output_tab_as_space = (  cpd.settings[UO_cmt_convert_tab_to_spaces].b
                                && chunk_is_comment(pc));
      if (pc->type == CT_NEWLINE)
      {
         for (size_t cnt = 0; cnt < pc->nl_count; cnt++)
         {
            add_char('\n');
         }
         cpd.did_newline = 1;
         cpd.column      = 1;
         LOG_FMT(LOUTIND, " xx\n");
      }
      else if (pc->type == CT_NL_CONT)
      {
         // FIXME: this really shouldn't be done here!
         if ((pc->flags & PCF_WAS_ALIGNED) == 0)
         {
            if (cpd.settings[UO_sp_before_nl_cont].a & AV_REMOVE)
            {
               pc->column = cpd.column + (cpd.settings[UO_sp_before_nl_cont].a == AV_FORCE);
            }
            else
            {
               // Try to keep the same relative spacing
               chunk_t *prev = chunk_get_prev(pc);

               if (prev && prev->type == CT_PP_IGNORE)
               {
                  /*
                   * Want to completely leave alone PP_IGNORE'd blocks because
                   * they likely have special column aligned newline
                   * continuations (common in multiline macros)
                   */
                  pc->column = pc->orig_col;
               }
               else
               {
                  // Try to keep the same relative spacing
                  while (  prev != NULL
                        && prev->orig_col == 0
                        && prev->nl_count == 0)
                  {
                     prev = chunk_get_prev(prev);
                  }

                  if (prev != NULL && prev->nl_count == 0)
                  {
                     int orig_sp = (pc->orig_col - prev->orig_col_end);
                     if ((int)(cpd.column + orig_sp) < 0)
                     {
                        char *outputMessage;
                        outputMessage = make_message("FATAL: negative value.\n   pc->orig_col=%zu prev->orig_col_end=%zu\n",
                                                     pc->orig_col, prev->orig_col_end);
                        fprintf(stderr, "%s", outputMessage);
                        free(outputMessage);
                        log_flush(true);
                        exit(EX_SOFTWARE);
                     }
                     pc->column = cpd.column + orig_sp;
                     if (  (cpd.settings[UO_sp_before_nl_cont].a != AV_IGNORE)
                        && (pc->column < (cpd.column + 1)))
                     {
                        pc->column = cpd.column + 1;
                     }
                  }
               }
            }
            output_to_column(pc->column, false);
         }
         else
         {
            output_to_column(pc->column, (cpd.settings[UO_indent_with_tabs].u == 2));
         }
         add_char('\\');
         add_char('\n');
         cpd.did_newline = 1;
         cpd.column      = 1;
         LOG_FMT(LOUTIND, " \\xx\n");
      }
      else if (pc->type == CT_COMMENT_MULTI)
      {
         if (cpd.settings[UO_cmt_indent_multi].b)
         {
            output_comment_multi(pc);
         }
         else
         {
            output_comment_multi_simple(pc, (pc->flags & PCF_INSERTED) != 0);  // forcing value to bool
         }
      }
      else if (pc->type == CT_COMMENT_CPP)
      {
         bool tmp = cpd.output_trailspace;
         /*
          * keep trailing spaces if they are still present in a chunk;
          * note that tokenize() already strips spaces in comments,
          * so if they made it up to here, they are to stay
          */
         cpd.output_trailspace = true;
         pc                    = output_comment_cpp(pc);
         cpd.output_trailspace = tmp;
      }
      else if (pc->type == CT_COMMENT)
      {
         pc = output_comment_c(pc);
      }
      else if (pc->type == CT_JUNK || pc->type == CT_IGNORED)
      {
         // do not adjust the column for junk
         add_text(pc->str, true);
      }
      else if (pc->len() == 0)
      {
         // don't do anything for non-visible stuff
         LOG_FMT(LOUTIND, " <%zu> -", pc->column);
      }
      else
      {
         bool allow_tabs;
         cpd.output_trailspace = (pc->type == CT_STRING_MULTI);
         // indent to the 'level' first
         if (cpd.did_newline)
         {
            if (cpd.settings[UO_indent_with_tabs].u == 1)
            {
               size_t lvlcol;
               /*
                * FIXME: it would be better to properly set column_indent in
                * indent_text(), but this hack for '}' and ':' seems to work.
                */
               if (  pc->type == CT_BRACE_CLOSE
                  || chunk_is_str(pc, ":", 1)
                  || pc->type == CT_PREPROC)
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
            allow_tabs = (cpd.settings[UO_indent_with_tabs].u == 2)
                         || (  chunk_is_comment(pc)
                            && cpd.settings[UO_indent_with_tabs].u != 0);

            LOG_FMT(LOUTIND, "%s(%d): orig_line is %zu, column is %zu, column_indent is %zu, cpd.column is %zu\n",
                    __func__, __LINE__, pc->orig_line, pc->column, pc->column_indent, cpd.column);
         }
         else
         {
            /*
             * Reformatting multi-line comments can screw up the column.
             * Make sure we don't mess up the spacing on this line.
             * This has to be done here because comments are not formatted
             * until the output phase.
             */
            if (pc->column < cpd.column)
            {
               reindent_line(pc, cpd.column);
            }

            // not the first item on a line
            chunk_t *prev = chunk_get_prev(pc);
            allow_tabs = (  cpd.settings[UO_align_with_tabs].b
                         && (pc->flags & PCF_WAS_ALIGNED)
                         && ((prev->column + prev->len() + 1) != pc->column));
            if (cpd.settings[UO_align_keep_tabs].b)
            {
               allow_tabs |= pc->after_tab;
            }
            LOG_FMT(LOUTIND, " %s(%d): at column %zu(%s)\n",
                    __func__, __LINE__, pc->column, (allow_tabs ? "true" : "FALSE"));
         }

         output_to_column(pc->column, allow_tabs);
         add_text(pc->str);
         if (pc->type == CT_PP_DEFINE)  // Issue #876
         {
            if (cpd.settings[UO_force_tab_after_define].b)
            {
               add_char('\t');
            }
         }
         cpd.did_newline       = chunk_is_newline(pc);
         cpd.output_trailspace = false;
      }
   }
} // output_text


static size_t cmt_parse_lead(const unc_text &line, bool is_last)
{
   size_t len = 0;

   while (len < 32 && len < line.size())  // TODO what is the meaning of 32?
   {
      if (len > 0 && line[len] == '/')
      {
         // ignore combined comments
         size_t tmp = len + 1;
         while (tmp < line.size() && unc_isspace(line[tmp]))
         {
            tmp++;
         }
         if (tmp < line.size() && line[tmp] == '/')
         {
            return(1);
         }
         break;
      }
      else if (strchr("*|\\#+", line[len]) == nullptr)
      {
         break;  // none of the characters '*|\#+' found in line
      }
      len++;
   }

   if (len > 30)  // TODO: what is the meaning of 30?
   {
      return(1);
   }

   if (  len > 0
      && (len >= line.size() || unc_isspace(line[len])))
   {
      return(len);
   }

   if (len == 1 && line[0] == '*')
   {
      return(len);
   }

   if (is_last && len > 0)
   {
      return(len);
   }
   return(0);
} // cmt_parse_lead


static void calculate_comment_body_indent(cmt_reflow &cmt, const unc_text &str)
{
   cmt.xtra_indent = 0;

   if (!cpd.settings[UO_cmt_indent_multi].b)
   {
      return;
   }

   size_t idx      = 0;
   size_t len      = str.size();
   size_t last_len = 0;
   if (cpd.settings[UO_cmt_multi_check_last].b)
   {
      // find the last line length
      for (idx = len - 1; idx > 0; idx--)
      {
         if (str[idx] == '\n' || str[idx] == '\r')
         {
            idx++;
            while (  idx < len
                  && (str[idx] == ' ' || str[idx] == '\t'))
            {
               idx++;
            }
            last_len = len - idx;
            break;
         }
      }
   }

   // find the first line length
   size_t first_len = 0;
   for (idx = 0; idx < len; idx++)
   {
      if (str[idx] == '\n' || str[idx] == '\r')
      {
         first_len = idx;
         while (str[first_len - 1] == ' ' || str[first_len - 1] == '\t')
         {
            first_len--;
         }

         // handle DOS endings
         if (str[idx] == '\r' && str[idx + 1] == '\n')
         {
            idx++;
         }
         idx++;
         break;
      }
   }

   // Scan the second line
   size_t width = 0;
   for ( ; idx < len - 1; idx++)
   {
      if (str[idx] == ' ' || str[idx] == '\t')
      {
         if (width > 0)
         {
            break;
         }
         continue;
      }
      if (str[idx] == '\n' || str[idx] == '\r')
      {
         break;  // Done with second line
      }

      // Count the leading chars
      if (  str[idx] == '*'
         || str[idx] == '|'
         || str[idx] == '\\'
         || str[idx] == '#'
         || str[idx] == '+')
      {
         width++;
      }
      else
      {
         if (width != 1 || str[idx - 1] != '*')
         {
            width = 0;
         }
         break;
      }
   }

   // LOG_FMT(LSYS, "%s: first=%d last=%d width=%d\n", __func__, first_len, last_len, width);

   /*
    * If the first and last line are the same length and don't contain any
    * alphanumeric chars and (the first line len > cmt_multi_first_len_minimum
    * or the second leader is the same as the first line length), then the
    * indent is 0.
    */
   if (  first_len == last_len
      && (  first_len > cpd.settings[UO_cmt_multi_first_len_minimum].u
         || first_len == width))
   {
      return;
   }

   cmt.xtra_indent = (width == 2) ? 0 : 1;
} // calculate_comment_body_indent


// TODO: can we use search_next_chunk here?
static chunk_t *get_next_function(chunk_t *pc)
{
   while ((pc = chunk_get_next(pc)) != nullptr)
   {
      if (  pc->type == CT_FUNC_DEF
         || pc->type == CT_FUNC_PROTO
         || pc->type == CT_FUNC_CLASS_DEF
         || pc->type == CT_FUNC_CLASS_PROTO
         || pc->type == CT_OC_MSG_DECL)
      {
         return(pc);
      }
   }
   return(nullptr);
}


static chunk_t *get_next_class(chunk_t *pc)
{
   return(chunk_get_next(chunk_search_next_cat(pc, CT_CLASS)));
}


static chunk_t *get_prev_category(chunk_t *pc)
{
   return(chunk_search_prev_cat(pc, CT_OC_CATEGORY));
}


static chunk_t *get_next_scope(chunk_t *pc)
{
   return(chunk_search_next_cat(pc, CT_OC_SCOPE));
}


static chunk_t *get_prev_oc_class(chunk_t *pc)
{
   return(chunk_search_prev_cat(pc, CT_OC_CLASS));
}


static int next_up(const unc_text &text, size_t idx, unc_text &tag)
{
   size_t offs = 0;

   while (idx < text.size() && unc_isspace(text[idx]))
   {
      idx++;
      offs++;
   }

   if (text.startswith(tag, idx))
   {
      return(offs);
   }
   return(-1);
}


static void add_comment_text(const unc_text &text,
                             cmt_reflow &cmt, bool esc_close)
{
   bool   was_star  = false;
   bool   was_slash = false;
   bool   in_word   = false;
   size_t len       = text.size();
   size_t ch_cnt    = 0; // chars since newline

   // If the '//' is included write it first else we may wrap an empty line
   size_t idx = 0;

   if (text.startswith("//"))
   {
      add_text("//");
      idx += 2;
      while (unc_isspace(text[idx]))
      {
         add_char(text[idx++]);
      }
   }

   for ( ; idx < len; idx++)  // TODO: avoid modifying idx in loop
   {
      // Split the comment
      if (text[idx] == '\n')
      {
         in_word = false;
         add_char('\n');
         cmt_output_indent(cmt.brace_col, cmt.base_col, cmt.column);
         if (cmt.xtra_indent > 0)
         {
            add_char(' ');
         }

         // hack to get escaped newlines to align and not duplicate the leading '//'
         int tmp = next_up(text, idx + 1, cmt.cont_text);
         if (tmp < 0)
         {
            add_text(cmt.cont_text);
         }
         else
         {
            idx += tmp;
         }
         ch_cnt = 0;
      }
      else if (  cmt.reflow
              && text[idx] == ' '
              && cpd.settings[UO_cmt_width].u > 0
              && (  cpd.column > cpd.settings[UO_cmt_width].u
                 || (ch_cnt > 1 && next_word_exceeds_limit(text, idx))))
      {
         in_word = false;
         add_char('\n');
         cmt_output_indent(cmt.brace_col, cmt.base_col, cmt.column);
         if (cmt.xtra_indent > 0)
         {
            add_char(' ');
         }

         add_text(cmt.cont_text);
         output_to_column(cmt.column + cpd.settings[UO_cmt_sp_after_star_cont].n,
                          false);
         ch_cnt = 0;
      }
      else
      {
         // Escape a C closure in a CPP comment
         if (  esc_close
            && (  (was_star && text[idx] == '/')
               || (was_slash && text[idx] == '*')))
         {
            add_char(' ');
         }
         if (!in_word && !unc_isspace(text[idx]))
         {
            cmt.word_count++;
         }
         in_word = !unc_isspace(text[idx]);

         add_char(text[idx]);
         was_star  = (text[idx] == '*');
         was_slash = (text[idx] == '/');
         ch_cnt++;
      }
   }
} // add_comment_text


static void output_cmt_start(cmt_reflow &cmt, chunk_t *pc)
{
   cmt.pc          = pc;
   cmt.column      = pc->column;
   cmt.brace_col   = pc->column_indent;
   cmt.base_col    = pc->column_indent;
   cmt.word_count  = 0;
   cmt.xtra_indent = 0;
   cmt.cont_text.clear();
   cmt.reflow = false;

   if ((pc->flags & PCF_INSERTED))
   {
      do_kw_subst(pc);
   }

   if (cmt.brace_col == 0)
   {
      cmt.brace_col = 1 + (pc->brace_level * cpd.settings[UO_output_tab_size].u);
   }

   // LOG_FMT(LSYS, "%s: line %d, brace=%d base=%d col=%d orig=%d aligned=%x\n",
   //        __func__, pc->orig_line, cmt.brace_col, cmt.base_col, cmt.column, pc->orig_col,
   //        pc->flags & (PCF_WAS_ALIGNED | PCF_RIGHT_COMMENT));

   if (  pc->parent_type == CT_COMMENT_START
      || pc->parent_type == CT_COMMENT_WHOLE)
   {
      if (  !cpd.settings[UO_indent_col1_comment].b
         && pc->orig_col == 1
         && !(pc->flags & PCF_INSERTED))
      {
         cmt.column    = 1;
         cmt.base_col  = 1;
         cmt.brace_col = 1;
      }
   }

   // tab aligning code
   if (  cpd.settings[UO_indent_cmt_with_tabs].b
      && (  pc->parent_type == CT_COMMENT_END
         || pc->parent_type == CT_COMMENT_WHOLE))
   {
      cmt.column = align_tab_column(cmt.column - 1);
      // LOG_FMT(LSYS, "%s: line %d, orig:%d new:%d\n",
      //        __func__, pc->orig_line, pc->column, cmt.column);
      pc->column = cmt.column;
   }
   cmt.base_col = cmt.column;

   // LOG_FMT(LSYS, "%s: -- brace=%d base=%d col=%d\n",
   //        __func__, cmt.brace_col, cmt.base_col, cmt.column);

   // Bump out to the column
   cmt_output_indent(cmt.brace_col, cmt.base_col, cmt.column);
} // output_cmt_start


static bool can_combine_comment(chunk_t *pc, cmt_reflow &cmt)
{
   // We can't combine if there is something other than a newline next
   if (pc->parent_type == CT_COMMENT_START)
   {
      return(false);
   }

   // next is a newline for sure, make sure it is a single newline
   chunk_t *next = chunk_get_next(pc);
   if (next != nullptr && next->nl_count == 1)
   {
      // Make sure the comment is the same type at the same column
      next = chunk_get_next(next);
      if (  next != nullptr
         && next->type == pc->type
         && (  (next->column == 1 && pc->column == 1)
            || (next->column == cmt.base_col && pc->column == cmt.base_col)
            || (next->column > cmt.base_col && pc->parent_type == CT_COMMENT_END)))
      {
         return(true);
      }
   }
   return(false);
} // can_combine_comment


static chunk_t *output_comment_c(chunk_t *first)
{
   cmt_reflow cmt;

   output_cmt_start(cmt, first);
   cmt.reflow = (cpd.settings[UO_cmt_reflow_mode].u != 1);

   // See if we can combine this comment with the next comment
   if (!cpd.settings[UO_cmt_c_group].b || !can_combine_comment(first, cmt))
   {
      // Just add the single comment
      cmt.cont_text = cpd.settings[UO_cmt_star_cont].b ? " * " : "   ";
      LOG_CONTTEXT();
      add_comment_text(first->str, cmt, false);
      return(first);
   }

   cmt.cont_text = cpd.settings[UO_cmt_star_cont].b ? " *" : "  ";
   LOG_CONTTEXT();

   add_text("/*");
   if (cpd.settings[UO_cmt_c_nl_start].b)
   {
      add_comment_text("\n", cmt, false);
   }
   chunk_t  *pc = first;
   unc_text tmp;
   while (can_combine_comment(pc, cmt))
   {
      tmp.set(pc->str, 2, pc->len() - 4);
      if (cpd.last_char == '*' && tmp[0] == '/')
      {
         add_text(" ");
      }
      add_comment_text(tmp, cmt, false);
      add_comment_text("\n", cmt, false);
      pc = chunk_get_next(chunk_get_next(pc));
   }
   tmp.set(pc->str, 2, pc->len() - 4);
   if (cpd.last_char == '*' && tmp[0] == '/')
   {
      add_text(" ");
   }
   add_comment_text(tmp, cmt, false);
   if (cpd.settings[UO_cmt_c_nl_end].b)
   {
      cmt.cont_text = " ";
      LOG_CONTTEXT();
      add_comment_text("\n", cmt, false);
   }
   add_comment_text("*/", cmt, false);
   return(pc);
} // output_comment_c


static chunk_t *output_comment_cpp(chunk_t *first)
{
   cmt_reflow cmt;

   output_cmt_start(cmt, first);
   cmt.reflow = (cpd.settings[UO_cmt_reflow_mode].u != 1);

   unc_text leadin = "//";                     // default setting to keep previous behaviour
   if (cpd.settings[UO_sp_cmt_cpp_doxygen].b)  // special treatment for doxygen style comments (treat as unity)
   {
      const char *sComment = first->text();
      bool       grouping  = (sComment[2] == '@');
      size_t     brace     = 3;
      if (sComment[2] == '/' || sComment[2] == '!') // doxygen style found!
      {
         leadin += sComment[2];                     // at least one additional char (either "///" or "//!")
         if (sComment[3] == '<')                    // and a further one (either "///<" or "//!<")
         {
            leadin += '<';
         }
         else
         {
            grouping = (sComment[3] == '@');  // or a further one (grouping)
            brace    = 4;
         }
      }
      if (  grouping
         && (sComment[brace] == '{' || sComment[brace] == '}'))
      {
         leadin += '@';
         leadin += sComment[brace];
      }
   }

   // Special treatment for Qt translator or meta-data comments (treat as unity)
   if (cpd.settings[UO_sp_cmt_cpp_qttr].b)
   {
      const int c = first->str[2];
      if (  c == ':'
         || c == '='
         || c == '~')
      {
         leadin += c;
      }
   }


   // CPP comments can't be grouped unless they are converted to C comments
   if (!cpd.settings[UO_cmt_cpp_to_c].b)
   {
      cmt.cont_text = leadin;
      if (cpd.settings[UO_sp_cmt_cpp_start].a != AV_REMOVE)
      {
         cmt.cont_text += ' ';
      }
      LOG_CONTTEXT();

      if (cpd.settings[UO_sp_cmt_cpp_start].a == AV_IGNORE)
      {
         add_comment_text(first->str, cmt, false);
      }
      else
      {
         size_t   iLISz = leadin.size();
         unc_text tmp(first->str, 0, iLISz);
         add_comment_text(tmp, cmt, false);

         tmp.set(first->str, iLISz, first->len() - iLISz);

         if (cpd.settings[UO_sp_cmt_cpp_start].a & AV_REMOVE)
         {
            while ((tmp.size() > 0) && unc_isspace(tmp[0]))
            {
               tmp.pop_front();
            }
         }
         if (tmp.size() > 0)
         {
            if (cpd.settings[UO_sp_cmt_cpp_start].a & AV_ADD)
            {
               if (!unc_isspace(tmp[0]) && (tmp[0] != '/'))
               {
                  add_comment_text(" ", cmt, false);
               }
            }
            add_comment_text(tmp, cmt, false);
         }
      }

      return(first);
   }

   // We are going to convert the CPP comments to C comments
   cmt.cont_text = cpd.settings[UO_cmt_star_cont].b ? " * " : "   ";
   LOG_CONTTEXT();

   unc_text tmp;
   // See if we can combine this comment with the next comment
   if (!cpd.settings[UO_cmt_cpp_group].b || !can_combine_comment(first, cmt))
   {
      // nothing to group: just output a single line
      add_text("/*");
      // patch # 32, 2012-03-23
      if (  !unc_isspace(first->str[2])
         && (cpd.settings[UO_sp_cmt_cpp_start].a & AV_ADD))
      {
         add_char(' ');
      }
      tmp.set(first->str, 2, first->len() - 2);
      add_comment_text(tmp, cmt, true);
      add_text(" */");
      return(first);
   }

   add_text("/*");
   if (cpd.settings[UO_cmt_cpp_nl_start].b)
   {
      add_comment_text("\n", cmt, false);
   }
   else
   {
      add_text(" ");
   }
   chunk_t *pc = first;
   int     offs;

   while (can_combine_comment(pc, cmt))
   {
      offs = unc_isspace(pc->str[2]) ? 1 : 0;
      tmp.set(pc->str, 2 + offs, pc->len() - (2 + offs));
      if (cpd.last_char == '*' && tmp[0] == '/')
      {
         add_text(" ");
      }
      add_comment_text(tmp, cmt, true);
      add_comment_text("\n", cmt, false);
      pc = chunk_get_next(chunk_get_next(pc));
   }
   offs = unc_isspace(pc->str[2]) ? 1 : 0;
   tmp.set(pc->str, 2 + offs, pc->len() - (2 + offs));
   add_comment_text(tmp, cmt, true);
   if (cpd.settings[UO_cmt_cpp_nl_end].b)
   {
      cmt.cont_text = "";
      LOG_CONTTEXT();
      add_comment_text("\n", cmt, false);
   }
   add_comment_text(" */", cmt, false);
   return(pc);
} // output_comment_cpp


static void cmt_trim_whitespace(unc_text &line, bool in_preproc)
{
   // Remove trailing whitespace on the line
   while (  line.size() > 0
         && (line.back() == ' ' || line.back() == '\t'))
   {
      line.pop_back();
   }

   // Shift back to the comment text, ...
   if (  in_preproc             // if in a preproc ...
      && line.size() > 1        // with a line that holds ...
      && line.back() == '\\')   // a backslash-newline ...
   {
      bool do_space = false;

      // If there was any space before the backslash, change it to 1 space
      line.pop_back();
      while (  line.size() > 0
            && (line.back() == ' ' || line.back() == '\t'))
      {
         do_space = true;
         line.pop_back();
      }
      if (do_space)
      {
         line.append(' ');
      }
      line.append('\\');
   }
} // cmt_trim_whitespace


static void output_comment_multi(chunk_t *pc)
{
   cmt_reflow cmt;

   // LOG_FMT(LSYS, "%s: line %d\n", __func__, pc->orig_line);

   output_cmt_start(cmt, pc);
   cmt.reflow = (cpd.settings[UO_cmt_reflow_mode].u != 1);

   size_t cmt_col  = cmt.base_col;
   int    col_diff = pc->orig_col - cmt.base_col;

   calculate_comment_body_indent(cmt, pc->str);

   cmt.cont_text = !cpd.settings[UO_cmt_indent_multi].b ? "" :
                   (cpd.settings[UO_cmt_star_cont].b ? "* " : "  ");
   LOG_CONTTEXT();

   // LOG_FMT(LSYS, "Indenting1 line %d to col %d (orig=%d) col_diff=%d xtra=%d cont='%s'\n",
   //        pc->orig_line, cmt_col, pc->orig_col, col_diff, cmt.xtra_indent, cmt.cont_text.c_str());

   size_t   line_count = 0;
   size_t   ccol       = pc->column; // the col of subsequent comment lines
   size_t   cmt_idx    = 0;
   bool     nl_end     = false;
   unc_text line;
   line.clear();
   while (cmt_idx < pc->len())
   {
      int ch = pc->str[cmt_idx++];

      // handle the CRLF and CR endings. convert both to LF
      if (ch == '\r')
      {
         ch = '\n';
         if (cmt_idx < pc->len() && pc->str[cmt_idx] == '\n')
         {
            cmt_idx++;
         }
      }

      // Find the start column
      if (line.size() == 0)
      {
         nl_end = false;
         if (ch == ' ')
         {
            ccol++;
            continue;
         }
         else if (ch == '\t')
         {
            ccol = calc_next_tab_column(ccol, cpd.settings[UO_input_tab_size].u);
            continue;
         }
         else
         {
            // LOG_FMT(LSYS, "%d] Text starts in col %d\n", line_count, ccol);
         }
      }

      /*
       * Now see if we need/must fold the next line with the current to enable
       * full reflow
       */
      if (  cpd.settings[UO_cmt_reflow_mode].u == 2
         && ch == '\n'
         && cmt_idx < pc->len())
      {
         int    next_nonempty_line = -1;
         int    prev_nonempty_line = -1;
         size_t nwidx              = line.size();
         bool   star_is_bullet     = false;

         // strip trailing whitespace from the line collected so far
         while (nwidx > 0)
         {
            nwidx--;
            if (  prev_nonempty_line < 0
               && !unc_isspace(line[nwidx])
               && line[nwidx] != '*'    // block comment: skip '*' at end of line
               && ((pc->flags & PCF_IN_PREPROC)
                   ? (  line[nwidx] != '\\'
                     || (line[nwidx + 1] != 'r' && line[nwidx + 1] != '\n'))
                   : true))
            {
               prev_nonempty_line = nwidx; // last non-whitespace char in the previous line
            }
         }

         size_t remaining = pc->len() - cmt_idx;
         for (size_t nxt_len = 0;
              (  nxt_len <= remaining
              && pc->str[nxt_len] != 'r'  // TODO: should this be \r ?
              && pc->str[nxt_len] != '\n');
              nxt_len++)
         {
            if (  next_nonempty_line < 0
               && !unc_isspace(pc->str[nxt_len])
               && pc->str[nxt_len] != '*'
               && (  nxt_len == remaining
                  || ((pc->flags & PCF_IN_PREPROC)
                      ? (  pc->str[nxt_len] != '\\'
                        || (  pc->str[nxt_len + 1] != 'r'  // TODO: should this be \r ?
                           && pc->str[nxt_len + 1] != '\n'))
                      : true)))
            {
               next_nonempty_line = nxt_len;  // first non-whitespace char in the next line
            }
         }

         /*
          * see if we should fold up; usually that'd be a YES, but there are a few
          * situations where folding/reflowing by merging lines is frowned upon:
          *
          * - ASCII art in the comments (most often, these are drawings done in +-\/|.,*)
          *
          * - Doxygen/JavaDoc/etc. parameters: these often start with \ or @, at least
          *   something clearly non-alphanumeric (you see where we're going with this?)
          *
          * - bullet lists that are closely spaced: bullets are always non-alphanumeric
          *   characters, such as '-' or '+' (or, oh horror, '*' - that's bloody ambiguous
          *   to parse :-( ... with or without '*' comment start prefix, that's the
          *   question, then.)
          *
          * - semi-HTML formatted code, e.g. <pre>...</pre> comment sections (NDoc, etc.)
          *
          * - New lines which form a new paragraph without there having been added an
          *   extra empty line between the last sentence and the new one.
          *   A bit like this, really; so it is opportune to check if the last line ended
          *   in a terminal (that would be the set '.:;!?') and the new line starts with
          *   a capital.
          *   Though new lines starting with comment delimiters, such as '(', should be
          *   pulled up.
          *
          * So it bores down to this: the only folding (& reflowing) that's going to happen
          * is when the next line starts with an alphanumeric character AND the last
          * line didn't end with an non-alphanumeric character, except: ',' AND the next
          * line didn't start with a '*' all of a sudden while the previous one didn't
          * (the ambiguous '*'-for-bullet case!)
          */
         if (  prev_nonempty_line >= 0
            && next_nonempty_line >= 0
            && (  (  (  unc_isalnum(line[prev_nonempty_line])
                     || strchr(",)]", line[prev_nonempty_line]))
                  && (  unc_isalnum(pc->str[next_nonempty_line])
                     || strchr("([", pc->str[next_nonempty_line])))
               || (  '.' == line[prev_nonempty_line] // dot followed by non-capital is NOT a new sentence start
                  && unc_isupper(pc->str[next_nonempty_line])))
            && !star_is_bullet)
         {
            // rewind the line to the last non-alpha:
            line.resize(prev_nonempty_line + 1);
            // roll the current line forward to the first non-alpha:
            cmt_idx += next_nonempty_line;
            // override the NL and make it a single whitespace:
            ch = ' ';
         }
      }

      line.append(ch);

      if (  ch == '\n'              // If we hit an end of line sign
         || cmt_idx == pc->len())   // or hit an end-of-comment
      {
         line_count++;

         // strip trailing tabs and spaces before the newline
         if (ch == '\n')
         {
            nl_end = true;
            line.pop_back();
            cmt_trim_whitespace(line, pc->flags & PCF_IN_PREPROC);
         }

         // LOG_FMT(LSYS, "[%3d]%s\n", ccol, line);

         if (line_count == 1)
         {
            // this is the first line - add unchanged
            add_comment_text(line, cmt, false);
            if (nl_end)
            {
               add_char('\n');
            }
         }
         else
         {
            /*
             * This is not the first line, so we need to indent to the
             * correct column. Each line is indented 0 or more spaces.
             */
            // Ensure ccol is not negative
            if (static_cast<int>(ccol) >= col_diff)
            {
               ccol -= col_diff;
            }

            if (ccol < (cmt_col + 3))
            {
               ccol = cmt_col + 3;
            }

            if (line.size() == 0)
            {
               // Empty line - just a '\n'
               if (cpd.settings[UO_cmt_star_cont].b)
               {
                  cmt.column = cmt_col + cpd.settings[UO_cmt_sp_before_star_cont].u;
                  cmt_output_indent(cmt.brace_col, cmt.base_col, cmt.column);
                  if (cmt.xtra_indent > 0)
                  {
                     add_char(' ');
                  }
                  add_text(cmt.cont_text);
               }
               add_char('\n');
            }
            else
            {
               /*
                * If this doesn't start with a '*' or '|'.
                * '\name' is a common parameter documentation thing.
                */
               if (  cpd.settings[UO_cmt_indent_multi].b
                  && line[0] != '*'
                  && line[0] != '|'
                  && line[0] != '#'
                  && (line[0] != '\\' || unc_isalpha(line[1]))
                  && line[0] != '+')
               {
                  size_t start_col = cmt_col + cpd.settings[UO_cmt_sp_before_star_cont].u;

                  if (cpd.settings[UO_cmt_star_cont].b)
                  {
                     cmt.column = start_col;
                     cmt_output_indent(cmt.brace_col, cmt.base_col, cmt.column);
                     if (cmt.xtra_indent > 0)
                     {
                        add_char(' ');
                     }
                     add_text(cmt.cont_text);
                     output_to_column(ccol + cpd.settings[UO_cmt_sp_after_star_cont].n,
                                      false);
                  }
                  else
                  {
                     cmt.column = ccol;
                     cmt_output_indent(cmt.brace_col, cmt.base_col, cmt.column);
                  }
               }
               else
               {
                  cmt.column = cmt_col + cpd.settings[UO_cmt_sp_before_star_cont].u;
                  cmt_output_indent(cmt.brace_col, cmt.base_col, cmt.column);
                  if (cmt.xtra_indent > 0)
                  {
                     add_char(' ');
                  }

                  size_t idx;

                  // Checks for and updates the lead chars.
                  // @return 0=not present, >0=number of chars that are part of the lead
                  idx = cmt_parse_lead(line, (cmt_idx == pc->len()));
                  if (idx > 0)
                  {
                     // >0=number of chars that are part of the lead
                     cmt.cont_text.set(line, 0, idx);
                     LOG_CONTTEXT();
                     if (  (line.size() >= 2)
                        && (line[0] == '*')
                        && unc_isalnum(line[1]))
                     {
                        line.insert(1, ' ');
                     }
                  }
                  else
                  {
                     // bug #653
                     if (cpd.lang_flags & LANG_D)
                     {
                        // 0=no lead char present
                        add_text(cmt.cont_text);
                     }
                  }
               }

               add_comment_text(line, cmt, false);
               if (nl_end)
               {
                  add_text("\n");
               }
            }
         }
         line.clear();
         ccol = 1;
      }
   }
} // output_comment_multi


static bool kw_fcn_filename(chunk_t *cmt, unc_text &out_txt)
{
   UNUSED(cmt);
   out_txt.append(path_basename(cpd.filename));
   return(true);
}


static bool kw_fcn_class(chunk_t *cmt, unc_text &out_txt)
{
   chunk_t *tmp = nullptr;

   if ((cpd.lang_flags & LANG_CPP) && cpd.lang_flags & LANG_OC)
   {
      chunk_t *fcn = get_next_function(cmt);
      if (fcn->type == CT_OC_MSG_DECL)
      {
         tmp = get_prev_oc_class(cmt);
      }
      else
      {
         tmp = get_next_class(cmt);
      }
   }
   else if (cpd.lang_flags & LANG_OC)
   {
      tmp = get_prev_oc_class(cmt);
   }
   if (tmp == nullptr)
   {
      tmp = get_next_class(cmt);
   }

   if (tmp)
   {
      out_txt.append(tmp->str);
      if (cpd.lang_flags & cpd.lang_flags) // TODO: strange condition ?
      {
         while ((tmp = chunk_get_next(tmp)) != nullptr)
         {
            if (tmp->type != CT_DC_MEMBER)
            {
               break;
            }
            tmp = chunk_get_next(tmp);
            if (tmp)
            {
               out_txt.append("::");
               out_txt.append(tmp->str);
            }
         }
      }
      return(true);
   }
   return(false);
} // kw_fcn_class


static bool kw_fcn_message(chunk_t *cmt, unc_text &out_txt)
{
   chunk_t *fcn = get_next_function(cmt);

   if (!fcn)
   {
      return(false);
   }

   out_txt.append(fcn->str);

   chunk_t *tmp  = chunk_get_next_ncnl(fcn);
   chunk_t *word = nullptr;
   while (tmp)
   {
      if (tmp->type == CT_BRACE_OPEN || tmp->type == CT_SEMICOLON)
      {
         break;
      }
      if (tmp->type == CT_OC_COLON)
      {
         if (word != nullptr)
         {
            out_txt.append(word->str);
            word = nullptr;
         }
         out_txt.append(":");
      }
      if (tmp->type == CT_WORD)
      {
         word = tmp;
      }
      tmp = chunk_get_next_ncnl(tmp);
   }
   return(true);
} // kw_fcn_message


static bool kw_fcn_category(chunk_t *cmt, unc_text &out_txt)
{
   chunk_t *category = get_prev_category(cmt);

   if (category)
   {
      out_txt.append('(');
      out_txt.append(category->str);
      out_txt.append(')');
   }
   return(true);
} // kw_fcn_category


static bool kw_fcn_scope(chunk_t *cmt, unc_text &out_txt)
{
   chunk_t *scope = get_next_scope(cmt);

   if (scope)
   {
      out_txt.append(scope->str);
      return(true);
   }
   return(false);
} // kw_fcn_scope


static bool kw_fcn_function(chunk_t *cmt, unc_text &out_txt)
{
   chunk_t *fcn = get_next_function(cmt);

   if (fcn)
   {
      if (fcn->parent_type == CT_OPERATOR)
      {
         out_txt.append("operator ");
      }
      if (fcn->prev && fcn->prev->type == CT_DESTRUCTOR)
      {
         out_txt.append('~');
      }
      out_txt.append(fcn->str);
      return(true);
   }
   return(false);
}


static bool kw_fcn_javaparam(chunk_t *cmt, unc_text &out_txt)
{
   chunk_t *fcn = get_next_function(cmt);

   if (!fcn)
   {
      return(false);
   }

   chunk_t *fpo;
   chunk_t *fpc;
   bool    has_param = true;
   bool    need_nl   = false;

   if (fcn->type == CT_OC_MSG_DECL)
   {
      chunk_t *tmp = chunk_get_next_ncnl(fcn);
      has_param = false;
      while (tmp)
      {
         if (tmp->type == CT_BRACE_OPEN || tmp->type == CT_SEMICOLON)
         {
            break;
         }

         if (has_param)
         {
            if (need_nl)
            {
               out_txt.append("\n");
            }
            need_nl = true;
            out_txt.append("@param");
            out_txt.append(" ");
            out_txt.append(tmp->str);
            out_txt.append(" TODO");
         }

         has_param = false;
         if (tmp->type == CT_PAREN_CLOSE)
         {
            has_param = true;
         }
         tmp = chunk_get_next_ncnl(tmp);
      }
      fpo = fpc = nullptr;
   }
   else
   {
      fpo = chunk_get_next_type(fcn, CT_FPAREN_OPEN, fcn->level);
      if (fpo == nullptr)
      {
         return(true);
      }
      fpc = chunk_get_next_type(fpo, CT_FPAREN_CLOSE, fcn->level);
      if (fpc == nullptr)
      {
         return(true);
      }
   }

   chunk_t *tmp;
   // Check for 'foo()' and 'foo(void)'
   if (chunk_get_next_ncnl(fpo) == fpc)
   {
      has_param = false;
   }
   else
   {
      tmp = chunk_get_next_ncnl(fpo);
      if ((tmp == chunk_get_prev_ncnl(fpc)) && chunk_is_str(tmp, "void", 4))
      {
         has_param = false;
      }
   }

   if (has_param)
   {
      chunk_t *prev = nullptr;
      tmp = fpo;
      while ((tmp = chunk_get_next(tmp)) != nullptr)
      {
         if (tmp->type == CT_COMMA || tmp == fpc)
         {
            if (need_nl)
            {
               out_txt.append("\n");
            }
            need_nl = true;
            out_txt.append("@param");
            if (prev != nullptr)
            {
               out_txt.append(" ");
               out_txt.append(prev->str);
               out_txt.append(" TODO");
            }
            prev = nullptr;
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

   // Do the return stuff
   tmp = chunk_get_prev_ncnl(fcn);
   // For Objective-C we need to go to the previous chunk
   if (tmp->parent_type == CT_OC_MSG_DECL && tmp->type == CT_PAREN_CLOSE)
   {
      tmp = chunk_get_prev_ncnl(tmp);
   }
   if (tmp != nullptr && !chunk_is_str(tmp, "void", 4))
   {
      if (need_nl)
      {
         out_txt.append("\n");
      }
      out_txt.append("@return TODO");
   }

   return(true);
} // kw_fcn_javaparam


static bool kw_fcn_fclass(chunk_t *cmt, unc_text &out_txt)
{
   chunk_t *fcn = get_next_function(cmt);

   if (!fcn)
   {
      return(false);
   }
   if (fcn->flags & PCF_IN_CLASS)
   {
      // if inside a class, we need to find to the class name
      chunk_t *tmp = chunk_get_prev_type(fcn, CT_BRACE_OPEN, fcn->level - 1);
      tmp = chunk_get_prev_type(tmp, CT_CLASS, tmp->level);
      tmp = chunk_get_next_ncnl(tmp);
      while (chunk_is_token(chunk_get_next_ncnl(tmp), CT_DC_MEMBER))
      {
         tmp = chunk_get_next_ncnl(tmp);
         tmp = chunk_get_next_ncnl(tmp);
      }

      if (tmp)
      {
         out_txt.append(tmp->str);
         return(true);
      }
   }
   else
   {
      // if outside a class, we expect "CLASS::METHOD(...)"
      chunk_t *tmp = chunk_get_prev_ncnl(fcn);
      if (tmp != nullptr && tmp->type == CT_OPERATOR)
      {
         tmp = chunk_get_prev_ncnl(tmp);
      }
      if (  tmp != nullptr
         && (tmp->type == CT_DC_MEMBER || tmp->type == CT_MEMBER))
      {
         tmp = chunk_get_prev_ncnl(tmp);
         out_txt.append(tmp->str);
         return(true);
      }
   }
   return(false);
} // kw_fcn_fclass


struct kw_subst_t
{
   const char *tag;
   bool       (*func)(chunk_t *cmt, unc_text &out_txt);
};


static const kw_subst_t kw_subst_table[] =
{
   { "$(filename)",  kw_fcn_filename  },
   { "$(class)",     kw_fcn_class     },
   { "$(message)",   kw_fcn_message   },
   { "$(category)",  kw_fcn_category  },
   { "$(scope)",     kw_fcn_scope     },
   { "$(function)",  kw_fcn_function  },
   { "$(javaparam)", kw_fcn_javaparam },
   { "$(fclass)",    kw_fcn_fclass    },
};


static void do_kw_subst(chunk_t *pc)
{
   for (const auto &kw : kw_subst_table)
   {
      int idx = pc->str.find(kw.tag);
      if (idx < 0)
      {
         continue;
      }

      unc_text tmp_txt;
      tmp_txt.clear();
      if (kw.func(pc, tmp_txt))
      {
         // if the replacement contains '\n' we need to fix the lead
         if (tmp_txt.find("\n") >= 0)
         {
            size_t nl_idx = pc->str.rfind("\n", idx);
            if (nl_idx > 0)
            {
               // idx and nl_idx are both positive
               unc_text nl_txt;
               nl_txt.append("\n");
               nl_idx++;
               while (  (nl_idx < static_cast<size_t>(idx))
                     && !unc_isalnum(pc->str[nl_idx]))
               {
                  nl_txt.append(pc->str[nl_idx++]);
               }
               tmp_txt.replace("\n", nl_txt);
            }
         }
         pc->str.replace(kw.tag, tmp_txt);
      }
   }
} // do_kw_subst


static void output_comment_multi_simple(chunk_t *pc, bool kw_subst)
{
   UNUSED(kw_subst);
   cmt_reflow cmt;
   output_cmt_start(cmt, pc);

   int col_diff = 0;
   if (chunk_is_newline(chunk_get_prev(pc)))
   {
      // The comment should be indented correctly
      col_diff = pc->orig_col - pc->column;
   }
   else
   {
      // The comment starts after something else
      col_diff = 0;
   }

   unc_text line;
   line.clear();
   size_t   line_count = 0;
   size_t   cmt_idx    = 0;
   size_t   ccol       = pc->column;
   bool     nl_end     = false;
   while (cmt_idx < pc->len())
   {
      int ch = pc->str[cmt_idx++];

      // handle the CRLF and CR endings. convert both to LF
      if (ch == '\r')
      {
         ch = '\n';
         if ((cmt_idx < pc->len()) && (pc->str[cmt_idx] == '\n'))
         {
            cmt_idx++;
         }
      }

      // Find the start column
      if (line.size() == 0)
      {
         nl_end = false;
         if (ch == ' ')
         {
            ccol++;
            continue;
         }
         else if (ch == '\t')
         {
            ccol = calc_next_tab_column(ccol, cpd.settings[UO_input_tab_size].u);
            continue;
         }
         else
         {
            // LOG_FMT(LSYS, "%d] Text starts in col %d, col_diff=%d, real=%d\n",
            //        line_count, ccol, col_diff, ccol - col_diff);
         }
      }

      line.append(ch);

      // If we just hit an end of line OR we just hit end-of-comment...
      if ((ch == '\n') || (cmt_idx == pc->len()))
      {
         line_count++;

         // strip trailing tabs and spaces before the newline
         if (ch == '\n')
         {
            line.pop_back();
            nl_end = true;

            // Say we aren't in a preproc to prevent changing any bs-nl
            cmt_trim_whitespace(line, false);
         }

         if (line_count > 1)
         {
            ccol -= col_diff;
         }

         if (line.size() > 0)
         {
            cmt.column = ccol;
            cmt_output_indent(cmt.brace_col, cmt.base_col, cmt.column);
            add_text(line);
         }
         if (nl_end)
         {
            add_char('\n');
         }
         line.clear();
         ccol = 1;
      }
   }
} // output_comment_multi_simple


static void generate_if_conditional_as_text(unc_text &dst, chunk_t *ifdef)
{
   int column = -1;

   dst.clear();
   for (chunk_t *pc = ifdef; pc != nullptr; pc = chunk_get_next(pc))
   {
      if (column == -1)
      {
         column = pc->column;
      }
      if (  pc->type == CT_NEWLINE
         || pc->type == CT_COMMENT_MULTI
         || pc->type == CT_COMMENT_CPP)
      {
         break;
      }
      else if (pc->type == CT_NL_CONT)
      {
         dst   += ' ';
         column = -1;
      }
      else if (pc->type == CT_COMMENT || pc->type == CT_COMMENT_EMBED)
      {
      }
      else // if (pc->type == CT_JUNK) || else
      {
         for (int spacing = pc->column - column; spacing > 0; spacing--)
         {
            dst += ' ';
            column++;
         }
         dst.append(pc->str);
         column += pc->len();
      }
   }
} // generate_if_conditional_as_text


void add_long_preprocessor_conditional_block_comment(void)
{
   chunk_t *pp_start = nullptr;
   chunk_t *pp_end   = nullptr;

   for (chunk_t *pc = chunk_get_head(); pc; pc = chunk_get_next_ncnl(pc))
   {
      // just track the preproc level:
      if (pc->type == CT_PREPROC)
      {
         pp_end = pp_start = pc;
      }

      if (pc->type != CT_PP_IF || !pp_start)
      {
         continue;
      }
#if 0
      if (pc->flags & PCF_IN_PREPROC)
      {
         continue;
      }
#endif

      chunk_t *br_close;
      chunk_t *br_open = pc;
      size_t  nl_count = 0;

      chunk_t *tmp = pc;
      while ((tmp = chunk_get_next(tmp)) != nullptr)
      {
         // just track the preproc level:
         if (tmp->type == CT_PREPROC)
         {
            pp_end = tmp;
         }

         if (chunk_is_newline(tmp))
         {
            nl_count += tmp->nl_count;
         }
         else if (  pp_end->pp_level == pp_start->pp_level
                 && (  tmp->type == CT_PP_ENDIF
                    || ((br_open->type == CT_PP_IF) ? (tmp->type == CT_PP_ELSE) : 0)))
         {
            br_close = tmp;

            LOG_FMT(LPPIF, "found #if / %s section on lines %zu and %zu, nl_count=%zu\n",
                    (tmp->type == CT_PP_ENDIF ? "#endif" : "#else"),
                    br_open->orig_line, br_close->orig_line, nl_count);

            // Found the matching #else or #endif - make sure a newline is next
            tmp = chunk_get_next(tmp);

            LOG_FMT(LPPIF, "next item type %d (is %s)\n",
                    (tmp ? tmp->type : -1), (tmp ? chunk_is_newline(tmp) ? "newline"
                                             : chunk_is_comment(tmp) ? "comment" : "other" : "---"));
            if (tmp == nullptr || tmp->type == CT_NEWLINE)  // chunk_is_newline(tmp))
            {
               size_t nl_min;

               if (br_close->type == CT_PP_ENDIF)
               {
                  nl_min = cpd.settings[UO_mod_add_long_ifdef_endif_comment].u;
               }
               else
               {
                  nl_min = cpd.settings[UO_mod_add_long_ifdef_else_comment].u;
               }

               const char *txt = !tmp ? "EOF" : ((tmp->type == CT_PP_ENDIF) ? "#endif" : "#else");
               LOG_FMT(LPPIF, "#if / %s section candidate for augmenting when over NL threshold %zu != 0 (nl_count=%zu)\n",
                       txt, nl_min, nl_count);

               if (nl_min > 0 && nl_count > nl_min)  // nl_count is 1 too large at all times as #if line was counted too
               {
                  // determine the added comment style
                  c_token_t style = (cpd.lang_flags & (LANG_CPP | LANG_CS)) ?
                                    CT_COMMENT_CPP : CT_COMMENT;

                  unc_text str;
                  generate_if_conditional_as_text(str, br_open);

                  LOG_FMT(LPPIF, "#if / %s section over threshold %zu (nl_count=%zu) --> insert comment after the %s: %s\n",
                          txt, nl_min, nl_count, txt, str.c_str());

                  // Add a comment after the close brace
                  insert_comment_after(br_close, style, str);
               }
            }

            // checks both the #else and #endif for a given level, only then look further in the main loop
            if (br_close->type == CT_PP_ENDIF)
            {
               break;
            }
         }
      }
   }
} // add_long_preprocessor_conditional_block_comment
