/**
 * @file symbols
 * Manages the table of punctuators.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

/**
 *
 *   Contet of the generated "punctuator_table.h" file is based off this.
 *
 *   NOTE: the tables below do not need to be sorted.
 */

// 6-char symbols
static const chunk_tag_t symbols6[] =
{
   { R"_(??(??))_", CT_TSQUARE, LANG_C | LANG_CPP | LANG_OC | FLAG_DIG }, // trigraph []
   { R"_(??!??!)_", CT_BOOL,    LANG_C | LANG_CPP | LANG_OC | FLAG_DIG }, // trigraph ||
   { R"_(??=??=)_", CT_PP,      LANG_C | LANG_CPP | LANG_OC | FLAG_DIG }, // trigraph ##
};

/* 5-char symbols */
static const chunk_tag_t symbols5[] =
{
   { R"_(??!=)_", CT_ASSIGN, LANG_C | LANG_CPP | LANG_OC | FLAG_DIG }, // trigraph |=
   { R"_(??'=)_", CT_ASSIGN, LANG_C | LANG_CPP | LANG_OC | FLAG_DIG }, // trigraph ^=
   { R"_(??=@)_", CT_POUND,  LANG_C | LANG_CPP | LANG_OC | FLAG_DIG }, // trigraph #@ MS extension
};

/* 4-char symbols */
static const chunk_tag_t symbols4[] =
{
   { "!<>=",      CT_COMPARE, LANG_D                                 },
   { ">>>=",      CT_ASSIGN,  LANG_D | LANG_JAVA | LANG_PAWN         },
   { R"_(<::>)_", CT_TSQUARE, LANG_C | LANG_CPP | LANG_OC | FLAG_DIG },   // digraph []
   { R"_(%:%:)_", CT_PP,      LANG_C | LANG_CPP | LANG_OC | FLAG_DIG },   // digraph ##
};

// 3-char symbols
static const chunk_tag_t symbols3[] =
{
   { "!<=",      CT_COMPARE,      LANG_D                                                       },
   { "!<>",      CT_COMPARE,      LANG_D                                                       },
   { "!==",      CT_COMPARE,      LANG_D | LANG_ECMA                                           },
   { "!>=",      CT_COMPARE,      LANG_D                                                       },
   { "<=>",      CT_COMPARE,      LANG_CPP                                                     },
   { "->*",      CT_MEMBER,       LANG_C | LANG_CPP | LANG_OC | LANG_D                         },
   { "...",      CT_ELLIPSIS,     LANG_C | LANG_CPP | LANG_OC | LANG_D | LANG_PAWN | LANG_JAVA },
   { "<<=",      CT_ASSIGN,       LANG_ALL                                                     },
   { "<>=",      CT_COMPARE,      LANG_D                                                       },
   { "===",      CT_COMPARE,      LANG_D | LANG_ECMA                                           },
   { ">>=",      CT_ASSIGN,       LANG_ALL                                                     },
   { ">>>",      CT_ARITH,        LANG_D | LANG_JAVA | LANG_PAWN | LANG_ECMA                   },
   { "%:@",      CT_POUND,        LANG_C | LANG_CPP | LANG_OC                                  },           // digraph  #@ MS extension
   { R"_(??=)_", CT_POUND,        LANG_C | LANG_CPP | LANG_OC | FLAG_DIG                       },           // trigraph #
   { R"_(??=)_", CT_COMPARE,      LANG_CS                                                      },           // cs: Null-Coalescing Assignment Operator
   { R"_(??()_", CT_SQUARE_OPEN,  LANG_C | LANG_CPP | LANG_OC | FLAG_DIG                       },           // trigraph [
   { R"_(??))_", CT_SQUARE_CLOSE, LANG_C | LANG_CPP | LANG_OC | FLAG_DIG                       },           // trigraph ]
   { R"_(??')_", CT_CARET,        LANG_C | LANG_CPP | LANG_OC | FLAG_DIG                       },           // trigraph ^
   { R"_(??<)_", CT_BRACE_OPEN,   LANG_C | LANG_CPP | LANG_OC | FLAG_DIG                       },           // trigraph {
   { R"_(??>)_", CT_BRACE_CLOSE,  LANG_C | LANG_CPP | LANG_OC | FLAG_DIG                       },           // trigraph }
   { R"_(??-)_", CT_INV,          LANG_C | LANG_CPP | LANG_OC | FLAG_DIG                       },           // trigraph ~
   { R"_(??!)_", CT_ARITH,        LANG_C | LANG_CPP | LANG_OC | FLAG_DIG                       },           // trigraph |
};
// { R"_(??/)_", CT_UNKNOWN,      LANG_C | LANG_CPP | LANG_OC | FLAG_DIG                       }, // trigraph '\'

