/**
 * @file c_token.c
 * This file breaks up the text stream into tokens or chunks.
 *
 * Each routine needs to set pc->len and pc->type.
 *
 * $Id$
 */

#include "cparse_types.h"
#include "char_table.h"
#include "prototypes.h"
#include "chunk_list.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

/* some quick debug enablers */
//#define DEBUG_FRM_STACK
//#define DEBUG_TOKENIZE

#ifdef DEBUG_TOKENIZE
#define DT_PR(n)     n
#else
#define DT_PR(n)
#endif

static chunk_t *insert_vbrace(chunk_t *pc, BOOL after,
                              struct parse_frame *frm);
#define insert_vbrace_after(pc, frm)      insert_vbrace(pc, TRUE, frm)
#define insert_vbrace_before(pc, frm)     insert_vbrace(pc, FALSE, frm)

void parse_cleanup(struct parse_frame *frm,
                   chunk_t *pc, chunk_t *prev);

void close_statement(struct parse_frame *frm, chunk_t *pc);
void handle_close_stage(struct parse_frame *frm, chunk_t *pc);

/**
 * Figure of the length of the comment at text.
 * The next bit of text starts with a '/', so it might be a comment.
 * Count the characters in the comment, 0 means is wasn't comment.
 *
 * @param text    The text to look at (zero terminated)
 * @return        Whether a comment was parsed
 */
BOOL parse_comment(chunk_t *pc)
{
   int len = 2;

   if ((pc->str[0] != '/') || ((pc->str[1] != '*') && (pc->str[1] != '/')))
   {
      return(FALSE);
   }

   cpd.column += 2;
   if (pc->str[1] == '/')
   {
      pc->type = CT_COMMENT_CPP;
      while ((pc->str[len] != '\n') && (pc->str[len] != 0))
      {
         len++;
      }
   }
   else
   {
      cpd.column += 2;
      pc->type    = CT_COMMENT;
      if ((pc->str[2] == '\n') || (pc->str[3] == '\n'))
      {
         pc->type   = CT_COMMENT_MULTI;
         cpd.column = 1;
         cpd.line_number++;
      }
      len = 4;
      while ((pc->str[len] != 0) &&
             ((pc->str[len - 2] != '*') || (pc->str[len - 1] != '/')))
      {
         if (pc->str[len] == '\n')
         {
            pc->type = CT_COMMENT_MULTI;
            cpd.line_number++;
            cpd.column = 0;
         }
         len++;
         cpd.column++;
      }
   }
   pc->len = len;
   return(TRUE);
}


/**
 * Count the number of characters in the number.
 * The next bit of text starts with a number (0-9), so it is a number.
 * Count the number of characters in the number.
 *
 * @param text    The text to look at (zero terminated)
 * @return        The number of bytes in the comment
 */
BOOL parse_number(chunk_t *pc)
{
   int len = 0;

   if (!isdigit(*pc->str))
   {
      return(FALSE);
   }

   /* Check for Hex, Octal, or Binary */
   if ((pc->str[0] == '0') && (pc->str[1] != '.'))
   {
      switch (toupper(pc->str[1]))
      {
      case 'X':               /* hex */
         len = 2;
         while (isxdigit(pc->str[len]))
         {
            len++;
         }
         break;

      case 'B':               /* binary? */
         len = 2;
         while ((pc->str[len] == '0') || (pc->str[len] == '1'))
         {
            len++;
         }
         break;

      default:                /* octal */
         len = 1;
         while ((pc->str[len] >= '0') && (pc->str[len] <= '7'))
         {
            len++;
         }
         break;
      }
   }
   else
   {
      int dotcount = 0;
      len          = 1;
      while (isdigit(pc->str[len]) || ((pc->str[len] == '.') && (dotcount == 0)))
      {
         len++;
         if (pc->str[len] == '.')
         {
            dotcount++;
         }
      }
   }

   /* do a suffix check */
   if (toupper(pc->str[len]) == 'E')
   {
      len++;
      if ((pc->str[len] == '-') || (pc->str[len] == '+'))
      {
         len++;
      }
      while (isdigit(pc->str[len]))
      {
         len++;
      }
   }
   if (toupper(pc->str[len]) == 'U')
   {
      len++;
   }
   if (toupper(pc->str[len]) == 'L')
   {
      len++;
   }
   if (toupper(pc->str[len]) == 'L')
   {
      len++;
   }

   pc->len     = len;
   pc->type    = CT_NUMBER;
   cpd.column += len;
   return(TRUE);
}


