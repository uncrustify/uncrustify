/**
 * @file tokenize.cpp
 * This file breaks up the text stream into tokens or chunks.
 *
 * Each routine needs to set pc.len and pc.type.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "tokenize.h"
#include "uncrustify_types.h"
#include "char_table.h"
#include "prototypes.h"
#include "chunk_list.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stack>
#include "unc_ctype.h"
#include "uncrustify.h"
#include "keywords.h"


using namespace std;


struct tok_info
{
   tok_info()
      : last_ch(0)
      , idx(0)
      , row(1)
      , col(1)
   {
   }

   size_t last_ch;
   size_t idx;
   size_t row;
   size_t col;
};


struct tok_ctx
{
   tok_ctx(const deque<int> &d)
      : data(d)
   {
   }


   //! save before trying to parse something that may fail
   void save()
   {
      save(s);
   }


   void save(tok_info &info)
   {
      info = c;
   }


   //! restore previous saved state
   void restore()
   {
      restore(s);
   }


   void restore(const tok_info &info)
   {
      c = info;
   }


   bool more()
   {
      return(c.idx < data.size());
   }


   size_t peek()
   {
      return(more() ? data[c.idx] : 0);
   }


   size_t peek(size_t idx)
   {
      idx += c.idx;
      return((idx < data.size()) ? data[idx] : 0);
   }


   size_t get()
   {
      if (more())
      {
         size_t ch = data[c.idx++];
         switch (ch)
         {
         case '\t':
            c.col = calc_next_tab_column(c.col, cpd.settings[UO_input_tab_size].u);
            break;

         case '\n':
            if (c.last_ch != '\r')
            {
               c.row++;
               c.col = 1;
            }
            break;

         case '\r':
            c.row++;
            c.col = 1;
            break;

         default:
            c.col++;
            break;
         }
         c.last_ch = ch;
         return(ch);
      }
      return(0);
   }


   bool expect(size_t ch)
   {
      if (peek() == ch)
      {
         get();
         return(true);
      }
      return(false);
   }


   const deque<int> &data;
   tok_info          c; //! current
   tok_info          s; //! saved
};


/**
 * Count the number of characters in a quoted string.
 * The next bit of text starts with a quote char " or ' or <.
 * Count the number of characters until the matching character.
 *
 * @param pc  The structure to update, str is an input.
 *
 * @return Whether a string was parsed
 */
static bool parse_string(tok_ctx &ctx, chunk_t &pc, size_t quote_idx, bool allow_escape);


/**
 * Literal string, ends with single "
 * Two "" don't end the string.
 *
 * @param pc  The structure to update, str is an input.
 *
 * @return Whether a string was parsed
 */
static bool parse_cs_string(tok_ctx &ctx, chunk_t &pc);


/**
 * VALA verbatim string, ends with three quotes (""")
 *
 * @param pc  The structure to update, str is an input.
 */
static void parse_verbatim_string(tok_ctx &ctx, chunk_t &pc);


static bool tag_compare(const deque<int> &d, size_t a_idx, size_t b_idx, size_t len);


/**
 * Parses a C++0x 'R' string. R"( xxx )" R"tag(  )tag" u8R"(x)" uR"(x)"
 * Newlines may be in the string.
 *
 * @param pc  structure to update, str is an input.
 */
static bool parse_cr_string(tok_ctx &ctx, chunk_t &pc, size_t q_idx);


/**
 * Count the number of whitespace characters.
 *
 * @param pc  The structure to update, str is an input.
 *
 * @return Whether whitespace was parsed
 */
static bool parse_whitespace(tok_ctx &ctx, chunk_t &pc);


/**
 * Called when we hit a backslash.
 * If there is nothing but whitespace until the newline, then this is a
 * backslash newline
 *
 * @param pc  structure to update, str is an input
 */
static bool parse_bs_newline(tok_ctx &ctx, chunk_t &pc);


/**
 * Parses any number of tab or space chars followed by a newline.
 * Does not change pc.len if a newline isn't found.
 * This is not the same as parse_whitespace() because it only consumes until
 * a single newline is encountered.
 */
static bool parse_newline(tok_ctx &ctx);


/**
 * PAWN #define is different than C/C++.
 *   #define PATTERN REPLACEMENT_TEXT
 * The PATTERN may not contain a space or '[' or ']'.
 * A generic whitespace check should be good enough.
 * Do not change the pattern.
 *
 * @param pc  structure to update, str is an input
 */
static void parse_pawn_pattern(tok_ctx &ctx, chunk_t &pc, c_token_t tt);


static bool parse_ignored(tok_ctx &ctx, chunk_t &pc);


/**
 * Skips the next bit of whatever and returns the type of block.
 *
 * pc.str is the input text.
 * pc.len in the output length.
 * pc.type is the output type
 * pc.column is output column
 *
 * @param pc  The structure to update, str is an input.
 *
 * @return true/false - whether anything was parsed
 */
static bool parse_next(tok_ctx &ctx, chunk_t &pc);


/**
 * Parses all legal D string constants.
 *
 * Quoted strings:
 *   r"Wysiwyg"      # WYSIWYG string
 *   x"hexstring"    # Hexadecimal array
 *   `Wysiwyg`       # WYSIWYG string
 *   'char'          # single character
 *   "reg_string"    # regular string
 *
 * Non-quoted strings:
 * \x12              # 1-byte hex constant
 * \u1234            # 2-byte hex constant
 * \U12345678        # 4-byte hex constant
 * \123              # octal constant
 * \&amp;            # named entity
 * \n                # single character
 *
 * @param pc  The structure to update, str is an input.
 *
 * @return Whether a string was parsed
 */
static bool d_parse_string(tok_ctx &ctx, chunk_t &pc);


/**
 * Figure of the length of the comment at text.
 * The next bit of text starts with a '/', so it might be a comment.
 * There are three types of comments:
 *  - C comments that start with  '/ *' and end with '* /'
 *  - C++ comments that start with //
 *  - D nestable comments '/+' '+/'
 *
 * @param pc  The structure to update, str is an input.
 *
 * @return Whether a comment was parsed
 */
static bool parse_comment(tok_ctx &ctx, chunk_t &pc);


/**
 * Figure of the length of the code placeholder at text, if present.
 * This is only for Xcode which sometimes inserts temporary code placeholder chunks, which in plaintext <#look like this#>.
 *
 * @param pc  The structure to update, str is an input.
 *
 * @return Whether a placeholder was parsed.
 */
static bool parse_code_placeholder(tok_ctx &ctx, chunk_t &pc);


/**
 * Parse any attached suffix, which may be a user-defined literal suffix.
 * If for a string, explicitly exclude common format and scan specifiers, ie,
 * PRIx32 and SCNx64.
 */
static void parse_suffix(tok_ctx &ctx, chunk_t &pc, bool forstring);


//! check if a symbol holds a boolean value
static bool is_bin(int ch);
static bool is_bin_(int ch);


//! check if a symbol holds a octal value
static bool is_oct(int ch);
static bool is_oct_(int ch);


//! check if a symbol holds a decimal value;
static bool is_dec(int ch);
static bool is_dec_(int ch);


