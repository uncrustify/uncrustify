/**
 * @file c_token.c
 * This file breaks up the text stream into tokens or chunks.
 */

#include "cparse_types.h"
#include "char_table.h"
#include "prototypes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <assert.h>

struct keyword_tag
{
   const char *str;
   int        flags;
   c_token_t  type;
};

/**
 * interesting keywords - keep sorted.
 * Table should include the Name, Type, and Language flags.
 */
static struct keyword_tag keywords[] =
{
   { "__const__",         9, CT_TYPE },     /* TODO: should be CT_QUALIFIER */
   { "__inline__",       10, CT_TYPE },     /* TODO: should be CT_QUALIFIER */
   { "__signed__",       10, CT_TYPE },
   { "__typeof__",       10, CT_SIZEOF },
   { "__volatile__",     12, CT_TYPE },     /* TODO: should be CT_QUALIFIER */
   { "_Bool",             5, CT_TYPE },
   { "_Complex",          8, CT_TYPE },
   { "_Imaginary",       10, CT_TYPE },
   { "alignof",           7, CT_SIZEOF },
   { "and",               3, CT_BOOL },
   { "and_eq",            6, CT_ASSIGN },
   { "asm",               3, CT_ASM },
   { "auto",              4, CT_TYPE },     /* TODO: should be CT_QUALIFIER */
   { "bitand",            6, CT_ARITH },
   { "bitor",             5, CT_ARITH },
   { "bool",              4, CT_TYPE },
   { "break",             5, CT_BREAK },
   { "case",              4, CT_CASE },
   { "catch",             5, CT_CATCH },
   { "char",              4, CT_TYPE },
   { "class",             5, CT_CLASS },
   { "compl",             5, CT_ARITH },
   { "const",             5, CT_TYPE },     /* TODO: should be CT_QUALIFIER */
   { "const_cast",       10, CT_TYPE_CAST },
   { "default",           7, CT_CASE },
   { "delete",            6, CT_DELETE },
   { "do",                2, CT_DO },
   { "double",            6, CT_TYPE },
   { "dynamic_cast",     12, CT_TYPE_CAST },
   { "else",              4, CT_ELSE },
   { "enum",              4, CT_ENUM },
   { "explicit",          8, CT_TYPE },
   { "export",            6, CT_EXPORT },
   { "extern",            6, CT_TYPE },
   { "false",             5, CT_TYPE },
   { "float",             5, CT_TYPE },
   { "for",               3, CT_FOR },
   { "friend",            6, CT_FRIEND },
   { "goto",              4, CT_GOTO },
   { "if",                2, CT_IF },
   { "inline",            6, CT_TYPE },     /* TODO: should be CT_QUALIFIER */
   { "int",               3, CT_TYPE },
   { "long",              4, CT_TYPE },
   { "mutable",           7, CT_MUTABLE },
   { "namespace",         9, CT_NAMESPACE },
   { "new",               3, CT_NEW },
   { "not",               3, CT_ARITH },
   { "not_eq",            6, CT_COMPARE },
   { "operator",          8, CT_OPERATOR },
   { "or",                2, CT_BOOL },
   { "or_eq",             5, CT_ASSIGN },
   { "private",           7, CT_PRIVATE },
   { "protected",         9, CT_PRIVATE },
   { "public",            6, CT_PRIVATE },
   { "register",          8, CT_TYPE },     /* TODO: should be CT_QUALIFIER */
   { "reinterpret_cast", 16, CT_TYPE_CAST },
   { "restrict",          8, CT_TYPE },     /* TODO: should be CT_QUALIFIER */
   { "return",            6, CT_RETURN },
   { "short",             5, CT_TYPE },
   { "signed",            6, CT_TYPE },
   { "sizeof",            6, CT_SIZEOF },
   { "static",            6, CT_TYPE },     /* TODO: should be CT_QUALIFIER */
   { "static_cast",      11, CT_TYPE_CAST },
   { "struct",            6, CT_STRUCT },
   { "switch",            6, CT_SWITCH },
   { "template",          8, CT_TEMPLATE },
   { "this",              4, CT_TYPE },
   { "throw",             5, CT_THROW },
   { "true",              4, CT_TYPE },
   { "try",               3, CT_TRY },
   { "typedef",           7, CT_TYPEDEF },
   { "typeid",            6, CT_SIZEOF },
   { "typename",          8, CT_TYPENAME },
   { "typeof",            6, CT_SIZEOF },
   { "union",             5, CT_UNION },
   { "unsigned",          8, CT_TYPE },
   { "using",             5, CT_USING },
   { "virtual",           7, CT_TYPE },
   { "void",              4, CT_TYPE },
   { "volatile",          8, CT_TYPE },     /* TODO: should be CT_QUALIFIER */
   { "wchar_t",           7, CT_TYPE },
   { "while",             5, CT_WHILE },
   { "xor",               3, CT_ARITH },
   { "xor_eq",            6, CT_ASSIGN },
};

