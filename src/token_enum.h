/**
 * @file token_enum.h
 * List of the different tokens used in the program.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015
 * @license GPL v2+
 */
#ifndef TOKEN_ENUM_H_INCLUDED
#define TOKEN_ENUM_H_INCLUDED

/**
 * abbreviations used:
 * CT = chunk type
 */


/**
 * This is an enum of all the different chunks/tokens/elements that the
 * program can work with.  The parser and scanner assigns one of these to
 * each chunk/token.
 *
 * The script 'make_token_names.sh' creates token_names.h, so be sure to run
 * that after adding or removing an entry.
 */
enum c_token_t
{
   CT_NONE,
   CT_EOF,
   CT_UNKNOWN,

   CT_JUNK,          // junk collected when parsing is disabled

   CT_WHITESPACE,    // whitespace without any newlines
   CT_SPACE,         // a fixed number of spaces to support weird spacing rules
   CT_NEWLINE,       // CRA, one or more newlines
   CT_NL_CONT,       // CRA, backslash-newline
   CT_COMMENT_CPP,   // C++ comment (always followed by CT_NEWLINE)
   CT_COMMENT,       // C-comment, single line
   CT_COMMENT_MULTI, // Multi-lined comment
   CT_COMMENT_EMBED, // comment parent_type: non-newline before and after
   CT_COMMENT_START, // comment parent_type: newline before
   CT_COMMENT_END,   // comment parent_type: newline after
   CT_COMMENT_WHOLE, // comment parent_type: newline before and after
   CT_COMMENT_ENDIF, // C-comment, single line, after ENDIF

   CT_IGNORED,       // a chunk of ignored text

   CT_WORD,          // variable, type, function name, etc
   CT_NUMBER,
   CT_NUMBER_FP,
   CT_STRING,        // quoted string "hi" or 'a' or <in> for include
   CT_STRING_MULTI,  // quoted string with embedded newline
   CT_IF,            // built-in keywords
   CT_ELSE,
   CT_ELSEIF,
   CT_FOR,
   CT_WHILE,
   CT_WHILE_OF_DO,
   CT_SWITCH,
   CT_CASE,
   CT_DO,
   CT_SYNCHRONIZED,
   CT_VOLATILE,
   CT_TYPEDEF,
   CT_STRUCT,
   CT_ENUM,
   CT_ENUM_CLASS,
   CT_SIZEOF,
   CT_RETURN,
   CT_BREAK,
   CT_UNION,
   CT_GOTO,
   CT_CONTINUE,
   CT_C_CAST,              // C-style cast:   "(int)5.6"
   CT_CPP_CAST,            // C++-style cast: "int(5.6)"
   CT_D_CAST,              // D-style cast:   "cast(type)" and "const(type)"
   CT_TYPE_CAST,           // static_cast<type>(exp)
   CT_TYPENAME,            // typename type
   CT_TEMPLATE,            // template<...>
   CT_WHERE_SPEC,          // 'where' : used in C# generic constraint

   CT_ASSIGN,              // =, +=, /=, etc
   CT_ASSIGN_NL,           // Assign followed by a newline - fake item for indenting
   CT_SASSIGN,             // 'and_eq'
   CT_COMPARE,             // ==, !=, <=, >=
   CT_SCOMPARE,            // compare op that is a string 'is', 'neq'
   CT_BOOL,                // || or &&
   CT_SBOOL,               // or, and
   CT_ARITH,               // +, -, /, <<, etc
   CT_SARITH,              // 'not', 'xor'
   CT_CARET,               // ^
   CT_DEREF,               // * dereference
   CT_INCDEC_BEFORE,       // ++a or --a
   CT_INCDEC_AFTER,        // a++ or a--
   CT_MEMBER,              // . or ->
   CT_DC_MEMBER,           // ::
   CT_C99_MEMBER,          // . in structure stuff
   CT_INV,                 // ~
   CT_DESTRUCTOR,          // ~
   CT_NOT,                 // !
   CT_D_TEMPLATE,          // ! as in Foo!(A)
   CT_ADDR,                // &
   CT_NEG,                 // - as in -1
   CT_POS,                 // + as in +1
   CT_STAR,                // * : raw char to be changed
   CT_PLUS,                // + : raw char to be changed
   CT_MINUS,               // - : raw char to be changed
   CT_AMP,                 // & : raw char to be changed
   CT_BYREF,               // & in function def/proto params