/**
 * Count the number of characters in a quoted string.
 * The next bit of text starts with a quote char " or ' or <.
 * Count the number of characters until the matching character.
 *
 * @param text    The text to look at (zero terminated)
 * @return        The number of bytes in the string
 */
BOOL parse_string(chunk_t *pc)
{
   int escaped = 0;
   int end_ch;
   int len = 1;

   end_ch = get_char_table(*pc->str) & 0xff;

   for (len = 1; pc->str[len] != 0; len++)
   {
      if (!escaped)
      {
         if (pc->str[len] == '\\')
         {
            escaped = 1;
         }
         else if (pc->str[len] == end_ch)
         {
            len++;
            break;
         }
         /* TODO: detect a newline in the string -- for an error condition? */
      }
      else
      {
         escaped = 0;
      }
   }
   pc->len     = len;
   pc->type    = CT_STRING;
   cpd.column += len;
   return(TRUE);
}


/**
 * Literal string, ends with single "
 * Two "" don't end the string.
 */
static BOOL parse_cs_string(chunk_t *pc)
{
   int len = 2;

   /* go until we hit a zero (end of file) or a single " */
   while (pc->str[len] != 0)
   {
      len++;
      if ((pc->str[len - 1] == '"') && (pc->str[len] != '"'))
      {
         break;
      }
   }

   pc->len     = len;
   pc->type    = CT_STRING;
   cpd.column += len;
   return(TRUE);
}


/**
 * Count the number of characters in a word.
 * The first character is already valid for a keyword
 *
 * @param text    The text to look at (zero terminated)
 * @return        The number of bytes in the word
 */
BOOL parse_word(chunk_t *pc, BOOL skipcheck)
{
   int               len = 1;
   const chunk_tag_t *tag;

   while ((pc->str[len] < 127) &&
          ((get_char_table(pc->str[len]) & CT_KW2) != 0))
   {
      len++;
   }
   cpd.column += len;
   pc->len     = len;
   pc->type    = CT_WORD;

   if (skipcheck)
   {
      return(TRUE);
   }

   /* Detect pre-processor functions now */
   if ((cpd.in_preproc == PP_DEFINE) &&
       (cpd.preproc_ncnl_count == 1))
   {
      if (pc->str[len] == '(')
      {
         pc->type = CT_MACRO_FUNC;
      }
      else
      {
         pc->type = CT_MACRO;
      }
   }

   /* Turn it into a keyword now */
   tag = find_keyword(pc->str, len);
   if (tag != NULL)
   {
      if (((tag->lang_flags & cpd.lang_flags) & LANG_ALL) != 0)
      {
         if (((tag->lang_flags & FLAG_PP) == 0) || (cpd.in_preproc != 0))
         {
            pc->type = tag->type;
         }
      }
   }
   return(TRUE);
}


/**
 * Count the number of whitespace characters.
 *
 * @param text    The text to look at (zero terminated)
 * @return        The number of chars
 */
BOOL parse_whitespace(chunk_t *pc)
{
   int len      = 0;
   int nl_count = 0;

   while ((pc->str[len] != 0) &&
          ((pc->str[len] <= ' ') || (pc->str[len] >= 127)))
   {
      switch (pc->str[len])
      {
      case '\n':
         nl_count++;
         cpd.column = 1;
         cpd.line_number++;
         break;

      case '\t':
         cpd.column = calc_next_tab_column(cpd.column,
                                           cpd.settings[UO_input_tab_size]);
         break;

      case ' ':
         cpd.column++;
         break;

      default:
         break;
      }
      len++;
   }

   if (len > 0)
   {
      pc->nl_count = nl_count;
      pc->type     = nl_count ? CT_NEWLINE : CT_WHITESPACE;
      pc->len      = len;
      pc->str      = "";
   }
   return(len != 0);
}


/**
 * Skips the next bit of whatever and returns the type of block.
 *
 * pc->str is the input text.
 * pc->len in the output length.
 * pc->type is the output type
 * pc->column is output column
 *
 * @param pc      The structure to update, str is an input.
 * @return        TRUE/FALSE - whether anything was parsed
 */
