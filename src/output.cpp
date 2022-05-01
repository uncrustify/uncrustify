/**
 * @file output.cpp
 * Does all the output & comment formatting.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel October 2015, 2021
 * @license GPL v2+
 */

#include "output.h"

#include "align_tab_column.h"
#include "braces.h"
#include "indent.h"
#include "prototypes.h"
#include "tokenize.h"
#include "unc_ctype.h"
#include "unicode.h"

#include <ctime>
#include <map>
#include <regex>
#include <set>


constexpr static auto LCURRENT = LOUTPUT;

using namespace uncrustify;


struct cmt_reflow
{
   Chunk    *pc         = nullptr;
   size_t   column      = 0;   //! Column of the comment start
   size_t   brace_col   = 0;   //! Brace column (for indenting with tabs)
   size_t   base_col    = 0;   //! Base column (for indenting with tabs)
   size_t   word_count  = 0;   //! number of words on this line
   size_t   xtra_indent = 0;   //! extra indent of non-first lines (0 or 1)
   unc_text cont_text;         //! fixed text to output at the start of a line (0 to 3 chars)
   bool     reflow = false;    //! reflow the current line
};


/**
 * A multiline comment
 * The only trick here is that we have to trim out whitespace characters
 * to get the comment to line up.
 */
static void output_comment_multi(Chunk *pc);


static bool kw_fcn_filename(Chunk *cmt, unc_text &out_txt);


static bool kw_fcn_class(Chunk *cmt, unc_text &out_txt);


static bool kw_fcn_message(Chunk *cmt, unc_text &out_txt);


static bool kw_fcn_category(Chunk *cmt, unc_text &out_txt);


static bool kw_fcn_scope(Chunk *cmt, unc_text &out_txt);


static bool kw_fcn_function(Chunk *cmt, unc_text &out_txt);


/**
 * Adds the javadoc-style @param and @return stuff, based on the params and
 * return value for pc.
 * If the arg list is '()' or '(void)', then no @params are added.
 * Likewise, if the return value is 'void', then no @return is added.
 */
static bool kw_fcn_javaparam(Chunk *cmt, unc_text &out_txt);


static bool kw_fcn_fclass(Chunk *cmt, unc_text &out_txt);


static bool kw_fcn_year(Chunk *cmt, unc_text &out_txt);


/**
 * Output a multiline comment without any reformatting other than shifting
 * it left or right to get the column right.
 *
 * Trims trailing whitespaces.
 */
static void output_comment_multi_simple(Chunk *pc);


/**
 * This renders the #if condition to a string buffer.
 *
 * @param[out] dst    unc_text buffer to be filled
 * @param[in]  ifdef  if conditional as chunk list
 */
static void generate_if_conditional_as_text(unc_text &dst, Chunk *ifdef);


/**
 * Do keyword substitution on a comment.
 * NOTE: it is assumed that a comment will contain at most one of each type
 * of keyword.
 */
static void do_kw_subst(Chunk *pc);


//! All output text is sent here, one char at a time.
static void add_char(UINT32 ch, bool is_literal = false);


static void add_text(const char *ascii_text);


static void add_text(const unc_text &text, bool is_ignored, bool is_literal);


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


static int next_up(const unc_text &text, size_t idx, const unc_text &tag);


/**
 * Outputs the C comment at pc.
 * C comment combining is done here
 *
 * @return the last chunk output'd
 */
static Chunk *output_comment_c(Chunk *pc);


/**
 * Outputs the CPP comment at pc.
 * CPP comment combining is done here
 *
 * @return the last chunk output'd
 */
static Chunk *output_comment_cpp(Chunk *pc);


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
static void add_comment_text(const unc_text &text, cmt_reflow &cmt, bool esc_close, size_t continuation_indent = 0);


static void output_cmt_start(cmt_reflow &cmt, Chunk *pc);


/**
 * Checks to see if the current comment can be combined with the next comment.
 * The two can be combined if:
 *  1. They are the same type
 *  2. There is exactly one newline between then
 *  3. They are indented to the same level
 */
static bool can_combine_comment(Chunk *pc, cmt_reflow &cmt);


#define LOG_CONTTEXT() \
   LOG_FMT(LCONTTEXT, "%s(%d): set cont_text to '%s'\n", __func__, __LINE__, cmt.cont_text.c_str())


static void add_spaces()
{
   while (cpd.spaces > 0)
   {
      write_char(' ');
      cpd.spaces--;
   }
}


static void add_char(UINT32 ch, bool is_literal)
{
   // If we did a '\r' and it isn't followed by a '\n', then output a newline
   if (  (cpd.last_char == '\r')
      && (ch != '\n'))
   {
      write_string(cpd.newline);
      cpd.column      = 1;
      cpd.did_newline = 1;
      cpd.spaces      = 0;
   }

   // convert a newline into the LF/CRLF/CR sequence
   if (ch == '\n')
   {
      add_spaces();
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
   else if (  (ch == '\t')
           && cpd.output_tab_as_space)
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
      if (  !is_literal
         && ch == '\t'
         && cpd.last_char == ' ')
      {
         log_rule_B("indent_with_tabs");

         if (options::indent_with_tabs() == 0)
         {
            size_t endcol = next_tab_column(cpd.column);

            while (cpd.column < endcol)
            {
               add_char(' ');
            }
            return;
         }
      }

      if (  (ch == ' ')
         && !cpd.output_trailspace)
      {
         cpd.spaces++;
         cpd.column++;
      }
      else
      {
         add_spaces();
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


static void add_text(const unc_text &text, bool is_ignored = false, bool is_literal = false)
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
         add_char(ch, is_literal);
      }
   }
}


static bool next_word_exceeds_limit(const unc_text &text, size_t idx)
{
   LOG_FMT(LCONTTEXT, "%s(%d): idx is %zu\n",
           __func__, __LINE__, idx);
   size_t length = 0;

   // Count any whitespace
   while (  (idx < text.size())
         && unc_isspace(text[idx]))
   {
      idx++;
      length++;
   }

   // Count non-whitespace
   while (  (idx < text.size())
         && !unc_isspace(text[idx]))
   {
      idx++;
      length++;
   }
   return((cpd.column + length - 1) > options::cmt_width());
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
   log_rule_B("indent_cmt_with_tabs");
   log_rule_B("indent_with_tabs");
   size_t iwt = options::indent_cmt_with_tabs() ? 2 :
                (options::indent_with_tabs() ? 1 : 0);

   size_t tab_col = (iwt == 0) ? 0 : ((iwt == 1) ? brace_col : base_col);

   // LOG_FMT(LSYS, "%s(brace=%zd base=%zd col=%zd iwt=%zd) tab=%zd cur=%zd\n",
   //        __func__, brace_col, base_col, column, iwt, tab_col, cpd.column);

   cpd.did_newline = 0;

   if (  iwt == 2
      || (  cpd.column == 1
         && iwt == 1))
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


void output_parsed(FILE *pfile, bool withOptions)
{
   const char *eol_marker = get_eol_marker();

   if (withOptions)
   {
      save_option_file(pfile, false, true);
   }
   fprintf(pfile, "# -=====-%s", eol_marker);
   fprintf(pfile, "# number of loops               = %d\n", cpd.changes);
   fprintf(pfile, "# -=====-%s", eol_marker);
   fprintf(pfile, "# language                      = %s\n", language_name_from_flags(cpd.lang_flags));
   fprintf(pfile, "# -=====-%s", eol_marker);
   // MAXLENGTHOFTHENAME must be consider at the format line at the file
   // output.cpp, line 427: fprintf(pfile, "# Line              Tag                Parent...
   // and              430: ... fprintf(pfile, "%s# %3zu>%19.19s[%19.19s] ...
   // here                                                xx xx   xx xx
#ifdef WIN32
   fprintf(pfile, "# Line                Tag         Parent_type  Type of the parent         Columns Br/Lvl/pp     Nl  Text");
#else // not WIN32
   fprintf(pfile, "# Line                Tag         Parent_type  Type of the parent         Columns Br/Lvl/pp         Flag   Nl  Text");
#endif // ifdef WIN32

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
#ifdef WIN32
      fprintf(pfile, "%s# %3d>%19.19s|%19.19s|%19.19s[%3d/%3d/%3d/%3d][%d/%d/%d][%d-%d]",
              eol_marker, (int)pc->orig_line, get_token_name(pc->type),
              get_token_name(get_chunk_parent_type(pc)), get_token_name(get_type_of_the_parent(pc)),
              (int)pc->column, (int)pc->orig_col, (int)pc->orig_col_end, (int)pc->orig_prev_sp,
              (int)pc->brace_level, (int)pc->level, (int)pc->pp_level, (int)pc->nl_count, pc->after_tab);
#else // not WIN32
      fprintf(pfile, "%s# %3zu>%19.19s|%19.19s|%19.19s[%3zu/%3zu/%3zu/%3d][%zu/%zu/%zu]",
              eol_marker, pc->orig_line, get_token_name(pc->type),
              get_token_name(get_chunk_parent_type(pc)), get_token_name(get_type_of_the_parent(pc)),
              pc->column, pc->orig_col, pc->orig_col_end, pc->orig_prev_sp,
              pc->brace_level, pc->level, pc->pp_level);
      // Print pc flags in groups of 4 hex characters
      char flag_string[20];
      sprintf(flag_string, "%12llx", static_cast<pcf_flags_t::int_t>(pc->flags));
      fprintf(pfile, "[%.4s %.4s %.4s]", flag_string, flag_string + 4, flag_string + 8);
      fprintf(pfile, "[%zu-%d]",
              pc->nl_count, pc->after_tab);
#endif // ifdef WIN32

      if (  pc->type != CT_NEWLINE
         && (pc->Len() != 0))
      {
         for (size_t cnt = 0; cnt < pc->column; cnt++)
         {
            fprintf(pfile, " ");
         }

         if (pc->type != CT_NL_CONT)
         {
            fprintf(pfile, "%s", pc->Text());
         }
         else
         {
            fprintf(pfile, "\\");
         }
      }
   }

   fprintf(pfile, "%s# -=====-%s", eol_marker, eol_marker);
   fflush(pfile);
} // output_parsed


