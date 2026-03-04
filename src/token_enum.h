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
 * program can work with. The parser and scanner assigns one of these to
 * each chunk/token.
 */
enum E_Token
{
   // see also const char *get_token_name(E_Token token)
   NONE,
   PARENT_NOT_SET,
   EOFILE,
   UNKNOWN,

   JUNK,              // junk collected when parsing is disabled

   WHITESPACE,        // whitespace without any newlines
   SPACE,             // a fixed number of spaces to support weird spacing rules
   NEWLINE,           // CRA, one or more newlines
   NL_CONT,           // CRA, backslash-newline
   FORM_FEED,         // character 12
   COMMENT_CPP,       // C++ comment (always followed by NEWLINE)
   COMMENT,           // C-comment, single line
   COMMENT_MULTI,     // Multi-lined comment
   COMMENT_EMBED,     // comment parent_type: non-newline before and after
   COMMENT_START,     // comment parent_type: newline before
   COMMENT_END,       // comment parent_type: newline after
   COMMENT_WHOLE,     // comment parent_type: newline before and after
   COMMENT_CPP_ENDIF, // C++ comment, single line, after #endif or #else
   COMMENT_ENDIF,     // C-comment, single line, after #endif or #else

   IGNORED,           // a chunk of ignored text

   WORD,              // variable, type, function name, etc
   NUMBER,
   NUMBER_FP,
   STRING,            // quoted string "hi" or 'a' or <in> for include
   STRING_MULTI,      // quoted string with embedded newline
   IF,                // built-in keywords
   ELSE,
   ELSEIF,
   CONSTEXPR,         // only when preceded by 'if' (otherwise QUALIFIER)
   FOR,
   WHILE,
   WHILE_OF_DO,
   SWITCH,
   CASE,
   DO,
   SYNCHRONIZED,
   VOLATILE,
   TYPEDEF,
   STRUCT,
   ENUM,
   ENUM_CLASS,
   SIZEOF,
   DECLTYPE,
   RETURN,
   BREAK,
   UNION,
   GOTO,
   CONTINUE,
   C_CAST,              // C-style cast:   "(int)5.6"
   CPP_CAST,            // C++-style cast: "int(5.6)"
   D_CAST,              // D-style cast:   "cast(type)" and "const(type)"
   TYPE_CAST,           // static_cast<type>(exp)
   TYPENAME,            // typename type
   TEMPLATE,            // template<...>
   PARAMETER_PACK,      // template<typename ... ARGS>
   WHERE_SPEC,          // 'where' : used in C# generic constraint

   ASSIGN,              // =, +=, /=, etc
   ASSIGN_NL,           // Assign followed by a newline - fake item for indenting
   SASSIGN,             // 'and_eq'

   ASSIGN_DEFAULT_ARG,  // Default argument such as
                        // Foo( int Foo = 5 );
   ASSIGN_FUNC_PROTO,   // function prototype modifier such as
                        // void* operator new(std::size_t) = delete;
                        // Foo( const Foo & ) = default;
                        // Foo( const Foo & ) = 0;

   COMPARE,             // ==, !=, <=, >=
   SCOMPARE,            // compare op that is a string 'is', 'neq'
   BOOL,                // || or &&
   SBOOL,               // or, and
   ARITH,               // +, -, /, etc
   SARITH,              // 'not', 'xor'
   SHIFT,               // <<, >>
   CARET,               // ^
   DEREF,               // * dereference
   INCDEC_BEFORE,       // ++a or --a
   INCDEC_AFTER,        // a++ or a--
   MEMBER,              // . or ->
   DC_MEMBER,           // ::
   C99_MEMBER,          // . in structure stuff
   INV,                 // ~
   DESTRUCTOR,          // ~
   NOT,                 // !
   D_TEMPLATE,          // ! as in Foo!(A)
   ADDR,                // &
   NEG,                 // - as in -1
   POS,                 // + as in +1
   STAR,                // * : raw char to be changed
   PLUS,                // + : raw char to be changed
   MINUS,               // - : raw char to be changed
   AMP,                 // & : raw char to be changed
   BYREF,               // & in function def/proto params

   // BITWISE_AND,         // &   // is a ARITH
   // BITWISE_OR,          // |   // is a ARITH
   // BITWISE_EXCLUSIVE_OR,// ^   // is a ARITH
   // BITWISE_NOT,         // ~   // is a ARITH