BOOL parse_next(chunk_t *pc)
{
   const chunk_tag_t *punc;

   if ((pc == NULL) || (pc->str == NULL) || (*pc->str == 0))
   {
      //fprintf(stderr, "All done!\n");
      return(FALSE);
   }

   /* Save off the current column */
   pc->orig_line = cpd.line_number;
   pc->column    = cpd.column;
   pc->orig_col  = cpd.column;
   pc->len       = 0;
   pc->type      = CT_NONE;
   pc->nl_count  = 0;

   /* Check for whitespace first */
   if (parse_whitespace(pc))
   {
      return(TRUE);
   }

   if (cpd.in_preproc == PP_OTHER)
   {
      /* Chunk to a newline or comment */
      pc->type = CT_PREPROC_BODY;
      while ((pc->str[pc->len] != 0) &&
             (pc->str[pc->len] != '\n') &&
             !((pc->str[pc->len] == '/') && ((pc->str[pc->len + 1] == '/') ||
                                             (pc->str[pc->len + 1] == '*'))))
      {
         pc->len++;
      }
      if (pc->len > 0)
      {
         return(TRUE);
      }
   }

   /* Detect backslash-newline */
   if ((pc->str[0] == '\\') && (pc->str[1] == '\n'))
   {
      pc->type     = CT_NL_CONT;
      pc->len      = 2;
      pc->nl_count = 1;
      cpd.column   = 1;
      cpd.line_number++;
      return(TRUE);
   }

   /* Check for C# literal strings, ie @"hello" */
   if (((cpd.lang_flags & LANG_CS) != 0) && (*pc->str == '@'))
   {
      if (pc->str[1] == '"')
      {
         parse_cs_string(pc);
         return(TRUE);
      }
      if (((get_char_table(pc->str[1]) & CT_KW1) != 0) &&
          parse_word(pc, TRUE))
      {
         return(TRUE);
      }
   }

   /* Check for L'a', L"abc", 'a', "abc", <abc> strings */
   if (((*pc->str == 'L') && ((pc->str[1] == '"') || (pc->str[1] == '\''))) ||
       (*pc->str == '"') ||
       (*pc->str == '\'') ||
       ((*pc->str == '<') && (cpd.in_preproc == PP_INCLUDE)))
   {
      parse_string(pc);
      return(TRUE);
   }

   if (((get_char_table(*pc->str) & CT_KW1) != 0) &&
       parse_word(pc, FALSE))
   {
      return(TRUE);
   }

   if (parse_comment(pc))
   {
      return(TRUE);
   }

   if ((punc = find_punctuator(pc->str, cpd.lang_flags)) != NULL)
   {
      pc->type    = punc->type;
      pc->len     = strlen(punc->tag);
      cpd.column += pc->len;
      return(TRUE);
   }

   if (parse_number(pc))
   {
      return(TRUE);
   }

   /* throw away this character */
   pc->type = CT_UNKNOWN;
   pc->len  = 1;

   LOG_FMT(LWARN, "Garbage: %x on line %d\n", *pc->str, pc->orig_line);

   return(TRUE);
}


/**
 * This function parses or tokenizes the whole buffer into a list.
 * It has to do some tricks to parse preprocessors.
 *
 * If output_text() were called immediately after, two things would happen:
 *  - trailing whitespace are removed.
 *  - leading space & tabs are converted to the appropriate format.
 */