static int kw_compare(const void *p1, const void *p2)
{
   const struct keyword_tag *t1 = p1;
   const struct keyword_tag *t2 = p2;

   return(strcmp(t1->str, t2->str));
}

/**
 * A pre-loaded hash table would be faster, but I don't think speed is all
 * that important at this point.
 */
const struct keyword_tag *find_keyword(const char *word, int len)
{
   struct keyword_tag tag;
   char               buf[32];

   if (len > (sizeof(buf) - 1))
   {
      return(NULL);
   }
   memcpy(buf, word, len);
   buf[len] = 0;

   tag.str = buf;

   return(bsearch(&tag, keywords, ARRAY_SIZE(keywords), sizeof(keywords[0]),
                  kw_compare));
}


struct symbol_tag
{
   const char *tag;
   c_token_t  type;
};

/* 4-char symbols */
static struct symbol_tag symbols4[] =
{
   { "%:%:", CT_PP },
};

/* 3-char symbols */
static struct symbol_tag symbols3[] =
{
   { "<<=", CT_ASSIGN },
   { ">>=", CT_ASSIGN },
   { "...", CT_ELIPSIS },
   { "->*", CT_MEMBER },
};

/* 2-char symbols */
static struct symbol_tag symbols2[] =
{
   { "++", CT_INCDEC_AFTER }, /* may change to CT_INCDEC_BEFORE */
   { "--", CT_INCDEC_AFTER }, /* may change to CT_INCDEC_BEFORE */
   { "%=", CT_ASSIGN },
   { "&=", CT_ASSIGN },
   { "*=", CT_ASSIGN },
   { "+=", CT_ASSIGN },
   { "-=", CT_ASSIGN },
   { "/=", CT_ASSIGN },
   { "^=", CT_ASSIGN },
   { "|=", CT_ASSIGN },
   { "!=", CT_COMPARE },
   { "<=", CT_COMPARE },
   { "==", CT_COMPARE },
   { ">=", CT_COMPARE },
   { "<<", CT_ARITH },
   { ">>", CT_ARITH },
   { "->", CT_MEMBER },
   { ".*", CT_MEMBER },
   { "::", CT_MEMBER },
   { "||", CT_BOOL },
   { "&&", CT_BOOL },
   { "##", CT_PP },
   { "<:", CT_SQUARE_OPEN },
   { ":>", CT_SQUARE_CLOSE },
   { "<%", CT_BRACE_OPEN },
   { "%>", CT_BRACE_CLOSE },
   { "%:", CT_POUND },
};

/* 1-char symbols */
static struct symbol_tag symbols1[] =
{
   { "#", CT_POUND },
   { "%", CT_ARITH },
   { "&", CT_ADDR },
   { "*", CT_STAR },          /* changed to CT_DEREF or CT_ARITH */
   { "+", CT_ARITH },
   { "^", CT_ARITH },
   { "-", CT_MINUS },         /* changed to CT_NEG or CT_ARITH */
   { "+", CT_PLUS },          /* may change to CT_ARITH */
   { "|", CT_ARITH },
   { "/", CT_ARITH },
   { "!", CT_NOT },
   { "~", CT_INV },
   { ",", CT_COMMA },
   { ".", CT_MEMBER },
   { ":", CT_COLON },
   { ";", CT_SEMICOLON },
   { "<", CT_COMPARE },
   { ">", CT_COMPARE },
   { "=", CT_ASSIGN },
   { "?", CT_QUESTION },
   { "(", CT_PAREN_OPEN },
   { ")", CT_PAREN_CLOSE },
   { "[", CT_SQUARE_OPEN },
   { "]", CT_SQUARE_CLOSE },
   { "{", CT_BRACE_OPEN },
   { "}", CT_BRACE_CLOSE },
};



/**
 * Figure of the length of the comment at text.
 * The next bit of text starts with a '/', so it might be a comment.
 * Count the characters in the comment, 0 means is wasn't comment.
 *
 * @param text    The text to look at (zero terminated)
 * @return        Whether a comment was parsed
 */
BOOL chunk_comment(struct chunk_s *pc)
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
//      cpd.column = 1;
//      cpd.line_number++;
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
BOOL chunk_number(struct chunk_s *pc)
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

      case 'B':               // binary
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
BOOL chunk_string(struct chunk_s *pc)
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
         /* TODO detect a newline in the string -- for an error condition? */
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
 * Count the number of characters in a word.
 * The first character is already valid for a keyword
 *
 * @param text    The text to look at (zero terminated)
 * @return        The number of bytes in the word
 */