   // CT_BITWISE_AND,         // &   // is a CT_ARITH
   // CT_BITWISE_OR,          // |   // is a CT_ARITH
   // CT_BITWISE_EXCLUSIVE_OR,// ^   // is a CT_ARITH
   // CT_BITWISE_NOT,         // ~   // is a CT_ARITH

   CT_POUND,               // #
   CT_PREPROC,             // # at the start of a line
   CT_PREPROC_INDENT,      // # at the start of a line that gets indented: #region
   CT_PREPROC_BODY,        // body of every preproc EXCEPT #define
   CT_PP,                  // ##
   CT_ELLIPSIS,            // ...
   CT_RANGE,               // ..
   CT_NULLCOND,            // ?.

   CT_SEMICOLON,
   CT_VSEMICOLON,          // virtual semicolon for PAWN
   CT_COLON,
   CT_ASM_COLON,
   CT_CASE_COLON,
   CT_CLASS_COLON,         // colon after a class def
   CT_CONSTR_COLON,        // colon after a constructor
   CT_D_ARRAY_COLON,       // D named array initializer colon
   CT_COND_COLON,          // conditional colon in  'b ? t : f'
   CT_WHERE_COLON,         // C# where-constraint colon (after the type)
   CT_QUESTION,
   CT_COMMA,

   CT_ASM,
   CT_ATTRIBUTE,
   CT_CATCH,
   CT_WHEN,
   CT_WHERE,            // C# where clause
   CT_CLASS,
   CT_DELETE,
   CT_EXPORT,
   CT_FRIEND,
   CT_NAMESPACE,
   CT_PACKAGE,
   CT_NEW,              // may turn into CT_PBRACED if followed by a '('
   CT_OPERATOR,
   CT_OPERATOR_VAL,
   CT_PRIVATE,
   CT_PRIVATE_COLON,
   CT_THROW,
   CT_NOEXCEPT,
   CT_TRY,
   CT_BRACED_INIT_LIST,
   CT_USING,
   CT_USING_STMT,       // using (xxx) ...
   CT_D_WITH,           // D: paren+braced
   CT_D_MODULE,
   CT_SUPER,
   CT_DELEGATE,
   CT_BODY,
   CT_DEBUG,
   CT_DEBUGGER,
   CT_INVARIANT,
   CT_UNITTEST,
   CT_UNSAFE,
   CT_FINALLY,
   CT_IMPORT,
   CT_D_SCOPE,
   CT_D_SCOPE_IF,
   CT_LAZY,
   CT_D_MACRO,
   CT_D_VERSION,        // turns into CT_D_VERSION_IF if not followed by '='
   CT_D_VERSION_IF,     // version(x) { }

   // note for paren/brace/square pairs: close MUST be open + 1
   CT_PAREN_OPEN,
   CT_PAREN_CLOSE,

   CT_ANGLE_OPEN,       // template<T*>
   CT_ANGLE_CLOSE,

   CT_SPAREN_OPEN,      // 'special' paren after if/for/switch/while/synchronized
   CT_SPAREN_CLOSE,

   CT_FPAREN_OPEN,      // 'function' paren after fcn/macro fcn
   CT_FPAREN_CLOSE,

   CT_TPAREN_OPEN,      // 'type' paren used in function types
   CT_TPAREN_CLOSE,

   CT_BRACE_OPEN,       // {...}
   CT_BRACE_CLOSE,

   CT_VBRACE_OPEN,      // virtual brace, i.e. brace inserted by uncrustify
   CT_VBRACE_CLOSE,

   CT_SQUARE_OPEN,      // [...]
   CT_SQUARE_CLOSE,

   CT_TSQUARE,          // special case of []

   CT_MACRO_OPEN,       // stuff specified via custom-pair
   CT_MACRO_CLOSE,
   CT_MACRO_ELSE,