void parse_buffer(const char *data, int data_len)
{
   int                idx = 0;
   chunk_t            chunk;
   chunk_t            *pc    = NULL;
   chunk_t            *rprev = NULL;
   chunk_t            *prev  = NULL;
   struct parse_frame frm;

   memset(&frm, 0, sizeof(frm));
   memset(&chunk, 0, sizeof(chunk));

   cpd.line_number = 1;
   cpd.column      = 1;

   while (idx < data_len)
   {
      chunk.str = &data[idx];
      if (!parse_next(&chunk))
      {
         LOG_FMT(LERR, "Bailed before the end?\n");
         break;
      }

      /* Bump up the index */
      idx += chunk.len;

      /* Don't create an entry for whitespace */
      if (chunk.type == CT_WHITESPACE)
      {
         continue;
      }

      /* Strip trailing whitespace (for CPP comments and PP blocks) */
      while ((chunk.len > 0) &&
             ((chunk.str[chunk.len - 1] == ' ') ||
              (chunk.str[chunk.len - 1] == '\t')))
      {
         chunk.len--;
      }

      /* Store off the end column */
      chunk.orig_col_end = cpd.column;

      /* Add the chunk to the list */
      rprev = pc;
      if (!chunk_is_newline(pc) && !chunk_is_comment(pc))
      {
         prev = pc;
      }
      else
      {
         if (prev != NULL)
         {
            pc->flags = prev->flags & PCF_COPY_FLAGS;

            /* a newline can't be in a preprocessor */
            if (pc->type == CT_NEWLINE)
            {
               pc->flags &= ~PCF_IN_PREPROC;
            }
         }
      }
      pc = chunk_add(&chunk);

      /* A newline marks the end of a preprocessor */
      if ((pc->type == CT_NEWLINE) || (pc->type == CT_COMMENT_MULTI))
      {
         if (cpd.in_preproc == PP_DEFINE_BODY)
         {
            /* out of the #define body, restore the frame */
            pf_pop(&frm);
         }

         cpd.in_preproc         = PP_NONE;
         cpd.preproc_ncnl_count = 0;
      }

      /* Assume the level won't change */
      pc->level       = frm.level;
      pc->brace_level = frm.brace_level;

      /* Special handling for preprocessor stuff */
      if (cpd.in_preproc)
      {
         pc->flags |= PCF_IN_PREPROC;


         /* Count words after the preprocessor */
         if (!chunk_is_comment(pc) && !chunk_is_newline(pc))
         {
            cpd.preproc_ncnl_count++;
            if ((cpd.in_preproc == PP_DEFINE) && (cpd.preproc_ncnl_count > 1))
            {
               /* a preproc body starts a new, blank frame */
               cpd.in_preproc = PP_DEFINE_BODY;
               pf_push(&frm);
               memset(&frm, 0, sizeof(frm));
               frm.level++;
               frm.brace_level++;
               frm.pse_tos++;
               frm.pse[frm.pse_tos].type  = CT_PP_DEFINE;
               frm.pse[frm.pse_tos].stage = BS_NONE;
            }

            if (cpd.in_preproc == PP_DEFINE_BODY)
            {
               parse_cleanup(&frm, pc, prev);
            }
         }

         /* Figure out the type of preprocessor for #include parsing */
         if (cpd.in_preproc == PP_UNKNOWN)
         {
            if (strcmp(pc->str, "include") == 0)
            {
               cpd.in_preproc = PP_INCLUDE;
               pc->type       = CT_PP_INCLUDE;
            }
            else if (strcmp(pc->str, "define") == 0)
            {
               cpd.in_preproc = PP_DEFINE;
               pc->type       = CT_PP_DEFINE;
            }
            else if (strncmp(pc->str, "if", 2) == 0)
            {
               cpd.in_preproc = PP_IF;
               pc->type       = CT_PP_IF;
            }
            else if (strncmp(pc->str, "el", 2) == 0)
            {
               cpd.in_preproc = PP_ELSE;
               pc->type       = CT_PP_ELSE;
            }
            else if (strncmp(pc->str, "en", 2) == 0)
            {
               cpd.in_preproc = PP_ENDIF;
               pc->type       = CT_PP_ENDIF;
            }
            else
            {
               cpd.in_preproc = PP_OTHER;
               pc->type       = CT_PP_OTHER;
            }
         }

         pf_check(&frm, pc);
      }
      else
      {
         if ((pc->type == CT_POUND) &&
             ((rprev == NULL) || (rprev->type == CT_NEWLINE)))
         {
            pc->type       = CT_PREPROC;
            pc->flags     |= PCF_IN_PREPROC;
            cpd.in_preproc = PP_UNKNOWN;

            //fprintf(stderr, "%s: %d] %s - tos %s\n", __func__,
            //        pc->orig_line, get_token_name(pc->type),
            //        get_token_name(frm.pse[frm.pse_tos].type));

            if ((frm.pse[frm.pse_tos].type == CT_VBRACE_OPEN) ||
                (frm.pse[frm.pse_tos].type == CT_IF) ||
                (frm.pse[frm.pse_tos].type == CT_FOR) ||
                (frm.pse[frm.pse_tos].type == CT_SWITCH) ||
                (frm.pse[frm.pse_tos].type == CT_DO) ||
                (frm.pse[frm.pse_tos].type == CT_WHILE))
            {
               //fprintf(stderr, "%s: closing on token %s\n",
               //        __func__, get_token_name(pc->type));
               close_statement(&frm, prev);
            }
         }

         if ((cpd.in_preproc == PP_NONE) ||
             ((cpd.in_preproc == PP_DEFINE) && (cpd.preproc_ncnl_count > 1)))
         {
            if (!chunk_is_newline(pc) && !chunk_is_comment(pc))
            {
               parse_cleanup(&frm, pc, prev);
            }
         }
      }
   }
}

