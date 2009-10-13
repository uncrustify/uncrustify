/**
 * @file tokenize.cpp
 * This file breaks up the text stream into tokens or chunks.
 *
 * Each routine needs to set pc->len and pc->type.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "uncrustify_types.h"
#include "char_table.h"
#include "prototypes.h"
#include "chunk_list.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "unc_ctype.h"


static bool parse_string(chunk_t *pc, int quote_idx, bool allow_escape);

#include "d.tokenize.cpp"

/**
 * A string-in-string search.  Like strstr() with a haystack length.
 */
static const char *str_search(const char *needle, const char *haystack, int haystack_len)
{
   int needle_len = strlen(needle);

   while (haystack_len-- >= needle_len)
   {
      if (memcmp(needle, haystack, needle_len) == 0)
      {
         return(haystack);
      }
      haystack++;
   }
   return(NULL);
}


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
static bool parse_comment(chunk_t *pc)
{
   int  len     = 2;
   bool is_d    = (cpd.lang_flags & LANG_D) != 0;
   int  d_level = 0;
   int  bs_cnt;

   if ((pc->str[0] != '/') ||
       ((pc->str[1] != '*') && (pc->str[1] != '/') &&
        ((pc->str[1] != '+') || !is_d)))
   {
      return(false);
   }

   /* account for opening two chars */
   cpd.column += 2;

   if (pc->str[1] == '/')
   {
      pc->type = CT_COMMENT_CPP;
      while (true)
      {
         bs_cnt = 0;
         while ((pc->str[len] != '\n') &&
                (pc->str[len] != '\r') &&
                (pc->str[len] != 0))
         {
            if (pc->str[len] == '\\')
            {
               bs_cnt++;
            }
            else
            {
               bs_cnt = 0;
            }
            len++;
         }

         /* If we hit an odd number of backslashes right before the newline,
          * then we keep going.
          */
         if (((bs_cnt & 1) == 0) || (pc->str[len] == 0))
         {
            break;
         }
         if (pc->str[len] == '\r')
         {
            len++;
         }
         if (pc->str[len] == '\n')
         {
            len++;
         }
      }
   }
   else if (pc->str[len] == 0)
   {
      /* unexpected end of file */
      return(false);
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

         if ((pc->str[len] == '\n') || (pc->str[len] == '\r'))
         {
            pc->type = CT_COMMENT_MULTI;
            pc->nl_count++;
            cpd.column = 0;
            cpd.line_number++;

            if (pc->str[len] == '\r')
            {
               if (pc->str[len + 1] == '\n')
               {
                  cpd.le_counts[LE_CRLF]++;
                  len++;
               }
               else
               {
                  cpd.le_counts[LE_CR]++;
               }
            }
            else
            {
               cpd.le_counts[LE_LF]++;
            }
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

            /* If there is another C comment right after this one, combine them */
            int tmp_len = len;
            while ((pc->str[tmp_len] == ' ') ||
                   (pc->str[tmp_len] == '\t'))
            {
               tmp_len++;
            }
            if ((pc->str[tmp_len] != '/') || (pc->str[tmp_len + 1] != '*'))
            {
               break;
            }
            pc->len = tmp_len;
         }

         if ((pc->str[len] == '\n') || (pc->str[len] == '\r'))
         {
            pc->type = CT_COMMENT_MULTI;
            pc->nl_count++;
            cpd.column = 0;
            cpd.line_number++;

            if (pc->str[len] == '\r')
            {
               if (pc->str[len + 1] == '\n')
               {
                  cpd.le_counts[LE_CRLF]++;
                  len++;
               }
               else
               {
                  cpd.le_counts[LE_CR]++;
               }
            }
            else
            {
               cpd.le_counts[LE_LF]++;
            }
         }
         len++;
         cpd.column++;
      }
   }
   pc->len = len;
   if (cpd.unc_off)
   {
      if (str_search(UNCRUSTIFY_ON_TEXT, pc->str, pc->len) != NULL)
      {
         LOG_FMT(LBCTRL, "Found '%s' on line %d\n", UNCRUSTIFY_ON_TEXT, pc->orig_line);
         cpd.unc_off = false;
      }
   }
   else
   {
      if (str_search(UNCRUSTIFY_OFF_TEXT, pc->str, pc->len) != NULL)
      {
         LOG_FMT(LBCTRL, "Found '%s' on line %d\n", UNCRUSTIFY_OFF_TEXT, pc->orig_line);
         cpd.unc_off = true;
      }
   }
   return(true);
}