//! check if a symbol holds a hexadecimal value
static bool is_hex(int ch);
static bool is_hex_(int ch);


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
 * @param[in,out] pc  The structure to update, str is an input.
 *
 * @return Whether a number was parsed
 */
static bool parse_number(tok_ctx &ctx, chunk_t &pc);


static bool d_parse_string(tok_ctx &ctx, chunk_t &pc)
{
   size_t ch = ctx.peek();

   if (  (ch == '"')
      || (ch == '\'')
      || (ch == '`'))
   {
      return(parse_string(ctx, pc, 0, true));
   }

   if (ch == '\\')
   {
      ctx.save();
      int cnt;
      pc.str.clear();
      while (ctx.peek() == '\\')
      {
         pc.str.append(ctx.get());
         // Check for end of file
         switch (ctx.peek())
         {
         case 'x':  // \x HexDigit HexDigit
            cnt = 3;
            while (cnt--)
            {
               pc.str.append(ctx.get());
            }
            break;

         case 'u':  // \u HexDigit (x4)
            cnt = 5;
            while (cnt--)
            {
               pc.str.append(ctx.get());
            }
            break;

         case 'U':  // \U HexDigit (x8)
            cnt = 9;
            while (cnt--)
            {
               pc.str.append(ctx.get());
            }
            break;

         case '0':
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
            // handle up to 3 octal digits
            pc.str.append(ctx.get());
            ch = ctx.peek();
            if ((ch >= '0') && (ch <= '7'))
            {
               pc.str.append(ctx.get());
               ch = ctx.peek();
               if ((ch >= '0') && (ch <= '7'))
               {
                  pc.str.append(ctx.get());
               }
            }
            break;

         case '&':
            // \& NamedCharacterEntity ;
            pc.str.append(ctx.get());
            while (unc_isalpha(ctx.peek()))
            {
               pc.str.append(ctx.get());
            }
            if (ctx.peek() == ';')
            {
               pc.str.append(ctx.get());
            }
            break;

         default:
            // Everything else is a single character
            pc.str.append(ctx.get());
            break;
         } // switch
      }

      if (pc.str.size() > 1)
      {
         pc.type = CT_STRING;
         return(true);
      }
      ctx.restore();
   }
   else if (  ((ch == 'r') || (ch == 'x'))
           && (ctx.peek(1) == '"'))
   {
      return(parse_string(ctx, pc, 1, false));
   }
   return(false);
} // d_parse_string


#if 0


//! A string-in-string search.  Like strstr() with a haystack length.
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
#endif