static void print_stack(struct parse_frame *frm, chunk_t *pc)
{
   if (log_sev_on(LFRMSTK))
   {
      int idx;

      log_fmt(LFRMSTK, "%2d> %2d", pc->orig_line, frm->pse_tos);

      for (idx = 1; idx <= frm->pse_tos; idx++)
      {
         log_fmt(LFRMSTK, " [%s/%d]",
                 get_token_name(frm->pse[idx].type), frm->pse[idx].stage);
      }
      log_fmt(LFRMSTK, "\n");
   }
}

void parse_cleanup(struct parse_frame *frm,
                   chunk_t *pc, chunk_t *prev)
{
   c_token_t parent = CT_NONE;

   prev = chunk_get_prev_ncnl(pc);

   /* Mark statement starts */
   if (((frm->stmt_count == 0) || (frm->expr_count == 0)) &&
       (pc->type != CT_SEMICOLON) &&
       (pc->type != CT_BRACE_CLOSE))
   {
      pc->flags |= PCF_EXPR_START;
      pc->flags |= (frm->stmt_count == 0) ? PCF_STMT_START : 0;
      LOG_FMT(LPCU, "%d] 1.marked %s as stmt start st:%d ex:%d\n",
              pc->orig_line, pc->str, frm->stmt_count, frm->expr_count);
   }
   frm->stmt_count++;
   frm->expr_count++;

   if (frm->sparen_count > 0)
   {
      pc->flags |= PCF_IN_SPAREN;
   }

   LOG_FMT(LTOK, "%s:%d] %16s - tos:%d/%16s stg:%d\n",
           __func__, pc->orig_line, get_token_name(pc->type),
           frm->pse_tos, get_token_name(frm->pse[frm->pse_tos].type),
           frm->pse[frm->pse_tos].stage);

   /* Check for an else after after the close of an if */
   while (frm->pse[frm->pse_tos].stage == BS_ELSE)
   {
      //fprintf(stderr, "Checking for else\n");
      if (pc->type == CT_ELSE)
      {
         //fprintf(stderr, "%2d> found else for if\n", pc->orig_line);
         frm->pse[frm->pse_tos].type  = CT_ELSE;
         frm->pse[frm->pse_tos].stage = BS_ELSEIF;
         print_stack(frm, pc);
         return;
      }
      else
      {
         //fprintf(stderr, "%2d> did not find else for if\n", pc->orig_line);
         /* the previous chunk ended the statment */
         close_statement(frm, prev);
      }
   }

   /* change CT_ELSE to CT_IF when we hit an "else if" */
   if ((frm->pse[frm->pse_tos].type == CT_ELSE) &&
       (frm->pse[frm->pse_tos].stage == BS_ELSEIF))
   {
      if (pc->type == CT_IF)
      {
         //fprintf(stderr, " -- Changed else to if --\n");
         frm->pse[frm->pse_tos].type  = CT_IF;
         frm->pse[frm->pse_tos].stage = BS_PAREN1;
         return;
      }
      frm->pse[frm->pse_tos].stage = BS_BRACE2;
   }

   if (frm->pse[frm->pse_tos].stage == BS_WHILE)
   {
      if (pc->type == CT_WHILE)
      {
         pc->type                     = CT_WHILE_OF_DO;
         frm->pse[frm->pse_tos].stage = BS_PAREN2;
         return;
      }
      else
      {
         LOG_FMT(LWARN, "%s: Error: Expected 'while', got '%s'\n",
                 __func__, pc->str);
         frm->pse_tos--;
      }
   }

   /* Insert an opening virtual brace */
   if (((frm->pse[frm->pse_tos].stage == BS_BRACE_DO) ||
        (frm->pse[frm->pse_tos].stage == BS_BRACE2)) &&
       (pc->type != CT_BRACE_OPEN))
   {
      parent = frm->pse[frm->pse_tos].type;
      insert_vbrace_before(pc, frm);
      frm->level++;
      frm->brace_level++;
      frm->pse_tos++;
      frm->pse[frm->pse_tos].type   = CT_VBRACE_OPEN;
      frm->pse[frm->pse_tos].stage  = BS_NONE;
      frm->pse[frm->pse_tos].parent = parent;

      print_stack(frm, pc);

      /* update the level of pc */
      pc->level       = frm->level;
      pc->brace_level = frm->brace_level;

      /* Mark as a start of a statment */
      frm->stmt_count = 0;
      frm->expr_count = 0;
      pc->flags      |= PCF_STMT_START | PCF_EXPR_START;
      frm->stmt_count = 1;
      frm->expr_count = 1;
      //fprintf(stderr, "%d] 2.marked %s as stmt start\n", pc->orig_line, pc->str);
   }

   /* Handle an end-of-statement */
   if (pc->type == CT_SEMICOLON)
   {
      close_statement(frm, pc);
   }

   if (prev != NULL)
   {
      /* Detect simple cases of CT_STAR -> CT_PTR_TYPE
       * Change "TYPE *", "QUAL *" and "TYPE **" into
       */
      if ((pc->type == CT_STAR) &&
          ((prev->type == CT_TYPE) ||
           (prev->type == CT_QUALIFIER) ||
           (prev->type == CT_PTR_TYPE)))
      {
         pc->type = CT_PTR_TYPE;
      }

      /* Set the parent of a brace when preceeded by a '=' */
      if ((prev->type == CT_ASSIGN) && (prev->str[0] == '=') &&
          (pc->type == CT_BRACE_OPEN))
      {
         parent = CT_ASSIGN;
      }

      /* Set parent type for parens and change paren type */
      if (pc->type == CT_PAREN_OPEN)
      {
         if (prev->type == CT_WORD)
         {
            prev->type = CT_FUNCTION;
            pc->type   = CT_FPAREN_OPEN;
            parent     = CT_FUNCTION;
         }
         else if ((prev->type == CT_IF) ||
                  (prev->type == CT_FOR) ||
                  (prev->type == CT_WHILE) ||
                  (prev->type == CT_WHILE_OF_DO) ||
                  (prev->type == CT_SWITCH))
         {
            pc->type = CT_SPAREN_OPEN;
            parent   = prev->type;
            frm->sparen_count++;
         }
         else
         {
            /* no need to change the parens */
         }
      }

      /* Set the parent for open braces */
      if (pc->type == CT_BRACE_OPEN)
      {
         if (prev->type == CT_FPAREN_CLOSE)
         {
            parent = CT_FUNCTION;
         }
         else if (prev->type == CT_SPAREN_CLOSE)
         {
            parent = prev->parent_type;
         }
         else if (prev->type == CT_ELSE)
         {
            parent = CT_ELSE;
         }
      }

      /* Change a WORD after ENUM/UNION/STRUCT to TYPE
       * Also change the first word in 'WORD WORD' to a type.
       */
      if (pc->type == CT_WORD)
      {
         if ((prev->type == CT_ENUM) ||
             (prev->type == CT_UNION) ||
             (prev->type == CT_STRUCT))
         {
            pc->type = CT_TYPE;
         }
         if (prev->type == CT_WORD)
         {
            prev->type = CT_TYPE;
         }
      }

      /* restart the current IF sequence if we hit an "else if" */
      if ((pc->type == CT_IF) && (prev->type == CT_ELSE))
      {
         frm->pse[frm->pse_tos].type  = CT_IF;
         frm->pse[frm->pse_tos].stage = 0;
      }
   }

   /* If we close a paren, change the type to match the open */
   if (pc->type == CT_PAREN_CLOSE)
   {
      if ((frm->pse[frm->pse_tos].type == CT_PAREN_OPEN) ||
          (frm->pse[frm->pse_tos].type == CT_FPAREN_OPEN) ||
          (frm->pse[frm->pse_tos].type == CT_SPAREN_OPEN))
      {
         pc->type = frm->pse[frm->pse_tos].type + 1;
         if (pc->type == CT_SPAREN_CLOSE)
         {
            frm->sparen_count--;
            pc->flags &= ~PCF_IN_SPAREN;
         }
      }
   }

   /* For closing braces/parens/squares, set the parent and handle the close.
    * Adjust the level.
    */
   if ((pc->type == CT_PAREN_CLOSE) ||
       (pc->type == CT_FPAREN_CLOSE) ||
       (pc->type == CT_SPAREN_CLOSE) ||
       (pc->type == CT_SQUARE_CLOSE) ||
       (pc->type == CT_BRACE_CLOSE))
   {
      if (pc->type == (frm->pse[frm->pse_tos].type + 1))
      {
         //fprintf(stderr, "%2d> closed on %s\n", pc->orig_line, get_token_name(pc->type));

         pc->parent_type = frm->pse[frm->pse_tos].parent;
         frm->level--;
         frm->pse_tos--;
         if (pc->type == CT_BRACE_CLOSE)
         {
            frm->brace_level--;
         }

         /* Update the close paren/brace level */
         pc->level       = frm->level;
         pc->brace_level = frm->brace_level;

         print_stack(frm, pc);

         handle_close_stage(frm, pc);
      }
      else
      {
         LOG_FMT(LWARN, "%s: Error: Unexpected '%s' on line %d- %s\n",
                 __func__, pc->str, pc->orig_line,
                 get_token_name(frm->pse[frm->pse_tos].type));
      }
   }

   /* Adjust the level for opens & create a stack entry */
   if ((pc->type == CT_BRACE_OPEN) ||
       (pc->type == CT_PAREN_OPEN) ||
       (pc->type == CT_FPAREN_OPEN) ||
       (pc->type == CT_SPAREN_OPEN) ||
       (pc->type == CT_SQUARE_OPEN))
   {
      frm->level++;
      if (pc->type == CT_BRACE_OPEN)
      {
         frm->brace_level++;
      }
      frm->pse_tos++;
      frm->pse[frm->pse_tos].type   = pc->type;
      frm->pse[frm->pse_tos].stage  = 0;
      frm->pse[frm->pse_tos].parent = parent;
      pc->parent_type               = parent;

      print_stack(frm, pc);
   }

   /* Create a stack entry for complex statments IF/DO/FOR/WHILE/SWITCH */
   if ((pc->type == CT_IF) ||
       (pc->type == CT_DO) ||
       (pc->type == CT_FOR) ||
       (pc->type == CT_WHILE) ||
       (pc->type == CT_SWITCH))
   {
      frm->pse_tos++;
      frm->pse[frm->pse_tos].type  = pc->type;
      frm->pse[frm->pse_tos].stage = (pc->type == CT_DO) ? BS_BRACE_DO : BS_PAREN1;
      //fprintf(stderr, "opening %s\n", pc->str);

      print_stack(frm, pc);
   }

   /* Mark simple statement/expression starts
    *  - after { or }
    *  - after ';', but not if the paren stack top is a paren
    *  - after '(' that has a parent type of CT_FOR
    */
   if (((pc->type == CT_BRACE_OPEN) && (pc->parent_type != CT_ASSIGN)) ||
       (pc->type == CT_BRACE_CLOSE) ||
       ((pc->type == CT_SPAREN_OPEN) && (pc->parent_type == CT_FOR)) ||
       ((pc->type == CT_SEMICOLON) &&
        (frm->pse[frm->pse_tos].type != CT_PAREN_OPEN) &&
        (frm->pse[frm->pse_tos].type != CT_FPAREN_OPEN) &&
        (frm->pse[frm->pse_tos].type != CT_SPAREN_OPEN)))
   {
      //fprintf(stderr, "%s: %d> reset stmt on %s\n", __func__, pc->orig_line, pc->str);
      frm->stmt_count = 0;
      frm->expr_count = 0;
   }

   /* Mark expression starts */
   if ((pc->type == CT_ARITH) ||
       (pc->type == CT_ASSIGN) ||
       (pc->type == CT_COMPARE) ||
       (pc->type == CT_RETURN) ||
       (pc->type == CT_GOTO) ||
       (pc->type == CT_CONTINUE) ||
       (pc->type == CT_PAREN_OPEN) ||
       (pc->type == CT_FPAREN_OPEN) ||
       (pc->type == CT_SPAREN_OPEN) ||
       (pc->type == CT_BRACE_OPEN) ||
       (pc->type == CT_SEMICOLON) ||
       (pc->type == CT_COMMA) ||
       (pc->type == CT_COLON) ||
       (pc->type == CT_QUESTION))
   {
      frm->expr_count = 0;
      //fprintf(stderr, "%s: %d> reset expr on %s\n", __func__, pc->orig_line, pc->str);
   }
}


