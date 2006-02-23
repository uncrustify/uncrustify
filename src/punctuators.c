/**
 * @file punctuators.c
 * Manages the table of punctutators.
 *
 * TODO: add a 'language' field for C/C++/D/Java
 *
 * $Id: punctuators.c,v 1.8 2006/02/10 05:41:32 bengardner Exp $
 */

#include "cparse_types.h"
#include <string.h>


/* 4-char symbols */
static const struct symbol_tag symbols4[] =
{
   { "%:%:", CT_PP,      LANG_C },
   { ">>>=", CT_ASSIGN,  LANG_D },
   { "!<>=", CT_COMPARE, LANG_D },
};

/* 3-char symbols */
static const struct symbol_tag symbols3[] =
{
   { "<<=", CT_ASSIGN,  LANG_ALL },
   { ">>=", CT_ASSIGN,  LANG_ALL },
   { "...", CT_ELIPSIS, LANG_ALL },
   { "->*", CT_MEMBER,  LANG_ALL },
   { "!is", CT_COMPARE, LANG_D },
   { ">>>", CT_ARITH,   LANG_D },
   { "!<>", CT_COMPARE, LANG_D },
   { "!>=", CT_COMPARE, LANG_D },
   { "!<=", CT_COMPARE, LANG_D },
};

/* 2-char symbols */
static const struct symbol_tag symbols2[] =
{
   { "++", CT_INCDEC_AFTER, LANG_ALL }, /* may change to CT_INCDEC_BEFORE */
   { "--", CT_INCDEC_AFTER, LANG_ALL }, /* may change to CT_INCDEC_BEFORE */
   { "%=", CT_ASSIGN,       LANG_ALL },
   { "&=", CT_ASSIGN,       LANG_ALL },
   { "*=", CT_ASSIGN,       LANG_ALL },
   { "+=", CT_ASSIGN,       LANG_ALL },
   { "-=", CT_ASSIGN,       LANG_ALL },
   { "/=", CT_ASSIGN,       LANG_ALL },
   { "^=", CT_ASSIGN,       LANG_ALL },
   { "|=", CT_ASSIGN,       LANG_ALL },
   { "!=", CT_COMPARE,      LANG_ALL },
   { "<=", CT_COMPARE,      LANG_ALL },
   { "==", CT_COMPARE,      LANG_ALL },
   { ">=", CT_COMPARE,      LANG_ALL },
   { "<<", CT_ARITH,        LANG_ALL },
   { ">>", CT_ARITH,        LANG_ALL },
   { "->", CT_MEMBER,       LANG_ALL },
   { ".*", CT_MEMBER,       LANG_ALL },
   { "::", CT_MEMBER,       LANG_ALL },
   { "||", CT_BOOL,         LANG_ALL },
   { "&&", CT_BOOL,         LANG_ALL },
   { "##", CT_PP,           LANG_C | LANG_CPP },
   { "<:", CT_SQUARE_OPEN,  LANG_C },
   { ":>", CT_SQUARE_CLOSE, LANG_C },
   { "<%", CT_BRACE_OPEN,   LANG_C },
   { "%>", CT_BRACE_CLOSE,  LANG_C },
   { "%:", CT_POUND,        LANG_C },
   { "<>", CT_COMPARE,      LANG_D },
   { "!>", CT_COMPARE,      LANG_D },
   { "!<", CT_COMPARE,      LANG_D },
};

/* 1-char symbols */
static const struct symbol_tag symbols1[] =
{
   { "#", CT_POUND,        LANG_C | LANG_CPP },
   { "%", CT_ARITH,        LANG_ALL },
   { "&", CT_AMP,          LANG_ALL },
   { "*", CT_STAR,         LANG_ALL },  /* changed to CT_DEREF or CT_ARITH */
   { "+", CT_ARITH,        LANG_ALL },
   { "^", CT_ARITH,        LANG_ALL },
   { "-", CT_MINUS,        LANG_ALL },  /* changed to CT_NEG or CT_ARITH */
   { "+", CT_PLUS,         LANG_ALL },  /* may change to CT_ARITH */
   { "|", CT_ARITH,        LANG_ALL },
   { "/", CT_ARITH,        LANG_ALL },
   { "!", CT_NOT,          LANG_ALL },
   { "~", CT_INV,          LANG_ALL },
   { ",", CT_COMMA,        LANG_ALL },
   { ".", CT_MEMBER,       LANG_ALL },
   { ":", CT_COLON,        LANG_ALL },
   { ";", CT_SEMICOLON,    LANG_ALL },
   { "<", CT_COMPARE,      LANG_ALL },
   { ">", CT_COMPARE,      LANG_ALL },
   { "=", CT_ASSIGN,       LANG_ALL },
   { "?", CT_QUESTION,     LANG_ALL },
   { "(", CT_PAREN_OPEN,   LANG_ALL },
   { ")", CT_PAREN_CLOSE,  LANG_ALL },
   { "[", CT_SQUARE_OPEN,  LANG_ALL },
   { "]", CT_SQUARE_CLOSE, LANG_ALL },
   { "{", CT_BRACE_OPEN,   LANG_ALL },
   { "}", CT_BRACE_CLOSE,  LANG_ALL },
};

const struct symbol_tag *find_punctuator(const char *str, uint8_t lang_flags)
{
   int i;

   /* Check 4 char symbols */
   for (i = 0; i < ARRAY_SIZE(symbols4); i++)
   {
      if (((lang_flags & symbols4[i].lang_flags) != 0) &&
          (strncmp(str, symbols4[i].tag, 4) == 0))
      {
         return(&symbols4[i]);
      }
   }

   /* Check 3 char symbols */
   for (i = 0; i < ARRAY_SIZE(symbols3); i++)
   {
      if (((lang_flags & symbols3[i].lang_flags) != 0) &&
          (strncmp(str, symbols3[i].tag, 3) == 0))
      {
         return(&symbols3[i]);
      }
   }

   /* Check 2 char symbols */
   for (i = 0; i < ARRAY_SIZE(symbols2); i++)
   {
      if (((lang_flags & symbols2[i].lang_flags) != 0) &&
          (strncmp(str, symbols2[i].tag, 2) == 0))
      {
         return(&symbols2[i]);
      }
   }

   /* Check 1 char symbols */
   for (i = 0; i < ARRAY_SIZE(symbols1); i++)
   {
      if (((lang_flags & symbols1[i].lang_flags) != 0) &&
          (*str == *symbols1[i].tag))
      {
         return(&symbols1[i]);
      }
   }

   return(NULL);
}