/**
 * Count the number of characters in the number.
 * The next bit of text starts with a number (0-9 or '.'), so it is a number.
 * Count the number of characters in the number.
 *
 * This should cover all number formats for all languages.
 * Note that this is not a strict parser. It will happily parse numbers in
 * an invalid format.
 *
 * For example, only D allows underscores in the numbers, but they are
 * allowed in all formats.
 *
 * @param pc   The structure to update, str is an input.
 * @return     Whether a number was parsed
 */
static bool parse_number(chunk_t *pc)
{
   int  len;
   int  tmp;
   bool is_float;
   bool did_hex = false;

   /* A number must start with a digit or a dot, followed by a digit */
   if (!unc_isdigit(pc->str[0]) &&
       ((pc->str[0] != '.') || !unc_isdigit(pc->str[1])))
   {
      return(false);
   }
   len = 1;

   is_float = (pc->str[0] == '.');
   if (is_float && (pc->str[1] == '.'))
   {
      return(false);
   }

   /* Check for Hex, Octal, or Binary
    * Note that only D and Pawn support binary, but who cares?
    */
   if (pc->str[0] == '0')
   {
      switch (unc_toupper(pc->str[1]))
      {
      case 'X':               /* hex */
         did_hex = true;
         do
         {
            len++;
         } while (unc_isxdigit(pc->str[len]) || (pc->str[len] == '_'));
         break;

      case 'B':               /* binary */
         do
         {
            len++;
         } while ((pc->str[len] == '0') || (pc->str[len] == '1') ||
                  (pc->str[len] == '_'));
         break;

      case '0':                /* octal */
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
         do
         {
            len++;
         } while (((pc->str[len] >= '0') && (pc->str[len] <= '7')) ||
                  (pc->str[len] == '_'));
         break;

      default:
         /* either just 0 or 0.1 or 0UL, etc */
         break;
      }
   }
   else
   {
      /* Regular int or float */
      while (unc_isdigit(pc->str[len]) || (pc->str[len] == '_'))
      {
         len++;
      }
   }

   /* Check if we stopped on a decimal point & make sure it isn't '..' */
   if ((pc->str[len] == '.') && (pc->str[len + 1] != '.'))
   {
      len++;
      is_float = true;
      if (did_hex)
      {
         while (unc_isxdigit(pc->str[len]) || (pc->str[len] == '_'))
         {
            len++;
         }
      }
      else
      {
         while (unc_isdigit(pc->str[len]) || (pc->str[len] == '_'))
         {
            len++;
         }
      }
   }

   /* Check exponent
    * Valid exponents per language (not that it matters):
    * C/C++/D/Java: eEpP
    * C#/Pawn:      eE
    */
   tmp = unc_toupper(pc->str[len]);
   if ((tmp == 'E') || (tmp == 'P'))
   {
      is_float = true;
      len++;
      if ((pc->str[len] == '+') || (pc->str[len] == '-'))
      {
         len++;
      }
      while (unc_isdigit(pc->str[len]) || (pc->str[len] == '_'))
      {
         len++;
      }
   }

   /* Check the suffixes
    * Valid suffixes per language (not that it matters):
    *        Integer       Float
    * C/C++: uUlL64        lLfF
    * C#:    uUlL          fFdDMm
    * D:     uUL           ifFL
    * Java:  lL            fFdD
    * Pawn:  (none)        (none)
    *
    * Note that i, f, d, and m only appear in floats.
    */
   while (1)
   {
      tmp = unc_toupper(pc->str[len]);
      if ((tmp == 'I') || (tmp == 'F') || (tmp == 'D') || (tmp == 'M'))
      {
         is_float = true;
      }
      else if ((tmp != 'L') && (tmp != 'U'))
      {
         break;
      }
      len++;
   }

   /* skip the Microsoft-specific '64' suffix */
   if ((pc->str[len] == '6') && (pc->str[len + 1] == '4'))
   {
      len += 2;
   }

   /* It there is anything left, then we are probably dealing with garbage or
    * some sick macro junk. Eat it.
    */
   while (unc_isalnum(pc->str[len]))
   {
      len++;
   }

   pc->len     = len;
   pc->type    = is_float ? CT_NUMBER_FP : CT_NUMBER;
   cpd.column += len;
   return(true);
}