static chunk_t *insert_vbrace(chunk_t *pc, BOOL after,
                              struct parse_frame *frm)
{
   chunk_t chunk;
   chunk_t *rv;
   chunk_t *ref;

   memset(&chunk, 0, sizeof(chunk));

   //   fprintf(stderr, "%s_%s: %s on line %d\n",
   //           __func__, after ? "after" : "before",
   //           get_token_name(pc->type), pc->orig_line);

   chunk.len         = 0;
   chunk.orig_line   = pc->orig_line;
   chunk.parent_type = frm->pse[frm->pse_tos].type;
   chunk.level       = frm->level;
   chunk.brace_level = frm->brace_level;
   chunk.flags       = pc->flags & PCF_COPY_FLAGS;
   if (after)
   {
      chunk.type = CT_VBRACE_CLOSE;
      rv         = chunk_add_after(&chunk, pc);
   }
   else
   {
      ref = chunk_get_prev(pc);
      while (chunk_is_newline(ref) || chunk_is_comment(ref))
      {
         ref = chunk_get_prev(ref);
      }
      chunk.type = CT_VBRACE_OPEN;
      rv         = chunk_add_after(&chunk, ref);
   }
   return(rv);
}

/*TODO */

/**
 * Called on the last chunk in a statement.
 *
 * This should be called on:
 *  - semicolons
 *  - CT_BRACE_CLOSE '}'
 *  - CT_VBRACE_CLOSE
 *
 * The action taken depends on top item on the stack.
 *  - if (tos.type == CT_IF) and (tos.stage == )
 */