   // aggregate types
   CT_LABEL,            // a non-case label
   CT_LABEL_COLON,      // the colon for a label
   CT_FUNCTION,         // function - unspecified, call mark_function()
   CT_FUNC_CALL,        // function call
   CT_FUNC_CALL_USER,   // function call (special user)
   CT_FUNC_DEF,         // function definition/implementation
   CT_FUNC_TYPE,        // function type - foo in "typedef void (*foo)(void)"
   CT_FUNC_VAR,         // foo and parent type of first parens in "void (*foo)(void)"
   CT_FUNC_PROTO,       // function prototype
   CT_FUNC_START,       // global DC member for functions(void ::func())
   CT_FUNC_CLASS_DEF,   // ctor or dtor for a class
   CT_FUNC_CLASS_PROTO, // ctor or dtor for a class
   CT_FUNC_CTOR_VAR,    // variable or class initialization
   CT_FUNC_WRAP,        // macro that wraps the function name
   CT_PROTO_WRAP,       // macro: "RETVAL PROTO_WRAP( fcn_name, (PARAMS))". Parens for PARAMS are optional.
   CT_MACRO_FUNC,       // function-like macro
   CT_MACRO,            // a macro def
   CT_QUALIFIER,        // static, const, etc
   CT_EXTERN,           // extern
   CT_DECLSPEC,         // __declspec
   CT_ALIGN,            // paren'd qualifier: align(4) struct a { }
   CT_TYPE,
   CT_PTR_TYPE,         // a '*' as part of a type
   CT_TYPE_WRAP,        // macro that wraps a type name
   CT_CPP_LAMBDA,       // parent for '[=](...){...}'
   CT_CPP_LAMBDA_RET,   // '->' in '[=](...) -> type {...}'
   CT_BIT_COLON,        // a ':' in a variable declaration

   CT_OC_DYNAMIC,
   CT_OC_END,           // ObjC: @end
   CT_OC_IMPL,          // ObjC: @implementation
   CT_OC_INTF,          // ObjC: @interface
   CT_OC_PROTOCOL,      // ObjC: @protocol or @protocol()
   CT_OC_PROTO_LIST,    // ObjC: protocol list < > (parent token only)
   CT_OC_GENERIC_SPEC,  // ObjC: specification of generic  < >
   CT_OC_PROPERTY,      // ObjC: @property
   CT_OC_CLASS,         // ObjC: the name after @interface or @implementation
   CT_OC_CLASS_EXT,     // ObjC: a pair of empty parens after the class name in a @interface or @implementation
   CT_OC_CATEGORY,      // ObjC: the category name in parens after the class name in a @interface or @implementation
   CT_OC_SCOPE,         // ObjC: the '-' or '+' in '-(void) func: (int) i;'
   CT_OC_MSG,           // ObjC: parent type to '[', ']' and ';' in '[class func : param name: param];'
   CT_OC_MSG_CLASS,     // ObjC: 'class' in  '[class func : param name: param];' (see also PCF_IN_OC_MSG)
   CT_OC_MSG_FUNC,      // ObjC: 'func' in  '[class func : param name: param];' (see also PCF_IN_OC_MSG)
   CT_OC_MSG_NAME,      // ObjC: 'name' in  '[class func : param name: param];' (see also PCF_IN_OC_MSG)
   CT_OC_MSG_SPEC,      // ObjC: msg spec '-(void) func: (int) i;'
   CT_OC_MSG_DECL,      // ObjC: msg decl '-(void) func: (int) i { }'
   CT_OC_RTYPE,         // ObjC: marks parens of the return type after scope
   CT_OC_ATYPE,         // ObjC: marks parens of the arg type after scope
   CT_OC_COLON,         // ObjC: the colon in a msg spec
   CT_OC_DICT_COLON,    // ObjC: colon in dictionary constant: "KEY: VALUE"
   CT_OC_SEL,           // ObjC: @selector
   CT_OC_SEL_NAME,      // ObjC: selector name
   CT_OC_BLOCK,         // ObjC: block parent type.
   CT_OC_BLOCK_ARG,     // ObjC: block arguments parent type.
   CT_OC_BLOCK_TYPE,    // ObjC: block declaration parent type, e.g. mainly the '(^block_t)' in 'void (^block_t)(int arg);'
   CT_OC_BLOCK_EXPR,    // ObjC: block expression with arg: '^(int arg) { arg++; };' and without (called a block literal): '^{ ... };'
   CT_OC_BLOCK_CARET,   // ObjC: block pointer caret: '^'
   CT_OC_AT,            // ObjC: boxed constants using '@'
   CT_OC_PROPERTY_ATTR, // ObjC: property attribute (strong, weak, readonly, etc...)

