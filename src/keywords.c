/**
 * @file keywords.c
 * Manages the table of keywords.
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
static chunk_tag_t keywords[] =
{
   { "__const__",        CT_QUALIFIER,  LANG_ALL },
   { "__inline__",       CT_QUALIFIER,  LANG_ALL },
   { "__signed__",       CT_TYPE,       LANG_ALL },
   { "__typeof__",       CT_SIZEOF,     LANG_ALL },
   { "__volatile__",     CT_QUALIFIER,  LANG_ALL },
   { "_Bool",            CT_TYPE,       LANG_ALL },
   { "_Complex",         CT_TYPE,       LANG_ALL },
   { "_Imaginary",       CT_TYPE,       LANG_ALL },
   { "alignof",          CT_SIZEOF,     LANG_ALL },
   { "and",              CT_BOOL,       LANG_ALL },
   { "and_eq",           CT_ASSIGN,     LANG_ALL },
   { "asm",              CT_ASM,        LANG_ALL },
   { "auto",             CT_QUALIFIER,  LANG_ALL },
   { "bitand",           CT_ARITH,      LANG_ALL },
   { "bitor",            CT_ARITH,      LANG_ALL },
   { "bool",             CT_TYPE,       LANG_ALL },
   { "break",            CT_BREAK,      LANG_ALL },
   { "case",             CT_CASE,       LANG_ALL },
   { "catch",            CT_CATCH,      LANG_ALL },
   { "char",             CT_TYPE,       LANG_ALL },
   { "class",            CT_CLASS,      LANG_ALL },
   { "compl",            CT_ARITH,      LANG_ALL },
   { "const",            CT_QUALIFIER,  LANG_ALL },
   { "const_cast",       CT_TYPE_CAST,  LANG_ALL },
   { "default",          CT_CASE,       LANG_ALL },
   { "defined",          CT_PP_DEFINED, LANG_ALL | FLAG_PP },
   { "delete",           CT_DELETE,     LANG_ALL },
   { "do",               CT_DO,         LANG_ALL },
   { "double",           CT_TYPE,       LANG_ALL },
   { "dynamic_cast",     CT_TYPE_CAST,  LANG_ALL },
   { "else",             CT_ELSE,       LANG_ALL },
   { "enum",             CT_ENUM,       LANG_ALL },
   { "explicit",         CT_TYPE,       LANG_ALL },
   { "export",           CT_EXPORT,     LANG_ALL },
   { "extern",           CT_TYPE,       LANG_ALL },
   { "false",            CT_TYPE,       LANG_ALL },
   { "float",            CT_TYPE,       LANG_ALL },
   { "for",              CT_FOR,        LANG_ALL },
   { "friend",           CT_FRIEND,     LANG_ALL },
   { "goto",             CT_GOTO,       LANG_ALL },
   { "if",               CT_IF,         LANG_ALL },
   { "in",               CT_COMPARE,    LANG_D },
   { "inline",           CT_QUALIFIER,  LANG_ALL },
   { "int",              CT_TYPE,       LANG_ALL },
   { "is",               CT_COMPARE,    LANG_D },
   { "long",             CT_TYPE,       LANG_ALL },
   { "mutable",          CT_MUTABLE,    LANG_ALL },
   { "namespace",        CT_NAMESPACE,  LANG_ALL },
   { "new",              CT_NEW,        LANG_ALL },
   { "not",              CT_ARITH,      LANG_ALL },
   { "not_eq",           CT_COMPARE,    LANG_ALL },
   { "operator",         CT_OPERATOR,   LANG_ALL },
   { "or",               CT_BOOL,       LANG_ALL },
   { "or_eq",            CT_ASSIGN,     LANG_ALL },
   { "private",          CT_PRIVATE,    LANG_ALL },
   { "protected",        CT_PRIVATE,    LANG_ALL },
   { "public",           CT_PRIVATE,    LANG_ALL },
   { "register",         CT_QUALIFIER,  LANG_ALL },
   { "reinterpret_cast", CT_TYPE_CAST,  LANG_ALL },
   { "restrict",         CT_QUALIFIER,  LANG_ALL },
   { "return",           CT_RETURN,     LANG_ALL },
   { "short",            CT_TYPE,       LANG_ALL },
   { "signed",           CT_TYPE,       LANG_ALL },
   { "sizeof",           CT_SIZEOF,     LANG_ALL },
   { "static",           CT_QUALIFIER,  LANG_ALL },
   { "static_cast",      CT_TYPE_CAST,  LANG_ALL },
   { "struct",           CT_STRUCT,     LANG_ALL },
   { "switch",           CT_SWITCH,     LANG_ALL },
   { "template",         CT_TEMPLATE,   LANG_ALL },
   { "this",             CT_TYPE,       LANG_ALL },
   { "throw",            CT_THROW,      LANG_ALL },
   { "true",             CT_TYPE,       LANG_ALL },
   { "try",              CT_TRY,        LANG_ALL },
   { "typedef",          CT_TYPEDEF,    LANG_ALL },
   { "typeid",           CT_SIZEOF,     LANG_ALL },
   { "typename",         CT_TYPENAME,   LANG_ALL },
   { "typeof",           CT_SIZEOF,     LANG_ALL },
   { "union",            CT_UNION,      LANG_ALL },
   { "unsigned",         CT_TYPE,       LANG_ALL },
   { "using",            CT_USING,      LANG_ALL },
   { "virtual",          CT_QUALIFIER,  LANG_ALL },
   { "void",             CT_TYPE,       LANG_ALL },
   { "volatile",         CT_QUALIFIER,  LANG_ALL },
   { "wchar_t",          CT_TYPE,       LANG_ALL },
   { "while",            CT_WHILE,      LANG_ALL },
   { "xor",              CT_ARITH,      LANG_ALL },
   { "xor_eq",           CT_ASSIGN,     LANG_ALL },
};

/**
 * Compares two chunk_tag_t entries using strcmp on the strings
 *
 * @param p1   The 'left' entry
 * @param p2   The 'right' entry
 */
static int kw_compare(const void *p1, const void *p2)
{
   const chunk_tag_t *t1 = p1;
   const chunk_tag_t *t2 = p2;

   return(strcmp(t1->tag, t2->tag));
}

/**
 * A pre-loaded hash table would be faster, but I don't think speed is all
 * that important at this point.
 */
const chunk_tag_t *find_keyword(const char *word, int len)
{
   chunk_tag_t tag;
   char        buf[32];

   if (len > (sizeof(buf) - 1))
   {
      return(NULL);
   }
   memcpy(buf, word, len);
   buf[len] = 0;

   tag.tag = buf;

   return(bsearch(&tag, keywords, ARRAY_SIZE(keywords), sizeof(keywords[0]),
                  kw_compare));
}