/**
 * Count the number of characters in a quoted string.
 * The next bit of text starts with a quote char " or ' or <.
 * Count the number of characters until the matching character.
 *
 * @param pc   The structure to update, str is an input.
 * @return     Whether a string was parsed
 */
static bool parse_string(chunk_t *pc, int quote_idx, bool allow_escape)
{
   bool escaped = 0;
   int  end_ch;
   int  len          = quote_idx;
   char escape_char  = cpd.settings[UO_string_escape_char].n;
   char escape_char2 = cpd.settings[UO_string_escape_char2].n;

   pc->type = CT_STRING;

   end_ch = CharTable::Get(pc->str[len]) & 0xff;
   len++;

   cpd.column += len;
   for (/* nada */; pc->str[len] != 0; len++)
   {
      cpd.column++;

      if ((pc->str[len] == '\n') ||
          ((pc->str[len] == '\r') && (pc->str[len + 1] != '\n')))
      {
         cpd.line_number++;
         cpd.column = 1;
         pc->nl_count++;
         pc->type = CT_STRING_MULTI;
      }
      if (!escaped)
      {
         if (pc->str[len] == escape_char)
         {
            escaped = (escape_char != 0);
         }
         else if ((pc->str[len] == escape_char2) &&
                  (pc->str[len + 1] == end_ch))
         {
            escaped = allow_escape;
         }
         else if (pc->str[len] == end_ch)
         {
            len++;
            break;
         }
      }
      else
      {
         escaped = false;
      }
   }

   /* D can have suffixes */
   if (((cpd.lang_flags & LANG_D) != 0) &&
       ((pc->str[len] == 'c') ||
        (pc->str[len] == 'w') ||
        (pc->str[len] == 'd')))
   {
      len++;
      cpd.column++;
   }
   pc->len = len;
   return(true);
}


/**
 * Literal string, ends with single "
 * Two "" don't end the string.
 *
 * @param pc   The structure to update, str is an input.
 * @return     Whether a string was parsed
 */
static bool parse_cs_string(chunk_t *pc)
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
   return(true);
}


/**
 * Count the number of characters in a word.
 * The first character is already valid for a keyword
 *
 * @param pc   The structure to update, str is an input.
 * @return     Whether a word was parsed (always true)
 */