BOOL chunk_word(struct chunk_s *pc)
{
   int                      len = 1;
   const struct keyword_tag *tag;

   if ((get_char_table(*pc->str) & CT_KW1) == 0)
   {
      return(FALSE);
   }

   while ((pc->str[len] < 127) && ((get_char_table(pc->str[len]) & CT_KW2) != 0))
   {
      len++;
   }
   cpd.column += len;
   pc->len     = len;
   pc->type    = CT_WORD;

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

   /* Scan the keywords */
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
 * @param text    The text to look at (zero terminated)
 * @return        The number of chars
 */
BOOL chunk_whitespace(struct chunk_s *pc)
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

   pc->nl_count = nl_count;
   pc->type     = nl_count ? CT_NEWLINE : CT_WHITESPACE;
   pc->len      = len;

   return(len != 0 ? TRUE : FALSE);
}


static BOOL chunk_punctuator(struct chunk_s *pc)
{
   int i;

   /* Check 4 char symbols */
   for (i = 0; i < ARRAY_SIZE(symbols4); i++)
   {
      if (strncmp(pc->str, symbols4[i].tag, 4) == 0)
      {
         pc->type    = symbols4[i].type;
         pc->len     = 4;
         cpd.column += 4;
         return(TRUE);
      }
   }

   /* Check 3 char symbols */
   for (i = 0; i < ARRAY_SIZE(symbols3); i++)
   {
      if (strncmp(pc->str, symbols3[i].tag, 3) == 0)
      {
         pc->type    = symbols3[i].type;
         pc->len     = 3;
         cpd.column += 3;
         return(TRUE);
      }
   }

   /* Check 2 char symbols */
   for (i = 0; i < ARRAY_SIZE(symbols2); i++)
   {
      if (strncmp(pc->str, symbols2[i].tag, 2) == 0)
      {
         pc->type    = symbols2[i].type;
         pc->len     = 2;
         cpd.column += 2;
         return(TRUE);
      }
   }

   /* Check 1 char symbols */
   for (i = 0; i < ARRAY_SIZE(symbols1); i++)
   {
      if (*pc->str == *symbols1[i].tag)
      {
         pc->type = symbols1[i].type;
         pc->len  = 1;
         cpd.column++;
         return(TRUE);
      }
   }

   return(FALSE);
}



/**
 * Skips the next bit of whatever and returns the type of block.
 *
 * pc->str is the input text.
 * pc->len in the output length.
 * pc->type is the output type
 * pc->column is output column
 *
 * @param pc      The structure to update
 * @return        TRUE/FALSE - whether anything was parsed
 */
BOOL chunk_next(struct chunk_s *pc)
{
   if ((pc == NULL) || (pc->str == NULL) || (*pc->str == 0))
   {
      return(FALSE);
   }

   /* Save off the current column */
   pc->orig_line = cpd.line_number;
   pc->column    = cpd.column;
   pc->orig_col  = cpd.column;
   pc->len       = 0;

   if (chunk_whitespace(pc))
   {
      return(TRUE);
   }

   if ((cpd.in_preproc != PP_UNKNOWN) &&
       (cpd.in_preproc != PP_NONE) &&
       (cpd.in_preproc != PP_DEFINE) &&
       (cpd.in_preproc != PP_INCLUDE))
   {
      /* Chunk to a newline */
      pc->type = CT_PREPROC_BODY;
      while ((pc->str[pc->len] != 0) && (pc->str[pc->len] != '\n'))
      {
         pc->len++;
      }
      return(TRUE);
   }

   if ((pc->str[0] == '\\') && (pc->str[1] == '\n'))
   {
      pc->type     = CT_NL_CONT;
      pc->len      = 2;
      pc->nl_count = 1;
      cpd.column   = 1;
      cpd.line_number++;
      return(TRUE);
   }

   /* Check for L'a', L"abc", 'a', "abc", <abc> strings */
   if (((*pc->str == 'L') && ((pc->str[1] == '"') || (pc->str[1] == '\''))) ||
       (*pc->str == '"') || (*pc->str == '\'') ||
       (((cpd.in_preproc == PP_INCLUDE) && (*pc->str == '<'))))
   {
      return(chunk_string(pc));
   }

   if (chunk_word(pc))
   {
      return(TRUE);
   }

   if ((*pc->str == '/') && chunk_comment(pc))
   {
      return(TRUE);
   }

   if (chunk_punctuator(pc))
   {
      return(TRUE);
   }

   if (chunk_number(pc))
   {
      return(TRUE);
   }

   /* throw away this character */
   pc->type = CT_UNKNOWN;
   pc->len  = 1;
   return(TRUE);
}

