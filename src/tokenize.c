/**
 * @file tokenize.c
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


/**
 * Figure of the length of the comment at text.
 * The next bit of text starts with a '/', so it might be a comment.
 * There are three types of comments:
 *  - C comments that start with  '/ *' and end with '* /'
 *  - C++ comments that start with //
 *  - D nestable comments '/+' '+/'
 *
 * @param pc   The structure to update, str is an input.
 * @return     Whether a comment was parsed
 */
BOOL parse_comment(chunk_t *pc)
{
   int  len     = 2;
   BOOL is_d    = (cpd.lang_flags & LANG_D) != 0;
   int  d_level = 0;

   if ((pc->str[0] != '/') ||
       ((pc->str[1] != '*') && (pc->str[1] != '/') &&
        ((pc->str[1] != '+') || !is_d)))
   {
      return(FALSE);
   }

   /* account for opening two chars */
   cpd.column += 2;

   if (pc->str[1] == '/')
   {
      pc->type = CT_COMMENT_CPP;
      while ((pc->str[len] != '\n') && (pc->str[len] != 0))
      {
         len++;
      }
   }
   else if (pc->str[len] == 0)
   {
      /* unexpected end of file */
      return(FALSE);
   }
   else if (pc->str[1] == '+')
   {
      pc->type = CT_COMMENT;
      d_level++;
      while ((d_level > 0) && (pc->str[len + 1] != 0))
      {
         if ((pc->str[len] == '+') && (pc->str[len + 1] == '/'))
         {
            len        += 2;
            cpd.column += 2;
            d_level--;
            continue;
         }

         if ((pc->str[len] == '/') && (pc->str[len + 1] == '+'))
         {
            len        += 2;
            cpd.column += 2;
            d_level++;
            continue;
         }

         if (pc->str[len] == '\n')
         {
            pc->type   = CT_COMMENT_MULTI;
            cpd.column = 0;
            cpd.line_number++;
         }
         len++;
         cpd.column++;
      }
   }
   else
   {
      pc->type = CT_COMMENT;
      while (pc->str[len + 1] != 0)
      {
         if ((pc->str[len] == '*') && (pc->str[len + 1] == '/'))
         {
            len        += 2;
            cpd.column += 2;
            break;
         }

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
 * TODO: I don't think this covers ALL number formats...
 *
 * @param pc   The structure to update, str is an input.
 * @return     Whether a number was parsed
 */
BOOL parse_number(chunk_t *pc)
{
   int  len              = 0;
   BOOL allow_underscore = ((cpd.lang_flags & LANG_D) != 0);

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
         while (isxdigit(pc->str[len]) ||
                (allow_underscore && (pc->str[len] == '_')))
         {
            len++;
         }
         break;

      case 'B':               /* binary? */
         len = 2;
         while ((pc->str[len] == '0') || (pc->str[len] == '1') ||
                (allow_underscore && (pc->str[len] == '_')))
         {
            len++;
         }
         break;

      default:                /* octal */
         len = 1;
         while (((pc->str[len] >= '0') && (pc->str[len] <= '7')) ||
                (allow_underscore && (pc->str[len] == '_')))
         {
            len++;
         }
         break;
      }
   }
   else
   {
      int dotcount = 0;
      len = 1;
      while (isdigit(pc->str[len]) ||
             ((pc->str[len] == '.') && (dotcount == 0)) ||
             (allow_underscore && (pc->str[len] == '_')))
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
   if (toupper(pc->str[len]) == 'F')
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
 * @param pc   The structure to update, str is an input.
 * @return     Whether a string was parsed
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
 *
 * @param pc   The structure to update, str is an input.
 * @return     Whether a string was parsed
 */
static BOOL parse_cs_string(chunk_t *pc)
{
   int len = 2;

   /* go until we hit a zero (end of file) or a single " */
   while (pc->str[len] != 0)
   {
      if ((pc->str[len] == '"') && (pc->str[len + 1] == '"'))
      {
         len += 2;
      }
      else
      {
         len++;
         if (pc->str[len - 1] == '"')
         {
            break;
         }
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
 * @param pc   The structure to update, str is an input.
 * @return     Whether a word was parsed (always TRUE)
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
      pc->type = tag->type;
   }
   return(TRUE);
}


/**
 * Count the number of whitespace characters.
 *
 * @param pc   The structure to update, str is an input.
 * @return     Whether whitespace was parsed
 */
BOOL parse_whitespace(chunk_t *pc)
{
   int  len          = 0;
   int  nl_count     = 0;
   BOOL last_was_tab = FALSE;

   while ((pc->str[len] != 0) &&
          ((pc->str[len] <= ' ') || (pc->str[len] >= 127)))
   {
      last_was_tab = FALSE;
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
         last_was_tab = TRUE;
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
      pc->nl_count  = nl_count;
      pc->type      = nl_count ? CT_NEWLINE : CT_WHITESPACE;
      pc->len       = len;
      pc->str       = "";
      pc->after_tab = last_was_tab;
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
static BOOL parse_next(chunk_t *pc)
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

   LOG_FMT(LWARN, "Garbage: %x on line %d, col %d\n", *pc->str, pc->orig_line, cpd.column);

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
void tokenize(const char *data, int data_len)
{
   int                idx = 0;
   chunk_t            chunk;
   chunk_t            *pc    = NULL;
   chunk_t            *rprev = NULL;
   chunk_t            *prev  = NULL;
   struct parse_frame frm;
   BOOL               last_was_tab = FALSE;

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
         last_was_tab = chunk.after_tab;
         continue;
      }
      if (chunk.type == CT_NEWLINE)
      {
         last_was_tab    = chunk.after_tab;
         chunk.after_tab = FALSE;
      }
      else
      {
         chunk.after_tab = last_was_tab;
         last_was_tab    = FALSE;
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
         cpd.in_preproc         = PP_NONE;
         cpd.preproc_ncnl_count = 0;
      }

      /* Special handling for preprocessor stuff */
      if (cpd.in_preproc != PP_NONE)
      {
         pc->flags |= PCF_IN_PREPROC;

         /* Count words after the preprocessor */
         if (!chunk_is_comment(pc) && !chunk_is_newline(pc))
         {
            cpd.preproc_ncnl_count++;
         }

         /* Figure out the type of preprocessor for #include parsing */
         if (cpd.in_preproc == PP_UNKNOWN)
         {
            if (pc->type == CT_PP_INCLUDE)
            {
               cpd.in_preproc = PP_INCLUDE;
            }
            else if (pc->type == CT_PP_DEFINE)
            {
               cpd.in_preproc = PP_DEFINE;
            }
            else if (pc->type == CT_PP_IF)
            {
               cpd.in_preproc = PP_IF;
            }
            else if (pc->type == CT_PP_ELSE)
            {
               cpd.in_preproc = PP_ELSE;
            }
            else if (pc->type == CT_PP_ENDIF)
            {
               cpd.in_preproc = PP_ENDIF;
            }
            else
            {
               cpd.in_preproc = PP_OTHER;
               pc->type       = CT_PP_OTHER;
            }
         }
      }
      else
      {
         /* Check for a preprocessor start */
         if ((pc->type == CT_POUND) &&
             ((rprev == NULL) || (rprev->type == CT_NEWLINE)))
         {
            pc->type       = CT_PREPROC;
            pc->flags     |= PCF_IN_PREPROC;
            cpd.in_preproc = PP_UNKNOWN;
         }
      }
   }
}