void output_parsed_csv(FILE *pfile)
{
   const char *eol_marker = get_eol_marker();

   fprintf(pfile, "number of loops,%d,\n", cpd.changes);
   fprintf(pfile, "language,%s,\n", language_name_from_flags(cpd.lang_flags));
   fprintf(pfile, "Line,Tag,Parent_type,Type of the parent,Column,Orig Col Strt,"
           "Orig Col End,Orig Sp Before,Br,Lvl,pp,Flags,Nl Before,Nl After,Text,");

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      fprintf(pfile, "%s%zu,%s,%s,%s,%zu,%zu,%zu,%d,%zu,%zu,%zu,",
              eol_marker, pc->orig_line, get_token_name(pc->type),
              get_token_name(get_chunk_parent_type(pc)), get_token_name(get_type_of_the_parent(pc)),
              pc->column, pc->orig_col, pc->orig_col_end, pc->orig_prev_sp,
              pc->brace_level, pc->level, pc->pp_level);

      auto pcf_flag_str = pcf_flags_str(pcf_flag_e(pc->flags));
#ifdef WIN32
      auto pcf_flag_str_start = pcf_flag_str.find("[") + 1;
#else // not WIN32
      auto pcf_flag_str_start = pcf_flag_str.find(":") + 1;
#endif // ifdef WIN32
      auto pcf_flag_str_end = pcf_flag_str.find("]");
      auto pcf_names        = pcf_flag_str.substr(pcf_flag_str_start,
                                                  pcf_flag_str_end - pcf_flag_str_start);
      fprintf(pfile, "\"%s\",", pcf_names.c_str());
      fprintf(pfile, "%zu,%d,",
              pc->nl_count, pc->after_tab);

      if (  pc->type != CT_NEWLINE
         && (pc->Len() != 0))
      {
         fprintf(pfile, "\"");

         for (size_t cnt = 0; cnt < pc->column; cnt++)
         {
            fprintf(pfile, " ");
         }

         if (pc->type != CT_NL_CONT)
         {
            for (auto *ch = pc->Text(); *ch != '\0'; ++ch)
            {
               fprintf(pfile, "%c", *ch);

               if (*ch == '"')
               {
                  // need to escape the double-quote for csv-format
                  fprintf(pfile, "\"");
               }
            }
         }
         else
         {
            fprintf(pfile, "\\");
         }
         fprintf(pfile, "\"");
      }
   }

   fflush(pfile);
} // output_parsed_csv