   POUND,               // #
   PREPROC,             // # at the start of a line
   PREPROC_INDENT,      // # at the start of a line that gets indented: #region
   PREPROC_BODY,        // body of every preproc EXCEPT #define
   PP,                  // ##
   ELLIPSIS,            // ...
   RANGE,               // ..
   NULLCOND,            // ?.

   SEMICOLON,
   VSEMICOLON,          // virtual semicolon for PAWN
   COLON,
   ASM_COLON,
   CASE_COLON,
   CASE_ELLIPSIS,       // '...' in `case 1 ... 5`:
   CLASS_COLON,         // colon after a class def
   CONSTR_COLON,        // colon after a constructor
   D_ARRAY_COLON,       // D named array initializer colon
   COND_COLON,          // conditional colon in 'b ? t : f'
   WHERE_COLON,         // C# where-constraint colon (after the type)
   QUESTION,            // conditional question in 'b ? t : f'
   COMMA,

   ASM,
   ATTRIBUTE,
   AUTORELEASEPOOL,     // OC: Autorelease Pool Blocks, used by iOS
   OC_AVAILABLE,
   OC_AVAILABLE_VALUE,
   CATCH,
   WHEN,
   WHERE,            // C# where clause
   CLASS,
   DELETE,
   EXPORT,
   FRIEND,
   NAMESPACE,
   PACKAGE,
   NEW,              // may turn into PBRACED if followed by a '('
   OPERATOR,
   OPERATOR_VAL,
   ASSIGN_OPERATOR,  // the value after 'operator' such as:
                     // Foo &operator= ( const Foo & );
   ACCESS,
   ACCESS_COLON,
   THROW,
   NOEXCEPT,
   TRY,
   BRACED_INIT_LIST,
   USING,
   USING_STMT,       // using (xxx) ...
   USING_ALIAS,      // using identifier attr(optional) = type-id
   D_WITH,           // D: parenthetis+braced
   D_MODULE,
   SUPER,
   DELEGATE,
   BODY,
   DEBUG,
   DEBUGGER,
   INVARIANT,
   UNITTEST,
   UNSAFE,
   FINALLY,
   FIXED,            // C# fixed
   IMPORT,
   D_SCOPE,
   D_SCOPE_IF,
   LAZY,
   D_MACRO,
   D_VERSION,        // turns into D_VERSION_IF if not followed by '='
   D_VERSION_IF,     // version(x) { }

   // note for parenthetis/brace/square pairs: close MUST be open + 1
   PAREN_OPEN,
   PAREN_CLOSE,

   ANGLE_OPEN,       // template<T*>
   ANGLE_CLOSE,

   SPAREN_OPEN,      // 'special' parenthetis after if/for/switch/while/synchronized/catch
   SPAREN_CLOSE,

   PPAREN_OPEN,      // 'protect' parenthetis to protect a type such as (*int)
   PPAREN_CLOSE,     // used at align_func_param

   FPAREN_OPEN,      // 'function' parenthetis after fcn/macro fcn
   FPAREN_CLOSE,

   LPAREN_OPEN,      // lambda-declarator parenthetis
   LPAREN_CLOSE,

   TPAREN_OPEN,      // 'type' parenthetis used in function types
   TPAREN_CLOSE,

   RPAREN_OPEN,      // functor                                    Issue #3914
   RPAREN_CLOSE,

   BRACE_OPEN,       // {...}
   BRACE_CLOSE,

   VBRACE_OPEN,      // virtual brace, i.e. brace inserted by uncrustify
   VBRACE_CLOSE,

   SQUARE_OPEN,      // [...]
   SQUARE_CLOSE,

   TSQUARE,          // special case of []

   MACRO_OPEN,       // stuff specified via custom-pair
   MACRO_CLOSE,
   MACRO_ELSE,
   MACRO_NO_INDENT,  // macro that should not affect indentation