static bool parse_comment(tok_ctx &ctx, chunk_t &pc)
{
   bool   is_d    = (cpd.lang_flags & LANG_D) != 0;          // forcing value to bool
   bool   is_cs   = (cpd.lang_flags & LANG_CS) != 0;         // forcing value to bool
   size_t d_level = 0;

   // does this start with '/ /' or '/ *' or '/ +' (d)
   if (  (ctx.peek() != '/')
      || (  (ctx.peek(1) != '*')
         && (ctx.peek(1) != '/')
         && ((ctx.peek(1) != '+') || !is_d)))
   {
      return(false);
   }

   ctx.save();

   // account for opening two chars
   pc.str = ctx.get();   // opening '/'
   size_t ch = ctx.get();
   pc.str.append(ch);    // second char

   if (ch == '/')
   {
      pc.type = CT_COMMENT_CPP;
      while (true)
      {
         int bs_cnt = 0;
         while (ctx.more())
         {
            ch = ctx.peek();
            if ((ch == '\r') || (ch == '\n'))
            {
               break;
            }
            if ((ch == '\\') && !is_cs) // backslashes aren't special in comments in C#
            {
               bs_cnt++;
            }
            else
            {
               bs_cnt = 0;
            }
            pc.str.append(ctx.get());
         }

         /*
          * If we hit an odd number of backslashes right before the newline,
          * then we keep going.
          */
         if (((bs_cnt & 1) == 0) || !ctx.more())
         {
            break;
         }
         if (ctx.peek() == '\r')
         {
            pc.str.append(ctx.get());
         }
         if (ctx.peek() == '\n')
         {
            pc.str.append(ctx.get());
         }
         pc.nl_count++;
         cpd.did_newline = true;
      }
   }
   else if (!ctx.more())
   {
      // unexpected end of file
      ctx.restore();
      return(false);
   }
   else if (ch == '+')
   {
      pc.type = CT_COMMENT;
      d_level++;
      while (d_level > 0 && ctx.more())
      {
         if ((ctx.peek() == '+') && (ctx.peek(1) == '/'))
         {
            pc.str.append(ctx.get());  // store the '+'
            pc.str.append(ctx.get());  // store the '/'
            d_level--;
            continue;
         }

         if ((ctx.peek() == '/') && (ctx.peek(1) == '+'))
         {
            pc.str.append(ctx.get());  // store the '/'
            pc.str.append(ctx.get());  // store the '+'
            d_level++;
            continue;
         }

         ch = ctx.get();
         pc.str.append(ch);
         if ((ch == '\n') || (ch == '\r'))
         {
            pc.type = CT_COMMENT_MULTI;
            pc.nl_count++;

            if (ch == '\r')
            {
               if (ctx.peek() == '\n')
               {
                  cpd.le_counts[LE_CRLF]++;
                  pc.str.append(ctx.get());  // store the '\n'
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
      }
   }
   else  // must be '/ *'
   {
      pc.type = CT_COMMENT;
      while (ctx.more())
      {
         if ((ctx.peek() == '*') && (ctx.peek(1) == '/'))
         {
            pc.str.append(ctx.get());  // store the '*'
            pc.str.append(ctx.get());  // store the '/'

            tok_info ss;
            ctx.save(ss);
            size_t   oldsize = pc.str.size();

            // If there is another C comment right after this one, combine them
            while ((ctx.peek() == ' ') || (ctx.peek() == '\t'))
            {
               pc.str.append(ctx.get());
            }
            if ((ctx.peek() != '/') || (ctx.peek(1) != '*'))
            {
               // undo the attempt to join
               ctx.restore(ss);
               pc.str.resize(oldsize);
               break;
            }
         }

         ch = ctx.get();
         pc.str.append(ch);
         if ((ch == '\n') || (ch == '\r'))
         {
            pc.type = CT_COMMENT_MULTI;
            pc.nl_count++;

            if (ch == '\r')
            {
               if (ctx.peek() == '\n')
               {
                  cpd.le_counts[LE_CRLF]++;
                  pc.str.append(ctx.get());  // store the '\n'
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
      }
   }

   if (cpd.unc_off)
   {
      const char *ontext = cpd.settings[UO_enable_processing_cmt].str;
      if (ontext == nullptr || ontext[0] == '\0')
      {
         ontext = UNCRUSTIFY_ON_TEXT;
      }

      if (pc.str.find(ontext) >= 0)
      {
         LOG_FMT(LBCTRL, "Found '%s' on line %zu\n", ontext, pc.orig_line);
         cpd.unc_off = false;
      }
   }
   else
   {
      const char *offtext = cpd.settings[UO_disable_processing_cmt].str;
      if (offtext == nullptr || !offtext[0])
      {
         offtext = UNCRUSTIFY_OFF_TEXT;
      }

      if (pc.str.find(offtext) >= 0)
      {
         LOG_FMT(LBCTRL, "Found '%s' on line %zu\n", offtext, pc.orig_line);
         cpd.unc_off = true;
         // Issue #842
         cpd.unc_off_used = true;
      }
   }
   return(true);
} // parse_comment


static bool parse_code_placeholder(tok_ctx &ctx, chunk_t &pc)
{
   if ((ctx.peek() != '<') || (ctx.peek(1) != '#'))
   {
      return(false);
   }

   ctx.save();

   // account for opening two chars '<#'
   pc.str = ctx.get();
   pc.str.append(ctx.get());

   // grab everything until '#>', fail if not found.
   size_t last1 = 0;
   while (ctx.more())
   {
      size_t last2 = last1;
      last1 = ctx.get();
      pc.str.append(last1);

      if ((last2 == '#') && (last1 == '>'))
      {
         pc.type = CT_WORD;
         return(true);
      }
   }
   ctx.restore();
   return(false);
}


static void parse_suffix(tok_ctx &ctx, chunk_t &pc, bool forstring = false)
{
   if (CharTable::IsKw1(ctx.peek()))
   {
      size_t slen    = 0;
      size_t oldsize = pc.str.size();

      // don't add the suffix if we see L" or L' or S"
      size_t p1 = ctx.peek();
      size_t p2 = ctx.peek(1);
      if (  forstring
         && (  (  (p1 == 'L')
               && ((p2 == '"') || (p2 == '\'')))
            || ((p1 == 'S') && (p2 == '"'))))
      {
         return;
      }

      tok_info ss;
      ctx.save(ss);
      while (ctx.more() && CharTable::IsKw2(ctx.peek()))
      {
         slen++;
         pc.str.append(ctx.get());
      }

      if (  forstring
         && slen >= 4
         && (  pc.str.startswith("PRI", oldsize)
            || pc.str.startswith("SCN", oldsize)))
      {
         ctx.restore(ss);
         pc.str.resize(oldsize);
      }
   }
}


static bool is_bin(int ch)
{
   return((ch == '0') || (ch == '1'));
}


static bool is_bin_(int ch)
{
   return(  is_bin(ch)
         || ch == '_'
         || ch == '\'');
}


static bool is_oct(int ch)
{
   return((ch >= '0') && (ch <= '7'));
}


static bool is_oct_(int ch)
{
   return(  is_oct(ch)
         || ch == '_'
         || ch == '\'');
}


static bool is_dec(int ch)
{
   return((ch >= '0') && (ch <= '9'));
}


static bool is_dec_(int ch)
{
   // number separators: JAVA: "_", C++14: "'"
   return(  is_dec(ch)
         || (ch == '_')
         || (ch == '\''));
}


static bool is_hex(int ch)
{
   return(  ((ch >= '0') && (ch <= '9'))
         || ((ch >= 'a') && (ch <= 'f'))
         || ((ch >= 'A') && (ch <= 'F')));
}


static bool is_hex_(int ch)
{
   return(  is_hex(ch)
         || ch == '_'
         || ch == '\'');
}


static bool parse_number(tok_ctx &ctx, chunk_t &pc)
{
   /*
    * A number must start with a digit or a dot, followed by a digit
    * (signs handled elsewhere)
    */
   if (  !is_dec(ctx.peek())
      && ((ctx.peek() != '.') || !is_dec(ctx.peek(1))))
   {
      return(false);
   }

   bool is_float = (ctx.peek() == '.');
   if (is_float && (ctx.peek(1) == '.')) // make sure it isn't '..'
   {
      return(false);
   }

   /*
    * Check for Hex, Octal, or Binary
    * Note that only D, C++14 and Pawn support binary
    */
   bool did_hex = false;
   if (ctx.peek() == '0')
   {
      size_t  ch;
      chunk_t pc_temp;

      pc.str.append(ctx.get());  // store the '0'
      pc_temp.str.append('0');

      // MS constant might have an "h" at the end. Look for it
      ctx.save();
      while (ctx.more() && CharTable::IsKw2(ctx.peek()))
      {
         ch = ctx.get();
         pc_temp.str.append(ch);
      }
      ch = pc_temp.str[pc_temp.len() - 1];
      ctx.restore();
      LOG_FMT(LGUY, "%s(%d): pc_temp:%s\n", __func__, __LINE__, pc_temp.text());

      if (ch == 'h') // TODO can we combine this in analyze_character
      {
         // we have an MS hexadecimal number with "h" at the end
         LOG_FMT(LGUY, "%s(%d): MS hexadecimal number\n", __func__, __LINE__);
         did_hex = true;
         do
         {
            pc.str.append(ctx.get()); // store the rest
         } while (is_hex_(ctx.peek()));
         pc.str.append(ctx.get());    // store the h
         LOG_FMT(LGUY, "%s(%d): pc:%s\n", __func__, __LINE__, pc.text());
      }
      else
      {
         switch (unc_toupper(ctx.peek()))
         {
         case 'X':               // hex
            did_hex = true;
            do
            {
               pc.str.append(ctx.get());  // store the 'x' and then the rest
            } while (is_hex_(ctx.peek()));
            break;

         case 'B':               // binary
            do
            {
               pc.str.append(ctx.get());  // store the 'b' and then the rest
            } while (is_bin_(ctx.peek()));
            break;

         case '0':               // octal or decimal
         case '1':
         case '2':
         case '3':
         case '4':
         case '5':
         case '6':
         case '7':
         case '8':
         case '9':
            do
            {
               pc.str.append(ctx.get());
            } while (is_oct_(ctx.peek()));
            break;

         default:
            // either just 0 or 0.1 or 0UL, etc
            break;
         }
      }
   }
   else
   {
      // Regular int or float
      while (is_dec_(ctx.peek()))
      {
         pc.str.append(ctx.get());
      }
   }

   // Check if we stopped on a decimal point & make sure it isn't '..'
   if ((ctx.peek() == '.') && (ctx.peek(1) != '.'))
   {
      // Issue #1265, 5.clamp()
      tok_info ss;
      ctx.save(ss);
      while (ctx.more() && CharTable::IsKw2(ctx.peek(1)))
      {
         // skip characters to check for paren open
         ctx.get();
      }
      if (ctx.peek(1) == '(')
      {
         ctx.restore(ss);
         pc.type = CT_NUMBER;
         return(true);
      }
      else
      {
         ctx.restore(ss);
      }

      pc.str.append(ctx.get());
      is_float = true;
      if (did_hex)
      {
         while (is_hex_(ctx.peek()))
         {
            pc.str.append(ctx.get());
         }
      }
      else
      {
         while (is_dec_(ctx.peek()))
         {
            pc.str.append(ctx.get());
         }
      }
   }

   /*
    * Check exponent
    * Valid exponents per language (not that it matters):
    * C/C++/D/Java: eEpP
    * C#/Pawn:      eE
    */
   size_t tmp = unc_toupper(ctx.peek());
   if ((tmp == 'E') || (tmp == 'P'))
   {
      is_float = true;
      pc.str.append(ctx.get());
      if ((ctx.peek() == '+') || (ctx.peek() == '-'))
      {
         pc.str.append(ctx.get());
      }
      while (is_dec_(ctx.peek()))
      {
         pc.str.append(ctx.get());
      }
   }

   /*
    * Check the suffixes
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
      size_t tmp2 = unc_toupper(ctx.peek());
      if (  (tmp2 == 'I')
         || (tmp2 == 'F')
         || (tmp2 == 'D')
         || (tmp2 == 'M'))
      {
         is_float = true;
      }
      else if ((tmp2 != 'L') && (tmp2 != 'U'))
      {
         break;
      }
      pc.str.append(ctx.get());
   }

   // skip the Microsoft-specific '64' suffix
   if ((ctx.peek() == '6') && (ctx.peek(1) == '4'))
   {
      pc.str.append(ctx.get());
      pc.str.append(ctx.get());
   }

   pc.type = is_float ? CT_NUMBER_FP : CT_NUMBER;

   /*
    * If there is anything left, then we are probably dealing with garbage or
    * some sick macro junk. Eat it.
    */
   parse_suffix(ctx, pc);

   return(true);
} // parse_number


static bool parse_string(tok_ctx &ctx, chunk_t &pc, size_t quote_idx, bool allow_escape)
{
   size_t escape_char        = cpd.settings[UO_string_escape_char].u;
   size_t escape_char2       = cpd.settings[UO_string_escape_char2].u;
   bool   should_escape_tabs = (  cpd.settings[UO_string_replace_tab_chars].b
                               && (cpd.lang_flags & LANG_ALLC));

   pc.str.clear();
   while (quote_idx-- > 0)
   {
      pc.str.append(ctx.get());
   }

   pc.type = CT_STRING;
   size_t end_ch = CharTable::Get(ctx.peek()) & 0xff;
   pc.str.append(ctx.get());  // store the "

   bool escaped = false;
   while (ctx.more())
   {
      size_t lastcol = ctx.c.col;
      size_t ch      = ctx.get();

      if ((ch == '\t') && should_escape_tabs)
      {
         ctx.c.col = lastcol + 2;
         pc.str.append(escape_char);
         pc.str.append('t');
         continue;
      }

      pc.str.append(ch);
      if (ch == '\n')
      {
         pc.nl_count++;
         pc.type = CT_STRING_MULTI;
         escaped = false;
         continue;
      }
      if ((ch == '\r') && (ctx.peek() != '\n'))
      {
         pc.str.append(ctx.get());
         pc.nl_count++;
         pc.type = CT_STRING_MULTI;
         escaped = false;
         continue;
      }
      if (!escaped)
      {
         if (ch == escape_char)
         {
            escaped = (escape_char != 0);
         }
         else if (ch == escape_char2 && (ctx.peek() == end_ch))
         {
            escaped = allow_escape;
         }
         else if (ch == end_ch)
         {
            break;
         }
      }
      else
      {
         escaped = false;
      }
   }

   parse_suffix(ctx, pc, true);
   return(true);
} // parse_string

enum cs_string_t
{
   CS_STRING_NONE         = 0,
   CS_STRING_STRING       = 1 << 0,    // is any kind of string
   CS_STRING_VERBATIM     = 1 << 1,    // @"" style string
   CS_STRING_INTERPOLATED = 1 << 2,    // $"" or $@"" style string
};

static cs_string_t operator |=(cs_string_t &value, cs_string_t other)
{
   return(value = static_cast<cs_string_t>(value | other));
}


static cs_string_t parse_cs_string_start(tok_ctx &ctx, chunk_t &pc)
{
   cs_string_t stringType = CS_STRING_NONE;
   int         offset     = 0;

   if (ctx.peek(offset) == '$')
   {
      stringType |= CS_STRING_INTERPOLATED;
      ++offset;
   }

   if (ctx.peek(offset) == '@')
   {
      stringType |= CS_STRING_VERBATIM;
      ++offset;
   }

   if (ctx.peek(offset) == '"')
   {
      stringType |= CS_STRING_STRING;

      pc.type = CT_STRING;

      for (int i = 0; i <= offset; ++i)
      {
         pc.str.append(ctx.get());
      }
   }
   else
   {
      stringType = CS_STRING_NONE;
   }

   return(stringType);
} // parse_cs_string_start


struct CsStringParseState
{
   cs_string_t type;
   int         braceDepth;


   CsStringParseState(cs_string_t stringType)
   {
      type       = stringType;
      braceDepth = 0;
   }
};


/**
 * C# strings are complex enough (mostly due to interpolation and nesting) that they need a custom parser.
 */
static bool parse_cs_string(tok_ctx &ctx, chunk_t &pc)
{
   cs_string_t stringType = parse_cs_string_start(ctx, pc);

   if (stringType == 0)
   {
      return(false);
   }

   // an interpolated string can contain {expressions}, which can contain $"strings", which in turn
   // can contain {expressions}, so we must track both as they are interleaved, in order to properly
   // parse the outermost string.

   std::stack<CsStringParseState> parseState; // each entry is a nested string
   parseState.push(CsStringParseState(stringType));

   bool should_escape_tabs = cpd.settings[UO_string_replace_tab_chars].b;

   while (ctx.more())
   {
      if (parseState.top().braceDepth > 0)
      {
         // all we can do when in an expr is look for expr close with }, or a new string opening. must do this first
         // so we can peek and potentially consume chars for new string openings, before the ch=get() happens later,
         // which is needed for newline processing.

         if (ctx.peek() == '}')
         {
            pc.str.append(ctx.get());

            if (ctx.peek() == '}')
            {
               pc.str.append(ctx.get()); // in interpolated string, `}}` is escape'd `}`
            }
            else
            {
               --parseState.top().braceDepth;
            }

            continue;
         }

         stringType = parse_cs_string_start(ctx, pc);
         if (stringType)
         {
            parseState.push(CsStringParseState(stringType));
            continue;
         }
      }

      int lastcol = ctx.c.col;
      int ch      = ctx.get();

      pc.str.append(ch);

      if (ch == '\n')
      {
         pc.type = CT_STRING_MULTI;
         pc.nl_count++;
      }
      else if (ch == '\r')
      {
         pc.type = CT_STRING_MULTI;
      }
      else if (parseState.top().braceDepth > 0)
      {
         // do nothing. if we're in a brace, we only want the newline handling, and skip the rest.
      }
      else if ((ch == '\t') && should_escape_tabs)
      {
         if (parseState.top().type & CS_STRING_VERBATIM)
         {
            if (!cpd.warned_unable_string_replace_tab_chars)
            {
               cpd.warned_unable_string_replace_tab_chars = true;

               log_sev_t warnlevel = (log_sev_t)cpd.settings[UO_warn_level_tabs_found_in_verbatim_string_literals].n;

               /*
                * a tab char can't be replaced with \\t because escapes don't
                * work in here-strings. best we can do is warn.
                */
               LOG_FMT(warnlevel, "%s:%zu Detected non-replaceable tab char in literal string\n",
                       cpd.filename.c_str(), pc.orig_line);
               if (warnlevel < LWARN)
               {
                  cpd.error_count++;
               }
            }
         }
         else
         {
            ctx.c.col = lastcol + 2;
            pc.str.pop_back(); // remove \t
            pc.str.append("\\t");

            continue;
         }
      }
      else if (ch == '\\' && !(parseState.top().type & CS_STRING_VERBATIM))
      {
         // catch escaped quote in order to avoid ending string (but also must handle \\ to avoid accidental 'escape' seq of `\\"`)
         if (ctx.peek() == '"' || ctx.peek() == '\\')
         {
            pc.str.append(ctx.get());
         }
      }
      else if (ch == '"')
      {
         if ((parseState.top().type & CS_STRING_VERBATIM) && (ctx.peek() == '"'))
         {
            // in verbatim string, `""` is escape'd `"`
            pc.str.append(ctx.get());
         }
         else
         {
            // end of string
            parseState.pop();
            if (parseState.empty())
            {
               break;
            }
         }
      }
      else if (parseState.top().type & CS_STRING_INTERPOLATED)
      {
         if (ch == '{')
         {
            if (ctx.peek() == '{')
            {
               pc.str.append(ctx.get()); // in interpolated string, `{{` is escape'd `{`
            }
            else
            {
               ++parseState.top().braceDepth;
            }
         }
      }
   }

   return(true);
} // parse_cs_string


static void parse_verbatim_string(tok_ctx &ctx, chunk_t &pc)
{
   pc.type = CT_STRING;

   // consume the initial """
   pc.str = ctx.get();
   pc.str.append(ctx.get());
   pc.str.append(ctx.get());

   // go until we hit a zero (end of file) or a """
   while (ctx.more())
   {
      size_t ch = ctx.get();
      pc.str.append(ch);
      if (  (ch == '"')
         && (ctx.peek() == '"')
         && (ctx.peek(1) == '"'))
      {
         pc.str.append(ctx.get());
         pc.str.append(ctx.get());
         break;
      }
      if ((ch == '\n') || (ch == '\r'))
      {
         pc.type = CT_STRING_MULTI;
         pc.nl_count++;
      }
   }
}


static bool tag_compare(const deque<int> &d, size_t a_idx, size_t b_idx, size_t len)
{
   if (a_idx != b_idx)
   {
      while (len-- > 0)
      {
         if (d[a_idx] != d[b_idx])
         {
            return(false);
         }
      }
   }
   return(true);
}


static bool parse_cr_string(tok_ctx &ctx, chunk_t &pc, size_t q_idx)
{
   size_t tag_idx = ctx.c.idx + q_idx + 1;
   size_t tag_len = 0;

   ctx.save();

   // Copy the prefix + " to the string
   pc.str.clear();
   int cnt = q_idx + 1;
   while (cnt--)
   {
      pc.str.append(ctx.get());
   }

   // Add the tag and get the length of the tag
   while (ctx.more() && (ctx.peek() != '('))
   {
      tag_len++;
      pc.str.append(ctx.get());
   }
   if (ctx.peek() != '(')
   {
      ctx.restore();
      return(false);
   }

   pc.type = CT_STRING;
   while (ctx.more())
   {
      if (  (ctx.peek() == ')')
         && (ctx.peek(tag_len + 1) == '"')
         && tag_compare(ctx.data, tag_idx, ctx.c.idx + 1, tag_len))
      {
         cnt = tag_len + 2;   // for the )"
         while (cnt--)
         {
            pc.str.append(ctx.get());
         }
         parse_suffix(ctx, pc);
         return(true);
      }
      if (ctx.peek() == '\n')
      {
         pc.str.append(ctx.get());
         pc.nl_count++;
         pc.type = CT_STRING_MULTI;
      }
      else
      {
         pc.str.append(ctx.get());
      }
   }
   ctx.restore();
   return(false);
} // parse_cr_string


/**
 * Count the number of characters in a word.
 * The first character is already valid for a keyword
 *
 * @param pc   The structure to update, str is an input.
 * @return     Whether a word was parsed (always true)
 */
static bool parse_word(tok_ctx &ctx, chunk_t &pc, bool skipcheck)
{
   static unc_text intr_txt("@interface");

   // The first character is already valid
   pc.str.clear();
   pc.str.append(ctx.get());

   while (ctx.more())
   {
      size_t ch = ctx.peek();
      if (CharTable::IsKw2(ch))
      {
         pc.str.append(ctx.get());
      }
      else if ((ch == '\\') && (unc_tolower(ctx.peek(1)) == 'u'))
      {
         pc.str.append(ctx.get());
         pc.str.append(ctx.get());
         skipcheck = true;
      }
      else
      {
         break;
      }

      // HACK: Non-ASCII character are only allowed in identifiers
      if (ch > 0x7f)
      {
         skipcheck = true;
      }
   }
   pc.type = CT_WORD;

   if (skipcheck)
   {
      return(true);
   }

   // Detect pre-processor functions now
   if (cpd.in_preproc == CT_PP_DEFINE && cpd.preproc_ncnl_count == 1)
   {
      if (ctx.peek() == '(')
      {
         pc.type = CT_MACRO_FUNC;
      }
      else
      {
         pc.type = CT_MACRO;
         if (cpd.settings[UO_pp_ignore_define_body].b)
         {
            /*
             * We are setting the PP_IGNORE preproc state because the following
             * chunks are part of the macro body and will have to be ignored.
             */
            cpd.in_preproc = CT_PP_IGNORE;
         }
      }
   }
   else
   {
      // '@interface' is reserved, not an interface itself
      if (  (cpd.lang_flags & LANG_JAVA)
         && pc.str.startswith("@")
         && !pc.str.equals(intr_txt))
      {
         pc.type = CT_ANNOTATION;
      }
      else
      {
         // Turn it into a keyword now
         pc.type = find_keyword_type(pc.text(), pc.str.size());

         /* Special pattern: if we're trying to redirect a preprocessor directive to PP_IGNORE,
          * then ensure we're actually part of a preprocessor before doing the swap, or we'll
          * end up with a function named 'define' as PP_IGNORE. This is necessary because with
          * the config 'set' feature, there's no way to do a pair of tokens as a word
          * substitution. */
         if (pc.type == CT_PP_IGNORE && !cpd.in_preproc)
         {
            pc.type = find_keyword_type(pc.text(), pc.str.size(), false);
         }
      }
   }

   return(true);
} // parse_word


static bool parse_whitespace(tok_ctx &ctx, chunk_t &pc)
{
   size_t nl_count = 0;
   size_t ch       = 0;

   // REVISIT: use a better whitespace detector?
   while (ctx.more() && unc_isspace(ctx.peek()))
   {
      ch = ctx.get();   // throw away the whitespace char
      switch (ch)
      {
      case '\r':
         if (ctx.expect('\n'))
         {
            // CRLF ending
            cpd.le_counts[LE_CRLF]++;
         }
         else
         {
            // CR ending
            cpd.le_counts[LE_CR]++;
         }
         nl_count++;
         pc.orig_prev_sp = 0;
         break;

      case '\n':
         // LF ending
         cpd.le_counts[LE_LF]++;
         nl_count++;
         pc.orig_prev_sp = 0;
         break;

      case '\t':
         pc.orig_prev_sp += calc_next_tab_column(cpd.column, cpd.settings[UO_input_tab_size].u) - cpd.column;
         break;

      case ' ':
         pc.orig_prev_sp++;
         break;

      default:
         break;
      }
   }

   if (ch != 0)
   {
      pc.str.clear();
      pc.nl_count  = nl_count;
      pc.type      = nl_count ? CT_NEWLINE : CT_WHITESPACE;
      pc.after_tab = (ctx.c.last_ch == '\t');
      return(true);
   }
   return(false);
} // parse_whitespace


static bool parse_bs_newline(tok_ctx &ctx, chunk_t &pc)
{
   ctx.save();
   ctx.get(); // skip the '\'

   size_t ch;
   while (ctx.more() && unc_isspace(ch = ctx.peek()))
   {
      ctx.get();
      if ((ch == '\r') || (ch == '\n'))
      {
         if (ch == '\r')
         {
            ctx.expect('\n');
         }
         pc.str      = "\\";
         pc.type     = CT_NL_CONT;
         pc.nl_count = 1;
         return(true);
      }
   }

   ctx.restore();
   return(false);
}


static bool parse_newline(tok_ctx &ctx)
{
   ctx.save();

   // Eat whitespace
   while ((ctx.peek() == ' ') || (ctx.peek() == '\t'))
   {
      ctx.get();
   }
   if ((ctx.peek() == '\r') || (ctx.peek() == '\n'))
   {
      if (!ctx.expect('\n'))
      {
         ctx.get();
         ctx.expect('\n');
      }
      return(true);
   }
   ctx.restore();
   return(false);
}


static void parse_pawn_pattern(tok_ctx &ctx, chunk_t &pc, c_token_t tt)
{
   pc.str.clear();
   pc.type = tt;
   while (!unc_isspace(ctx.peek()))
   {
      // end the pattern on an escaped newline
      if (ctx.peek() == '\\')
      {
         size_t ch = ctx.peek(1);
         if ((ch == '\n') || (ch == '\r'))
         {
            break;
         }
      }
      pc.str.append(ctx.get());
   }
}


static bool parse_ignored(tok_ctx &ctx, chunk_t &pc)
{
   size_t nl_count = 0;

   // Parse off newlines/blank lines
   while (parse_newline(ctx))
   {
      nl_count++;
   }
   if (nl_count > 0)
   {
      pc.nl_count = nl_count;
      pc.type     = CT_NEWLINE;
      return(true);
   }

   // See if the UO_enable_processing_cmt or #pragma endasm / #endasm text is on this line
   ctx.save();
   pc.str.clear();
   while (  ctx.more()
         && (ctx.peek() != '\r')
         && (ctx.peek() != '\n'))
   {
      pc.str.append(ctx.get());
   }
   if (pc.str.size() == 0)
   {
      // end of file?
      return(false);
   }
   // HACK: turn on if we find '#endasm' or '#pragma' and 'endasm' separated by blanks
   if (  (  ((pc.str.find("#pragma ") >= 0) || (pc.str.find("#pragma	") >= 0))
         && ((pc.str.find(" endasm") >= 0) || (pc.str.find("	endasm") >= 0)))
      || (pc.str.find("#endasm") >= 0))
   {
      cpd.unc_off = false;
      ctx.restore();
      pc.str.clear();
      return(false);
   }
   // Note that we aren't actually making sure this is in a comment, yet
   const char *ontext = cpd.settings[UO_enable_processing_cmt].str;
   if (ontext == nullptr || ontext[0] == '\0')
   {
      ontext = UNCRUSTIFY_ON_TEXT;
   }
   if (pc.str.find(ontext) < 0)
   {
      pc.type = CT_IGNORED;
      return(true);
   }
   ctx.restore();

   // parse off whitespace leading to the comment
   if (parse_whitespace(ctx, pc))
   {
      pc.type = CT_IGNORED;
      return(true);
   }

   // Look for the ending comment and let it pass
   if (parse_comment(ctx, pc) && !cpd.unc_off)
   {
      return(true);
   }

   // Reset the chunk & scan to until a newline
   pc.str.clear();
   while (  ctx.more()
         && (ctx.peek() != '\r')
         && (ctx.peek() != '\n'))
   {
      pc.str.append(ctx.get());
   }
   if (pc.str.size() > 0)
   {
      pc.type = CT_IGNORED;
      return(true);
   }
   return(false);
} // parse_ignored


static bool parse_next(tok_ctx &ctx, chunk_t &pc)
{
   if (!ctx.more())
   {
      return(false);
   }

   // Save off the current column
   pc.orig_line = ctx.c.row;
   pc.column    = ctx.c.col;
   pc.orig_col  = ctx.c.col;
   pc.type      = CT_NONE;
   pc.nl_count  = 0;
   pc.flags     = 0;

   // If it is turned off, we put everything except newlines into CT_UNKNOWN
   if (cpd.unc_off)
   {
      if (parse_ignored(ctx, pc))
      {
         return(true);
      }
   }

   // Parse whitespace
   if (parse_whitespace(ctx, pc))
   {
      return(true);
   }

   // Handle unknown/unhandled preprocessors
   if (cpd.in_preproc > CT_PP_BODYCHUNK && cpd.in_preproc <= CT_PP_OTHER)
   {
      pc.str.clear();
      tok_info ss;
      ctx.save(ss);
      // Chunk to a newline or comment
      pc.type = CT_PREPROC_BODY;
      size_t last = 0;
      while (ctx.more())
      {
         size_t ch = ctx.peek();

         if ((ch == '\n') || (ch == '\r'))
         {
            // Back off if this is an escaped newline
            if (last == '\\')
            {
               ctx.restore(ss);
               pc.str.pop_back();
            }
            break;
         }

         // Quit on a C++ comment start
         if ((ch == '/') && (ctx.peek(1) == '/'))
         {
            break;
         }
         last = ch;
         ctx.save(ss);

         pc.str.append(ctx.get());
      }
      if (pc.str.size() > 0)
      {
         return(true);
      }
   }

   // Detect backslash-newline
   if ((ctx.peek() == '\\') && parse_bs_newline(ctx, pc))
   {
      return(true);
   }

   // Parse comments
   if (parse_comment(ctx, pc))
   {
      return(true);
   }

   // Parse code placeholders
   if (parse_code_placeholder(ctx, pc))
   {
      return(true);
   }

   if (cpd.lang_flags & LANG_CS)
   {
      if (parse_cs_string(ctx, pc))
      {
         //parse_cs_string(ctx, pc);
         return(true);
      }
      // check for non-keyword identifiers such as @if @switch, etc
      if ((ctx.peek() == '@') && CharTable::IsKw1(ctx.peek(1)))
      {
         parse_word(ctx, pc, true);
         return(true);
      }
   }

   // handle VALA """ strings """
   if (  (cpd.lang_flags & LANG_VALA)
      && (ctx.peek() == '"')
      && (ctx.peek(1) == '"')
      && (ctx.peek(2) == '"'))
   {
      parse_verbatim_string(ctx, pc);
      return(true);
   }

   /*
    * handle C++(11) string/char literal prefixes u8|u|U|L|R including all
    * possible combinations and optional R delimiters: R"delim(x)delim"
    */
   auto ch = ctx.peek();
   if (  ((cpd.lang_flags & LANG_CPP) || (cpd.lang_flags & LANG_C))
      && (  ch == 'u'
         || ch == 'U'
         || ch == 'R'
         || ch == 'L'))
   {
      auto idx     = size_t {};
      auto is_real = false;

      if (ch == 'u' && ctx.peek(1) == '8')
      {
         idx = 2;
      }
      else if (unc_tolower(ch) == 'u' || ch == 'L')
      {
         idx++;
      }

      if (  ((cpd.lang_flags & LANG_CPP) || (cpd.lang_flags & LANG_C))
         && ctx.peek(idx) == 'R')
      {
         idx++;
         is_real = true;
      }

      const auto quote = ctx.peek(idx);

      if (is_real)
      {
         if (quote == '"' && parse_cr_string(ctx, pc, idx))
         {
            return(true);
         }
      }
      else if (  (quote == '"' || quote == '\'')
              && parse_string(ctx, pc, idx, true))
      {
         return(true);
      }
   }

   // PAWN specific stuff
   if (cpd.lang_flags & LANG_PAWN)
   {
      if (  cpd.preproc_ncnl_count == 1
         && (cpd.in_preproc == CT_PP_DEFINE || cpd.in_preproc == CT_PP_EMIT))
      {
         parse_pawn_pattern(ctx, pc, CT_MACRO);
         return(true);
      }
      // Check for PAWN strings: \"hi" or !"hi" or !\"hi" or \!"hi"
      if ((ctx.peek() == '\\') || (ctx.peek() == '!'))
      {
         if (ctx.peek(1) == '"')
         {
            parse_string(ctx, pc, 1, (ctx.peek() == '!'));
            return(true);
         }

         if (  ((ctx.peek(1) == '\\') || (ctx.peek(1) == '!'))
            && (ctx.peek(2) == '"'))
         {
            parse_string(ctx, pc, 2, false);
            return(true);
         }
      }

      // handle PAWN preprocessor args %0 .. %9
      if (  cpd.in_preproc == CT_PP_DEFINE
         && (ctx.peek() == '%')
         && unc_isdigit(ctx.peek(1)))
      {
         pc.str.clear();
         pc.str.append(ctx.get());
         pc.str.append(ctx.get());
         pc.type = CT_WORD;
         return(true);
      }
   }

   // Parse strings and character constants

//parse_word(ctx, pc_temp, true);
//ctx.restore(ctx.c);
   if (parse_number(ctx, pc))
   {
      return(true);
   }

   if (cpd.lang_flags & LANG_D)
   {
      // D specific stuff
      if (d_parse_string(ctx, pc))
      {
         return(true);
      }
   }
   else
   {
      // Not D stuff

      // Check for L'a', L"abc", 'a', "abc", <abc> strings
      ch = ctx.peek();
      size_t ch1 = ctx.peek(1);
      if (  (  ((ch == 'L') || (ch == 'S'))
            && ((ch1 == '"') || (ch1 == '\'')))
         || (ch == '"')
         || (ch == '\'')
         || ((ch == '<') && cpd.in_preproc == CT_PP_INCLUDE))
      {
         parse_string(ctx, pc, unc_isalpha(ch) ? 1 : 0, true);
         return(true);
      }

      if ((ch == '<') && cpd.in_preproc == CT_PP_DEFINE)
      {
         if (chunk_get_tail() != nullptr && chunk_get_tail()->type == CT_MACRO)
         {
            // We have "#define XXX <", assume '<' starts an include string
            parse_string(ctx, pc, 0, false);
            return(true);
         }
      }

      /* Inside clang's __has_include() could be "path/to/file.h" or system-style <path/to/file.h> */
      if ((ch == '(') && ((chunk_get_tail()->type == CT_CNG_HASINC) || (chunk_get_tail()->type == CT_CNG_HASINCN)))
      {
         parse_string(ctx, pc, 0, false);
         return(true);
      }
   }

   // Check for Objective C literals and VALA identifiers ('@1', '@if')
   if ((cpd.lang_flags & (LANG_OC | LANG_VALA)) && (ctx.peek() == '@'))
   {
      size_t nc = ctx.peek(1);
      if ((nc == '"') || (nc == '\''))
      {
         // literal string
         parse_string(ctx, pc, 1, true);
         return(true);
      }

      if ((nc >= '0') && (nc <= '9'))
      {
         // literal number
         pc.str.append(ctx.get());  // store the '@'
         parse_number(ctx, pc);
         return(true);
      }
   }

   // Check for pawn/ObjectiveC/Java and normal identifiers
   if (  CharTable::IsKw1(ctx.peek())
      || ((ctx.peek() == '\\') && (unc_tolower(ctx.peek(1)) == 'u'))
      || ((ctx.peek() == '@') && CharTable::IsKw1(ctx.peek(1))))
   {
      parse_word(ctx, pc, false);
      return(true);
   }

   // see if we have a punctuator
   char punc_txt[7];
   punc_txt[0] = ctx.peek();
   punc_txt[1] = ctx.peek(1);
   punc_txt[2] = ctx.peek(2);
   punc_txt[3] = ctx.peek(3);
   punc_txt[4] = ctx.peek(4);
   punc_txt[5] = ctx.peek(5);
   punc_txt[6] = '\0';
   const chunk_tag_t *punc;
   if ((punc = find_punctuator(punc_txt, cpd.lang_flags)) != nullptr)
   {
      int cnt = strlen(punc->tag);
      while (cnt--)
      {
         pc.str.append(ctx.get());
      }
      pc.type   = punc->type;
      pc.flags |= PCF_PUNCTUATOR;
      return(true);
   }

   /* When parsing C/C++ files and running into some unknown token,
    * check if matches Objective-C as a last resort, before
    * considering it as garbage.
    */
   int probe_lang_flags = 0;
   if (cpd.lang_flags & (LANG_C | LANG_CPP))
   {
      probe_lang_flags = cpd.lang_flags | LANG_OC;
   }

   if (probe_lang_flags != 0)
   {
      if ((punc = find_punctuator(punc_txt, probe_lang_flags)) != NULL)
      {
         cpd.lang_flags = probe_lang_flags;
         int cnt = strlen(punc->tag);
         while (cnt--)
         {
            pc.str.append(ctx.get());
         }
         pc.type   = punc->type;
         pc.flags |= PCF_PUNCTUATOR;
         return(true);
      }
   }

   // throw away this character
   pc.type = CT_UNKNOWN;
   pc.str.append(ctx.get());

   LOG_FMT(LWARN, "%s:%zu Garbage in col %d: %x\n",
           cpd.filename.c_str(), pc.orig_line, (int)ctx.c.col, pc.str[0]);
   cpd.error_count++;
   return(true);
} // parse_next


void tokenize(const deque<int> &data, chunk_t *ref)
{
   tok_ctx ctx(data);
   chunk_t chunk;
   chunk_t *pc          = nullptr;
   chunk_t *rprev       = nullptr;
   bool    last_was_tab = false;
   size_t  prev_sp      = 0;

   cpd.unc_stage = unc_stage_e::TOKENIZE;

   while (ctx.more())
   {
      chunk.reset();
      if (!parse_next(ctx, chunk))
      {
         LOG_FMT(LERR, "%s:%zu Bailed before the end?\n",
                 cpd.filename.c_str(), ctx.c.row);
         cpd.error_count++;
         break;
      }

      // Don't create an entry for whitespace
      if (chunk.type == CT_WHITESPACE)
      {
         last_was_tab = chunk.after_tab;
         prev_sp      = chunk.orig_prev_sp;
         continue;
      }
      chunk.orig_prev_sp = prev_sp;
      prev_sp            = 0;

      if (chunk.type == CT_NEWLINE)
      {
         last_was_tab    = chunk.after_tab;
         chunk.after_tab = false;
         chunk.str.clear();
      }
      else if (chunk.type == CT_NL_CONT)
      {
         last_was_tab    = chunk.after_tab;
         chunk.after_tab = false;
         chunk.str       = "\\\n";
      }
      else
      {
         chunk.after_tab = last_was_tab;
         last_was_tab    = false;
      }

      if (chunk.type != CT_IGNORED)
      {
         // Issue #1338
         // Strip trailing whitespace (for CPP comments and PP blocks)
         while (  (chunk.str.size() > 0)
               && (  (chunk.str[chunk.str.size() - 1] == ' ')
                  || (chunk.str[chunk.str.size() - 1] == '\t')))
         {
            // If comment contains backslash '\' followed by whitespace chars, keep last one;
            // this will prevent it from turning '\' into line continuation.
            if ((chunk.str.size() > 1) && (chunk.str[chunk.str.size() - 2] == '\\'))
            {
               break;
            }
            chunk.str.pop_back();
         }
      }

      // Store off the end column
      chunk.orig_col_end = ctx.c.col;

      // Add the chunk to the list
      rprev = pc;
      if (rprev != nullptr)
      {
         chunk_flags_set(pc, rprev->flags & PCF_COPY_FLAGS);

         // a newline can't be in a preprocessor
         if (pc->type == CT_NEWLINE)
         {
            chunk_flags_clr(pc, PCF_IN_PREPROC);
         }
      }
      if (ref != nullptr)
      {
         chunk.flags |= PCF_INSERTED;
      }
      else
      {
         chunk.flags &= ~PCF_INSERTED;
      }
      pc = chunk_add_before(&chunk, ref);

      // A newline marks the end of a preprocessor
      if (pc->type == CT_NEWLINE) // || pc->type == CT_COMMENT_MULTI)
      {
         cpd.in_preproc         = CT_NONE;
         cpd.preproc_ncnl_count = 0;
      }

      // Disable indentation when #asm directive found
      if (pc->type == CT_PP_ASM)
      {
         LOG_FMT(LBCTRL, "Found a directive %s on line %zu\n", "#asm", pc->orig_line);
         cpd.unc_off = true;
      }

      // Special handling for preprocessor stuff
      if (cpd.in_preproc != CT_NONE)
      {
         chunk_flags_set(pc, PCF_IN_PREPROC);

         // Count words after the preprocessor
         if (!chunk_is_comment(pc) && !chunk_is_newline(pc))
         {
            cpd.preproc_ncnl_count++;
         }

         // Disable indentation if a #pragma asm directive is found
         if (cpd.in_preproc == CT_PP_PRAGMA)
         {
            if (memcmp(pc->text(), "asm", 3) == 0)
            {
               LOG_FMT(LBCTRL, "Found a pragma %s on line %zu\n", "asm", pc->orig_line);
               cpd.unc_off = true;
            }
         }

         // Figure out the type of preprocessor for #include parsing
         if (cpd.in_preproc == CT_PREPROC)
         {
            if (pc->type < CT_PP_DEFINE || pc->type > CT_PP_OTHER)
            {
               set_chunk_type(pc, CT_PP_OTHER);
            }
            cpd.in_preproc = pc->type;
         }
         else if (cpd.in_preproc == CT_PP_IGNORE)
         {
            // ASSERT(cpd.settings[UO_pp_ignore_define_body].b);
            if (pc->type != CT_NL_CONT && pc->type != CT_COMMENT_CPP)
            {
               set_chunk_type(pc, CT_PP_IGNORE);
            }
         }
         else if (  cpd.in_preproc == CT_PP_DEFINE && pc->type == CT_PAREN_CLOSE
                 && cpd.settings[UO_pp_ignore_define_body].b)
         {
            // When we have a PAREN_CLOSE in a PP_DEFINE we should be terminating a MACRO_FUNC
            // arguments list. Therefore we can enter the PP_IGNORE state and ignore next chunks.
            cpd.in_preproc = CT_PP_IGNORE;
         }
         else if (cpd.in_preproc == CT_PP_IGNORE)
         {
            // ASSERT(cpd.settings[UO_pp_ignore_define_body].b);
            if (pc->type != CT_NL_CONT && pc->type != CT_COMMENT_CPP)
            {
               set_chunk_type(pc, CT_PP_IGNORE);
            }
         }
         else if (  cpd.in_preproc == CT_PP_DEFINE
                 && pc->type == CT_PAREN_CLOSE
                 && cpd.settings[UO_pp_ignore_define_body].b)
         {
            // When we have a PAREN_CLOSE in a PP_DEFINE we should be terminating a MACRO_FUNC
            // arguments list. Therefore we can enter the PP_IGNORE state and ignore next chunks.
            cpd.in_preproc = CT_PP_IGNORE;
         }
      }
      else
      {
         // Check for a preprocessor start
         if (  pc->type == CT_POUND
            && (rprev == nullptr || rprev->type == CT_NEWLINE))
         {
            set_chunk_type(pc, CT_PREPROC);
            pc->flags     |= PCF_IN_PREPROC;
            cpd.in_preproc = CT_PREPROC;
         }
      }
      if (pc->type == CT_NEWLINE)
      {
         LOG_FMT(LGUY, "%s(%d): orig_line is %zu, orig_col is %zu, <Newline>, nl is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->nl_count);
      }
      else if (pc->type == CT_VBRACE_OPEN)
      {
         LOG_FMT(LGUY, "%s(%d): orig_line is %zu, orig_col is %zu, type is %s, orig_col_end is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, get_token_name(pc->type), pc->orig_col_end);
      }
      else
      {
         LOG_FMT(LGUY, "%s(%d): orig_line is %zu, orig_col is %zu, text() '%s', type is %s, orig_col_end is %zu\n",
                 __func__, __LINE__, pc->orig_line, pc->orig_col, pc->text(), get_token_name(pc->type), pc->orig_col_end);
      }
   }

   // Set the cpd.newline string for this file
   if (  cpd.settings[UO_newlines].le == LE_LF
      || (  cpd.settings[UO_newlines].le == LE_AUTO
         && (cpd.le_counts[LE_LF] >= cpd.le_counts[LE_CRLF])
         && (cpd.le_counts[LE_LF] >= cpd.le_counts[LE_CR])))
   {
      // LF line ends
      cpd.newline = "\n";
      LOG_FMT(LLINEENDS, "Using LF line endings\n");
   }
   else if (  cpd.settings[UO_newlines].le == LE_CRLF
           || (  cpd.settings[UO_newlines].le == LE_AUTO
              && (cpd.le_counts[LE_CRLF] >= cpd.le_counts[LE_LF])
              && (cpd.le_counts[LE_CRLF] >= cpd.le_counts[LE_CR])))
   {
      // CRLF line ends
      cpd.newline = "\r\n";
      LOG_FMT(LLINEENDS, "Using CRLF line endings\n");
   }
   else
   {
      // CR line ends
      cpd.newline = "\r";
      LOG_FMT(LLINEENDS, "Using CR line endings\n");
   }
} // tokenize
