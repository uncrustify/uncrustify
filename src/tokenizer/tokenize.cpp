/**
 * @file tokenize.cpp
 * This file breaks up the text stream into tokens or chunks.
 *
 * Each routine needs to set pc.len and pc.type.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "tokenizer/tokenize.h"

#include "keywords.h"
#include "prototypes.h"
#include "punctuators.h"
#include "unc_ctype.h"

#include <regex>
#include <stack>


#define LE_COUNT(x)    cpd.le_counts[static_cast<size_t>(LE_ ## x)]


constexpr static auto LCURRENT = LTOK;


using namespace std;
using namespace uncrustify;


struct TokenInfo
{
   TokenInfo()
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


struct TokenContext
{
   TokenContext(const deque<int> &d)
      : data(d)
   {
   }


   //! save before trying to parse something that may fail
   void save()
   {
      save(s);
   }


   void save(TokenInfo &info)
   {
      info = c;
   }


   //! restore previous saved state
   void restore()
   {
      restore(s);
   }


   void restore(const TokenInfo &info)
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
            log_rule_B("input_tab_size");
            c.col = calc_next_tab_column(c.col, options::input_tab_size());
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
   TokenInfo        c; //! current
   TokenInfo        s; //! saved
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
static bool parse_string(TokenContext &ctx, Chunk &pc, size_t quote_idx, bool allow_escape);


/**
 * Literal string, ends with single "
 * Two "" don't end the string.
 *
 * @param pc  The structure to update, str is an input.
 *
 * @return Whether a string was parsed
 */
static bool parse_cs_string(TokenContext &ctx, Chunk &pc);


/**
 * VALA verbatim string, ends with three quotes (""")
 *
 * @param pc  The structure to update, str is an input.
 */
static void parse_verbatim_string(TokenContext &ctx, Chunk &pc);


static bool tag_compare(const deque<int> &d, size_t a_idx, size_t b_idx, size_t len);


/**
 * Parses a C++0x 'R' string. R"( xxx )" R"tag(  )tag" u8R"(x)" uR"(x)"
 * Newlines may be in the string.
 *
 * @param pc  structure to update, str is an input.
 */
static bool parse_cr_string(TokenContext &ctx, Chunk &pc, size_t q_idx);


/**
 * Count the number of whitespace characters.
 *
 * @param pc  The structure to update, str is an input.
 *
 * @return Whether whitespace was parsed
 */
static bool parse_whitespace(TokenContext &ctx, Chunk &pc);


/**
 * Called when we hit a backslash.
 * If there is nothing but whitespace until the newline, then this is a
 * backslash newline
 *
 * @param pc  structure to update, str is an input
 */
static bool parse_bs_newline(TokenContext &ctx, Chunk &pc);


/**
 * Parses any number of tab or space chars followed by a newline.
 * Does not change pc.len if a newline isn't found.
 * This is not the same as parse_whitespace() because it only consumes until
 * a single newline is encountered.
 */
static bool parse_newline(TokenContext &ctx);


/**
 * PAWN #define is different than C/C++.
 *   #define PATTERN REPLACEMENT_TEXT
 * The PATTERN may not contain a space or '[' or ']'.
 * A generic whitespace check should be good enough.
 * Do not change the pattern.
 *
 * @param pc  structure to update, str is an input
 */
static void parse_pawn_pattern(TokenContext &ctx, Chunk &pc, E_Token tt);


static bool parse_ignored(TokenContext &ctx, Chunk &pc);


/**
 * Skips the next bit of whatever and returns the type of block.
 *
 * pc.str is the input text.
 * pc.len in the output length.
 * pc.type is the output type
 * pc.column is output column
 *
 * @param pc  The structure to update, str is an input.
 * @param prev_pc  The previous structure
 *
 * @return true/false - whether anything was parsed
 */
static bool parse_next(TokenContext &ctx, Chunk &pc, const Chunk *prev_pc);


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
static bool d_parse_string(TokenContext &ctx, Chunk &pc);


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
static bool parse_comment(TokenContext &ctx, Chunk &pc);


/**
 * Figure of the length of the code placeholder at text, if present.
 * This is only for Xcode which sometimes inserts temporary code placeholder chunks, which in plaintext <#look like this#>.
 *
 * @param pc  The structure to update, str is an input.
 *
 * @return Whether a placeholder was parsed.
 */
static bool parse_code_placeholder(TokenContext &ctx, Chunk &pc);


/**
 * Parse any attached suffix, which may be a user-defined literal suffix.
 * If for a string, explicitly exclude common format and scan specifiers, ie,
 * PRIx32 and SCNx64.
 */
static void parse_suffix(TokenContext &ctx, Chunk &pc, bool forstring);


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
static bool parse_number(TokenContext &ctx, Chunk &pc);