   // aggregate types
   LABEL,             // a non-case label
   LABEL_COLON,       // the colon for a label
   FUNCTION,          // function - unspecified, call mark_function()
   FUNC_CALL,         // function call
   FUNC_CALL_USER,    // function call (special user)
   FUNC_DEF,          // function definition/implementation
   FUNC_TYPE,         // function type - foo in "typedef void (*foo)(void)"
   FUNC_VAR,          // foo and parent type of first parens in "void (*foo)(void)"
   FUNC_PROTO,        // function prototype
   FUNC_START,        // global DC member for functions(void ::func())
   FUNC_CLASS_DEF,    // ctor or dtor for a class
   FUNC_CLASS_PROTO,  // ctor or dtor for a class
   FUNC_CTOR_VAR,     // variable or class initialization
   FUNC_WRAP,         // macro that wraps the function name
   PROTO_WRAP,        // macro: "RETVAL PROTO_WRAP( fcn_name, (PARAMS))". Parens for PARAMS are optional.
   MACRO_FUNC,        // function-like macro
   MACRO_FUNC_CALL,   // function-like macro call
   MACRO_NO_FMT_ARGS, // macros whose args should not be formatted (e.g. NS_SWIFT_NAME(getter:description()) will not format the `getter:description()` part)
   MACRO,             // a macro def
   QUALIFIER,         // static, const, etc
   EXTERN,            // extern
   DECLSPEC,          // __declspec
   ALIGN,             // paren'd qualifier: align(4) struct a { }
   TYPE,
   PTR_TYPE,          // a '*' as part of a type
   TYPE_WRAP,         // macro that wraps a type name
   CPP_LAMBDA,        // parent for '[=](...){...}'
   CPP_LAMBDA_RET,    // '->' in '[=](...) -> type {...}'
   EXECUTION_CONTEXT, // Keyword for use in lambda statement: [] EXECUTION_CONTEXT ()->{}
   TRAILING_RET,      // '->' in 'auto fname(...) -> type;'
                      // '->' in 'auto fname(...) const -> type;'
   BIT_COLON,         // a ':' in a variable declaration
   ENUM_COLON,        // a ':' in a enum definition

   OC_DYNAMIC,
   OC_END,           // ObjC: @end
   OC_IMPL,          // ObjC: @implementation
   OC_INTF,          // ObjC: @interface
   OC_PROTOCOL,      // ObjC: @protocol or @protocol()
   OC_PROTO_LIST,    // ObjC: protocol list < > (parent token only)
   OC_GENERIC_SPEC,  // ObjC: specification of generic  < >
   OC_PROPERTY,      // ObjC: @property
   OC_CLASS,         // ObjC: the name after @interface or @implementation
   OC_CLASS_EXT,     // ObjC: a pair of empty parens after the class name in a @interface or @implementation
   OC_CATEGORY,      // ObjC: the category name in parens after the class name in a @interface or @implementation
   OC_SCOPE,         // ObjC: the '-' or '+' in '-(void) func: (int) i;'
   OC_MSG,           // ObjC: parent type to '[', ']' and ';' in '[class func : param name: param];'
   OC_MSG_CLASS,     // ObjC: 'class' in  '[class func : param name: param];' (see also PCF_IN_OC_MSG)
   OC_MSG_FUNC,      // ObjC: 'func' in  '[class func : param name: param];' (see also PCF_IN_OC_MSG)
   OC_MSG_NAME,      // ObjC: 'name' in  '[class func : param name: param];' (see also PCF_IN_OC_MSG)
   OC_MSG_SPEC,      // ObjC: msg spec '-(void) func: (int) i;'
   OC_MSG_DECL,      // ObjC: msg decl '-(void) func: (int) i { }'
   OC_RTYPE,         // ObjC: marks parens of the return type after scope
   OC_ATYPE,         // ObjC: marks parens of the arg type after scope
   OC_COLON,         // ObjC: the colon in a msg spec
   OC_DICT_COLON,    // ObjC: colon in dictionary constant: "KEY: VALUE"
   OC_SEL,           // ObjC: @selector
   OC_SEL_NAME,      // ObjC: selector name
   OC_BLOCK,         // ObjC: block parent type.
   OC_BLOCK_ARG,     // ObjC: block arguments parent type.
   OC_BLOCK_TYPE,    // ObjC: block declaration parent type, e.g. mainly the '(^block_t)' in 'void (^block_t)(int arg);'
   OC_BLOCK_EXPR,    // ObjC: block expression with arg: '^(int arg) { arg++; };' and without (called a block literal): '^{ ... };'
   OC_BLOCK_CARET,   // ObjC: block pointer caret: '^'
   OC_AT,            // ObjC: boxed constants using '@'
   OC_PROPERTY_ATTR, // ObjC: property attribute (strong, weak, readonly, etc...)