bool parse_word(chunk_t *pc, bool skipcheck)
{
   int               len = 1;
   const chunk_tag_t *tag;

   while ((pc->str[len] < 127) && CharTable::IsKw2(pc->str[len]))
   {
      len++;
   }
   cpd.column += len;
   pc->len     = len;
   pc->type    = CT_WORD;

   if (skipcheck)
   {
      return(true);
   }

   /* Detect pre-processor functions now */
   if ((cpd.in_preproc == CT_PP_DEFINE) &&
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
   return(true);
}


/**
 * Count the number of whitespace characters.
 *
 * @param pc   The structure to update, str is an input.
 * @return     Whether whitespace was parsed
 */
static bool parse_whitespace(chunk_t *pc)
{
   int  len          = 0;
   int  nl_count     = 0;
   bool last_was_tab = false;

   while ((pc->str[len] != 0) &&
          ((pc->str[len] <= ' ') || (pc->str[len] >= 127)))
   {
      last_was_tab = false;
      switch (pc->str[len])
      {
      case '\r':
         if (pc->str[len + 1] == '\n')
         {
            /* CRLF ending */
            len++;
            cpd.le_counts[LE_CRLF]++;
         }
         else
         {
            /* CR ending */
            cpd.le_counts[LE_CR]++;
         }
         nl_count++;
         cpd.column = 1;
         cpd.line_number++;
         break;

      case '\n':
         /* LF ending */
         cpd.le_counts[LE_LF]++;
         nl_count++;
         cpd.column = 1;
         cpd.line_number++;
         break;

      case '\t':
         cpd.column = calc_next_tab_column(cpd.column,
                                           cpd.settings[UO_input_tab_size].n);
         last_was_tab = true;
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
      pc->after_tab = last_was_tab;
   }
   return(len != 0);
}


/**
 * Count the number of non-ascii characters at the start of a file.
 *
 * @param pc   The structure to update, str is an input.
 * @return     Whether a BOM was parsed
 */
static bool parse_bom(chunk_t *pc)
{
   int len = 0;

   while ((pc->str[len] != 0) &&
          ((pc->str[len] <= 0) || (pc->str[len] >= 127)))
   {
      len++;
   }

   if (len > 0)
   {
      pc->type = CT_SOF;
      pc->len  = len;
   }
   return(len != 0);
}


/**
 * Called when we hit a backslash.
 * If there is nothing but whitespace until the newline, then this is a
 * backslash newline
 */
static bool parse_bs_newline(chunk_t *pc)
{
   pc->len = 1;

   while (unc_isspace(pc->str[pc->len]))
   {
      if ((pc->str[pc->len] == '\r') || (pc->str[pc->len] == '\n'))
      {
         if (pc->str[pc->len] == '\n')
         {
            pc->len++;
         }
         else /* it is '\r' */
         {
            pc->len++;
            if (pc->str[pc->len] == '\n')
            {
               pc->len++;
            }
         }
         pc->type     = CT_NL_CONT;
         pc->nl_count = 1;
         return(true);
      }
      pc->len++;
   }
   return(false);
}


/**
 * Parses any number of tab or space chars followed by a newline.
 * Does not change pc->len if a newline isn't found.
 * This is not the same as parse_whitespace() because it only consumes until
 * a single newline is encountered.
 */
static bool parse_newline(chunk_t *pc)
{
   int len = pc->len;

   while ((pc->str[len] == ' ') || (pc->str[len] == '\t'))
   {
      len++;
   }
   if ((pc->str[len] == '\r') || (pc->str[len] == '\n'))
   {
      if (pc->str[len] == '\n')
      {
         len++;
      }
      else /* it is '\r' */
      {
         len++;
         if (pc->str[len] == '\n')
         {
            len++;
         }
      }
      pc->len = len;
      return(true);
   }
   return(false);
}


static bool parse_ignored(chunk_t *pc)
{
   int nl_count = 0;

   /* Parse off newlines */
   while (parse_newline(pc))
   {
      nl_count++;
   }
   if (nl_count > 0)
   {
      cpd.column       = 1;
      cpd.line_number += nl_count;
      pc->nl_count     = nl_count;
      pc->type         = CT_NEWLINE;
      return(true);
   }

   /* Look for the ending comment and let it pass */
   if (parse_comment(pc) && !cpd.unc_off)
   {
      return(true);
   }
   pc->len = 0;

   /* Reset the chunk & scan to until a newline */
   while ((pc->str[pc->len] != 0) &&
          (pc->str[pc->len] != '\r') &&
          (pc->str[pc->len] != '\n'))
   {
      pc->len++;
   }
   if (pc->len > 0)
   {
      pc->type = CT_IGNORED;
      return(true);
   }
   return(false);
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
 * @return        true/false - whether anything was parsed
 */
static bool parse_next(chunk_t *pc)
{
   const chunk_tag_t *punc;

   if ((pc == NULL) || (pc->str == NULL) || (*pc->str == 0))
   {
      //fprintf(stderr, "All done!\n");
      return(false);
   }

   /* Save off the current column */
   pc->orig_line = cpd.line_number;
   pc->column    = cpd.column;
   pc->orig_col  = cpd.column;
   pc->len       = 0;
   pc->type      = CT_NONE;
   pc->nl_count  = 0;
   pc->flags     = 0;

   /* If it is turned off, we put everything except newlines into CT_UNKNOWN */
   if (cpd.unc_off)
   {
      if (parse_ignored(pc))
      {
         return(true);
      }
   }

   if (chunk_get_head() == NULL)
   {
      if (parse_bom(pc))
      {
         return(true);
      }
   }

   /**
    *  Parse whitespace
    */
   if (parse_whitespace(pc))
   {
      return(true);
   }

   /**
    *  Handle unknown/unhandled preprocessors
    */
   if ((cpd.in_preproc > CT_PP_BODYCHUNK) &&
       (cpd.in_preproc <= CT_PP_OTHER))
   {
      /* Chunk to a newline or comment */
      pc->type = CT_PREPROC_BODY;
      char last = 0;
      while (pc->str[pc->len] != 0)
      {
         char ch = pc->str[pc->len];

         if ((ch == '\n') || (ch == '\r'))
         {
            /* Back off if this is an escaped newline */
            if (last == '\\')
            {
               pc->len--;
            }
            break;
         }

         /* Quit on a C++ comment start */
         if ((ch == '/') && (pc->str[pc->len + 1] == '/'))
         {
            break;
         }
         last = ch;
         pc->len++;
      }
      if (pc->len > 0)
      {
         return(true);
      }
   }

   /**
    *   Detect backslash-newline
    *
    * REVISIT: does this need to handle other line endings?
    */
   if ((pc->str[0] == '\\') && parse_bs_newline(pc))
   {
      cpd.column = 1;
      cpd.line_number++;
      return(true);
   }

   /**
    *  Parse comments
    */
   if (parse_comment(pc))
   {
      return(true);
   }

   /* Check for C# literal strings, ie @"hello" */
   if (((cpd.lang_flags & LANG_CS) != 0) && (*pc->str == '@'))
   {
      if (pc->str[1] == '"')
      {
         parse_cs_string(pc);
         return(true);
      }
      if (CharTable::IsKw1(pc->str[1]) && parse_word(pc, true))
      {
         return(true);
      }
   }

   /* Check for Obj-C NSString constants, ie @"hello" */
   if (((cpd.lang_flags & LANG_OC) != 0) && (*pc->str == '@'))
   {
      if (pc->str[1] == '"')
      {
         parse_string(pc, 1, true);
         return(true);
      }
   }

   /* PAWN specific stuff */
   if ((cpd.lang_flags & LANG_PAWN) != 0)
   {
      /* Check for PAWN strings: \"hi" or !"hi" or !\"hi" or \!"hi" */
      if ((pc->str[0] == '\\') || (pc->str[0] == '!'))
      {
         if (pc->str[1] == '"')
         {
            parse_string(pc, 1, (*pc->str == '!'));
            return(true);
         }
         else if (((pc->str[1] == '\\') || (pc->str[1] == '!')) &&
                  (pc->str[2] == '"'))
         {
            parse_string(pc, 2, false);
            return(true);
         }
      }
   }

   /**
    *  Parse strings and character constants
    */

   if (parse_number(pc))
   {
      return(true);
   }

   if ((cpd.lang_flags & LANG_D) != 0)
   {
      /* D specific stuff */
      if (d_parse_string(pc))
      {
         return(true);
      }
   }
   else
   {
      /* Not D stuff */

      /* Check for L'a', L"abc", 'a', "abc", <abc> strings */
      if ((((*pc->str == 'L') || (*pc->str == 'S')) &&
           ((pc->str[1] == '"') || (pc->str[1] == '\''))) ||
          (*pc->str == '"') ||
          (*pc->str == '\'') ||
          ((*pc->str == '<') && (cpd.in_preproc == CT_PP_INCLUDE)))
      {
         parse_string(pc, unc_isalpha(*pc->str) ? 1 : 0, true);
         return(true);
      }

      if ((*pc->str == '<') && (cpd.in_preproc == CT_PP_DEFINE))
      {
         if (chunk_get_tail()->type == CT_MACRO)
         {
            /* We have "#define XXX <", assume '<' starts an include string */
            parse_string(pc, 0, false);
            return(true);
         }
      }
   }

   /* Check for pawn/ObjectiveC identifiers */
   if ((*pc->str == '@') &&
       CharTable::IsKw2(pc->str[1]) &&
       parse_word(pc, false))
   {
      return(true);
   }

   if (CharTable::IsKw1(*pc->str) && parse_word(pc, false))
   {
      return(true);
   }

   if ((punc = find_punctuator(pc->str, cpd.lang_flags)) != NULL)
   {
      pc->type    = punc->type;
      pc->len     = strlen(punc->tag);
      cpd.column += pc->len;
      pc->flags  |= PCF_PUNCTUATOR;
      return(true);
   }


   /* throw away this character */
   pc->type = CT_UNKNOWN;
   pc->len  = 1;

   LOG_FMT(LWARN, "%s:%d Garbage in col %d: %x\n",
           cpd.filename, pc->orig_line, cpd.column, *pc->str);
   cpd.error_count++;
   return(true);
}


/**
 * This function parses or tokenizes the whole buffer into a list.
 * It has to do some tricks to parse preprocessors.
 *
 * If output_text() were called immediately after, two things would happen:
 *  - trailing whitespace are removed.
 *  - leading space & tabs are converted to the appropriate format.
 *
 * All the tokens are inserted before ref. If ref is NULL, they are inserted
 * at the end of the list.  Line numbers are relative to the start of the data.
 */
void tokenize(const char *data, int data_len, chunk_t *ref)
{
   int                idx = 0;
   chunk_t            chunk;
   chunk_t            *pc    = NULL;
   chunk_t            *rprev = NULL;
   chunk_t            *prev  = NULL;
   struct parse_frame frm;
   bool               last_was_tab = false;

   memset(&frm, 0, sizeof(frm));
   memset(&chunk, 0, sizeof(chunk));

   cpd.line_number = 1;
   cpd.column      = 1;

   while (idx < data_len)
   {
      chunk.str = &data[idx];
      if (!parse_next(&chunk))
      {
         LOG_FMT(LERR, "%s:%d Bailed before the end?\n",
                 cpd.filename, cpd.line_number);
         cpd.error_count++;
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

      if (chunk.type == CT_SOF)
      {
         cpd.bom = chunk_dup(&chunk);
         continue;
      }

      if (chunk.type == CT_NEWLINE)
      {
         last_was_tab    = chunk.after_tab;
         chunk.after_tab = false;
         chunk.len       = 0;
      }
      else if (chunk.type == CT_NL_CONT)
      {
         last_was_tab    = chunk.after_tab;
         chunk.after_tab = false;
         chunk.len       = 2;
         chunk.str       = "\\\n";
      }
      else
      {
         chunk.after_tab = last_was_tab;
         last_was_tab    = false;
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
      if (rprev != NULL)
      {
         pc->flags |= rprev->flags & PCF_COPY_FLAGS;

         /* a newline can't be in a preprocessor */
         if (pc->type == CT_NEWLINE)
         {
            pc->flags &= ~PCF_IN_PREPROC;
         }
      }
      if (ref != NULL)
      {
         chunk.flags |= PCF_INSERTED;
      }
      else
      {
         chunk.flags &= ~PCF_INSERTED;
      }
      pc = chunk_add_before(&chunk, ref);

      /* A newline marks the end of a preprocessor */
      if (pc->type == CT_NEWLINE) // || (pc->type == CT_COMMENT_MULTI))
      {
         cpd.in_preproc         = CT_NONE;
         cpd.preproc_ncnl_count = 0;
      }

      /* Special handling for preprocessor stuff */
      if (cpd.in_preproc != CT_NONE)
      {
         pc->flags |= PCF_IN_PREPROC;

         /* Count words after the preprocessor */
         if (!chunk_is_comment(pc) && !chunk_is_newline(pc))
         {
            cpd.preproc_ncnl_count++;
         }

         /* Figure out the type of preprocessor for #include parsing */
         if (cpd.in_preproc == CT_PREPROC)
         {
            if ((pc->type < CT_PP_DEFINE) || (pc->type > CT_PP_OTHER))
            {
               pc->type = CT_PP_OTHER;
            }
            cpd.in_preproc = pc->type;
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
            cpd.in_preproc = CT_PREPROC;
         }
      }
   }

   /* Set the cpd.newline string for this file */
   if ((cpd.settings[UO_newlines].le == LE_LF) ||
       ((cpd.settings[UO_newlines].le == LE_AUTO) &&
        (cpd.le_counts[LE_LF] >= cpd.le_counts[LE_CRLF]) &&
        (cpd.le_counts[LE_LF] >= cpd.le_counts[LE_CR])))
   {
      /* LF line ends */
      strcpy(cpd.newline, "\n");
      LOG_FMT(LLINEENDS, "Using LF line endings\n");
   }
   else if ((cpd.settings[UO_newlines].le == LE_CRLF) ||
            ((cpd.settings[UO_newlines].le == LE_AUTO) &&
             (cpd.le_counts[LE_CRLF] >= cpd.le_counts[LE_LF]) &&
             (cpd.le_counts[LE_CRLF] >= cpd.le_counts[LE_CR])))
   {
      /* CRLF line ends */
      strcpy(cpd.newline, "\r\n");
      LOG_FMT(LLINEENDS, "Using CRLF line endings\n");
   }
   else
   {
      /* CR line ends */
      strcpy(cpd.newline, "\r");
      LOG_FMT(LLINEENDS, "Using CR line endings\n");
   }
}


/**
 * A simplistic fixed-sized needle in the fixed-size haystack string search.
 */
int str_find(const char *needle, int needle_len,
             const char *haystack, int haystack_len)
{
   int idx;

   for (idx = 0; idx < (haystack_len - needle_len); idx++)
   {
      if (memcmp(needle, haystack + idx, needle_len) == 0)
      {
         return(idx);
      }
   }
   return(-1);
}