   // start PP types
   CT_PP_DEFINE,        // #define
   CT_PP_DEFINED,       // #if defined
   CT_PP_INCLUDE,       // #include
   CT_PP_IF,            // #if, #ifdef, or #ifndef
   CT_PP_ELSE,          // #else or #elif
   CT_PP_ENDIF,         // #endif
   CT_PP_ASSERT,
   CT_PP_EMIT,
   CT_PP_ENDINPUT,
   CT_PP_ERROR,
   CT_PP_FILE,
   CT_PP_LINE,
   CT_PP_SECTION,
   CT_PP_ASM,           // start of assembly code section
   CT_PP_UNDEF,
   CT_PP_PROPERTY,

   CT_PP_BODYCHUNK,     // everything after this gets put in CT_PREPROC_BODY

   CT_PP_PRAGMA,        // pragma's should not be altered
   CT_PP_REGION,        // C# #region
   CT_PP_ENDREGION,     // C# #endregion
   CT_PP_REGION_INDENT, // Dummy token for indenting a C# #region
   CT_PP_IF_INDENT,     // Dummy token for indenting a #if stuff
   CT_PP_IGNORE,        // Dummy token for ignoring a certain preprocessor directive (do not do any processing)
   CT_PP_OTHER,         // #line, #error, #pragma, etc
   // end PP types

   // PAWN stuff
   CT_CHAR,
   CT_DEFINED,
   CT_FORWARD,
   CT_NATIVE,
   CT_STATE,
   CT_STOCK,
   CT_TAGOF,
   CT_DOT,
   CT_TAG,
   CT_TAG_COLON,

   // C-sharp
   CT_LOCK,             // lock/unlock
   CT_AS,
   CT_IN,               // "foreach (T c in x)" or "foo(in char c)" or "in { ..."
   CT_BRACED,           // simple braced items: try {}
   CT_THIS,             // may turn into CT_PBRACED if followed by a '('
   CT_BASE,             // C# thingy
   CT_DEFAULT,          // may be changed into CT_CASE
   CT_GETSET,           // must be followed by CT_BRACE_OPEN or reverts to CT_WORD
   CT_GETSET_EMPTY,     // get/set/add/remove followed by a semicolon
   CT_CONCAT,           // The '~' between strings
   CT_CS_SQ_STMT,       // '[assembly: xxx]' or '[Attribute()]' or '[Help()]', etc
   CT_CS_SQ_COLON,      // the colon in one of those [] thingys
   CT_CS_PROPERTY,      // word or ']' followed by '{'

   // Embedded SQL - always terminated with a semicolon
   CT_SQL_EXEC,         // the 'EXEC' in 'EXEC SQL ...'
   CT_SQL_BEGIN,        // the 'BEGINN' in 'EXEC SQL BEGIN ...'
   CT_SQL_END,          // the 'END' in 'EXEC SQL END ...'
   CT_SQL_WORD,         // CT_WORDs in the 'EXEC SQL' statement

   // Vala stuff
   CT_CONSTRUCT,        // braced "construct { }" or qualifier "(construct int x)"
   CT_LAMBDA,

   // Java
   CT_ASSERT,       // assert EXP1 [ : EXP2 ] ;
   CT_ANNOTATION,   // @interface or @something(...)
   CT_FOR_COLON,    // colon in "for ( TYPE var: EXPR ) { ... }"
   CT_DOUBLE_BRACE, // parent for double brace

   /* Clang */
   CT_CNG_HASINC,   // Clang: __has_include()
   CT_CNG_HASINCN,  // Clang: __has_include_next()

   // extensions for Qt macros
   CT_Q_EMIT,       // guy 2015-10-16
   CT_Q_FOREACH,    // guy 2015-09-23
   CT_Q_FOREVER,    // guy 2015-10-18
   CT_Q_GADGET,     // guy 2016-05-04
   CT_Q_OBJECT,     // guy 2015-10-16

   // Machine Modes
   CT_MODE,         // guy 2016-03-11
   CT_DI,           // guy 2016-03-11
   CT_HI,           // guy 2016-03-11
   CT_QI,           // guy 2016-03-11
   CT_SI,           // guy 2016-03-11
   CT_NOTHROW,      // guy 2016-03-11
   CT_WORD_,        // guy 2016-03-11

   // Token to ignore the content of a block
   CT_IGNORE_CONTENT,

   CT_TOKEN_COUNT_  // NOTE: Keep this the last entry because it's used as a counter.
};

#endif /* TOKEN_ENUM_H_INCLUDED */