static bool d_parse_string(TokenContext &ctx, Chunk &pc)
{
   size_t ch = ctx.peek();

   if (  ch == '"'             // 34
      || ch == '\'')           // 39
   {
      return(parse_string(ctx, pc, 0, true));
   }

   if (ch == '`')              // 96
   {
      return(parse_string(ctx, pc, 0, false));
   }

   if (  (  ch == 'r'          // 114
         || ch == 'x')         // 120
      && ctx.peek(1) == '"')   //  34
   {
      return(parse_string(ctx, pc, 1, false));
   }

   if (ch != '\\')
   {
      return(false);
   }
   ctx.save();
   int cnt;

   pc.Str().clear();

   while (ctx.peek() == '\\')   // 92
   {
      pc.Str().append(ctx.get());

      // Check for end of file
      switch (ctx.peek())
      {
      case 'x':  // \x HexDigit HexDigit
         cnt = 3;

         while (cnt--)
         {
            pc.Str().append(ctx.get());
         }
         break;

      case 'u':  // \u HexDigit (x4)
         cnt = 5;

         while (cnt--)
         {
            pc.Str().append(ctx.get());
         }
         break;

      case 'U':  // \U HexDigit (x8)
         cnt = 9;

         while (cnt--)
         {
            pc.Str().append(ctx.get());
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
         pc.Str().append(ctx.get());
         ch = ctx.peek();

         if (  (ch >= '0')
            && (ch <= '7'))
         {
            pc.Str().append(ctx.get());
            ch = ctx.peek();

            if (  (ch >= '0')
               && (ch <= '7'))
            {
               pc.Str().append(ctx.get());
            }
         }
         break;

      case '&':
         // \& NamedCharacterEntity ;
         pc.Str().append(ctx.get());

         while (unc_isalpha(ctx.peek()))
         {
            pc.Str().append(ctx.get());
         }

         if (ctx.peek() == ';')          // 59
         {
            pc.Str().append(ctx.get());
         }
         break;

      default:
         // Everything else is a single character
         pc.Str().append(ctx.get());
         break;
      } // switch
   }

   if (pc.GetStr().size() < 1)
   {
      ctx.restore();
      return(false);
   }
   pc.SetType(CT_STRING);
   return(true);
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


static bool parse_comment(TokenContext &ctx, Chunk &pc)
{
   bool   is_d    = language_is_set(lang_flag_e::LANG_D);
   bool   is_cs   = language_is_set(lang_flag_e::LANG_CS);
   size_t d_level = 0;

   // does this start with '/ /' or '/ *' or '/ +' (d)
   if (  (ctx.peek() != '/')
      || (  (ctx.peek(1) != '*')
         && (ctx.peek(1) != '/')
         && (  (ctx.peek(1) != '+')
            || !is_d)))
   {
      return(false);
   }
   ctx.save();

   // account for opening two chars
   pc.Str() = ctx.get();   // opening '/'
   size_t ch = ctx.get();

   pc.Str().append(ch);    // second char

   if (ch == '/')          // 47
   {
      pc.SetType(CT_COMMENT_CPP);

      while (true)
      {
         int bs_cnt = 0;

         while (ctx.more())
         {
            ch = ctx.peek();

            if (  (ch == '\r')
               || (ch == '\n'))
            {
               break;
            }

            if (  (ch == '\\') // 92
               && !is_cs)      // backslashes aren't special in comments in C#
            {
               bs_cnt++;
            }
            else
            {
               bs_cnt = 0;
            }
            pc.Str().append(ctx.get());
         }

         /*
          * If we hit an odd number of backslashes right before the newline,
          * then we keep going.
          */
         if (  ((bs_cnt & 1) == 0)
            || !ctx.more())
         {
            break;
         }

         if (ctx.peek() == '\r')
         {
            pc.Str().append(ctx.get());
         }

         if (ctx.peek() == '\n')
         {
            pc.Str().append(ctx.get());
         }
         pc.SetNlCount(pc.GetNlCount() + 1);
         cpd.did_newline = true;
      }
   }
   else if (!ctx.more())
   {
      // unexpected end of file
      ctx.restore();
      return(false);
   }
   else if (ch == '+')                         // 43
   {
      pc.SetType(CT_COMMENT);
      d_level++;

      while (  d_level > 0
            && ctx.more())
      {
         if (  (ctx.peek() == '+')             // 43
            && (ctx.peek(1) == '/'))           // 47
         {
            pc.Str().append(ctx.get());  // store the '+'
            pc.Str().append(ctx.get());  // store the '/'
            d_level--;
            continue;
         }

         if (  (ctx.peek() == '/')           // 47
            && (ctx.peek(1) == '+'))         // 43
         {
            pc.Str().append(ctx.get());  // store the '/'
            pc.Str().append(ctx.get());  // store the '+'
            d_level++;
            continue;
         }
         ch = ctx.get();
         pc.Str().append(ch);

         if (  (ch == '\n')
            || (ch == '\r'))
         {
            pc.SetType(CT_COMMENT_MULTI);
            pc.SetNlCount(pc.GetNlCount() + 1);

            if (ch == '\r')
            {
               if (ctx.peek() == '\n')
               {
                  ++LE_COUNT(CRLF);
                  pc.Str().append(ctx.get());  // store the '\n'
               }
               else
               {
                  ++LE_COUNT(CR);
               }
            }
            else
            {
               ++LE_COUNT(LF);
            }
         }
      }
   }
   else  // must be '/ *'
   {
      pc.SetType(CT_COMMENT);

      while (ctx.more())
      {
         if (  (ctx.peek() == '*')         // 43
            && (ctx.peek(1) == '/'))       // 47
         {
            pc.Str().append(ctx.get());  // store the '*'
            pc.Str().append(ctx.get());  // store the '/'

            TokenInfo ss;
            ctx.save(ss);
            size_t    oldsize = pc.GetStr().size();

            // If there is another C comment right after this one, combine them
            while (  (ctx.peek() == ' ')         // 32
                  || (ctx.peek() == '\t'))       // tab
            {
               pc.Str().append(ctx.get());
            }

            if (  (ctx.peek() != '/')
               || (ctx.peek(1) != '*'))
            {
               // undo the attempt to join
               ctx.restore(ss);
               pc.Str().resize(oldsize);
               break;
            }
         }
         ch = ctx.get();
         pc.Str().append(ch);

         if (  (ch == '\n')
            || (ch == '\r'))
         {
            pc.SetType(CT_COMMENT_MULTI);
            pc.SetNlCount(pc.GetNlCount() + 1);

            if (ch == '\r')
            {
               if (ctx.peek() == '\n')
               {
                  ++LE_COUNT(CRLF);
                  pc.Str().append(ctx.get());  // store the '\n'
               }
               else
               {
                  ++LE_COUNT(CR);
               }
            }
            else
            {
               ++LE_COUNT(LF);
            }
         }
      }
   }

   if (cpd.unc_off)
   {
      bool found_enable_marker = (find_enable_processing_comment_marker(pc.GetStr()) >= 0);

      if (found_enable_marker)
      {
         const auto &ontext = options::enable_processing_cmt();

         LOG_FMT(LBCTRL, "%s(%d): Found '%s' on line %zu\n",
                 __func__, __LINE__, ontext.c_str(), pc.GetOrigLine());
         cpd.unc_off = false;
      }
   }
   else
   {
      auto position_disable_processing_cmt = find_disable_processing_comment_marker(pc.GetStr());
      bool found_disable_marker            = (position_disable_processing_cmt >= 0);

      if (found_disable_marker)
      {
         /**
          * the user may wish to disable processing part of a multiline comment,
          * in which case we'll handle at a late time. Check to see if processing
          * is re-enabled elsewhere in this comment
          */
         auto position_enable_processing_cmt = find_enable_processing_comment_marker(pc.GetStr());

         if (position_enable_processing_cmt < position_disable_processing_cmt)
         {
            const auto &offtext = options::disable_processing_cmt();

            LOG_FMT(LBCTRL, "%s(%d): Found '%s' on line %zu\n",
                    __func__, __LINE__, offtext.c_str(), pc.GetOrigLine());
            cpd.unc_off = true;
            // Issue #842
            cpd.unc_off_used = true;
         }
      }
   }
   return(true);
} // parse_comment


static bool parse_code_placeholder(TokenContext &ctx, Chunk &pc)
{
   if (  (ctx.peek() != '<')
      || (ctx.peek(1) != '#'))
   {
      return(false);
   }
   ctx.save();

   // account for opening two chars '<#'
   pc.Str() = ctx.get();
   pc.Str().append(ctx.get());

   // grab everything until '#>', fail if not found.
   size_t last1 = 0;

   while (ctx.more())
   {
      size_t last2 = last1;
      last1 = ctx.get();
      pc.Str().append(last1);

      if (  (last2 == '#')            // 35
         && (last1 == '>'))           // 62
      {
         pc.SetType(CT_WORD);
         return(true);
      }
   }
   ctx.restore();
   return(false);
}


static void parse_suffix(TokenContext &ctx, Chunk &pc, bool forstring = false)
{
   if (CharTable::IsKw1(ctx.peek()))
   {
      size_t slen    = 0;
      size_t oldsize = pc.GetStr().size();

      // don't add the suffix if we see L" or L' or S"
      size_t p1 = ctx.peek();
      size_t p2 = ctx.peek(1);

      if (  forstring
         && (  (  (p1 == 'L')          // 76
               && (  (p2 == '"')       // 34
                  || (p2 == '\'')))    // 39
            || (  (p1 == 'S')          // 83
               && (p2 == '"'))))       // 34
      {
         return;
      }
      TokenInfo ss;
      ctx.save(ss);

      while (  ctx.more()
            && CharTable::IsKw2(ctx.peek()))
      {
         slen++;
         pc.Str().append(ctx.get());
      }

      if (  forstring
         && slen >= 4
         && (  pc.GetStr().startswith("PRI", oldsize)
            || pc.GetStr().startswith("SCN", oldsize)))
      {
         ctx.restore(ss);
         pc.Str().resize(oldsize);
      }
   }
} // parse_suffix


static bool is_bin(int ch)
{
   return(  (ch == '0')           // 48
         || (ch == '1'));         // 49
}


static bool is_bin_(int ch)
{
   return(  is_bin(ch)
         || ch == '_'            // 95
         || ch == '\'');         // 39
}


static bool is_oct(int ch)
{
   return(  (ch >= '0')         // 48
         && (ch <= '7'));       // 55
}


static bool is_oct_(int ch)
{
   return(  is_oct(ch)
         || ch == '_'            // 95
         || ch == '\'');         // 39
}


static bool is_dec(int ch)
{
   return(  (ch >= '0')          // 48
         && (ch <= '9'));        // 57
}


static bool is_dec_(int ch)
{
   // number separators: JAVA: "_", C++14: "'"
   return(  is_dec(ch)
         || (ch == '_')           // 95
         || (ch == '\''));        // 39
}


static bool is_hex(int ch)
{
   return(  (  (ch >= '0')            // 48
            && (ch <= '9'))           // 57
         || (  (ch >= 'a')            // 97
            && (ch <= 'f'))           // 102
         || (  (ch >= 'A')            // 65
            && (ch <= 'F')));         // 70
}


static bool is_hex_(int ch)
{
   return(  is_hex(ch)
         || ch == '_'              // 95
         || ch == '\'');           // 39
}


static bool parse_number(TokenContext &ctx, Chunk &pc)
{
   /*
    * A number must start with a digit or a dot, followed by a digit
    * (signs handled elsewhere)
    */
   if (  !is_dec(ctx.peek())
      && (  (ctx.peek() != '.')         // 46
         || !is_dec(ctx.peek(1))))
   {
      return(false);
   }
   bool is_float = (ctx.peek() == '.');         // 46

   if (  is_float
      && (ctx.peek(1) == '.')) // make sure it isn't '..'  46
   {
      return(false);
   }
   /*
    * Check for Hex, Octal, or Binary
    * Note that only D, C++14 and Pawn support binary
    * Fixes the issue # 1591
    * In c# the numbers starting with 0 are not treated as octal numbers.
    */
   bool did_hex = false;

   if (  ctx.peek() == '0'                   // 48
      && !language_is_set(lang_flag_e::LANG_CS))
   {
      size_t ch;
      Chunk  pc_temp;

      pc.Str().append(ctx.get());  // store the '0'
      pc_temp.Str().append('0');

      // MS constant might have an "h" at the end. Look for it
      ctx.save();

      while (  ctx.more()
            && CharTable::IsKw2(ctx.peek()))
      {
         ch = ctx.get();
         pc_temp.Str().append(ch);
      }
      ch = pc_temp.GetStr()[pc_temp.Len() - 1];
      ctx.restore();
      LOG_FMT(LBCTRL, "%s(%d): pc_temp:%s\n", __func__, __LINE__, pc_temp.Text());

      if (ch == 'h') // TODO can we combine this in analyze_character  104
      {
         // we have an MS hexadecimal number with "h" at the end
         LOG_FMT(LBCTRL, "%s(%d): MS hexadecimal number\n", __func__, __LINE__);
         did_hex = true;

         do
         {
            pc.Str().append(ctx.get()); // store the rest
         } while (is_hex_(ctx.peek()));

         pc.Str().append(ctx.get());    // store the h
         LOG_FMT(LBCTRL, "%s(%d): pc:%s\n", __func__, __LINE__, pc.Text());
      }
      else
      {
         switch (unc_toupper(ctx.peek()))
         {
         case 'X':               // hex
            did_hex = true;

            do
            {
               pc.Str().append(ctx.get());  // store the 'x' and then the rest
            } while (is_hex_(ctx.peek()));

            break;

         case 'B':               // binary

            do
            {
               pc.Str().append(ctx.get());  // store the 'b' and then the rest
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
               pc.Str().append(ctx.get());
            } while (is_oct_(ctx.peek()));

            break;

         default:
            // either just 0 or 0.1 or 0UL, etc
            break;
         } // switch
      }
   }
   else
   {
      // Regular int or float
      while (is_dec_(ctx.peek()))
      {
         pc.Str().append(ctx.get());
      }
   }

   // Check if we stopped on a decimal point & make sure it isn't '..'
   if (  (ctx.peek() == '.')                 // 46
      && (ctx.peek(1) != '.'))               // 46
   {
      pc.Str().append(ctx.get());
      is_float = true;

      if (did_hex)
      {
         while (is_hex_(ctx.peek()))
         {
            pc.Str().append(ctx.get());
         }
      }
      else
      {
         while (is_dec_(ctx.peek()))
         {
            pc.Str().append(ctx.get());
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

   if (  (tmp == 'E')                 // 69
      || (tmp == 'P'))                // 80
   {
      is_float = true;
      pc.Str().append(ctx.get());

      if (  (ctx.peek() == '+')          // 43
         || (ctx.peek() == '-'))         // 45
      {
         pc.Str().append(ctx.get());
      }

      while (is_dec_(ctx.peek()))
      {
         pc.Str().append(ctx.get());
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

      // https://en.cppreference.com/w/cpp/language/floating_literal
      if (  (tmp2 == 'I')    // 73
         || (tmp2 == 'F')    // 70
         || (tmp2 == 'D')    // 68
         || (tmp2 == 'M'))   // 77
      {
         // is a decimal point found?                     Issue #4027
         const char *test_it    = pc.Text();
         size_t     test_long   = strlen(test_it);
         bool       point_found = false;

         for (size_t ind = 0; ind < test_long; ind++)
         {
            if (test_it[ind] == '.')
            {
               point_found = true;
               break;
            }
         }

         if (point_found)
         {
            is_float = true;
         }
         else
         {
            // append the char(s) until is not IsKw2
            while (ctx.more())
            {
               size_t ch = ctx.peek();

               if (CharTable::IsKw2(ch))
               {
                  pc.Str().append(ctx.get());
               }
               else
               {
                  break;
               }
            }
            pc.SetType(CT_WORD);
            return(true);
         }
      }
      else if (  (tmp2 != 'L')   // 76
              && (tmp2 != 'U'))  // 85
      {
         break;
      }
      pc.Str().append(ctx.get());
   }

   // skip the Microsoft-specific '8' suffix
   if ((ctx.peek() == '8'))      // 56
   {
      pc.Str().append(ctx.get());
   }

   // skip the Microsoft-specific '16', '32' and '64' suffix
   if (  (  (ctx.peek() == '1')     // 49
         && (ctx.peek(1) == '6'))   // 54
      || (  (ctx.peek() == '3')     // 51
         && (ctx.peek(1) == '2'))   // 50
      || (  (ctx.peek() == '6')     // 54
         && (ctx.peek(1) == '4')))  // 52
   {
      pc.Str().append(ctx.get());
      pc.Str().append(ctx.get());
   }

   // skip the Microsoft-specific '128' suffix
   if ((  (ctx.peek() == '1')      // 49
       && (ctx.peek(1) == '2')     // 50
       && (ctx.peek(2) == '8')))   // 56
   {
      pc.Str().append(ctx.get());
      pc.Str().append(ctx.get());
      pc.Str().append(ctx.get());
   }
   pc.SetType(is_float ? CT_NUMBER_FP : CT_NUMBER);

   /*
    * If there is anything left, then we are probably dealing with garbage or
    * some sick macro junk. Eat it.
    */
   parse_suffix(ctx, pc);

   return(true);
} // parse_number


static bool parse_string(TokenContext &ctx, Chunk &pc, size_t quote_idx, bool allow_escape)
{
   log_rule_B("string_escape_char");
   const size_t escape_char = options::string_escape_char();

   log_rule_B("string_escape_char2");
   const size_t escape_char2 = options::string_escape_char2();

   log_rule_B("string_replace_tab_chars");
   const bool should_escape_tabs = (  allow_escape
                                   && options::string_replace_tab_chars()
                                   && language_is_set(lang_flag_e::LANG_ALLC));

   pc.Str().clear();

   while (quote_idx-- > 0)
   {
      pc.Str().append(ctx.get());
   }
   pc.SetType(CT_STRING);
   const size_t termination_character = CharTable::Get(ctx.peek()) & 0xff;

   pc.Str().append(ctx.get());                          // store the "

   bool escaped = false;

   while (ctx.more())
   {
      const size_t ch = ctx.get();

      // convert char 9 (\t) to chars \t
      if (  (ch == '\t')
         && should_escape_tabs)
      {
         const size_t lastcol = ctx.c.col - 1;
         ctx.c.col = lastcol + 2;
         pc.Str().append(escape_char);
         pc.Str().append('t');
         continue;
      }
      pc.Str().append(ch);

      if (ch == '\n')
      {
         pc.SetNlCount(pc.GetNlCount() + 1);
         pc.SetType(CT_STRING_MULTI);
      }
      else if (  ch == '\r'
              && ctx.peek() != '\n')
      {
         pc.Str().append(ctx.get());
         pc.SetNlCount(pc.GetNlCount() + 1);
         pc.SetType(CT_STRING_MULTI);
      }

      // if last char in prev loop was escaped the one in the current loop isn't
      if (escaped)
      {
         escaped = false;
         continue;
      }

      // see if the current char is a escape char
      if (allow_escape)
      {
         if (ch == escape_char)
         {
            escaped = (escape_char != 0);
            continue;
         }

         if (  ch == escape_char2
            && (ctx.peek() == termination_character))
         {
            escaped = allow_escape;
            continue;
         }
      }

      if (ch == termination_character)
      {
         break;
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

static cs_string_t operator|=(cs_string_t &value, cs_string_t other)
{
   return(value = static_cast<cs_string_t>(value | other));
}


static cs_string_t parse_cs_string_start(TokenContext &ctx, Chunk &pc)
{
   cs_string_t stringType = CS_STRING_NONE;
   int         offset     = 0;

   if (ctx.peek(offset) == '$')                     // 36
   {
      stringType |= CS_STRING_INTERPOLATED;
      ++offset;
   }

   if (ctx.peek(offset) == '@')                     // 64
   {
      stringType |= CS_STRING_VERBATIM;
      ++offset;
   }

   if (ctx.peek(offset) == '"')                     // 34
   {
      stringType |= CS_STRING_STRING;

      pc.SetType(CT_STRING);

      for (int i = 0; i <= offset; ++i)
      {
         pc.Str().append(ctx.get());
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
static bool parse_cs_string(TokenContext &ctx, Chunk &pc)
{
   cs_string_t stringType = parse_cs_string_start(ctx, pc);

   if (stringType == CS_STRING_NONE)
   {
      return(false);
   }
   // an interpolated string can contain {expressions}, which can contain $"strings", which in turn
   // can contain {expressions}, so we must track both as they are interleaved, in order to properly
   // parse the outermost string.

   std::stack<CsStringParseState> parseState; // each entry is a nested string

   parseState.push(CsStringParseState(stringType));

   log_rule_B("string_replace_tab_chars");
   bool should_escape_tabs = options::string_replace_tab_chars();

   while (ctx.more())
   {
      if (parseState.top().braceDepth > 0)
      {
         // all we can do when in an expr is look for expr close with }, or a new string opening. must do this first
         // so we can peek and potentially consume chars for new string openings, before the ch=get() happens later,
         // which is needed for newline processing.

         if (ctx.peek() == '}')              // 125
         {
            pc.Str().append(ctx.get());

            if (ctx.peek() == '}')              // 125
            {
               pc.Str().append(ctx.get()); // in interpolated string, `}}` is escape'd `}`
            }
            else
            {
               --parseState.top().braceDepth;
            }
            continue;
         }
         stringType = parse_cs_string_start(ctx, pc);

         if (stringType != CS_STRING_NONE)
         {
            parseState.push(CsStringParseState(stringType));
            continue;
         }
      }
      int lastcol = ctx.c.col;
      int ch      = ctx.get();

      pc.Str().append(ch);

      if (ch == '\n')
      {
         pc.SetType(CT_STRING_MULTI);
         pc.SetNlCount(pc.GetNlCount() + 1);
      }
      else if (ch == '\r')
      {
         pc.SetType(CT_STRING_MULTI);
      }
      else if (parseState.top().braceDepth > 0)
      {
         // do nothing. if we're in a brace, we only want the newline handling, and skip the rest.
      }
      else if (  (ch == '\t')
              && should_escape_tabs)
      {
         if (parseState.top().type & CS_STRING_VERBATIM)
         {
            if (!cpd.warned_unable_string_replace_tab_chars)
            {
               cpd.warned_unable_string_replace_tab_chars = true;

               log_rule_B("warn_level_tabs_found_in_verbatim_string_literals");
               log_sev_t warnlevel = (log_sev_t)options::warn_level_tabs_found_in_verbatim_string_literals();

               /*
                * a tab char can't be replaced with \\t because escapes don't
                * work in here-strings. best we can do is warn.
                */
               LOG_FMT(warnlevel, "%s(%d): %s: orig line is %zu, orig col is %zu, Detected non-replaceable tab char in literal string\n",
                       __func__, __LINE__, cpd.filename.c_str(), pc.GetOrigLine(), pc.GetOrigCol());
               LOG_FMT(warnlevel, "%s(%d): Warning is given if doing tab-to-\\t replacement and we have found one in a C# verbatim string literal.\n",
                       __func__, __LINE__);

               if (warnlevel < LWARN)
               {
                  // TODO: replace the code ?? cpd.error_count++;
               }
            }
         }
         else
         {
            ctx.c.col = lastcol + 2;
            pc.Str().pop_back(); // remove \t
            pc.Str().append("\\t");

            continue;
         }
      }
      else if (  ch == '\\'
              && !(parseState.top().type & CS_STRING_VERBATIM))
      {
         // catch escaped quote in order to avoid ending string (but also must handle \\ to avoid accidental 'escape' seq of `\\"`)
         if (  ctx.peek() == '"'                  // 34
            || ctx.peek() == '\\')                // 92
         {
            pc.Str().append(ctx.get());
         }
      }
      else if (ch == '"')                           // 34
      {
         if (  (parseState.top().type & CS_STRING_VERBATIM)
            && (ctx.peek() == '"'))                           // 34
         {
            // in verbatim string, `""` is escape'd `"`
            pc.Str().append(ctx.get());
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
         if (ch == '{')                     // 123
         {
            if (ctx.peek() == '{')          // 123
            {
               pc.Str().append(ctx.get()); // in interpolated string, `{{` is escape'd `{`
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


static void parse_verbatim_string(TokenContext &ctx, Chunk &pc)
{
   pc.SetType(CT_STRING);

   // consume the initial """
   pc.Str() = ctx.get();
   pc.Str().append(ctx.get());
   pc.Str().append(ctx.get());

   // go until we hit a zero (end of file) or a """
   while (ctx.more())
   {
      size_t ch = ctx.get();
      pc.Str().append(ch);

      if (  (ch == '"')                      // 34
         && (ctx.peek() == '"')              // 34
         && (ctx.peek(1) == '"'))            // 34
      {
         pc.Str().append(ctx.get());
         pc.Str().append(ctx.get());
         break;
      }

      if (  (ch == '\n')
         || (ch == '\r'))
      {
         pc.SetType(CT_STRING_MULTI);
         pc.SetNlCount(pc.GetNlCount() + 1);
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


static bool parse_cr_string(TokenContext &ctx, Chunk &pc, size_t q_idx)
{
   size_t tag_idx = ctx.c.idx + q_idx + 1;
   size_t tag_len = 0;

   ctx.save();

   // Copy the prefix + " to the string
   pc.Str().clear();
   int cnt = q_idx + 1;

   while (cnt--)
   {
      pc.Str().append(ctx.get());
   }

   // Add the tag and get the length of the tag
   while (  ctx.more()
         && (ctx.peek() != '('))
   {
      tag_len++;
      pc.Str().append(ctx.get());
   }

   if (ctx.peek() != '(')
   {
      ctx.restore();
      return(false);
   }
   pc.SetType(CT_STRING);

   while (ctx.more())
   {
      if (  (ctx.peek() == ')')                 // 41
         && (ctx.peek(tag_len + 1) == '"')      // 34
         && tag_compare(ctx.data, tag_idx, ctx.c.idx + 1, tag_len))
      {
         cnt = tag_len + 2;   // for the )"

         while (cnt--)
         {
            pc.Str().append(ctx.get());
         }
         parse_suffix(ctx, pc);
         return(true);
      }

      if (ctx.peek() == '\n')
      {
         pc.Str().append(ctx.get());
         pc.SetNlCount(pc.GetNlCount() + 1);
         pc.SetType(CT_STRING_MULTI);
      }
      else
      {
         pc.Str().append(ctx.get());
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
static bool parse_word(TokenContext &ctx, Chunk &pc, bool skipcheck)
{
   static UncText intr_txt("@interface");

   // The first character is already valid
   pc.Str().clear();
   pc.Str().append(ctx.get());

   while (ctx.more())
   {
      size_t ch = ctx.peek();

      if (CharTable::IsKw2(ch))
      {
         pc.Str().append(ctx.get());
      }
      else if (  (ch == '\\')                            // 92
              && (unc_tolower(ctx.peek(1)) == 'u'))      // 117
      {
         pc.Str().append(ctx.get());
         pc.Str().append(ctx.get());
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
   pc.SetType(CT_WORD);

   if (skipcheck)
   {
      return(true);
   }

   // Detect pre-processor functions now
   if (  cpd.in_preproc == CT_PP_DEFINE
      && cpd.preproc_ncnl_count == 1)
   {
      if (ctx.peek() == '(')               // 40
      {
         pc.SetType(CT_MACRO_FUNC);
      }
      else
      {
         pc.SetType(CT_MACRO);

         log_rule_B("pp_ignore_define_body");

         if (options::pp_ignore_define_body())
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
      if (  language_is_set(lang_flag_e::LANG_JAVA)
         && pc.GetStr().startswith("@")
         && !pc.GetStr().equals(intr_txt))
      {
         pc.SetType(CT_ANNOTATION);
      }
      else
      {
         // Turn it into a keyword now
         // Issue #1460 will return "COMMENT_CPP"
         pc.SetType(find_keyword_type(pc.Text(), pc.GetStr().size()));

         /* Special pattern: if we're trying to redirect a preprocessor directive to PP_IGNORE,
          * then ensure we're actually part of a preprocessor before doing the swap, or we'll
          * end up with a function named 'define' as PP_IGNORE. This is necessary because with
          * the config 'set' feature, there's no way to do a pair of tokens as a word
          * substitution. */
         if (  pc.GetType() == CT_PP_IGNORE
            && !cpd.in_preproc)
         {
            pc.SetType(find_keyword_type(pc.Text(), pc.GetStr().size()));
         }
         else if (pc.GetType() == CT_COMMENT_CPP)   // Issue #1460
         {
            size_t ch;
            bool   is_cs = language_is_set(lang_flag_e::LANG_CS);

            // read until EOL
            while (true)
            {
               int bs_cnt = 0;

               while (ctx.more())
               {
                  ch = ctx.peek();

                  if (  (ch == '\r')
                     || (ch == '\n'))
                  {
                     break;
                  }

                  if (  (ch == '\\') // 92
                     && !is_cs)      // backslashes aren't special in comments in C#
                  {
                     bs_cnt++;
                  }
                  else
                  {
                     bs_cnt = 0;
                  }
                  pc.Str().append(ctx.get());
               }

               /*
                * If we hit an odd number of backslashes right before the newline,
                * then we keep going.
                */
               if (  ((bs_cnt & 1) == 0)
                  || !ctx.more())
               {
                  break;
               }

               if (ctx.peek() == '\r')
               {
                  pc.Str().append(ctx.get());
               }

               if (ctx.peek() == '\n')
               {
                  pc.Str().append(ctx.get());
               }
               pc.SetNlCount(pc.GetNlCount() + 1);
               cpd.did_newline = true;
            }
            // Store off the end column
            pc.SetOrigColEnd(ctx.c.col);
         }
      }
   }
   return(true);
} // parse_word


static size_t parse_attribute_specifier_sequence(TokenContext &ctx)
{
   size_t nested = 0;
   size_t offset = 0;
   size_t parens = 0;
   auto   ch1    = ctx.peek(offset++);

   while (ch1)
   {
      auto ch2 = ctx.peek(offset++);

      while (  ch2 == ' '       // 32
            || ch2 == '\n'
            || ch2 == '\r'
            || ch2 == '\t')
      {
         ch2 = ctx.peek(offset++);
      }

      if (  nested == 0
         && ch2 != '[')        // 91
      {
         break;
      }

      if (ch1 == '(')          // 40
      {
         ++parens;
         ch1 = ch2;
         continue;
      }

      if (ch1 == ')')          // 41
      {
         if (parens == 0)
         {
            break;
         }
         --parens;
         ch1 = ch2;
         continue;
      }

      if (  ch1 != '['
         && ch1 != ']')
      {
         ch1 = ch2;
         continue;
      }

      if (ch2 != ch1)
      {
         if (parens == 0)
         {
            break;
         }
         ch1 = ch2;
         continue;
      }

      if (ch1 == '[')                    // 91
      {
         if (  nested != 0
            && parens == 0)
         {
            break;
         }
         ++nested;
      }
      else if (--nested == 0)
      {
         return(offset);
      }
      ch1 = ctx.peek(offset++);
   }
   return(0);
} // parse_attribute_specifier_sequence


static bool extract_attribute_specifier_sequence(TokenContext &ctx, Chunk &pc, size_t length)
{
   pc.Str().clear();

   while (length--)
   {
      pc.Str().append(ctx.get());
   }
   pc.SetType(CT_ATTRIBUTE);
   return(true);
} // extract_attribute_specifier_sequence


static bool parse_whitespace(TokenContext &ctx, Chunk &pc)
{
   size_t nl_count = 0;
   size_t ch       = 0;

   // REVISIT: use a better whitespace detector?
   while (  ctx.more()
         && unc_isspace(ctx.peek()))
   {
      int lastcol = ctx.c.col;
      ch = ctx.get();   // throw away the whitespace char

      switch (ch)
      {
      case '\r':

         if (ctx.expect('\n'))
         {
            // CRLF ending
            ++LE_COUNT(CRLF);
         }
         else
         {
            // CR ending
            ++LE_COUNT(CR);
         }
         nl_count++;
         pc.SetOrigPrevSp(0);
         break;

      case '\n':
         // LF ending
         ++LE_COUNT(LF);
         nl_count++;
         pc.SetOrigPrevSp(0);
         break;

      case '\t':
         pc.SetOrigPrevSp(pc.GetOrigPrevSp() + ctx.c.col - lastcol);
         break;

      case ' ':
         pc.SetOrigPrevSp(pc.GetOrigPrevSp() + 1);
         break;

      default:
         break;
      }
   }

   if (ch != 0)
   {
      pc.Str().clear();
      pc.SetType(nl_count ? CT_NEWLINE : CT_WHITESPACE);
      pc.SetNlCount(nl_count);
      pc.SetAfterTab((ctx.c.last_ch == '\t'));
      return(true);
   }
   return(false);
} // parse_whitespace


static bool parse_bs_newline(TokenContext &ctx, Chunk &pc)
{
   ctx.save();
   ctx.get(); // skip the '\'

   size_t ch;

   while (  ctx.more()
         && unc_isspace(ch = ctx.peek()))
   {
      ctx.get();

      if (  (ch == '\r')
         || (ch == '\n'))
      {
         if (ch == '\r')
         {
            ctx.expect('\n');
         }
         pc.SetType(CT_NL_CONT);
         pc.Str() = "\\";
         pc.SetNlCount(1);
         return(true);
      }
   }
   ctx.restore();
   return(false);
}


static bool parse_newline(TokenContext &ctx)
{
   ctx.save();

   // Eat whitespace
   while (  (ctx.peek() == ' ')               // 32
         || (ctx.peek() == '\t'))
   {
      ctx.get();
   }

   if (  (ctx.peek() == '\r')
      || (ctx.peek() == '\n'))
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


static void parse_pawn_pattern(TokenContext &ctx, Chunk &pc, E_Token tt)
{
   pc.Str().clear();
   pc.SetType(tt);

   while (!unc_isspace(ctx.peek()))
   {
      // end the pattern on an escaped newline
      if (ctx.peek() == '\\')             // 92
      {
         size_t ch = ctx.peek(1);

         if (  (ch == '\n')
            || (ch == '\r'))
         {
            break;
         }
      }
      pc.Str().append(ctx.get());
   }
}


static bool parse_off_newlines(TokenContext &ctx, Chunk &pc)
{
   size_t nl_count = 0;

   // Parse off newlines/blank lines
   while (parse_newline(ctx))
   {
      nl_count++;
   }

   if (nl_count > 0)
   {
      pc.SetNlCount(nl_count);
      pc.SetType(CT_NEWLINE);
      return(true);
   }
   return(false);
}


static bool parse_macro(TokenContext &ctx, Chunk &pc, const Chunk *prev_pc)
{
   if (parse_off_newlines(ctx, pc))
   {
      return(true);
   }

   if (parse_comment(ctx, pc))  // allow CT_COMMENT_MULTI within macros
   {
      return(true);
   }
   ctx.save();
   pc.Str().clear();

   if (prev_pc->IsNullChunk())
   {
      return(false);
   }
   bool continued = (  prev_pc->Is(CT_NL_CONT)
                    || prev_pc->Is(CT_COMMENT_MULTI));

   while (ctx.more())
   {
      size_t pk = ctx.peek(), pk1 = ctx.peek(1);
      bool   nl = (  pk == '\n'
                  || pk == '\r');
      bool   nl_cont = (  pk == '\\'             // 92
                       && (  pk1 == '\n'
                          || pk1 == '\r'));

      if (  (  nl_cont
            || (  continued
               && nl))
         && pc.GetStr().size() > 0)
      {
         pc.SetType(CT_PP_IGNORE);
         return(true);
      }
      else if (nl)
      {
         break;
      }
      pc.Str().append(ctx.get());
   }
   pc.Str().clear();
   ctx.restore();
   return(false);
} // parse_macro


static bool parse_ignored(TokenContext &ctx, Chunk &pc)
{
   if (parse_off_newlines(ctx, pc))
   {
      return(true);
   }
   // See if the options::enable_processing_cmt() or #pragma endasm / #endasm text is on this line
   ctx.save();
   pc.Str().clear();

   while (  ctx.more()
         && (ctx.peek() != '\r')
         && (ctx.peek() != '\n'))
   {
      pc.Str().append(ctx.get());
   }

   if (pc.GetStr().size() == 0)
   {
      // end of file?
      return(false);
   }

   // HACK: turn on if we find '#endasm' or '#pragma' and 'endasm' separated by blanks
   if (  (  (  (pc.GetStr().find("#pragma ") >= 0)
            || (pc.GetStr().find("#pragma	") >= 0))
         && (  (pc.GetStr().find(" endasm") >= 0)
            || (pc.GetStr().find("	endasm") >= 0)))
      || (pc.GetStr().find("#endasm") >= 0))
   {
      cpd.unc_off = false;
      ctx.restore();
      pc.Str().clear();
      return(false);
   }
   // Note that we aren't actually making sure this is in a comment, yet
   log_rule_B("enable_processing_cmt");
   const auto &ontext = options::enable_processing_cmt();

   if (!ontext.empty())
   {
      bool found_enable_pattern = false;

      if (  ontext != UNCRUSTIFY_ON_TEXT
         && options::processing_cmt_as_regex())
      {
         std::wstring pc_wstring(pc.GetStr().get().cbegin(),
                                 pc.GetStr().get().cend());
         std::wregex  criteria(std::wstring(ontext.cbegin(),
                                            ontext.cend()));

         found_enable_pattern = std::regex_search(pc_wstring.cbegin(),
                                                  pc_wstring.cend(),
                                                  criteria);
      }
      else
      {
         found_enable_pattern = (pc.GetStr().find(ontext.c_str()) >= 0);
      }

      if (!found_enable_pattern)
      {
         pc.SetType(CT_IGNORED);
         return(true);
      }
   }
   ctx.restore();

   // parse off whitespace leading to the comment
   if (parse_whitespace(ctx, pc))
   {
      pc.SetType(CT_IGNORED);
      return(true);
   }

   // Look for the ending comment and let it pass
   if (  parse_comment(ctx, pc)
      && !cpd.unc_off)
   {
      return(true);
   }
   // Reset the chunk & scan to until a newline
   pc.Str().clear();

   while (  ctx.more()
         && (ctx.peek() != '\r')
         && (ctx.peek() != '\n'))
   {
      pc.Str().append(ctx.get());
   }

   if (pc.GetStr().size() > 0)
   {
      pc.SetType(CT_IGNORED);
      return(true);
   }
   return(false);
} // parse_ignored


static bool parse_next(TokenContext &ctx, Chunk &pc, const Chunk *prev_pc)
{
   if (!ctx.more())
   {
      return(false);
   }
   // Save off the current column
   pc.SetType(CT_NONE);
   pc.SetOrigLine(ctx.c.row);
   pc.SetColumn(ctx.c.col);
   pc.SetOrigCol(ctx.c.col);
   pc.SetNlCount(0);
   pc.SetFlags(PCF_NONE);

   // If it is turned off, we put everything except newlines into CT_UNKNOWN
   if (cpd.unc_off)
   {
      if (parse_ignored(ctx, pc))
      {
         return(true);
      }
   }
   log_rule_B("disable_processing_nl_cont");

   // Parse macro blocks
   if (options::disable_processing_nl_cont())
   {
      if (parse_macro(ctx, pc, prev_pc))
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
   if (  cpd.in_preproc > CT_PP_BODYCHUNK
      && cpd.in_preproc <= CT_PP_OTHER)
   {
      pc.Str().clear();
      TokenInfo ss;
      ctx.save(ss);
      // Chunk to a newline or comment
      pc.SetType(CT_PREPROC_BODY);
      size_t last = 0;

      while (ctx.more())
      {
         size_t ch = ctx.peek();

         // Fix for issue #1752
         // Ignoring extra spaces after ' \ ' for preproc body continuations
         if (  last == '\\'             // 92
            && ch == ' ')               // 32
         {
            ctx.get();
            continue;
         }

         if (  (ch == '\n')
            || (ch == '\r'))
         {
            // Back off if this is an escaped newline
            if (last == '\\')            // 92
            {
               ctx.restore(ss);
               pc.Str().pop_back();
            }
            break;
         }

         // Quit on a C or C++ comment start           Issue #1966
         if (  (ch == '/')               // 47
            && (  (ctx.peek(1) == '/')   // 47
               || (ctx.peek(1) == '*'))) // 42
         {
            break;
         }
         last = ch;
         ctx.save(ss);

         pc.Str().append(ctx.get());
      }

      if (pc.GetStr().size() > 0)
      {
         return(true);
      }
   }

   // Detect backslash-newline
   if (  (ctx.peek() == '\\')                   // 92
      && parse_bs_newline(ctx, pc))
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

   if (language_is_set(lang_flag_e::LANG_CS))
   {
      if (parse_cs_string(ctx, pc))
      {
         return(true);
      }
   }

   if (  language_is_set(lang_flag_e::LANG_CS)
      || language_is_set(lang_flag_e::LANG_VALA))
   {
      // check for non-keyword identifiers such as @if @switch, etc
      // Vala also allows numeric identifiers if prefixed with '@'
      if (  ctx.peek() == '@'                          // 64
         && (  CharTable::IsKw1(ctx.peek(1))
            || (  language_is_set(lang_flag_e::LANG_VALA)
               && CharTable::IsKw2(ctx.peek(1)))))
      {
         parse_word(ctx, pc, true);
         return(true);
      }
   }

   // handle VALA """ strings """
   if (  language_is_set(lang_flag_e::LANG_VALA)
      && (ctx.peek() == '"')                 // 34
      && (ctx.peek(1) == '"')                // 34
      && (ctx.peek(2) == '"'))               // 34
   {
      parse_verbatim_string(ctx, pc);
      return(true);
   }
   /*
    * handle C++(11) string/char literal prefixes u8|u|U|L|R including all
    * possible combinations and optional R delimiters: R"delim(x)delim"
    */
   auto ch = ctx.peek();

   if (  (  language_is_set(lang_flag_e::LANG_C)
         || language_is_set(lang_flag_e::LANG_CPP))
      && (  ch == 'u'                     // 117
         || ch == 'U'                     // 85
         || ch == 'R'                     // 82
         || ch == 'L'))                   // 76
   {
      auto idx     = size_t{};
      auto is_real = false;

      if (  ch == 'u'                    // 117
         && ctx.peek(1) == '8')          // 56
      {
         idx = 2;
      }
      else if (  unc_tolower(ch) == 'u'     // 117
              || ch == 'L')                 // 76
      {
         idx++;
      }

      if (  (  language_is_set(lang_flag_e::LANG_C)
            || language_is_set(lang_flag_e::LANG_CPP))
         && ctx.peek(idx) == 'R')             // 82
      {
         idx++;
         is_real = true;
      }
      const auto quote = ctx.peek(idx);

      if (is_real)
      {
         if (  quote == '"'                  // 34
            && parse_cr_string(ctx, pc, idx))
         {
            return(true);
         }
      }
      else if (  (  quote == '"'                  // 34
                 || quote == '\'')                // 39
              && parse_string(ctx, pc, idx, true))
      {
         return(true);
      }
   }

   // PAWN specific stuff
   if (language_is_set(lang_flag_e::LANG_PAWN))
   {
      if (  cpd.preproc_ncnl_count == 1
         && (  cpd.in_preproc == CT_PP_DEFINE
            || cpd.in_preproc == CT_PP_EMIT))
      {
         parse_pawn_pattern(ctx, pc, CT_MACRO);
         return(true);
      }

      // Check for PAWN strings: \"hi" or !"hi" or !\"hi" or \!"hi"
      if (  (ctx.peek() == '\\')      // 92
         || (ctx.peek() == '!'))      // 33
      {
         if (ctx.peek(1) == '"')      // 32
         {
            parse_string(ctx, pc, 1, (ctx.peek() == '!'));  // 33
            return(true);
         }

         if (  (  (ctx.peek(1) == '\\')           // 92
               || (ctx.peek(1) == '!'))           // 33
            && (ctx.peek(2) == '"'))              // 32
         {
            parse_string(ctx, pc, 2, false);
            return(true);
         }
      }

      // handle PAWN preprocessor args %0 .. %9
      if (  cpd.in_preproc == CT_PP_DEFINE
         && (ctx.peek() == '%')               // 37
         && unc_isdigit(ctx.peek(1)))
      {
         pc.Str().clear();
         pc.Str().append(ctx.get());
         pc.Str().append(ctx.get());
         pc.SetType(CT_WORD);
         return(true);
      }
   }
   // Parse strings and character constants

   bool pn = parse_number(ctx, pc);

   if (pn)
   {
      return(true);
   }

   if (language_is_set(lang_flag_e::LANG_D))
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

      if (  (  (  (ch == 'L')            // 76
               || (ch == 'S'))           // 83
            && (  (ch1 == '"')           // 34
               || (ch1 == '\'')))        // 39
         || (ch == '"')                  // 34
         || (ch == '\'')                 // 39
         || (  (ch == '<')               // 60
            && cpd.in_preproc == CT_PP_INCLUDE))
      {
         parse_string(ctx, pc, unc_isalpha(ch) ? 1 : 0, true);

         if (cpd.in_preproc == CT_PP_INCLUDE)
         {
            pc.SetParentType(CT_PP_INCLUDE);
         }
         return(true);
      }

      if (  (ch == '<')                    // 60
         && cpd.in_preproc == CT_PP_DEFINE)
      {
         if (Chunk::GetTail()->Is(CT_MACRO))
         {
            // We have "#define XXX <", assume '<' starts an include string
            parse_string(ctx, pc, 0, false);
            return(true);
         }
      }
      /* Inside clang's __has_include() could be "path/to/file.h" or system-style <path/to/file.h> */
      Chunk *tail = Chunk::GetTail();

      if (  (ch == '(')                 // 40
         && (tail->IsNotNullChunk())
         && (  tail->Is(CT_CNG_HASINC)
            || tail->Is(CT_CNG_HASINCN)))
      {
         parse_string(ctx, pc, 0, false);
         return(true);
      }
   }

   // Check for Vala string templates
   if (  language_is_set(lang_flag_e::LANG_VALA)
      && (ctx.peek() == '@'))            // 64
   {
      size_t nc = ctx.peek(1);

      if (nc == '"')                     // 34
      {
         // literal string
         parse_string(ctx, pc, 1, true);
         return(true);
      }
   }

   // Check for Objective C literals
   if (  language_is_set(lang_flag_e::LANG_OC)
      && (ctx.peek() == '@'))            // 64
   {
      size_t nc = ctx.peek(1);

      if (nc == 'R') // Issue #2720  82
      {
         if (ctx.peek(2) == '"')          // 34
         {
            if (parse_cr_string(ctx, pc, 2)) // Issue #3027
            {
               return(true);
            }
            // parse string without escaping
            parse_string(ctx, pc, 2, false);
            return(true);
         }
      }

      if (  (nc == '"')          // 34
         || (nc == '\''))        // 39
      {
         // literal string
         parse_string(ctx, pc, 1, true);
         return(true);
      }

      if (  (nc >= '0')
         && (nc <= '9'))
      {
         // literal number
         pc.Str().append(ctx.get());  // store the '@'
         parse_number(ctx, pc);
         return(true);
      }
   }

   // Check for pawn/ObjectiveC/Java and normal identifiers
   if (  CharTable::IsKw1(ctx.peek())
      || (  (ctx.peek() == '\\')                    // 92
         && (unc_tolower(ctx.peek(1)) == 'u'))      // 117
      || (  (ctx.peek() == '@')                     // 64
         && CharTable::IsKw1(ctx.peek(1))))
   {
      parse_word(ctx, pc, false);
      return(true);
   }

   // Check for C++11/14/17/20 attribute specifier sequences
   if (  language_is_set(lang_flag_e::LANG_CPP)
      && ctx.peek() == '[')                   // 91
   {
      if (  !language_is_set(lang_flag_e::LANG_OC)
         || (  prev_pc->IsNotNullChunk()
            && !prev_pc->Is(CT_OC_AT)))
      {
         if (auto length = parse_attribute_specifier_sequence(ctx))
         {
            extract_attribute_specifier_sequence(ctx, pc, length);
            return(true);
         }
      }
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
         pc.Str().append(ctx.get());
      }
      pc.SetType(punc->type);
      pc.SetFlagBits(PCF_PUNCTUATOR);
      return(true);
   }
   /* When parsing C/C++ files and running into some unknown token,
    * check if matches Objective-C as a last resort, before
    * considering it as garbage.
    */
   int probe_lang_flags = 0;

   if (  language_is_set(lang_flag_e::LANG_C)
      || language_is_set(lang_flag_e::LANG_CPP))
   {
      probe_lang_flags = cpd.lang_flags | e_LANG_OC;
   }

   if (probe_lang_flags != 0)
   {
      if ((punc = find_punctuator(punc_txt, probe_lang_flags)) != nullptr)
      {
         cpd.lang_flags = probe_lang_flags;
         int cnt = strlen(punc->tag);

         while (cnt--)
         {
            pc.Str().append(ctx.get());
         }
         pc.SetType(punc->type);
         pc.SetFlagBits(PCF_PUNCTUATOR);
         return(true);
      }
   }
   // throw away this character
   pc.SetType(CT_UNKNOWN);
   pc.Str().append(ctx.get());

   LOG_FMT(LWARN, "%s:%zu Garbage in col %zu: %x\n",
           cpd.filename.c_str(), pc.GetOrigLine(), ctx.c.col, pc.GetStr()[0]);
   exit(EX_SOFTWARE);
} // parse_next


int find_disable_processing_comment_marker(const UncText &text,
                                           std::size_t   start_idx)
{
   log_rule_B("disable_processing_cmt");
   const auto &offtext = options::disable_processing_cmt();
   int        idx      = -1;

   if (  !offtext.empty()
      && start_idx < text.size())
   {
      if (  offtext != UNCRUSTIFY_OFF_TEXT
         && options::processing_cmt_as_regex())
      {
         std::wsmatch match;
         std::wstring pc_wstring(text.get().cbegin() + start_idx,
                                 text.get().cend());
         std::wregex  criteria(std::wstring(offtext.cbegin(),
                                            offtext.cend()));

         std::regex_search(pc_wstring.cbegin(),
                           pc_wstring.cend(),
                           match,
                           criteria);

         if (!match.empty())
         {
            idx = int(match.position() + start_idx);
         }
      }
      else
      {
         idx = text.find(offtext.c_str(),
                         start_idx);

         if (idx >= 0)
         {
            idx += int(offtext.size());
         }
      }

      /**
       *  update the position to the start of the current line
       */
      while (  idx > 0
            && text[idx - 1] != '\n')
      {
         --idx;
      }
   }
   return(idx);
} // find_disable_processing_comment_marker


int find_enable_processing_comment_marker(const UncText &text,
                                          std::size_t   start_idx)
{
   log_rule_B("enable_processing_cmt");
   const auto &ontext = options::enable_processing_cmt();
   int        idx     = -1;

   if (  !ontext.empty()
      && start_idx < text.size())
   {
      if (  ontext != UNCRUSTIFY_ON_TEXT
         && options::processing_cmt_as_regex())
      {
         std::wsmatch match;
         std::wstring pc_wstring(text.get().cbegin() + start_idx,
                                 text.get().cend());
         std::wregex  criteria(std::wstring(ontext.cbegin(),
                                            ontext.cend()));

         std::regex_search(pc_wstring.cbegin(),
                           pc_wstring.cend(),
                           match,
                           criteria);

         if (!match.empty())
         {
            idx = int(start_idx + match.position() + match.size());
         }
      }
      else
      {
         idx = text.find(ontext.c_str(),
                         start_idx);

         if (idx >= 0)
         {
            idx += int(ontext.size());
         }
      }

      /**
       * update the position to the end of the current line
       */
      if (idx >= 0)
      {
         while (  idx < int(text.size())
               && text[idx] != '\n')
         {
            ++idx;
         }
      }
   }
   return(idx);
} // find_enable_processing_comment_marker


void tokenize(const deque<int> &data, Chunk *ref)
{
   TokenContext ctx(data);
   Chunk        chunk;
   Chunk        *pc          = Chunk::NullChunkPtr;
   Chunk        *rprev       = Chunk::NullChunkPtr;
   bool         last_was_tab = false;
   size_t       prev_sp      = 0;
   int          num_stripped = 0;               // Issue #1966

   cpd.unc_stage = unc_stage_e::TOKENIZE;

   while (ctx.more())
   {
      chunk.Reset();
      chunk.SetPpLevel(0);

      if (!parse_next(ctx, chunk, pc))
      {
         LOG_FMT(LERR, "%s:%zu Bailed before the end?\n",
                 cpd.filename.c_str(), ctx.c.row);
         exit(EX_SOFTWARE);
      }

      if (  language_is_set(lang_flag_e::LANG_JAVA)
         && chunk.GetType() == CT_MEMBER
         && !memcmp(chunk.Text(), "->", 2))
      {
         chunk.SetType(CT_LAMBDA);
      }

      // Don't create an entry for whitespace
      if (chunk.GetType() == CT_WHITESPACE)
      {
         last_was_tab = chunk.GetAfterTab();
         prev_sp      = chunk.GetOrigPrevSp();
         continue;
      }
      chunk.SetOrigPrevSp(prev_sp);
      prev_sp = 0;

      if (chunk.GetType() == CT_NEWLINE)
      {
         last_was_tab = chunk.GetAfterTab();
         chunk.SetAfterTab(false);
         chunk.Str().clear();
      }
      else if (chunk.GetType() == CT_NL_CONT)
      {
         last_was_tab = chunk.GetAfterTab();
         chunk.SetAfterTab(false);
         chunk.Str() = "\\\n";
      }
      else
      {
         chunk.SetAfterTab(last_was_tab);
         last_was_tab = false;
      }
      num_stripped = 0; // Issue #1966 and #3565

      if (chunk.GetType() != CT_IGNORED)
      {
         // Issue #1338
         // Strip trailing whitespace (for CPP comments and PP blocks)
         while (  (chunk.GetStr().size() > 0)
               && (  (chunk.GetStr()[chunk.GetStr().size() - 1] == ' ')         // 32
                  || (chunk.GetStr()[chunk.GetStr().size() - 1] == '\t')))
         {
            // If comment contains backslash '\' followed by whitespace chars, keep last one;
            // this will prevent it from turning '\' into line continuation.
            if (  (chunk.GetStr().size() > 1)
               && (chunk.GetStr()[chunk.GetStr().size() - 2] == '\\'))
            {
               break;
            }
            chunk.Str().pop_back();
            num_stripped++;                    // Issue #1966
         }
      }
      // Store off the end column
      chunk.SetOrigColEnd(ctx.c.col - num_stripped); // Issue #1966 and #3565

      // Make the whitespace we disposed of be attributed to the next chunk
      prev_sp = num_stripped;

      // Add the chunk to the list
      rprev = pc;

      if (rprev->IsNotNullChunk())
      {
         pc->SetFlagBits(rprev->GetFlags() & PCF_COPY_FLAGS);

         // a newline can't be in a preprocessor
         if (pc->Is(CT_NEWLINE))
         {
            pc->ResetFlagBits(PCF_IN_PREPROC);
         }
      }

      if (ref->IsNotNullChunk())
      {
         chunk.SetFlagBits(PCF_INSERTED);
      }
      else
      {
         chunk.ResetFlagBits(PCF_INSERTED);
      }
      pc = chunk.CopyAndAddBefore(ref);

      // A newline marks the end of a preprocessor
      if (pc->Is(CT_NEWLINE)) // || pc->Is(CT_COMMENT_MULTI))
      {
         cpd.in_preproc         = CT_NONE;
         cpd.preproc_ncnl_count = 0;
      }

      // Disable indentation when #asm directive found
      if (pc->Is(CT_PP_ASM))
      {
         LOG_FMT(LBCTRL, "Found a directive %s on line %zu\n", "#asm", pc->GetOrigLine());
         cpd.unc_off = true;
      }

      // Special handling for preprocessor stuff
      if (cpd.in_preproc != CT_NONE)
      {
         pc->SetFlagBits(PCF_IN_PREPROC);
         // Issue #2225
         LOG_FMT(LBCTRL, "%s(%d): orig line is %zu, orig col is %zu, type is %s, parentType is %s\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(),
                 get_token_name(pc->GetType()), get_token_name(pc->GetParentType()));

         if (  pc->Is(CT_STRING_MULTI)
            && pc->GetParentType() == CT_PP_INCLUDE)
         {
            LOG_FMT(LWARN, "%s:%zu: File name is not possible %s\n",
                    cpd.filename.c_str(), pc->GetOrigLine(), pc->Text());
            exit(EX_SOFTWARE);
         }

         // Count words after the preprocessor
         if (!pc->IsCommentOrNewline())
         {
            cpd.preproc_ncnl_count++;
         }

         // Disable indentation if a #pragma asm directive is found
         if (cpd.in_preproc == CT_PP_PRAGMA)
         {
            if (strncmp(pc->Text(), "asm", 3) == 0)
            {
               LOG_FMT(LBCTRL, "Found a pragma %s on line %zu\n", "asm", pc->GetOrigLine());
               cpd.unc_off = true;
            }
         }

         // Figure out the type of preprocessor for #include parsing
         if (cpd.in_preproc == CT_PREPROC)
         {
            if (  pc->GetType() < CT_PP_DEFINE
               || pc->GetType() > CT_PP_OTHER)
            {
               pc->SetType(CT_PP_OTHER);
            }
            cpd.in_preproc = pc->GetType();
         }
         else if (cpd.in_preproc == CT_PP_IGNORE)
         {
            if (  !pc->Is(CT_NL_CONT)
               && !pc->IsComment())        // Issue #1966
            {
               pc->SetType(CT_PP_IGNORE);
            }
         }
         else if (  cpd.in_preproc == CT_PP_DEFINE
                 && pc->Is(CT_PAREN_CLOSE)
                 && options::pp_ignore_define_body())
         {
            log_rule_B("pp_ignore_define_body");
            // When we have a PAREN_CLOSE in a PP_DEFINE we should be terminating a MACRO_FUNC
            // arguments list. Therefore we can enter the PP_IGNORE state and ignore next chunks.
            cpd.in_preproc = CT_PP_IGNORE;
         }
      }
      else
      {
         // Check for a preprocessor start
         if (  pc->Is(CT_POUND)
            && (  rprev->IsNullChunk()
               || rprev->Is(CT_NEWLINE)))
         {
            pc->SetType(CT_PREPROC);
            pc->SetFlagBits(PCF_IN_PREPROC);
            cpd.in_preproc = CT_PREPROC;
         }
      }

      if (pc->Is(CT_NEWLINE))
      {
         LOG_FMT(LBCTRL, "%s(%d): orig line is %zu, orig col is %zu, <Newline>, nl is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->GetNlCount());
      }
      else if (pc->Is(CT_VBRACE_OPEN))
      {
         LOG_FMT(LBCTRL, "%s(%d): orig line is %zu, orig col is %zu, type is %s, orig col end is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), get_token_name(pc->GetType()), pc->GetOrigColEnd());
      }
      else
      {
         char copy[1000];
         LOG_FMT(LBCTRL, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s, orig col end is %zu\n",
                 __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->ElidedText(copy), get_token_name(pc->GetType()), pc->GetOrigColEnd());
      }
   }
   // Set the cpd.newline string for this file
   log_rule_B("newlines");

   if (  options::newlines() == LE_LF
      || (  options::newlines() == LE_AUTO
         && (LE_COUNT(LF) >= LE_COUNT(CRLF))
         && (LE_COUNT(LF) >= LE_COUNT(CR))))
   {
      // LF line ends
      cpd.newline = "\n";
      LOG_FMT(LLINEENDS, "Using LF line endings\n");
   }
   else if (  options::newlines() == LE_CRLF
           || (  options::newlines() == LE_AUTO
              && (LE_COUNT(CRLF) >= LE_COUNT(LF))
              && (LE_COUNT(CRLF) >= LE_COUNT(CR))))
   {
      // CRLF line ends
      cpd.newline = "\r\n";
      LOG_FMT(LLINEENDS, "Using CRLF line endings\r\n");
   }
   else
   {
      // CR line ends
      cpd.newline = "\r";
      LOG_FMT(LLINEENDS, "Using CR line endings\n");
   }
} // tokenize