   // start PP types
   PP_DEFINE,        // #define
   PP_DEFINED,       // #if defined
   PP_INCLUDE,       // #include
   PP_IF,            // #if, #ifdef, or #ifndef
   PP_ELSE,          // #else or #elif
   PP_ENDIF,         // #endif
   PP_ASSERT,
   PP_EMIT,
   PP_ENDASM,        // end of assembly code section
   PP_ENDINPUT,
   PP_ERROR,
   PP_FILE,
   PP_LINE,
   PP_SECTION,
   PP_ASM,           // start of assembly code section
   PP_UNDEF,
   PP_PROPERTY,

   PP_BODYCHUNK,     // everything after this gets put in PREPROC_BODY

   PP_PRAGMA,        // pragma's should not be altered
   PP_REGION,        // C# #region
   PP_ENDREGION,     // C# #endregion
   PP_REGION_INDENT, // Dummy token for indenting a C# #region
   PP_IF_INDENT,     // Dummy token for indenting a #if stuff
   PP_IGNORE,        // Dummy token for ignoring a certain preprocessor directive (do not do any processing)
   PP_OTHER,         // #line, #error, #pragma, etc
   // end PP types

   // PAWN stuff
   CHAR,
   DEFINED,
   FORWARD,
   NATIVE,
   STATE,
   STOCK,
   TAGOF,
   DOT,
   TAG,
   TAG_COLON,

   // C-sharp
   LOCK,             // lock/unlock
   AS,
   IN,               // "foreach (T c in x)" or "foo(in char c)" or "in { ..."
   BRACED,           // simple braced items: try {}
   THIS,             // may turn into PBRACED if followed by a '('
   BASE,             // C# thingy
   DEFAULT,          // may be changed into CASE
   GETSET,           // must be followed by BRACE_OPEN or reverts to WORD
   GETSET_EMPTY,     // get/set/add/remove followed by a semicolon
   CONCAT,           // The '~' between strings
   CS_SQ_STMT,       // '[assembly: xxx]' or '[Attribute()]' or '[Help()]', etc
   CS_SQ_COLON,      // the colon in one of those [] thingys
   CS_PROPERTY,      // word or ']' followed by '{'

   // Embedded SQL - always terminated with a semicolon
   SQL_EXEC,         // the 'EXEC' in 'EXEC SQL ...'
   SQL_BEGIN,        // the 'BEGINN' in 'EXEC SQL BEGIN ...'
   SQL_END,          // the 'END' in 'EXEC SQL END ...'
   SQL_WORD,         // WORDs in the 'EXEC SQL' statement
   SQL_ASSIGN,       // :=

   // Vala stuff
   CONSTRUCT,        // braced "construct { }" or qualifier "(construct int x)"
   LAMBDA,

   // Java
   ASSERT,       // assert EXP1 [ : EXP2 ] ;
   ANNOTATION,   // @interface or @something(...)
   FOR_COLON,    // colon in "for ( TYPE var: EXPR ) { ... }"
   DOUBLE_BRACE, // parent for double brace

   /* Clang */
   CNG_HASINC,   // Clang: __has_include()
   CNG_HASINCN,  // Clang: __has_include_next()

   // extensions for Qt macros
   Q_EMIT,       // guy 2015-10-16
   Q_FOREACH,    // guy 2015-09-23
   Q_FOREVER,    // guy 2015-10-18
   Q_GADGET,     // guy 2016-05-04
   Q_OBJECT,     // guy 2015-10-16

   // Machine Modes
   MODE,         // guy 2016-03-11
   DI,           // guy 2016-03-11
   HI,           // guy 2016-03-11
   QI,           // guy 2016-03-11
   SI,           // guy 2016-03-11
   NOTHROW,      // guy 2016-03-11
   WORD_,        // guy 2016-03-11

   TOKEN_COUNT_  // NOTE: Keep this the last entry because it's used as a counter.
};

#endif /* TOKEN_ENUM_H_INCLUDED */
