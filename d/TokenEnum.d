/**
 * @file TokenEnum.d
 * List of the different tokens used in the program.
 *
 * $Id: token_enum.h 136 2006-03-28 03:23:34Z bengardner $
 */

module uncrustify.tokenenum;

/**
 * This is an enum of all the different chunks/tokens/elements that the
 * program can work with.  The parser and scanner assigns one of these to
 * each chunk/token.
 *
 * The script 'make_chunk_names.sh' creates token_names.h, so be sure to run
 * that after adding or removing an entry.
 */
enum Token
{
   NONE,
   SOF,           /* Start of the file */
   EOF,
   UNKNOWN,

   WHITESPACE,    /* whitespace without any newlines */
   NEWLINE,       /* CRA, one or more newlines */
   NL_CONT,       /* CRA, backslash-newline */
   COMMENT_CPP,   /* C++ comment (always followed by CT_NEWLINE) */
   COMMENT,       /* C-comment, single line */
   COMMENT_MULTI, /* Multi-lined comment */
   COMMENT_EMBED, /* comment parent_type: non-newline before and after */
   COMMENT_START, /* comment parent_type: newline before */
   COMMENT_END,   /* comment parent_type: newline after */
   COMMENT_WHOLE, /* comment parent_type: newline before and after */

   WORD,          /* variable, type, function name, etc */
   NUMBER,
   STRING,        /* quoted string "hi" or 'a' or <in> for include */
   IF,            /* built-in keywords */
   ELSE,
   FOR,
   WHILE,
   WHILE_OF_DO,
   SWITCH,
   CASE,
   DO,
   VOLATILE,
   TYPEDEF,
   STRUCT,
   ENUM,
   SIZEOF,
   RETURN,
   BREAK,
   UNION,
   GOTO,
   CONTINUE,
   CAST,                /* cast(exp) -- as used in D */
   TYPE_CAST,           /* static_cast<type>(exp) */
   TYPENAME,            /* typename type */
   TEMPLATE,            /* template<...> */

   ASSIGN,              /* =, +=, /=, etc */
   SASSIGN,             /* 'and_eq' */
   COMPARE,             /* ==, !=, <=, >= */
   SCOMPARE,            /* compare op that is a string 'is', 'neq' */
   BOOL,                /* || or && */
   SBOOL,               /* or, and */
   ARITH,               /* +, -, /, <<, etc */
   SARITH,              /* 'not', 'xor' */
   DEREF,               /* * dereference */
   INCDEC_BEFORE,       /* ++a or --a */
   INCDEC_AFTER,        /* a++ or a-- */
   MEMBER,              /* . or -> */
   DC_MEMBER,           /* :: */
   INV,                 /* ~ */
   DESTRUCTOR,          /* ~ */
   NOT,                 /* ! */
   D_TEMPLATE,          /* ! as in Foo!(A) */
   ADDR,                /* & */
   NEG,                 /* - as in -1 */
   POS,                 /* + as in +1 */
   STAR,                /* * : raw char to be changed */
   PLUS,                /* + : raw char to be changed */
   MINUS,               /* - : raw char to be changed */
   AMP,                 /* & : raw char to be changed */

   POUND,               /* # */
   PREPROC,             /* # at the start of a line */
   PREPROC_BODY,        /* body of every preproc EXCEPT #define */
   PP,                  /* ## */
   ELIPSIS,             /* ... */

   SEMICOLON,
   COLON,
   CASE_COLON,
   CLASS_COLON,         /* colon after a class def or constructor */
   Q_COLON,
   QUESTION,
   COMMA,

   ASM,
   CATCH,
   CLASS,
   DELETE,
   EXPORT,
   FRIEND,
   MUTABLE,
   NAMESPACE,
   NEW,              /* may turn into CT_PBRACED if followed by a '(' */
   OPERATOR,
   PRIVATE,
   THROW,
   TRY,
   USING,
   SUPER,
   DELEGATE,

   /* note for paren/brace/square pairs: close MUST be open + 1 */
   PAREN_OPEN,
   PAREN_CLOSE,

   ANGLE_OPEN,       /* template<T*> */
   ANGLE_CLOSE,

   SPAREN_OPEN,      /* 'special' paren after if/for/switch/while */
   SPAREN_CLOSE,

   FPAREN_OPEN,      /* 'function' paren after fcn/macro fcn */
   FPAREN_CLOSE,

   BRACE_OPEN,
   BRACE_CLOSE,

   VBRACE_OPEN,
   VBRACE_CLOSE,

   SQUARE_OPEN,
   SQUARE_CLOSE,

   TSQUARE,          /* special case of [] */

   /* agregate types */
   LABEL,            /* a non-case label */
   LABEL_COLON,      /* the colon for a label */
   FUNCTION,         /* function - unspecified, call mark_function() */
   FUNC_CALL,        /* function call */
   FUNC_DEF,         /* function definition/implementation */
   FUNC_PROTO,       /* function prototype */
   MACRO_FUNC,       /* function-like macro */
   MACRO,            /* a macro def */
   QUALIFIER,        /* static, const, etc */
   ALIGN,            /* paren'd qualifier: align(4) struct a { } */
   TYPE,
   PTR_TYPE,         /* a '*' as part of a type */

   BIT_COLON,        /* a ':' in a variable declaration */

   PP_DEFINE,        /* #define */
   PP_DEFINED,       /* #if defined */
   PP_INCLUDE,       /* #include */
   PP_IF,            /* #if, #ifdef, or #ifndef */
   PP_ELSE,          /* #else or #elif */
   PP_ENDIF,         /* #endif */
   PP_OTHER,         /* #line, #error, #pragma, etc */

   PRAGMA,

   /* C-sharp crap */
   LOCK,             /* lock/unlock */
   AS,
   IN,               /* "foreach (T c in x)" or "foo(in char c)" or "in { ..." */
   BRACED,           /* simple braced items: try {} */
   PBRACED,          /* simple paren-braced: version (x) { } */
   POBRACED,         /* simple optional-paren-braced: catch (x) { } */
   VBRACED,          /* some value followed by braces: namespace foo { */
   VERSION,          /* turns into CT_IF if not followed by '=' */
   THIS,             /* may turn into CT_PBRACED if followed by a '(' */
   BASE,             /* C# thingy */
   DEFAULT,          /* may be changed into CT_CASE */
   GETSET,           /* must be followed by CT_BRACE_OPEN or reverts to CT_WORD */
   CONCAT,           /* The '~' between strings */
};

