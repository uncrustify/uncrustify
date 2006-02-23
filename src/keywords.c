/**
 * @file keywords.c
 * Manages the table of keywords.
 *
 * TODO: add a 'language' field for C/C++/D/Java
 *
 * $Id: keywords.c,v 1.6 2006/02/14 03:25:19 bengardner Exp $
 */

#include "cparse_types.h"
#include <string.h>
#include <stdlib.h>


/**
 * interesting keywords - keep sorted.
 * Table should include the Name, Type, and Language flags.
 */
static struct keyword_tag keywords[] =
{
   { "__const__",         9, CT_QUALIFIER,  LANG_ALL },
   { "__inline__",       10, CT_QUALIFIER,  LANG_ALL },
   { "__signed__",       10, CT_TYPE,       LANG_ALL },
   { "__typeof__",       10, CT_SIZEOF,     LANG_ALL },
   { "__volatile__",     12, CT_QUALIFIER,  LANG_ALL },
   { "_Bool",             5, CT_TYPE,       LANG_ALL },
   { "_Complex",          8, CT_TYPE,       LANG_ALL },
   { "_Imaginary",       10, CT_TYPE,       LANG_ALL },
   { "alignof",           7, CT_SIZEOF,     LANG_ALL },
   { "and",               3, CT_BOOL,       LANG_ALL },
   { "and_eq",            6, CT_ASSIGN,     LANG_ALL },
   { "asm",               3, CT_ASM,        LANG_ALL },
   { "auto",              4, CT_QUALIFIER,  LANG_ALL },
   { "bitand",            6, CT_ARITH,      LANG_ALL },
   { "bitor",             5, CT_ARITH,      LANG_ALL },
   { "bool",              4, CT_TYPE,       LANG_ALL },
   { "break",             5, CT_BREAK,      LANG_ALL },
   { "case",              4, CT_CASE,       LANG_ALL },
   { "catch",             5, CT_CATCH,      LANG_ALL },
   { "char",              4, CT_TYPE,       LANG_ALL },
   { "class",             5, CT_CLASS,      LANG_ALL },
   { "compl",             5, CT_ARITH,      LANG_ALL },
   { "const",             5, CT_QUALIFIER,  LANG_ALL },
   { "const_cast",       10, CT_TYPE_CAST,  LANG_ALL },
   { "default",           7, CT_CASE,       LANG_ALL },
   { "defined",           7, CT_PP_DEFINED, LANG_ALL | FLAG_PP },
   { "delete",            6, CT_DELETE,     LANG_ALL },
   { "do",                2, CT_DO,         LANG_ALL },
   { "double",            6, CT_TYPE,       LANG_ALL },
   { "dynamic_cast",     12, CT_TYPE_CAST,  LANG_ALL },
   { "else",              4, CT_ELSE,       LANG_ALL },
   { "enum",              4, CT_ENUM,       LANG_ALL },
   { "explicit",          8, CT_TYPE,       LANG_ALL },
   { "export",            6, CT_EXPORT,     LANG_ALL },
   { "extern",            6, CT_TYPE,       LANG_ALL },
   { "false",             5, CT_TYPE,       LANG_ALL },
   { "float",             5, CT_TYPE,       LANG_ALL },
   { "for",               3, CT_FOR,        LANG_ALL },
   { "friend",            6, CT_FRIEND,     LANG_ALL },
   { "goto",              4, CT_GOTO,       LANG_ALL },
   { "if",                2, CT_IF,         LANG_ALL },
   { "in",                2, CT_COMPARE,    LANG_D },
   { "inline",            6, CT_QUALIFIER,  LANG_ALL },
   { "int",               3, CT_TYPE,       LANG_ALL },
   { "is",                2, CT_COMPARE,    LANG_D },
   { "long",              4, CT_TYPE,       LANG_ALL },
   { "mutable",           7, CT_MUTABLE,    LANG_ALL },
   { "namespace",         9, CT_NAMESPACE,  LANG_ALL },
   { "new",               3, CT_NEW,        LANG_ALL },
   { "not",               3, CT_ARITH,      LANG_ALL },
   { "not_eq",            6, CT_COMPARE,    LANG_ALL },
   { "operator",          8, CT_OPERATOR,   LANG_ALL },
   { "or",                2, CT_BOOL,       LANG_ALL },
   { "or_eq",             5, CT_ASSIGN,     LANG_ALL },
   { "private",           7, CT_PRIVATE,    LANG_ALL },
   { "protected",         9, CT_PRIVATE,    LANG_ALL },
   { "public",            6, CT_PRIVATE,    LANG_ALL },
   { "register",          8, CT_QUALIFIER,  LANG_ALL },
   { "reinterpret_cast", 16, CT_TYPE_CAST,  LANG_ALL },
   { "restrict",          8, CT_QUALIFIER,  LANG_ALL },
   { "return",            6, CT_RETURN,     LANG_ALL },
   { "short",             5, CT_TYPE,       LANG_ALL },
   { "signed",            6, CT_TYPE,       LANG_ALL },
   { "sizeof",            6, CT_SIZEOF,     LANG_ALL },
   { "static",            6, CT_QUALIFIER,  LANG_ALL },
   { "static_cast",      11, CT_TYPE_CAST,  LANG_ALL },
   { "struct",            6, CT_STRUCT,     LANG_ALL },
   { "switch",            6, CT_SWITCH,     LANG_ALL },
   { "template",          8, CT_TEMPLATE,   LANG_ALL },
   { "this",              4, CT_TYPE,       LANG_ALL },
   { "throw",             5, CT_THROW,      LANG_ALL },
   { "true",              4, CT_TYPE,       LANG_ALL },
   { "try",               3, CT_TRY,        LANG_ALL },
   { "typedef",           7, CT_TYPEDEF,    LANG_ALL },
   { "typeid",            6, CT_SIZEOF,     LANG_ALL },
   { "typename",          8, CT_TYPENAME,   LANG_ALL },
   { "typeof",            6, CT_SIZEOF,     LANG_ALL },
   { "union",             5, CT_UNION,      LANG_ALL },
   { "unsigned",          8, CT_TYPE,       LANG_ALL },
   { "using",             5, CT_USING,      LANG_ALL },
   { "virtual",           7, CT_QUALIFIER,  LANG_ALL },
   { "void",              4, CT_TYPE,       LANG_ALL },
   { "volatile",          8, CT_QUALIFIER,  LANG_ALL },
   { "wchar_t",           7, CT_TYPE,       LANG_ALL },
   { "while",             5, CT_WHILE,      LANG_ALL },
   { "xor",               3, CT_ARITH,      LANG_ALL },
   { "xor_eq",            6, CT_ASSIGN,     LANG_ALL },
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