void output_text(FILE *pfile)
{
   bool tracking = cpd.html_file != nullptr;                 // special for debugging

   cpd.fout        = pfile;
   cpd.did_newline = 1;
   cpd.column      = 1;

   if (cpd.bom)
   {
      write_bom();
   }
   Chunk *pc;

   if (cpd.frag_cols > 0)
   {
      size_t indent = cpd.frag_cols - 1;

      // loop over the whole chunk list
      for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
      {
         pc->column        += indent;
         pc->column_indent += indent;
      }

      cpd.frag_cols = 0;
   }

   if (tracking)
   {
      add_text("<html>\n");
      add_text("<head>\n");
      add_text("   <meta http-equiv=\"content-type\" content=\"text/html; charset=utf-8\"/>\n");
      add_text("   <title>Uncrustify: where do the Spaces options work</title>\n");
      add_text("</head>\n");
      add_text("<body lang=\"en-US\">\n");
      add_text("<p>\n");
      add_text("</p>\n");
      add_text("<pre>\n");
   }
   bool write_in_tracking = false;

   // loop over the whole chunk list
   for (pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      char copy[1000];
      LOG_FMT(LCONTTEXT, "%s(%d): Text() is '%s', type is %s, orig_line is %zu, column is %zu, nl is %zu\n",
              __func__, __LINE__, pc->ElidedText(copy), get_token_name(pc->type), pc->orig_line, pc->column, pc->nl_count);
      log_rule_B("cmt_convert_tab_to_spaces");
      cpd.output_tab_as_space = (  options::cmt_convert_tab_to_spaces()
                                && pc->IsComment());

      if (chunk_is_token(pc, CT_NEWLINE))
      {
         for (size_t cnt = 0; cnt < pc->nl_count; cnt++)
         {
            if (  cnt > 0
               && pc->nl_column > 1)
            {
               log_rule_B("indent_with_tabs");
               output_to_column(pc->nl_column, (options::indent_with_tabs() == 2));
            }
            add_char('\n');
         }

         cpd.did_newline = 1;
         cpd.column      = 1;
         LOG_FMT(LOUTIND, " xx\n");
      }
      else if (chunk_is_token(pc, CT_NL_CONT))
      {
         // FIXME: this really shouldn't be done here!
         if (!pc->flags.test(PCF_WAS_ALIGNED))
         {
            // Add or remove space before a backslash-newline at the end of a line.
            log_rule_B("sp_before_nl_cont");

            if (options::sp_before_nl_cont() & IARF_REMOVE)
            {
               log_rule_B("sp_before_nl_cont");
               pc->column = cpd.column + (options::sp_before_nl_cont() == IARF_FORCE);
            }
            else
            {
               // Try to keep the same relative spacing
               Chunk *prev = pc->GetPrev();

               if (chunk_is_token(prev, CT_PP_IGNORE))
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
                  while (  prev != nullptr
                        && prev->IsNotNullChunk()
                        && prev->orig_col == 0
                        && prev->nl_count == 0)
                  {
                     prev = prev->GetPrev();
                  }

                  if (  prev != nullptr
                     && prev->IsNotNullChunk()
                     && prev->nl_count == 0)
                  {
                     int orig_sp = (pc->orig_col - prev->orig_col_end);

                     if ((int)(cpd.column + orig_sp) < 0)
                     {
#ifdef WIN32
                        fprintf(stderr, "FATAL: negative value.\n   pc->orig_col is %d, prev->orig_col_end is %d\n",
                                (int)pc->orig_col, (int)prev->orig_col_end);
#else // not WIN32
                        fprintf(stderr, "FATAL: negative value.\n   pc->orig_col is %zu, prev->orig_col_end is %zu\n",
                                pc->orig_col, prev->orig_col_end);
#endif // ifdef WIN32
                        log_flush(true);
                        exit(EX_SOFTWARE);
                     }
                     pc->column = cpd.column + orig_sp;

                     // Add or remove space before a backslash-newline at the end of a line.
                     log_rule_B("sp_before_nl_cont");

                     if (  (options::sp_before_nl_cont() != IARF_IGNORE)
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
            log_rule_B("indent_with_tabs");
            output_to_column(pc->column, (options::indent_with_tabs() == 2));
         }
         add_char('\\');
         add_char('\n');
         cpd.did_newline = 1;
         cpd.column      = 1;
         LOG_FMT(LOUTIND, " \\xx\n");
      }
      else if (chunk_is_token(pc, CT_COMMENT_MULTI))
      {
         log_rule_B("cmt_indent_multi");

         if (options::cmt_indent_multi())
         {
            output_comment_multi(pc);
         }
         else
         {
            output_comment_multi_simple(pc);
         }
      }
      else if (chunk_is_token(pc, CT_COMMENT_CPP))
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
      else if (chunk_is_token(pc, CT_COMMENT))
      {
         pc = output_comment_c(pc);
      }
      else if (  chunk_is_token(pc, CT_JUNK)
              || chunk_is_token(pc, CT_IGNORED))
      {
         LOG_FMT(LOUTIND, "%s(%d): orig_line is %zu, orig_col is %zu,\npc->Text() >%s<, pc->str.size() is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->Text(), pc->str.size());
         // do not adjust the column for junk
         add_text(pc->str, true);
      }
      else if (pc->Len() == 0)
      {
         // don't do anything for non-visible stuff
         LOG_FMT(LOUTIND, "%s(%d): orig_line is %zu, column is %zu, non-visible stuff: type is %s\n",
                 __func__, __LINE__, pc->orig_line, pc->column, get_token_name(pc->type));
      }
      else
      {
         bool allow_tabs;
         cpd.output_trailspace = (chunk_is_token(pc, CT_STRING_MULTI));

         // indent to the 'level' first
         if (cpd.did_newline)
         {
            log_rule_B("indent_with_tabs");

            if (options::indent_with_tabs() == 1)
            {
               size_t lvlcol;

               /*
                * FIXME: it would be better to properly set column_indent in
                * indent_text(), but this hack for '}' and '#' seems to work.
                */
               if (  chunk_is_token(pc, CT_BRACE_CLOSE)
                  || chunk_is_token(pc, CT_CASE_COLON)
                  || chunk_is_token(pc, CT_PREPROC))
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
            log_rule_B("indent_with_tabs");
            allow_tabs = (options::indent_with_tabs() == 2)
                         || (  pc->IsComment()
                            && options::indent_with_tabs() != 0);

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
            Chunk *prev = pc->GetPrev();
            log_rule_B("align_with_tabs");
            allow_tabs = (  options::align_with_tabs()
                         && pc->flags.test(PCF_WAS_ALIGNED)
                         && ((prev->column + prev->Len() + 1) != pc->column));

            log_rule_B("align_keep_tabs");

            if (options::align_keep_tabs())
            {
               allow_tabs |= pc->after_tab;
            }
            LOG_FMT(LOUTIND, "%s(%d): at column %zu(%s)\n",
                    __func__, __LINE__, pc->column, (allow_tabs ? "true" : "FALSE"));
         }
         output_to_column(pc->column, allow_tabs);

         if (write_in_tracking)
         {
            if (chunk_is_token(pc, CT_ANGLE_OPEN))
            {
               add_text("&lt;", false, false);
            }
            else if (chunk_is_token(pc, CT_ANGLE_CLOSE))
            {
               add_text("&gt;", false, false);
            }
            else
            {
               add_text(pc->str, false, chunk_is_token(pc, CT_STRING));
            }
            write_in_tracking = false;
         }
         else
         {
            add_text(pc->str, false, chunk_is_token(pc, CT_STRING));
         }

         if (chunk_is_token(pc, CT_PP_DEFINE))  // Issue #876
         {
            // If true, a <TAB> is inserted after #define.
            log_rule_B("force_tab_after_define");

            if (options::force_tab_after_define())
            {
               add_char('\t');
            }
         }
         cpd.did_newline       = chunk_is_newline(pc);
         cpd.output_trailspace = false;
      }

      if (pc->tracking != nullptr)
      {
         LOG_FMT(LGUY, " Tracking info are: \n");
         LOG_FMT(LGUY, "  number of track(s) %zu\n", pc->tracking->size());
         add_text("<a title=\"");
         char tempText[80];

         for (size_t track = 0; track < pc->tracking->size(); track++)
         {
            track_list *A       = pc->tracking;
            Track_nr   B        = A->at(track);
            size_t     Bfirst   = B.first;
            char       *Bsecond = B.second;

            sprintf(tempText, "%zu", Bfirst);
            add_text(tempText);
            add_text(",");

            if (track == pc->tracking->size() - 1)
            {
               sprintf(tempText, "%s", Bsecond);
               add_text(tempText);
            }
            LOG_FMT(LGUY, "  %zu, tracking number is %zu\n", track, Bfirst);
            LOG_FMT(LGUY, "  %zu, rule            is %s\n", track, Bsecond);
         }

         add_text("\"><font color=\"red\">M</font></a>");
         write_in_tracking = true;
      }
   }

   if (tracking)
   {
      add_text("</pre>\n");
      add_text("</body>\n");
      add_text("</html>\n");
   }
} // output_text


void dump_step(const char *filename, const char *step_description)
{
   static int file_num = 0;
   char       buffer[256];
   FILE       *dump_file;

   if (  filename == nullptr
      || strlen(filename) == 0)
   {
      return;
   }

   // On the first call, also save the options in use
   if (file_num == 0)
   {
      snprintf(buffer, 256, "New dump file: %s_%03d.log - Options in use", filename, file_num);
      log_rule_B(buffer);

      snprintf(buffer, 256, "%s_%03d.log", filename, file_num);
      ++file_num;

      dump_file = fopen(buffer, "wb");

      if (dump_file != nullptr)
      {
         save_option_file(dump_file, false, true);
         fclose(dump_file);
      }
   }
   snprintf(buffer, 256, "New dump file: %s_%03d.log - %s", filename, file_num, step_description);
   log_rule_B(buffer);

   snprintf(buffer, 256, "%s_%03d.log", filename, file_num);
   ++file_num;

   dump_file = fopen(buffer, "wb");

   if (dump_file != nullptr)
   {
      fprintf(dump_file, "STEP: %s\n--------------\n", step_description);
      output_parsed(dump_file, false);
      fclose(dump_file);
   }
} // dump_step


static size_t cmt_parse_lead(const unc_text &line, bool is_last)
{
   size_t len = 0;

   while (  len < 32
         && len < line.size()) // TODO what is the meaning of 32?
   {
      if (  len > 0
         && line[len] == '/')
      {
         // ignore combined comments
         size_t tmp = len + 1;

         while (  tmp < line.size()
               && unc_isspace(line[tmp]))
         {
            tmp++;
         }

         if (  tmp < line.size()
            && line[tmp] == '/')
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
      && (  len >= line.size()
         || unc_isspace(line[len])))
   {
      return(len);
   }

   if (  len == 1
      && line[0] == '*')
   {
      return(len);
   }

   if (  is_last
      && len > 0)
   {
      return(len);
   }
   return(0);
} // cmt_parse_lead


/**
 * Eat whitespace characters starting at the specified index in the forward or reverse direction
 * within a single line
 * @param  str     the input string containing the comment text
 * @param  idx     the starting index
 * @param  forward if true, searches in the forward direction;
 *                 if false, searches in the reverse direction
 * @return         the first index at which a non-whitespace character is encountered, including
 *                 a newline character
 */
template<typename String>
static int eat_line_whitespace(const String &str,
                               int idx, bool
                               forward = true)
{
   auto advance_index = [&](int i)
   {
      return(forward ? i + 1 : i - 1);
   };

   auto index_in_range = [&](int i)
   {
      // TODO: the following BREAKS with source code formatting; uncrustify seems to
      //       think that the following is a template. This will NEED to be fixed!!!
      //       For now, reformulate the statement
      //return(forward ? i<int(str.size()) : i> = 0);
      return(forward ? (i < int(str.size())) : (i >= 0));
   };

   while (  index_in_range(idx)
         && str[idx] != '\n'
         && str[idx] != '\r'
         && unc_isspace(str[idx]))
   {
      idx = advance_index(idx);
   }
   return(idx);
} // eat_line_whitespace


/**
 * Returns whether or not a javaparam tag is the leading
 * text in a comment line, with only a sequence of whitespace
 * and/or '*' characters preceding it
 * @param  str the input string containing the comment text
 * @param  idx the starting index
 * @return     true/false
 */
template<typename String>
static bool javaparam_tag_is_start_of_line(const String &str, int idx)
{
   idx = eat_line_whitespace(str,
                             str[idx] == '@' ? idx - 1 : idx,
                             false);

   while (true)
   {
      if (  idx < 0
         || str[idx] == '\n'
         || str[idx] == '\r')
      {
         return(true);
      }

      if (str[idx] == '*')
      {
         idx = eat_line_whitespace(str,
                                   idx - 1,
                                   false);
      }
      else
      {
         return(false);
      }
   }
} // javaparam_tag_is_start_of_line


/**
 * Attempts to match a doxygen/javadoc-style comment tag
 * @param  str the input string containing the comment text
 * @param  idx the starting index
 * @return     the index of the character immediately following the matched tag,
 *             or -1 if no match is found
 */
static int match_doxygen_javadoc_tag(const std::wstring &str, size_t idx)
{
   std::wsmatch match;

   if (str[idx] == L'@')
   {
      // Issue #3357
      std::wregex criteria(L"(@(?:author|"
                           L"deprecated|"
                           L"exception|"
                           L"param(?:\\s*?\\[\\s*(?:in\\s*,\\s*out|in|out)\\s*?\\])?|"
                           L"return|"
                           L"see|"
                           L"since|"
                           L"throws|"
                           L"version)(?=\\s))");

      if (  std::regex_search(str.cbegin() + idx, str.cend(), match, criteria)
         && match[1].matched
         && match.position(1) == std::wsmatch::difference_type(0))
      {
         std::set<std::wstring> block_tags =
         {
            L"@author",
            L"@deprecated",
            L"@exception",
            L"@param",
            L"@param[in]",
            L"@param[in,out]",
            L"@param[out]",
            L"@return",
            L"@see",
            L"@since",
            L"@throws",
            L"@version"
         };
         std::wstring           result(match[1]);
         result.erase(std::remove_if(result.begin(), result.end(), ::isspace), result.end());
         auto                   &&it_block_tag = block_tags.find(result);

         if (  it_block_tag != block_tags.end()
            && javaparam_tag_is_start_of_line(str, idx))
         {
            return(int(idx + match[1].length()));
         }
      }
   }
   return(-1);
} // match_javadoc_block_tag


static void calculate_doxygen_javadoc_indent_alignment(const std::wstring &str,
                                                       size_t             &doxygen_javadoc_param_name_indent,
                                                       size_t             &doxygen_javadoc_continuation_indent)
{
   log_rule_B("cmt_align_doxygen_javadoc_tags");

   doxygen_javadoc_continuation_indent = 0;
   doxygen_javadoc_param_name_indent   = 0;

   if (!options::cmt_align_doxygen_javadoc_tags())
   {
      return;
   }

   for (size_t idx = 0; idx < str.size(); ++idx)
   {
      int start_idx = idx;
      int end_idx   = match_doxygen_javadoc_tag(str, start_idx);

      if (end_idx > start_idx)
      {
         size_t block_tag_width = 1 + std::count_if(str.begin() + start_idx,
                                                    str.begin() + end_idx,
                                                    [](wchar_t ch) {
            return(!unc_isspace(ch));
         });

         if (block_tag_width > doxygen_javadoc_param_name_indent)
         {
            doxygen_javadoc_param_name_indent = block_tag_width;
         }
         idx = eat_line_whitespace(str, end_idx);

         size_t param_name_width = 0;

         if (str.find(L"@param", start_idx) == size_t(start_idx))
         {
            param_name_width = 1;

            while (true)
            {
               while (  !unc_isspace(str[idx])
                     && str[idx] != ',')
               {
                  ++param_name_width;
                  ++idx;
               }
               idx = eat_line_whitespace(str, idx);

               if (str[idx] != ',')
               {
                  break;
               }
               param_name_width += 2;
               idx               = eat_line_whitespace(str, idx + 1);
            }
         }

         if (param_name_width > doxygen_javadoc_continuation_indent)
         {
            doxygen_javadoc_continuation_indent = param_name_width;
         }
      }
   }

   if (doxygen_javadoc_param_name_indent > 0)
   {
      log_rule_B("cmt_sp_before_doxygen_javadoc_tags");

      doxygen_javadoc_param_name_indent   += options::cmt_sp_before_doxygen_javadoc_tags();
      doxygen_javadoc_continuation_indent += doxygen_javadoc_param_name_indent;
   }
} // calculate_doxygen_javadoc_indent_alignment


static void calculate_comment_body_indent(cmt_reflow &cmt, const unc_text &str)
{
   cmt.xtra_indent = 0;

   log_rule_B("cmt_indent_multi");

   if (!options::cmt_indent_multi())
   {
      return;
   }
   size_t idx      = 0;
   size_t len      = str.size();
   size_t last_len = 0;

   log_rule_B("cmt_multi_check_last");

   if (options::cmt_multi_check_last())
   {
      // find the last line length
      for (idx = len - 1; idx > 0; idx--)
      {
         if (  str[idx] == '\n'
            || str[idx] == '\r')
         {
            idx++;

            while (  idx < len
                  && (  str[idx] == ' '
                     || str[idx] == '\t'))
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
      if (  str[idx] == '\n'
         || str[idx] == '\r')
      {
         first_len = idx;

         while (  str[first_len - 1] == ' '
               || str[first_len - 1] == '\t')
         {
            if (first_len == 0)
            {
               fprintf(stderr, "%s(%d): first_len is ZERO, cannot be decremented.\n",
                       __func__, __LINE__);
               log_flush(true);
               exit(EX_SOFTWARE);
            }
            first_len--;
         }

         // handle DOS endings
         if (  str[idx] == '\r'
            && str[idx + 1] == '\n')
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
      if (  str[idx] == ' '
         || str[idx] == '\t')
      {
         if (width > 0)
         {
            break;
         }
         continue;
      }

      if (  str[idx] == '\n'
         || str[idx] == '\r')
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
         if (  width != 1
            || str[idx - 1] != '*')
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
   log_rule_B("cmt_multi_first_len_minimum");

   if (  first_len == last_len
      && (  first_len > options::cmt_multi_first_len_minimum()
         || first_len == width))
   {
      return;
   }
   cmt.xtra_indent = (width == 2) ? 0 : 1;
} // calculate_comment_body_indent


// TODO: can we use search_next_chunk here?
static Chunk *get_next_function(Chunk *pc)
{
   if (pc == nullptr)
   {
      pc = Chunk::NullChunkPtr;
   }

   while ((pc = pc->GetNext())->IsNotNullChunk())
   {
      if (  chunk_is_token(pc, CT_FUNC_DEF)
         || chunk_is_token(pc, CT_FUNC_PROTO)
         || chunk_is_token(pc, CT_FUNC_CLASS_DEF)
         || chunk_is_token(pc, CT_FUNC_CLASS_PROTO)
         || chunk_is_token(pc, CT_OC_MSG_DECL))
      {
         return(pc);
      }
   }
   return(nullptr);
}


static Chunk *get_next_class(Chunk *pc)
{
   return(chunk_search_next_cat(pc, CT_CLASS)->GetNext());
}


static Chunk *get_prev_category(Chunk *pc)
{
   return(chunk_search_prev_cat(pc, CT_OC_CATEGORY));
}


static Chunk *get_next_scope(Chunk *pc)
{
   return(chunk_search_next_cat(pc, CT_OC_SCOPE));
}


static Chunk *get_prev_oc_class(Chunk *pc)
{
   return(chunk_search_prev_cat(pc, CT_OC_CLASS));
}


static int next_up(const unc_text &text, size_t idx, const unc_text &tag)
{
   size_t offs = 0;

   while (  idx < text.size()
         && unc_isspace(text[idx]))
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
                             cmt_reflow     &cmt,
                             bool           esc_close,
                             size_t         continuation_indent)
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
         int tmp = next_up(text, idx + 1, "//");

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
              && options::cmt_width() > 0
              && (  cpd.column > options::cmt_width()
                 || (  ch_cnt > 1
                    && next_word_exceeds_limit(text, idx))))
      {
         log_rule_B("cmt_width");
         in_word = false;
         add_char('\n');
         cmt_output_indent(cmt.brace_col, cmt.base_col, cmt.column);

         if (cmt.xtra_indent > 0)
         {
            add_char(' ');
         }
         // The number of spaces to insert after the star on subsequent comment lines.
         log_rule_B("cmt_sp_after_star_cont");

         /**
          * calculate the output column
          */
         size_t column = options::cmt_sp_after_star_cont();

         if (  text[idx + 1] == 42                 // this is star *
            && text[idx + 2] == 47)                // this is      /
         {
            LOG_FMT(LCONTTEXT, "%s(%d): we have a comment end\n",
                    __func__, __LINE__);

            column += cmt.column;
         }
         else
         {
            add_text(cmt.cont_text);

            if (continuation_indent > 0)
            {
               if (options::cmt_align_doxygen_javadoc_tags())
               {
                  log_rule_B("cmt_align_doxygen_javadoc_tags");
               }
               else if (options::cmt_reflow_indent_to_paragraph_start())
               {
                  log_rule_B("cmt_reflow_indent_to_paragraph_start");
               }
               column += continuation_indent;

               log_rule_B("cmt_sp_after_star_cont");

               if (column >= options::cmt_sp_after_star_cont())
               {
                  column -= options::cmt_sp_after_star_cont();
               }
            }
            /**
             * count the number trailing spaces in the comment continuation text
             */
            size_t num_trailing_sp = 0;

            while (  num_trailing_sp < cmt.cont_text.size()
                  && unc_isspace(cmt.cont_text[cmt.cont_text.size() - 1 - num_trailing_sp]))
            {
               ++num_trailing_sp;
            }
            column += cpd.column;

            if (column >= num_trailing_sp)
            {
               column -= num_trailing_sp;
            }
         }
         output_to_column(column,
                          false);
         ch_cnt = 0;
      }
      else
      {
         // Escape a C closure in a CPP comment
         if (  esc_close
            && (  (  was_star
                  && text[idx] == '/')
               || (  was_slash
                  && text[idx] == '*')))
         {
            add_char(' ');
         }

         if (  !in_word
            && !unc_isspace(text[idx]))
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


static void output_cmt_start(cmt_reflow &cmt, Chunk *pc)
{
   cmt.pc          = pc;
   cmt.column      = pc->column;
   cmt.brace_col   = pc->column_indent;
   cmt.base_col    = pc->column_indent;
   cmt.word_count  = 0;
   cmt.xtra_indent = 0;
   cmt.cont_text.clear();
   cmt.reflow = false;

   // Issue #2752
   log_rule_B("cmt_insert_file_header");
   log_rule_B("cmt_insert_file_footer");
   log_rule_B("cmt_insert_func_header)");
   log_rule_B("cmt_insert_class_header");
   log_rule_B("cmt_insert_oc_msg_header");

   if (  options::cmt_insert_file_header().size() > 0
      || options::cmt_insert_file_footer().size() > 0
      || options::cmt_insert_func_header().size() > 0
      || options::cmt_insert_class_header().size() > 0
      || options::cmt_insert_oc_msg_header().size() > 0)
   {
      LOG_FMT(LCONTTEXT, "%s(%d): cmt_insert_file\n", __func__, __LINE__);
      do_kw_subst(pc);
   }
   else
   {
      LOG_FMT(LCONTTEXT, "%s(%d): no cmt_insert_file\n", __func__, __LINE__);
   }

   if (cmt.brace_col == 0)
   {
      log_rule_B("output_tab_size");
      cmt.brace_col = 1 + (pc->brace_level * options::output_tab_size());
   }
   // LOG_FMT(LSYS, "%s: line %zd, brace=%zd base=%zd col=%zd orig=%zd aligned=%x\n",
   //        __func__, pc->orig_line, cmt.brace_col, cmt.base_col, cmt.column, pc->orig_col,
   //        pc->flags & (PCF_WAS_ALIGNED | PCF_RIGHT_COMMENT));

   if (  get_chunk_parent_type(pc) == CT_COMMENT_START
      || get_chunk_parent_type(pc) == CT_COMMENT_WHOLE)
   {
      log_rule_B("indent_col1_comment");

      if (  !options::indent_col1_comment()
         && pc->orig_col == 1
         && !pc->flags.test(PCF_INSERTED))
      {
         cmt.column    = 1;
         cmt.base_col  = 1;
         cmt.brace_col = 1;
      }
   }
   // tab aligning code
   log_rule_B("indent_cmt_with_tabs");

   if (  options::indent_cmt_with_tabs()
      && (  get_chunk_parent_type(pc) == CT_COMMENT_END
         || get_chunk_parent_type(pc) == CT_COMMENT_WHOLE))
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


static bool can_combine_comment(Chunk *pc, cmt_reflow &cmt)
{
   // We can't combine if there is something other than a newline next
   if (get_chunk_parent_type(pc) == CT_COMMENT_START)
   {
      return(false);
   }

   // next is a newline for sure, make sure it is a single newline
   if (pc == nullptr)
   {
      pc = Chunk::NullChunkPtr;
   }
   Chunk *next = pc->GetNext();

   if (  next->IsNotNullChunk()
      && next->nl_count == 1)
   {
      // Make sure the comment is the same type at the same column
      next = next->GetNext();

      if (  chunk_is_token(next, pc->type)
         && (  (  next->column == 1
               && pc->column == 1)
            || (  next->column == cmt.base_col
               && pc->column == cmt.base_col)
            || (  next->column > cmt.base_col
               && get_chunk_parent_type(pc) == CT_COMMENT_END)))
      {
         return(true);
      }
   }
   return(false);
} // can_combine_comment


static Chunk *output_comment_c(Chunk *first)
{
   cmt_reflow cmt;

   output_cmt_start(cmt, first);
   log_rule_B("cmt_reflow_mode");
   cmt.reflow = (options::cmt_reflow_mode() != 1);

   // See if we can combine this comment with the next comment
   log_rule_B("cmt_c_group");

   if (  !options::cmt_c_group()
      || !can_combine_comment(first, cmt))
   {
      // Just add the single comment
      log_rule_B("cmt_star_cont");
      cmt.cont_text = options::cmt_star_cont() ? " * " : "   ";
      LOG_CONTTEXT();

      log_rule_B("cmt_trailing_single_line_c_to_cpp");

      if (options::cmt_trailing_single_line_c_to_cpp() && chunk_is_last_on_line(first))
      {
         add_text("//");

         unc_text tmp;
         tmp.set(first->str, 2, first->Len() - 4);
         cmt_trim_whitespace(tmp, false);
         add_comment_text(tmp, cmt, false);
      }
      else
      {
         add_comment_text(first->str, cmt, false);
      }
      return(first);
   }
   log_rule_B("cmt_star_cont");
   cmt.cont_text = options::cmt_star_cont() ? " *" : "  ";
   LOG_CONTTEXT();

   add_text("/*");

   log_rule_B("cmt_c_nl_start");

   if (options::cmt_c_nl_start())
   {
      add_comment_text("\n", cmt, false);
   }
   Chunk    *pc = first;
   unc_text tmp;

   while (can_combine_comment(pc, cmt))
   {
      LOG_FMT(LCONTTEXT, "%s(%d): Text() is '%s'\n",
              __func__, __LINE__, pc->Text());
      tmp.set(pc->str, 2, pc->Len() - 4);

      if (  cpd.last_char == '*'
         && (  tmp[0] == '/'
            || tmp[0] != ' '))                 // Issue #1908
      {
         LOG_FMT(LCONTTEXT, "%s(%d): add_text a " "\n", __func__, __LINE__);
         add_text(" ");
      }
      // In case of reflow, original comment could contain trailing spaces before closing the comment, we don't need them after reflow
      LOG_FMT(LCONTTEXT, "%s(%d): trim\n", __func__, __LINE__);
      cmt_trim_whitespace(tmp, false);
      LOG_FMT(LCONTTEXT, "%s(%d): add_comment_text(tmp is '%s')\n",
              __func__, __LINE__, tmp.c_str());
      add_comment_text(tmp, cmt, false);
      LOG_FMT(LCONTTEXT, "%s(%d): add_comment_text(newline)\n",
              __func__, __LINE__);
      add_comment_text("\n", cmt, false);
      pc = pc->GetNext();
      pc = pc->GetNext();
   }
   tmp.set(pc->str, 2, pc->Len() - 4);

   if (  cpd.last_char == '*'
      && tmp[0] == '/')
   {
      add_text(" ");
   }
   // In case of reflow, original comment could contain trailing spaces before closing the comment, we don't need them after reflow
   cmt_trim_whitespace(tmp, false);
   add_comment_text(tmp, cmt, false);

   log_rule_B("cmt_c_nl_end");

   if (options::cmt_c_nl_end())
   {
      cmt.cont_text = " ";
      LOG_CONTTEXT();
      add_comment_text("\n", cmt, false);
   }
   add_comment_text("*/", cmt, false);
   return(pc);
} // output_comment_c


static Chunk *output_comment_cpp(Chunk *first)
{
   cmt_reflow cmt;

   output_cmt_start(cmt, first);
   log_rule_B("cmt_reflow_mode");
   cmt.reflow = (options::cmt_reflow_mode() != 1);

   unc_text leadin = "//";             // default setting to keep previous behaviour

   // If true, space is added with sp_cmt_cpp_start will be added after doxygen
   // sequences like '///', '///<', '//!' and '//!<'.
   log_rule_B("sp_cmt_cpp_doxygen");

   if (options::sp_cmt_cpp_doxygen())  // special treatment for doxygen style comments (treat as unity)
   {
      const char *sComment = first->Text();
      bool       grouping  = (sComment[2] == '@');
      size_t     brace     = 3;

      if (  sComment[2] == '/'
         || sComment[2] == '!') // doxygen style found!
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
         && (  sComment[brace] == '{'
            || sComment[brace] == '}'))
      {
         leadin += '@';
         leadin += sComment[brace];
      }
   }
   // Special treatment for Qt translator or meta-data comments (treat as unity)
   // If true, space is added with sp_cmt_cpp_start will be added after Qt
   // translator or meta-data comments like '//:', '//=', and '//~'.
   log_rule_B("sp_cmt_cpp_qttr");

   if (options::sp_cmt_cpp_qttr())
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
   log_rule_B("cmt_cpp_to_c");

   if (!options::cmt_cpp_to_c())
   {
      auto const *cmt_text = first->str.c_str() + 2;
      // Add or remove space after the opening of a C++ comment,
      // i.e. '// A' vs. '//A'.
      auto *sp_cmt = &options::sp_cmt_cpp_start;

      cmt.cont_text = leadin;

      // Get start of comment text
      while (  *cmt_text != '\0'
            && unc_isspace(*cmt_text))
      {
         ++cmt_text;
      }

      // Determine if we are dealing with a region marker
      if (  (  !first->prev
            || first->prev->orig_line != first->orig_line)
         && (  strncmp(cmt_text, "BEGIN", 5) == 0
            || strncmp(cmt_text, "END", 3) == 0))
      {
         // If sp_cmt_cpp_region is not ignore, use that instead of
         // sp_cmt_cpp_start
         if (options::sp_cmt_cpp_region() != IARF_IGNORE)
         {
            sp_cmt = &options::sp_cmt_cpp_region;
         }
      }
      // Add or remove space after the opening of a C++ comment,
      // i.e. '// A' vs. '//A'.
      log_rule_B(sp_cmt->name());

      if ((*sp_cmt)() != IARF_REMOVE)
      {
         cmt.cont_text += ' ';
      }
      LOG_CONTTEXT();

      // Add or remove space after the opening of a C++ comment,
      // i.e. '// A' vs. '//A'.
      log_rule_B(sp_cmt->name());

      if ((*sp_cmt)() == IARF_IGNORE)
      {
         add_comment_text(first->str, cmt, false);
      }
      else
      {
         size_t   iLISz = leadin.size();
         unc_text tmp(first->str, 0, iLISz);
         add_comment_text(tmp, cmt, false);

         tmp.set(first->str, iLISz, first->Len() - iLISz);

         // Add or remove space after the opening of a C++ comment,
         // i.e. '// A' vs. '//A'.
         log_rule_B("sp_cmt_cpp_start");

         if ((*sp_cmt)() & IARF_REMOVE)
         {
            while (  (tmp.size() > 0)
                  && unc_isspace(tmp[0]))
            {
               tmp.pop_front();
            }
         }

         if (tmp.size() > 0)
         {
            // Add or remove space after the opening of a C++ comment,
            // i.e. '// A' vs. '//A'.
            log_rule_B("sp_cmt_cpp_start");

            if ((*sp_cmt)() & IARF_ADD)
            {
               if (  !unc_isspace(tmp[0])
                  && (tmp[0] != '/'))
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
   log_rule_B("cmt_star_cont");
   cmt.cont_text = options::cmt_star_cont() ? " * " : "   ";
   LOG_CONTTEXT();

   unc_text tmp;

   // See if we can combine this comment with the next comment
   log_rule_B("cmt_cpp_group");

   if (  !options::cmt_cpp_group()
      || !can_combine_comment(first, cmt))
   {
      // nothing to group: just output a single line
      add_text("/*");

      // patch # 32, 2012-03-23
      // Add or remove space after the opening of a C++ comment,
      // i.e. '// A' vs. '//A'.
      log_rule_B("sp_cmt_cpp_start");

      if (  !unc_isspace(first->str[2])
         && (options::sp_cmt_cpp_start() & IARF_ADD))
      {
         add_char(' ');
      }
      tmp.set(first->str, 2, first->Len() - 2);
      add_comment_text(tmp, cmt, true);
      add_text(" */");
      return(first);
   }
   add_text("/*");

   log_rule_B("cmt_cpp_nl_start");

   if (options::cmt_cpp_nl_start())
   {
      add_comment_text("\n", cmt, false);
   }
   else
   {
      add_text(" ");
   }
   Chunk *pc = first;
   int   offs;

   while (can_combine_comment(pc, cmt))
   {
      offs = unc_isspace(pc->str[2]) ? 1 : 0;
      tmp.set(pc->str, 2 + offs, pc->Len() - (2 + offs));

      if (  cpd.last_char == '*'
         && tmp[0] == '/')
      {
         add_text(" ");
      }
      add_comment_text(tmp, cmt, true);
      add_comment_text("\n", cmt, false);
      pc = pc->GetNext()->GetNext();
   }
   offs = unc_isspace(pc->str[2]) ? 1 : 0;
   tmp.set(pc->str, 2 + offs, pc->Len() - (2 + offs));
   add_comment_text(tmp, cmt, true);

   log_rule_B("cmt_cpp_nl_end");

   if (options::cmt_cpp_nl_end())
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
         && (  line.back() == ' '
            || line.back() == '\t'))
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
            && (  line.back() == ' '
               || line.back() == '\t'))
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


/**
 * Return an indexed-map of reflow fold end of line/beginning of line regex pairs read
 * from file
 */
static std::map<std::size_t, std::pair<std::wregex, std::wregex> > get_reflow_fold_regex_map()
{
   /**
    * TODO: should the following be static to prevent initializing it multiple times?
    */
   static std::map<std::size_t, std::pair<std::wregex, std::wregex> > regex_map;

   if (regex_map.empty())
   {
      if (!options::cmt_reflow_fold_regex_file().empty())
      {
         std::wstring raw_wstring(cpd.reflow_fold_regex.raw.begin(),
                                  cpd.reflow_fold_regex.raw.end());

         std::wregex criteria(L"\\s*(?:(?:(beg_of_next)|(end_of_prev))_line_regex)"
                              "\\s*\\[\\s*([0-9]+)\\s*\\]\\s*=\\s*\"(.*)\"\\s*"
                              "(?=\\r\\n|\\r|\\n|$)");
         std::wsregex_iterator it_regex(raw_wstring.cbegin(), raw_wstring.cend(), criteria);
         std::wsregex_iterator it_regex_end = std::wsregex_iterator();

         while (it_regex != it_regex_end)
         {
            std::wsmatch match = *it_regex;

            if (  ((  match[1].matched
                   || match[2].matched))
               && match[3].matched
               && match[4].matched)
            {
               auto        &&index   = std::stoi(match[3].str());
               std::wregex *p_wregex = match[1].matched ? &regex_map[index].second
                                                        : &regex_map[index].first;
               *p_wregex = match[4].str();
            }
            ++it_regex;
         }
      }
      else
      {
         regex_map.emplace(0L, std::make_pair(L"[\\w,\\]\\)]$", L"^[\\w,\\[\\(]"));
         regex_map.emplace(1L, std::make_pair(L"\\.$", L"^[A-Z]"));
      }
   }
   return(regex_map);
} // get_reflow_fold_regex_map


static void output_comment_multi(Chunk *pc)
{
   if (pc == nullptr)
   {
      return;
   }
   cmt_reflow cmt;

   char       copy[1000];

   LOG_FMT(LCONTTEXT, "%s(%d): Text() is '%s', type is %s, orig_col is %zu, column is %zu\n",
           __func__, __LINE__, pc->ElidedText(copy), get_token_name(pc->type), pc->orig_col, pc->column);

   output_cmt_start(cmt, pc);
   log_rule_B("cmt_reflow_mode");
   cmt.reflow = (options::cmt_reflow_mode() != 1);

   size_t cmt_col  = cmt.base_col;
   int    col_diff = pc->orig_col - cmt.base_col;

   calculate_comment_body_indent(cmt, pc->str);

   log_rule_B("cmt_indent_multi");
   log_rule_B("cmt_star_cont");
   cmt.cont_text = !options::cmt_indent_multi() ? "" :
                   (options::cmt_star_cont() ? "* " : "  ");
   LOG_CONTTEXT();

   std::wstring pc_wstring(pc->str.get().cbegin(),
                           pc->str.get().cend());

   size_t doxygen_javadoc_param_name_indent    = 0;
   size_t doxygen_javadoc_continuation_indent  = 0;
   size_t reflow_paragraph_continuation_indent = 0;

   calculate_doxygen_javadoc_indent_alignment(pc_wstring,
                                              doxygen_javadoc_param_name_indent,
                                              doxygen_javadoc_continuation_indent);

   size_t   line_count                   = 0;
   size_t   ccol                         = pc->column; // the col of subsequent comment lines
   size_t   cmt_idx                      = 0;
   bool     nl_end                       = false;
   bool     doxygen_javadoc_indent_align = false;
   unc_text line;

   /*
    * Get a map of regex pairs that define expressions to match at both the end
    * of the previous line and the beginning of the next line
    */
   auto &&cmt_reflow_regex_map = get_reflow_fold_regex_map();

   line.clear();
   LOG_FMT(LCONTTEXT, "%s(%d): pc->Len() is %zu\n",
           __func__, __LINE__, pc->Len());
   //LOG_FMT(LCONTTEXT, "%s(%d): pc->str is %s\n",
   //        __func__, __LINE__, pc->str.c_str());

   /**
    * check for enable/disable processing comment strings that may
    * both be embedded within the same multi-line comment
    */
   auto disable_processing_cmt_idx = find_disable_processing_comment_marker(pc->str);
   auto enable_processing_cmt_idx  = find_enable_processing_comment_marker(pc->str);

   while (cmt_idx < pc->Len())
   {
      int ch = pc->str[cmt_idx];
      cmt_idx++;

      if (  cmt_idx > std::size_t(disable_processing_cmt_idx)
         && enable_processing_cmt_idx > disable_processing_cmt_idx)
      {
         auto     length = enable_processing_cmt_idx - disable_processing_cmt_idx;
         unc_text verbatim_text(pc->str,
                                disable_processing_cmt_idx,
                                length);

         add_text(verbatim_text);

         cmt_idx = enable_processing_cmt_idx;

         /**
          * check for additional enable/disable processing comment strings that may
          * both be embedded within the same multi-line comment
          */
         disable_processing_cmt_idx = find_disable_processing_comment_marker(pc->str,
                                                                             enable_processing_cmt_idx);
         enable_processing_cmt_idx = find_enable_processing_comment_marker(pc->str,
                                                                           enable_processing_cmt_idx);

         /**
          * it's probably necessary to reset the line count to prevent line
          * continuation characters from being added to the end of the current line
          */
         line_count = 0;
      }

      // handle the CRLF and CR endings. convert both to LF
      if (ch == '\r')
      {
         ch = '\n';

         if (  cmt_idx < pc->Len()
            && pc->str[cmt_idx] == '\n')
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
            log_rule_B("input_tab_size");
            ccol = calc_next_tab_column(ccol, options::input_tab_size());
            continue;
         }
         else
         {
            LOG_FMT(LCONTTEXT, "%s(%d):ch is %d, %c\n", __func__, __LINE__, ch, char(ch));
         }
      }

      if (  ch == '@'
         && options::cmt_align_doxygen_javadoc_tags())
      {
         int start_idx = cmt_idx - 1;
         int end_idx   = match_doxygen_javadoc_tag(pc_wstring, start_idx);

         if (end_idx > start_idx)
         {
            doxygen_javadoc_indent_align = true;

            std::string match(pc->str.get().cbegin() + start_idx,
                              pc->str.get().cbegin() + end_idx);

            match.erase(std::remove_if(match.begin(),
                                       match.end(),
                                       ::isspace),
                        match.end());

            /**
             * remove whitespace before the '@'
             */
            int line_size_before_indent = line.size();

            while (  line_size_before_indent > 0
                  && unc_isspace(line.back()))
            {
               line.pop_back();
               --line_size_before_indent;
            }
            log_rule_B("cmt_sp_before_doxygen_javadoc_tags");

            int indent = options::cmt_sp_before_doxygen_javadoc_tags();

            while (indent-- > 0)
            {
               line.append(' ');
            }
            cmt_idx += (end_idx - start_idx);
            line.append(match.c_str());

            bool is_exception_tag = match.find("@exception") != std::string::npos;
            bool is_param_tag     = match.find("@param") != std::string::npos;
            bool is_throws_tag    = match.find("@throws") != std::string::npos;

            if (  is_exception_tag
               || is_param_tag
               || is_throws_tag)
            {
               indent = int(doxygen_javadoc_param_name_indent) - int(line.size());

               while (indent-- > -line_size_before_indent)
               {
                  line.append(' ');
               }

               while (true)
               {
                  cmt_idx = eat_line_whitespace(pc->str,
                                                cmt_idx);

                  while (  cmt_idx < pc->Len()
                        && !unc_isspace(pc->str[cmt_idx])
                        && pc->str[cmt_idx] != ',')
                  {
                     line.append(pc->str[cmt_idx++]);
                  }

                  if (!is_param_tag)
                  {
                     break;
                  }
                  /**
                   * check for the possibility that comma-separated parameter names are present
                   */
                  cmt_idx = eat_line_whitespace(pc->str,
                                                cmt_idx);

                  if (pc->str[cmt_idx] != ',')
                  {
                     break;
                  }
                  ++cmt_idx;
                  line.append(", ");
               }
            }
            cmt_idx = eat_line_whitespace(pc->str,
                                          cmt_idx);
            indent = int(doxygen_javadoc_continuation_indent) - int(line.size());

            while (indent-- > -line_size_before_indent)
            {
               line.append(' ');
            }

            while (  cmt_idx < pc->Len()
                  && !unc_isspace(pc->str[cmt_idx]))
            {
               line.append(pc->str[cmt_idx++]);
            }
            continue;
         }
      }
      /*
       * Now see if we need/must fold the next line with the current to enable
       * full reflow
       */
      log_rule_B("cmt_reflow_mode");

      if (  options::cmt_reflow_mode() == 2
         && ch == '\n'
         && cmt_idx < pc->Len())
      {
         int    next_nonempty_line = -1;
         int    prev_nonempty_line = -1;
         size_t nwidx              = line.size();

         // strip trailing whitespace from the line collected so far
         while (nwidx > 0)
         {
            nwidx--;

            if (  prev_nonempty_line < 0
               && !unc_isspace(line[nwidx])
               && line[nwidx] != '*'    // block comment: skip '*' at end of line
               && (pc->flags.test(PCF_IN_PREPROC)
                   ? (  line[nwidx] != '\\'
                     || (  line[nwidx + 1] != '\r'
                        && line[nwidx + 1] != '\n'))
                   : true))
            {
               prev_nonempty_line = nwidx; // last non-whitespace char in the previous line
            }
         }

         for (size_t nxt_idx = cmt_idx;
              (  nxt_idx < pc->Len()
              && pc->str[nxt_idx] != '\r'
              && pc->str[nxt_idx] != '\n');
              nxt_idx++)
         {
            if (  next_nonempty_line < 0
               && !unc_isspace(pc->str[nxt_idx])
               && pc->str[nxt_idx] != '*'
               && (pc->flags.test(PCF_IN_PREPROC)
                   ? (  pc->str[nxt_idx] != '\\'
                     || (  pc->str[nxt_idx + 1] != '\r'
                        && pc->str[nxt_idx + 1] != '\n'))
                   : true))
            {
               next_nonempty_line = nxt_idx;  // first non-whitespace char in the next line
            }
         }

         if (  options::cmt_reflow_indent_to_paragraph_start()
            && next_nonempty_line >= 0
            && (  prev_nonempty_line <= 0
               || doxygen_javadoc_indent_align))
         {
            log_rule_B("cmt_reflow_indent_to_paragraph_start");

            int cmt_star_indent = 0;

            while (  next_nonempty_line > cmt_star_indent
                  && pc->str[next_nonempty_line - cmt_star_indent - 1] != '*')
            {
               ++cmt_star_indent;
            }
            reflow_paragraph_continuation_indent = size_t(cmt_star_indent);
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
            && next_nonempty_line >= int(cmt_idx))
         {
            std::wstring prev_line(line.get().cbegin(),
                                   line.get().cend());
            std::wstring next_line(pc->str.get().cbegin() + next_nonempty_line,
                                   pc->str.get().cend());

            for (auto &&cmt_reflow_regex_map_entry : cmt_reflow_regex_map)
            {
               auto         &&cmt_reflow_regex_pair  = cmt_reflow_regex_map_entry.second;
               auto         &&end_of_prev_line_regex = cmt_reflow_regex_pair.first;
               auto         &&beg_of_next_line_regex = cmt_reflow_regex_pair.second;
               std::wsmatch match[2];

               if (  std::regex_search(prev_line, match[0], end_of_prev_line_regex)
                  && match[0].position(0) + match[0].length(0) == std::wsmatch::difference_type(line.size())
                  && std::regex_search(next_line, match[1], beg_of_next_line_regex)
                  && match[1].position(0) == 0)
               {
                  // rewind the line to the last non-alpha:
                  line.resize(prev_nonempty_line + 1);

                  // roll the current line forward to the first non-alpha:
                  cmt_idx = next_nonempty_line;
                  // override the NL and make it a single whitespace:
                  ch = ' ';

                  break;
               }
            }
         }
      }

      if (ch == '\n')
      {
         LOG_FMT(LCONTTEXT, "%s(%d):ch is newline\n", __func__, __LINE__);
      }
      else
      {
         LOG_FMT(LCONTTEXT, "%s(%d):ch is %d, %c\n", __func__, __LINE__, ch, char(ch));
      }
      line.append(ch);

      // If we just hit an end of line OR we just hit end-of-comment...
      if (  ch == '\n'
         || cmt_idx == pc->Len())
      {
         if (ch == '\n')
         {
            LOG_FMT(LCONTTEXT, "%s(%d):ch is newline\n", __func__, __LINE__);
         }
         else
         {
            LOG_FMT(LCONTTEXT, "%s(%d):ch is %d, %c\n", __func__, __LINE__, ch, char(ch));
         }
         line_count++;
         LOG_FMT(LCONTTEXT, "%s(%d):line_count is %zu\n", __func__, __LINE__, line_count);

         // strip trailing tabs and spaces before the newline
         if (ch == '\n')
         {
            nl_end = true;
            line.pop_back();
            cmt_trim_whitespace(line, pc->flags.test(PCF_IN_PREPROC));
         }

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
               log_rule_B("cmt_star_cont");

               if (options::cmt_star_cont())
               {
                  // The number of spaces to insert at the start of subsequent comment lines.
                  log_rule_B("cmt_sp_before_star_cont");
                  cmt.column = cmt_col + options::cmt_sp_before_star_cont();
                  cmt_output_indent(cmt.brace_col, cmt.base_col, cmt.column);

                  if (cmt.xtra_indent > 0)
                  {
                     add_char(' ');
                  }
                  // multiline comments can have empty lines with some spaces in them for alignment
                  // while adding * symbol and aligning them we don't want to keep these trailing spaces
                  unc_text tmp = unc_text(cmt.cont_text);
                  cmt_trim_whitespace(tmp, false);
                  add_text(tmp);
               }
               add_char('\n');
            }
            else
            {
               /*
                * If this doesn't start with a '*' or '|'.
                * '\name' is a common parameter documentation thing.
                */
               log_rule_B("cmt_indent_multi");

               if (  options::cmt_indent_multi()
                  && line[0] != '*'
                  && line[0] != '|'
                  && line[0] != '#'
                  && (  line[0] != '\\'
                     || unc_isalpha(line[1]))
                  && line[0] != '+')
               {
                  // The number of spaces to insert at the start of subsequent comment lines.
                  log_rule_B("cmt_sp_before_star_cont");
                  size_t start_col = cmt_col + options::cmt_sp_before_star_cont();

                  log_rule_B("cmt_star_cont");

                  if (options::cmt_star_cont())
                  {
                     cmt.column = start_col;
                     cmt_output_indent(cmt.brace_col, cmt.base_col, cmt.column);

                     if (cmt.xtra_indent > 0)
                     {
                        add_char(' ');
                     }
                     add_text(cmt.cont_text);
                     // The number of spaces to insert after the star on subsequent comment lines.
                     log_rule_B("cmt_sp_after_star_cont");
                     output_to_column(ccol + options::cmt_sp_after_star_cont(), false);
                  }
                  else
                  {
                     cmt.column = ccol;
                     cmt_output_indent(cmt.brace_col, cmt.base_col, cmt.column);
                  }
               }
               else
               {
                  // The number of spaces to insert at the start of subsequent comment lines.
                  log_rule_B("cmt_sp_before_star_cont");
                  cmt.column = cmt_col + options::cmt_sp_before_star_cont();
                  cmt_output_indent(cmt.brace_col, cmt.base_col, cmt.column);

                  if (cmt.xtra_indent > 0)
                  {
                     add_char(' ');
                  }
                  size_t idx;

                  // Checks for and updates the lead chars.
                  // @return 0=not present, >0=number of chars that are part of the lead
                  idx = cmt_parse_lead(line, (cmt_idx == pc->Len()));

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
                     if (language_is_set(LANG_D))
                     {
                        // 0=no lead char present
                        add_text(cmt.cont_text);
                     }
                  }
               }
               size_t continuation_indent = 0;

               if (doxygen_javadoc_indent_align)
               {
                  continuation_indent = doxygen_javadoc_continuation_indent;
               }
               else if (reflow_paragraph_continuation_indent > 0)
               {
                  continuation_indent = reflow_paragraph_continuation_indent;
               }
               add_comment_text(line,
                                cmt,
                                false,
                                continuation_indent);

               if (nl_end)
               {
                  add_text("\n");
               }
            }
         }
         line.clear();
         doxygen_javadoc_indent_align = false;
         ccol                         = 1;
      }
   }
} // output_comment_multi


static bool kw_fcn_filename(Chunk *cmt, unc_text &out_txt)
{
   UNUSED(cmt);
   out_txt.append(path_basename(cpd.filename.c_str()));
   return(true);
}


static bool kw_fcn_class(Chunk *cmt, unc_text &out_txt)
{
   Chunk *tmp = nullptr;

   if (language_is_set(LANG_CPP | LANG_OC))
   {
      Chunk *fcn = get_next_function(cmt);

      if (chunk_is_token(fcn, CT_OC_MSG_DECL))
      {
         tmp = get_prev_oc_class(cmt);
      }
      else
      {
         tmp = get_next_class(cmt);
      }
   }
   else if (language_is_set(LANG_OC))
   {
      tmp = get_prev_oc_class(cmt);
   }

   if (tmp == nullptr)
   {
      tmp = get_next_class(cmt);
   }

   if (tmp != nullptr)
   {
      out_txt.append(tmp->str);

      while ((tmp = tmp->GetNext())->IsNotNullChunk())
      {
         if (tmp->type != CT_DC_MEMBER)
         {
            break;
         }
         tmp = tmp->GetNext();

         if (tmp->IsNotNullChunk())
         {
            out_txt.append("::");
            out_txt.append(tmp->str);
         }
      }
      return(true);
   }
   return(false);
} // kw_fcn_class


static bool kw_fcn_message(Chunk *cmt, unc_text &out_txt)
{
   Chunk *fcn = get_next_function(cmt);

   if (fcn == nullptr)
   {
      return(false);
   }
   out_txt.append(fcn->str);

   Chunk *tmp  = fcn->GetNextNcNnl();
   Chunk *word = Chunk::NullChunkPtr;

   while (tmp->IsNotNullChunk())
   {
      if (  chunk_is_token(tmp, CT_BRACE_OPEN)
         || chunk_is_token(tmp, CT_SEMICOLON))
      {
         break;
      }

      if (chunk_is_token(tmp, CT_OC_COLON))
      {
         if (word->IsNotNullChunk())
         {
            out_txt.append(word->str);
            word = Chunk::NullChunkPtr;
         }
         out_txt.append(":");
      }

      if (chunk_is_token(tmp, CT_WORD))
      {
         word = tmp;
      }
      tmp = tmp->GetNextNcNnl();
   }
   return(true);
} // kw_fcn_message


static bool kw_fcn_category(Chunk *cmt, unc_text &out_txt)
{
   Chunk *category = get_prev_category(cmt);

   if (category)
   {
      out_txt.append('(');
      out_txt.append(category->str);
      out_txt.append(')');
   }
   return(true);
} // kw_fcn_category


static bool kw_fcn_scope(Chunk *cmt, unc_text &out_txt)
{
   Chunk *scope = get_next_scope(cmt);

   if (scope)
   {
      out_txt.append(scope->str);
      return(true);
   }
   return(false);
} // kw_fcn_scope


static bool kw_fcn_function(Chunk *cmt, unc_text &out_txt)
{
   Chunk *fcn = get_next_function(cmt);

   if (fcn)
   {
      if (get_chunk_parent_type(fcn) == CT_OPERATOR)
      {
         out_txt.append("operator ");
      }

      if (  fcn->prev != nullptr
         && fcn->prev->type == CT_DESTRUCTOR)
      {
         out_txt.append('~');
      }
      out_txt.append(fcn->str);
      return(true);
   }
   return(false);
}


static bool kw_fcn_javaparam(Chunk *cmt, unc_text &out_txt)
{
   Chunk *fcn = get_next_function(cmt);

   if (fcn == nullptr)
   {
      return(false);
   }
   Chunk *fpo;
   Chunk *fpc;
   bool  has_param = true;
   bool  need_nl   = false;

   if (chunk_is_token(fcn, CT_OC_MSG_DECL))
   {
      Chunk *tmp = fcn->GetNextNcNnl();
      has_param = false;

      while (tmp->IsNotNullChunk())
      {
         if (  chunk_is_token(tmp, CT_BRACE_OPEN)
            || chunk_is_token(tmp, CT_SEMICOLON))
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

         if (chunk_is_token(tmp, CT_PAREN_CLOSE))
         {
            has_param = true;
         }
         tmp = tmp->GetNextNcNnl();
      }
      fpo = fpc = Chunk::NullChunkPtr;
   }
   else
   {
      fpo = fcn->GetNextType(CT_FPAREN_OPEN, fcn->level);

      if (fpo->IsNullChunk())
      {
         return(true);
      }
      fpc = fpo->GetNextType(CT_FPAREN_CLOSE, fcn->level);

      if (fpc->IsNullChunk())
      {
         return(true);
      }
   }
   Chunk *tmp;

   // Check for 'foo()' and 'foo(void)'
   if (fpo->IsNotNullChunk())
   {
      if (fpo->GetNextNcNnl() == fpc)
      {
         has_param = false;
      }
      else
      {
         tmp = fpo->GetNextNcNnl();

         if (  (tmp == fpc->GetPrevNcNnl())
            && chunk_is_str(tmp, "void"))
         {
            has_param = false;
         }
      }
   }

   if (has_param)
   {
      Chunk *prev = Chunk::NullChunkPtr;
      tmp = fpo;

      while ((tmp = tmp->GetNext())->IsNotNullChunk())
      {
         if (  chunk_is_token(tmp, CT_COMMA)
            || tmp == fpc)
         {
            if (need_nl)
            {
               out_txt.append("\n");
            }
            need_nl = true;
            out_txt.append("@param");

            if (prev->IsNotNullChunk())
            {
               out_txt.append(" ");
               out_txt.append(prev->str);
               out_txt.append(" TODO");
            }
            prev = Chunk::NullChunkPtr;

            if (tmp == fpc)
            {
               break;
            }
         }

         if (chunk_is_token(tmp, CT_WORD))
         {
            prev = tmp;
         }
      }
   }
   // Do the return stuff
   tmp = fcn->GetPrevNcNnl();

   // For Objective-C we need to go to the previous chunk
   if (  tmp->IsNotNullChunk()
      && get_chunk_parent_type(tmp) == CT_OC_MSG_DECL
      && chunk_is_token(tmp, CT_PAREN_CLOSE))
   {
      tmp = tmp->GetPrevNcNnl();
   }

   if (  tmp->IsNotNullChunk()
      && !chunk_is_str(tmp, "void"))
   {
      if (need_nl)
      {
         out_txt.append("\n");
      }
      out_txt.append("@return TODO");
   }
   return(true);
} // kw_fcn_javaparam


static bool kw_fcn_fclass(Chunk *cmt, unc_text &out_txt)
{
   Chunk *fcn = get_next_function(cmt);

   if (!fcn)
   {
      return(false);
   }

   if (fcn->flags.test(PCF_IN_CLASS))
   {
      // if inside a class, we need to find to the class name
      Chunk *tmp = fcn->GetPrevType(CT_BRACE_OPEN, fcn->level - 1);
      tmp = tmp->GetPrevType(CT_CLASS, tmp->level);

      if (tmp->IsNullChunk())
      {
         tmp = Chunk::NullChunkPtr;
      }
      else
      {
         tmp = tmp->GetNextNcNnl();
      }

      while (  tmp->IsNotNullChunk()
            && chunk_is_token(tmp->GetNextNcNnl(), CT_DC_MEMBER))
      {
         tmp = tmp->GetNextNcNnl();
         tmp = tmp->GetNextNcNnl();
      }

      if (tmp->IsNotNullChunk())
      {
         out_txt.append(tmp->str);
         return(true);
      }
   }
   else
   {
      // if outside a class, we expect "CLASS::METHOD(...)"
      Chunk *tmp = fcn->GetPrevNcNnl();

      if (chunk_is_token(tmp, CT_OPERATOR))
      {
         tmp = tmp->GetPrevNcNnl();
      }

      if (  tmp->IsNotNullChunk()
         && (  chunk_is_token(tmp, CT_DC_MEMBER)
            || chunk_is_token(tmp, CT_MEMBER)))
      {
         tmp = tmp->GetPrevNcNnl();
         out_txt.append(tmp->str);
         return(true);
      }
   }
   return(false);
} // kw_fcn_fclass


static bool kw_fcn_year(Chunk *cmt, unc_text &out_txt)
{
   UNUSED(cmt);
   time_t now = time(nullptr);

   out_txt.append(std::to_string(1900 + localtime(&now)->tm_year));
   return(true);
}


struct kw_subst_t
{
   const char *tag;
   bool       (*func)(Chunk *cmt, unc_text &out_txt);
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
   { "$(year)",      kw_fcn_year      },
};


static void do_kw_subst(Chunk *pc)
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


static void output_comment_multi_simple(Chunk *pc)
{
   if (  pc == nullptr
      && pc->IsNotNullChunk())
   {
      return;
   }
   cmt_reflow cmt;

   LOG_FMT(LCONTTEXT, "%s(%d): Text() is '%s', type is %s, orig_col is %zu, column is %zu\n",
           __func__, __LINE__, pc->Text(), get_token_name(pc->type), pc->orig_col, pc->column);

   output_cmt_start(cmt, pc);

   // The multiline comment is saved inside one chunk. If the comment is
   // shifted all lines of the comment need to be shifted by the same amount.
   // Save the difference of initial and current position to apply it on every
   // line_column
   const int col_diff = [pc]()
   {
      int diff = 0;

      if (chunk_is_newline(pc->GetPrev()))
      {
         // The comment should be indented correctly
         diff = pc->column - pc->orig_col;
      }
      return(diff);
   }();

   /**
    * check for enable/disable processing comment strings that may
    * both be embedded within the same multi-line comment
    */
   auto     disable_processing_cmt_idx = find_disable_processing_comment_marker(pc->str);
   auto     enable_processing_cmt_idx  = find_enable_processing_comment_marker(pc->str);

   unc_text line;
   size_t   line_count  = 0;
   size_t   line_column = pc->column;
   size_t   cmt_idx     = 0;

   while (cmt_idx < pc->Len())
   {
      int ch = pc->str[cmt_idx];
      cmt_idx++;

      if (  cmt_idx > std::size_t(disable_processing_cmt_idx)
         && enable_processing_cmt_idx > disable_processing_cmt_idx)
      {
         auto     length = enable_processing_cmt_idx - disable_processing_cmt_idx;
         unc_text verbatim_text(pc->str,
                                disable_processing_cmt_idx,
                                length);

         add_text(verbatim_text);

         cmt_idx = enable_processing_cmt_idx;

         /**
          * check for additional enable/disable processing comment strings that may
          * both be embedded within the same multi-line comment
          */
         disable_processing_cmt_idx = find_disable_processing_comment_marker(pc->str,
                                                                             enable_processing_cmt_idx);
         enable_processing_cmt_idx = find_enable_processing_comment_marker(pc->str,
                                                                           enable_processing_cmt_idx);

         line.clear();

         continue;
      }
      // 1: step through leading tabs and spaces to find the start column
      log_rule_B("cmt_convert_tab_to_spaces");

      if (  line.size() == 0
         && (  line_column < cmt.base_col
            || options::cmt_convert_tab_to_spaces()))
      {
         if (ch == ' ')
         {
            line_column++;
            continue;
         }
         else if (ch == '\t')
         {
            log_rule_B("input_tab_size");
            line_column = calc_next_tab_column(line_column, options::input_tab_size());
            continue;
         }
         else
         {
            LOG_FMT(LCONTTEXT, "%s(%d):ch is %d, %c\n", __func__, __LINE__, ch, char(ch));
         }
      }

      // 2: add chars to line, handle the CRLF and CR endings (convert both to LF)
      if (ch == '\r')
      {
         ch = '\n';

         if (  (cmt_idx < pc->Len())
            && (pc->str[cmt_idx] == '\n'))
         {
            cmt_idx++;
         }
      }
      LOG_FMT(LCONTTEXT, "%s(%d):Line is %s\n", __func__, __LINE__, line.c_str());
      line.append(ch);
      LOG_FMT(LCONTTEXT, "%s(%d):Line is %s\n", __func__, __LINE__, line.c_str());

      // If we just hit an end of line OR we just hit end-of-comment...
      if (  ch == '\n'
         || cmt_idx == pc->Len())
      {
         line_count++;
         LOG_FMT(LCONTTEXT, "%s(%d):line_count is %zu\n", __func__, __LINE__, line_count);

         // strip trailing tabs and spaces before the newline
         if (ch == '\n')
         {
            line.pop_back();

            // Say we aren't in a preproc to prevent changing any bs-nl
            cmt_trim_whitespace(line, false);

            line.append('\n');
         }

         if (line.size() > 0)
         {
            // unless line contains only a single newline char, indent if the
            // line consists of either:
            if (  line.size() > 1 // more than a single newline char or
               || ch != '\n')     // (end-of-comment) a single non newline char
            {
               if (line_count > 1)
               {
                  // apply comment column shift without underflowing
                  line_column = (  col_diff < 0
                                && (cast_abs(line_column, col_diff) > line_column))
                                ? 0 : line_column + col_diff;
               }
               cmt.column = line_column;
               cmt_output_indent(cmt.brace_col, cmt.base_col, cmt.column);
            }
            add_text(line);

            line.clear();
         }
         line_column = 1;
      }
   }
} // output_comment_multi_simple


static void generate_if_conditional_as_text(unc_text &dst, Chunk *ifdef)
{
   int column = -1;

   dst.clear();

   for (Chunk *pc = ifdef; pc != nullptr && pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      if (column == -1)
      {
         column = pc->column;
      }

      if (  chunk_is_token(pc, CT_NEWLINE)
         || chunk_is_token(pc, CT_COMMENT_MULTI)
         || chunk_is_token(pc, CT_COMMENT_CPP))
      {
         break;
      }
      else if (chunk_is_token(pc, CT_NL_CONT))
      {
         dst   += ' ';
         column = -1;
      }
      else if (  chunk_is_token(pc, CT_COMMENT)
              || chunk_is_token(pc, CT_COMMENT_EMBED))
      {
      }
      else // if (chunk_is_token(pc, CT_JUNK)) || else
      {
         for (int spacing = pc->column - column; spacing > 0; spacing--)
         {
            dst += ' ';
            column++;
         }

         dst.append(pc->str);
         column += pc->Len();
      }
   }
} // generate_if_conditional_as_text


void add_long_preprocessor_conditional_block_comment(void)
{
   Chunk *pp_start = nullptr;
   Chunk *pp_end   = nullptr;

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNextNcNnl())
   {
      // just track the preproc level:
      if (chunk_is_token(pc, CT_PREPROC))
      {
         pp_end = pp_start = pc;
      }

      if (  pc->type != CT_PP_IF
         || !pp_start)
      {
         continue;
      }
#if 0
      if (pc->flags.test(PCF_IN_PREPROC))
      {
         continue;
      }
#endif

      Chunk  *br_close;
      Chunk  *br_open = pc;
      size_t nl_count = 0;

      Chunk  *tmp = pc;

      while ((tmp = tmp->GetNext())->IsNotNullChunk())
      {
         // just track the preproc level:
         if (chunk_is_token(tmp, CT_PREPROC))
         {
            pp_end = tmp;
         }

         if (chunk_is_newline(tmp))
         {
            nl_count += tmp->nl_count;
         }
         else if (  pp_end->pp_level == pp_start->pp_level
                 && (  chunk_is_token(tmp, CT_PP_ENDIF)
                    || ((chunk_is_token(br_open, CT_PP_IF)) ? (chunk_is_token(tmp, CT_PP_ELSE)) : 0)))
         {
            br_close = tmp;

            LOG_FMT(LPPIF, "found #if / %s section on lines %zu and %zu, nl_count=%zu\n",
                    (chunk_is_token(tmp, CT_PP_ENDIF) ? "#endif" : "#else"),
                    br_open->orig_line, br_close->orig_line, nl_count);

            // Found the matching #else or #endif - make sure a newline is next
            tmp = tmp->GetNext();

            LOG_FMT(LPPIF, "next item type %d (is %s)\n",
                    (tmp ? tmp->type : -1), (tmp ? chunk_is_newline(tmp) ? "newline"
                                             : tmp->IsComment() ? "comment" : "other" : "---"));

            if (  tmp->IsNullChunk()
               || chunk_is_token(tmp, CT_NEWLINE)) // chunk_is_newline(tmp))
            {
               size_t nl_min;

               if (chunk_is_token(br_close, CT_PP_ENDIF))
               {
                  log_rule_B("mod_add_long_ifdef_endif_comment");
                  nl_min = options::mod_add_long_ifdef_endif_comment();
               }
               else
               {
                  log_rule_B("mod_add_long_ifdef_else_comment");
                  nl_min = options::mod_add_long_ifdef_else_comment();
               }
               const char *txt = !tmp ? "EOF" : ((chunk_is_token(tmp, CT_PP_ENDIF)) ? "#endif" : "#else");
               LOG_FMT(LPPIF, "#if / %s section candidate for augmenting when over NL threshold %zu != 0 (nl_count=%zu)\n",
                       txt, nl_min, nl_count);

               if (  nl_min > 0
                  && nl_count > nl_min) // nl_count is 1 too large at all times as #if line was counted too
               {
                  // determine the added comment style
                  E_Token style = (language_is_set(LANG_CPP)) ?
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
            if (chunk_is_token(br_close, CT_PP_ENDIF))
            {
               break;
            }
         }
      }
   }
} // add_long_preprocessor_conditional_block_comment