void close_statement(struct parse_frame *frm, chunk_t *pc)
{
   chunk_t *vbc = pc;

   LOG_FMT(LTOK, "%s:%d] %s'%s' type %s stage %d\n", __func__,
           pc->orig_line,
           get_token_name(pc->type), pc->str,
           get_token_name(frm->pse[frm->pse_tos].type),
           frm->pse[frm->pse_tos].stage);

   if (pc->type != CT_VBRACE_CLOSE)
   {
      frm->expr_count = 1;
      if (frm->pse[frm->pse_tos].type != CT_SPAREN_OPEN)
      {
         frm->stmt_count = 1;
      }
   }

   /* See if we are done with a complex statement */
   if ((frm->pse[frm->pse_tos].stage == BS_PAREN2) ||
       (frm->pse[frm->pse_tos].stage == BS_BRACE2) ||
       (frm->pse[frm->pse_tos].stage == BS_ELSE))
   {
      frm->pse_tos--;

      //fprintf(stderr, "%2d> CS1: closed on %s\n", pc->orig_line, get_token_name(pc->type));

      print_stack(frm, pc);

      handle_close_stage(frm, vbc);
   }

   /* If we are in a virtual brace -- close it */
   if (frm->pse[frm->pse_tos].type == CT_VBRACE_OPEN)
   {
      frm->level--;
      frm->brace_level--;
      frm->pse_tos--;

      //fprintf(stderr, "%2d> CS1: closed on %s\n", pc->orig_line, get_token_name(pc->type));

      print_stack(frm, pc);

      vbc             = insert_vbrace_after(pc, frm);
      frm->stmt_count = 1;
      frm->expr_count = 1;
      handle_close_stage(frm, vbc);
   }
}

