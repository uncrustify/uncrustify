/**
 * @file token_enum.c
 * List of the different tokens used in the program.
 *
 * $Id$
 */
#ifndef TOKEN_ENUM_H_INCLUDED
#define TOKEN_ENUM_H_INCLUDED

/**
 * This is an enum of all the different chunks/tokens/elements that the
 * program can work with.  The parser and scanner assigns one of these to
 * each chunk/token.
 *
 * The script 'make_chunk_names.sh' creates token_names.h, so be sure to run
 * that after adding or removing an entry.
 */
typedef enum
{
   CT_NONE,
   CT_SOF,           /* Start of the file */
   CT_EOF,
   CT_UNKNOWN,

   CT_WHITESPACE,    /* whitespace without any newlines */
   CT_NEWLINE,       /* CRA, one or more newlines */
   CT_NL_CONT,       /* CRA, backslash-newline */
   CT_COMMENT_CPP,   /* C++ comment (always followed by CT_NEWLINE) */
   CT_COMMENT,       /* C-comment, single line */
   CT_COMMENT_MULTI, /* Multi-lined comment */
   CT_COMMENT_EMBED, /* comment parent_type: non-newline before and after */
   CT_COMMENT_START, /* comment parent_type: newline before */
   CT_COMMENT_END,   /* comment parent_type: newline after */
   CT_COMMENT_WHOLE, /* comment parent_type: newline before and after */

   CT_WORD,          /* variable, type, function name, etc */
   CT_NUMBER,
   CT_STRING,        /* quoted string "hi" or 'a' or <in> for include */
   CT_IF,            /* built-in keywords */
   CT_ELSE,
   CT_FOR,
   CT_WHILE,
   CT_WHILE_OF_DO,
   CT_SWITCH,
   CT_CASE,
   CT_DO,
   CT_VOLATILE,
   CT_TYPEDEF,
   CT_STRUCT,
   CT_ENUM,
   CT_SIZEOF,
   CT_RETURN,
   CT_BREAK,
   CT_UNION,
   CT_GOTO,
   CT_CONTINUE,
   CT_CAST,                /* cast(exp) -- as used in D */
   CT_TYPE_CAST,           /* static_cast<type>(exp) */
   CT_TYPENAME,            /* typename type */
   CT_TEMPLATE,            /* template<...> */

   CT_ASSIGN,              /* =, +=, /=, etc */
   CT_SASSIGN,             /* 'and_eq' */
   CT_COMPARE,             /* ==, !=, <=, >= */
   CT_SCOMPARE,            /* compare op that is a string 'is', 'neq' */
   CT_BOOL,                /* || or && */
   CT_SBOOL,               /* or, and */
   CT_ARITH,               /* +, -, /, <<, etc */
   CT_SARITH,              /* 'not', 'xor' */
   CT_DEREF,               /* * dereference */
   CT_INCDEC_BEFORE,       /* ++a or --a */
   CT_INCDEC_AFTER,        /* a++ or a-- */
   CT_MEMBER,              /* . or -> */
   CT_INV,                 /* ~ */
   CT_NOT,                 /* ! */
   CT_ADDR,                /* & */
   CT_NEG,                 /* - as in -1 */
   CT_POS,                 /* + as in +1 */
   CT_STAR,                /* * : raw char to be changed */
   CT_PLUS,                /* + : raw char to be changed */
   CT_MINUS,               /* - : raw char to be changed */
   CT_AMP,                 /* & : raw char to be changed */

   CT_POUND,               /* # */
   CT_PREPROC,             /* # at the start of a line */
   CT_PREPROC_BODY,        /* body of every preproc EXCEPT #define */
   CT_PP,                  /* ## */
   CT_ELIPSIS,             /* ... */

   CT_SEMICOLON,
   CT_COLON,
   CT_CASE_COLON,
   CT_Q_COLON,
   CT_QUESTION,
   CT_COMMA,

   CT_ASM,
   CT_CATCH,
   CT_CLASS,
   CT_DELETE,
   CT_EXPORT,
   CT_FRIEND,
   CT_MUTABLE,
   CT_NAMESPACE,
   CT_NEW,              /* may turn into CT_PBRACED if followed by a '(' */
   CT_OPERATOR,
   CT_PRIVATE,
   CT_THROW,
   CT_TRY,
   CT_USING,
   CT_SUPER,
   CT_DELEGATE,

   /* note for paren/brace/square pairs: close MUST be open + 1 */
   CT_PAREN_OPEN,
   CT_PAREN_CLOSE,

   CT_ANGLE_OPEN,       /* template<T*> */
   CT_ANGLE_CLOSE,

   CT_SPAREN_OPEN,      /* 'special' paren after if/for/switch/while */
   CT_SPAREN_CLOSE,

   CT_FPAREN_OPEN,      /* 'function' paren after fcn/macro fcn */
   CT_FPAREN_CLOSE,

   CT_BRACE_OPEN,
   CT_BRACE_CLOSE,

   CT_VBRACE_OPEN,
   CT_VBRACE_CLOSE,

   CT_SQUARE_OPEN,
   CT_SQUARE_CLOSE,

   CT_TSQUARE,          /* special case of [] */

   /* agregate types */
   CT_LABEL,            /* a non-case label */
   CT_LABEL_COLON,      /* the colon for a label */
   CT_FUNCTION,         /* function - unspecified, call mark_function() */
   CT_FUNC_CALL,        /* function call */
   CT_FUNC_DEF,         /* function definition/implementation */
   CT_FUNC_PROTO,       /* function prototype */
   CT_MACRO_FUNC,       /* function-like macro */
   CT_MACRO,            /* a macro def */
   CT_QUALIFIER,        /* static, const, etc */
   CT_PQUALIFIER,       /* paren'd qualifier: align(4) struct a { } */
   CT_TYPE,
   CT_PTR_TYPE,         /* a '*' as part of a type */

   CT_BIT_COLON,        /* a ':' in a variable declaration */

   CT_PP_DEFINE,        /* #define */
   CT_PP_DEFINED,       /* #if defined */
   CT_PP_INCLUDE,       /* #include */
   CT_PP_IF,            /* #if, #ifdef, or #ifndef */
   CT_PP_ELSE,          /* #else or #elif */
   CT_PP_ENDIF,         /* #endif */
   CT_PP_OTHER,         /* #line, #error, #pragma, etc */

   CT_PRAGMA,

   /* C-sharp crap */
   CT_LOCK,             /* lock/unlock */
   CT_AS,
   CT_IN,               /* "foreach (T c in x)" or "foo(in char c)" or "in { ..." */
   CT_BRACED,           /* simple braced items: try {} */
   CT_PBRACED,          /* simple paren-braced: version (x) { } */
   CT_POBRACED,         /* simple optional-paren-braced: catch (x) { } */
   CT_VBRACED,          /* some value followed by braces: namespace foo { */
   CT_VERSION,          /* turns into CT_IF if not followed by '=' */
   CT_THIS,             /* may turn into CT_PBRACED if followed by a '(' */
   CT_BASE,             /* C# thingy */
   CT_DEFAULT,          /* may be changed into CT_CASE */
   CT_GETSET,           /* must be followed by CT_BRACE_OPEN or reverts to CT_WORD */
} c_token_t;

#endif   /* TOKEN_ENUM_H_INCLUDED */