// 2-char symbols
static const chunk_tag_t symbols2[] =
{
   { "!<",      CT_COMPARE,      LANG_D                                 },                     // 0
   { "!=",      CT_COMPARE,      LANG_ALL                               },                     // 1
   { "!>",      CT_COMPARE,      LANG_D                                 },                     // 2
   { "!~",      CT_COMPARE,      LANG_D                                 },                     // 3
   { "##",      CT_PP,           LANG_C | LANG_CPP | LANG_OC            },                     // 4
   { "#@",      CT_POUND,        LANG_C | LANG_CPP | LANG_OC            },                     // MS extension
   { "%=",      CT_ASSIGN,       LANG_ALL                               },                     // 6
   { "&&",      CT_BOOL,         LANG_ALL                               },                     // 7
   { "&=",      CT_ASSIGN,       LANG_ALL                               },                     // 8
   { "*=",      CT_ASSIGN,       LANG_ALL                               },                     // 9
   { "++",      CT_INCDEC_AFTER, LANG_ALL                               },                     // 10
   { "+=",      CT_ASSIGN,       LANG_ALL                               },                     // 11
   { "--",      CT_INCDEC_AFTER, LANG_ALL                               },                     // 12
   { "-=",      CT_ASSIGN,       LANG_ALL                               },                     // 13
   { "->",      CT_MEMBER,       LANG_ALLC                              },                     // 14
   { ".*",      CT_MEMBER,       LANG_C | LANG_CPP | LANG_OC | LANG_D   },                     // 15
   { "..",      CT_RANGE,        LANG_D                                 },                     // 16
   { "?.",      CT_NULLCOND,     LANG_CS                                },                     // null conditional operator
   { "/=",      CT_ASSIGN,       LANG_ALL                               },                     // 18
   { "::",      CT_DC_MEMBER,    LANG_ALLC                              },                     // 19
   { "<<",      CT_SHIFT,        LANG_ALL                               },                     // 20
   { "<=",      CT_COMPARE,      LANG_ALL                               },                     // 21
   { "<>",      CT_COMPARE,      LANG_D                                 },                     // 22
   { "==",      CT_COMPARE,      LANG_ALL                               },                     // 23
   { ">=",      CT_COMPARE,      LANG_ALL                               },                     // 24
   { ">>",      CT_SHIFT,        LANG_ALL                               },                     // 25
   { "[]",      CT_TSQUARE,      LANG_ALL                               },                     // 26
   { "^=",      CT_ASSIGN,       LANG_ALL                               },                     // 27
   { "|=",      CT_ASSIGN,       LANG_ALL                               },                     // 28
   { "||",      CT_BOOL,         LANG_ALL                               },                     // 29
   { "~=",      CT_COMPARE,      LANG_D                                 },                     // 30
   { "~~",      CT_COMPARE,      LANG_D                                 },                     // 31
   { "=>",      CT_LAMBDA,       LANG_VALA | LANG_CS | LANG_D           },                     // 32
   { "??",      CT_COMPARE,      LANG_CS | LANG_VALA                    },                     // 33
   { R"_(<%)_", CT_BRACE_OPEN,   LANG_C | LANG_CPP | LANG_OC | FLAG_DIG },                     // digraph {
   { R"_(%>)_", CT_BRACE_CLOSE,  LANG_C | LANG_CPP | LANG_OC | FLAG_DIG },                     // digraph }
   { R"_(<:)_", CT_SQUARE_OPEN,  LANG_C | LANG_CPP | LANG_OC | FLAG_DIG },                     // digraph [
   { R"_(:>)_", CT_SQUARE_CLOSE, LANG_C | LANG_CPP | LANG_OC | FLAG_DIG },                     // digraph ]
   { R"_(%:)_", CT_POUND,        LANG_C | LANG_CPP | LANG_OC | FLAG_DIG },                     // digraph #
};

// *INDENT-OFF*
// 1-char symbols
static const chunk_tag_t symbols1[] =
{
   { R"_()_", CT_FORM_FEED,    LANG_ALL                            },
   { "!",       CT_NOT,          LANG_ALL                            },
   { "#",       CT_POUND,        LANG_ALL & ~(LANG_JAVA | LANG_ECMA) },
   { "$",       CT_COMPARE,      LANG_D                              },
   { "%",       CT_ARITH,        LANG_ALL                            },
   { "&",       CT_AMP,          LANG_ALL                            },
   { "(",       CT_PAREN_OPEN,   LANG_ALL                            },
   { ")",       CT_PAREN_CLOSE,  LANG_ALL                            },
   { "*",       CT_STAR,         LANG_ALL                            },
   { "+",       CT_PLUS,         LANG_ALL                            },
   { ",",       CT_COMMA,        LANG_ALL                            },
   { "-",       CT_MINUS,        LANG_ALL                            },
   { ".",       CT_DOT,          LANG_ALL                            },
   { "/",       CT_ARITH,        LANG_ALL                            },
   { ":",       CT_COLON,        LANG_ALL                            },
   { ";",       CT_SEMICOLON,    LANG_ALL                            },
   { "<",       CT_ANGLE_OPEN,   LANG_ALL                            },
   { "=",       CT_ASSIGN,       LANG_ALL                            },
   { ">",       CT_ANGLE_CLOSE,  LANG_ALL                            },
   { "@",       CT_OC_AT,        LANG_OC                             },
   { "?",       CT_QUESTION,     LANG_ALL                            },
   { "[",       CT_SQUARE_OPEN,  LANG_ALL                            },
   { "]",       CT_SQUARE_CLOSE, LANG_ALL                            },
   { "^",       CT_CARET,        LANG_ALL                            },
   { "{",       CT_BRACE_OPEN,   LANG_ALL                            },
   { "|",       CT_ARITH,        LANG_ALL                            },
   { "}",       CT_BRACE_CLOSE,  LANG_ALL                            },
   { "~",       CT_INV,          LANG_ALL                            },
};
// *INDENT-ON*