void handle_close_stage(struct parse_frame *frm, chunk_t *pc)
{
   LOG_FMT(LTOK, "%s-top: line %d pse_tos=%12s stage=%d pc=%s\n",
           __func__, pc->orig_line,
           get_token_name(frm->pse[frm->pse_tos].type),
           frm->pse[frm->pse_tos].stage,
           get_token_name(pc->type));

   /* see if we just closed a do/if/else/for/switch/while section */
   switch (frm->pse[frm->pse_tos].stage)
   {
   case BS_PAREN1:   /* if/for/switch/while () ended */
      frm->pse[frm->pse_tos].stage = BS_BRACE2;
      break;

   case BS_PAREN2:   /* do/while () ended */
      close_statement(frm, pc);
      break;

   case BS_BRACE_DO:   /* do {} ended */
      frm->pse[frm->pse_tos].stage = BS_WHILE;
      break;

   case BS_BRACE2:   /* if/else/for/while/switch {} ended */
      if (frm->pse[frm->pse_tos].type == CT_IF)
      {
         frm->pse[frm->pse_tos].stage = BS_ELSE;
      }
      else
      {
         close_statement(frm, pc);
      }
      break;

   case BS_ELSE:     /* else {} ended */
   case BS_WHILE:    /* do/while () ended */
      LOG_FMT(LWARN, "Unexpect stage %d on line %d\n",
              frm->pse[frm->pse_tos].stage, pc->orig_line);
      break;

   case BS_NONE:
   default:
      /* nothing to do */
      break;
   }

   //   fprintf(stderr, "%s-end: line %d pse_tos=%12s stage=%d\n",
   //        __func__,  pc->orig_line,
   //        get_token_name(frm->pse[frm->pse_tos].type),
   //        frm->pse[frm->pse_tos].stage);
}

