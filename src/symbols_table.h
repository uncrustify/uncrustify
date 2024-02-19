/**
 * @file symbols
 * Manages the table of punctuators.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

/**
 *
 *   Content of the generated "punctuator_table.h" file is based off this.
 *
 *   NOTE: the tables below do not need to be sorted.
 */

#include "uncrustify_types.h"

// 6-char symbols
static const chunk_tag_t symbols6[] =
{
   { R"_(??(??))_", CT_TSQUARE, e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG }, // trigraph []
   { R"_(??!??!)_", CT_BOOL,    e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG }, // trigraph ||
   { R"_(??=??=)_", CT_PP,      e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG }, // trigraph ##
};

/* 5-char symbols */
static const chunk_tag_t symbols5[] =
{
   { R"_(??!=)_", CT_ASSIGN, e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG }, // trigraph |=
   { R"_(??'=)_", CT_ASSIGN, e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG }, // trigraph ^=
   { R"_(??=@)_", CT_POUND,  e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG }, // trigraph #@ MS extension
};

/* 4-char symbols */
static const chunk_tag_t symbols4[] =
{
   { "!<>=",      CT_COMPARE, e_LANG_D                                       },
   { ">>>=",      CT_ASSIGN,  e_LANG_D | e_LANG_JAVA | e_LANG_PAWN           },
   { R"_(<::>)_", CT_TSQUARE, e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG },   // digraph []
   { R"_(%:%:)_", CT_PP,      e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG },   // digraph ##
};

// 3-char symbols
static const chunk_tag_t symbols3[] =
{
   { "!<=",      CT_COMPARE,      e_LANG_D                                                                 },
   { "!<>",      CT_COMPARE,      e_LANG_D                                                                 },
   { "!==",      CT_COMPARE,      e_LANG_D | e_LANG_ECMA                                                   },
   { "!>=",      CT_COMPARE,      e_LANG_D                                                                 },
   { "<=>",      CT_COMPARE,      e_LANG_CPP                                                               },
   { "->*",      CT_MEMBER,       e_LANG_C | e_LANG_CPP | e_LANG_OC | e_LANG_D                             },
   { "...",      CT_ELLIPSIS,     e_LANG_C | e_LANG_CPP | e_LANG_OC | e_LANG_D | e_LANG_PAWN | e_LANG_JAVA },
   { "<<=",      CT_ASSIGN,       e_LANG_ALL                                                               },
   { "<>=",      CT_COMPARE,      e_LANG_D                                                                 },
   { "===",      CT_COMPARE,      e_LANG_D | e_LANG_ECMA                                                   },
   { ">>=",      CT_ASSIGN,       e_LANG_ALL                                                               },
   { ">>>",      CT_ARITH,        e_LANG_D | e_LANG_JAVA | e_LANG_PAWN | e_LANG_ECMA                       },
   { "%:@",      CT_POUND,        e_LANG_C | e_LANG_CPP | e_LANG_OC                                        }, // digraph  #@ MS extension
   { R"_(??=)_", CT_POUND,        e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph #
   { R"_(??=)_", CT_COMPARE,      e_LANG_CS                                                                }, // cs: Null-Coalescing Assignment Operator
   { R"_(??()_", CT_SQUARE_OPEN,  e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph [
   { R"_(??))_", CT_SQUARE_CLOSE, e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph ]
   { R"_(??')_", CT_CARET,        e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph ^
   { R"_(??<)_", CT_BRACE_OPEN,   e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph {
   { R"_(??>)_", CT_BRACE_CLOSE,  e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph }
   { R"_(??-)_", CT_INV,          e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph ~
   { R"_(??!)_", CT_ARITH,        e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph |
};
// { R"_(??/)_", CT_UNKNOWN,      e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                       }, // trigraph '\'

// 2-char symbols
static const chunk_tag_t symbols2[] =
{
   { "!<",      CT_COMPARE,      e_LANG_D                                       },               // 0
   { "!=",      CT_COMPARE,      e_LANG_ALL                                     },               // 1
   { "!>",      CT_COMPARE,      e_LANG_D                                       },               // 2
   { "!~",      CT_COMPARE,      e_LANG_D                                       },               // 3
   { "##",      CT_PP,           e_LANG_C | e_LANG_CPP | e_LANG_OC              },               // 4
   { "#@",      CT_POUND,        e_LANG_C | e_LANG_CPP | e_LANG_OC              },               // MS extension
   { "%=",      CT_ASSIGN,       e_LANG_ALL                                     },               // 6
   { "&&",      CT_BOOL,         e_LANG_ALL                                     },               // 7
   { "&=",      CT_ASSIGN,       e_LANG_ALL                                     },               // 8
   { "*=",      CT_ASSIGN,       e_LANG_ALL                                     },               // 9
   { "++",      CT_INCDEC_AFTER, e_LANG_ALL                                     },               // 10
   { "+=",      CT_ASSIGN,       e_LANG_ALL                                     },               // 11
   { "--",      CT_INCDEC_AFTER, e_LANG_ALL                                     },               // 12
   { "-=",      CT_ASSIGN,       e_LANG_ALL                                     },               // 13
   { "->",      CT_MEMBER,       e_LANG_ALLC                                    },               // 14
   { ".*",      CT_MEMBER,       e_LANG_C | e_LANG_CPP | e_LANG_OC | e_LANG_D   },               // 15
   { "..",      CT_RANGE,        e_LANG_D                                       },               // 16
   { "?.",      CT_NULLCOND,     e_LANG_CS                                      },               // null conditional operator
   { "/=",      CT_ASSIGN,       e_LANG_ALL                                     },               // 18
   { "::",      CT_DC_MEMBER,    e_LANG_ALLC                                    },               // 19
   { "<<",      CT_SHIFT,        e_LANG_ALL                                     },               // 20
   { "<=",      CT_COMPARE,      e_LANG_ALL                                     },               // 21
   { "<>",      CT_COMPARE,      e_LANG_D                                       },               // 22
   { "==",      CT_COMPARE,      e_LANG_ALL                                     },               // 23
   { ">=",      CT_COMPARE,      e_LANG_ALL                                     },               // 24
   { ">>",      CT_SHIFT,        e_LANG_ALL                                     },               // 25
   { "[]",      CT_TSQUARE,      e_LANG_ALL                                     },               // 26
   { "^=",      CT_ASSIGN,       e_LANG_ALL                                     },               // 27
   { "|=",      CT_ASSIGN,       e_LANG_ALL                                     },               // 28
   { "||",      CT_BOOL,         e_LANG_ALL                                     },               // 29
   { "~=",      CT_COMPARE,      e_LANG_D                                       },               // 30
   { "~~",      CT_COMPARE,      e_LANG_D                                       },               // 31
   { "=>",      CT_LAMBDA,       e_LANG_VALA | e_LANG_CS | e_LANG_D             },               // 32
   { "??",      CT_COMPARE,      e_LANG_CS | e_LANG_VALA                        },               // 33
   { R"_(<%)_", CT_BRACE_OPEN,   e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG },               // digraph {
   { R"_(%>)_", CT_BRACE_CLOSE,  e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG },               // digraph }
   { R"_(<:)_", CT_SQUARE_OPEN,  e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG },               // digraph [
   { R"_(:>)_", CT_SQUARE_CLOSE, e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG },               // digraph ]
   { R"_(%:)_", CT_POUND,        e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG },               // digraph #
};

// *INDENT-OFF*
// 1-char symbols
static const chunk_tag_t symbols1[] =
{
   { R"_()_", CT_FORM_FEED,    e_LANG_ALL                            },
   { "!",       CT_NOT,          e_LANG_ALL                            },
   { "#",       CT_POUND,        e_LANG_ALL & ~(e_LANG_JAVA | e_LANG_ECMA) },
   { "$",       CT_COMPARE,      e_LANG_D                              },
   { "%",       CT_ARITH,        e_LANG_ALL                            },
   { "&",       CT_AMP,          e_LANG_ALL                            },
   { "(",       CT_PAREN_OPEN,   e_LANG_ALL                            },
   { ")",       CT_PAREN_CLOSE,  e_LANG_ALL                            },
   { "*",       CT_STAR,         e_LANG_ALL                            },
   { "+",       CT_PLUS,         e_LANG_ALL                            },
   { ",",       CT_COMMA,        e_LANG_ALL                            },
   { "-",       CT_MINUS,        e_LANG_ALL                            },
   { ".",       CT_DOT,          e_LANG_ALL                            },
   { "/",       CT_ARITH,        e_LANG_ALL                            },
   { ":",       CT_COLON,        e_LANG_ALL                            },
   { ";",       CT_SEMICOLON,    e_LANG_ALL                            },
   { "<",       CT_ANGLE_OPEN,   e_LANG_ALL                            },
   { "=",       CT_ASSIGN,       e_LANG_ALL                            },
   { ">",       CT_ANGLE_CLOSE,  e_LANG_ALL                            },
   { "@",       CT_OC_AT,        e_LANG_OC                             },
   { "?",       CT_QUESTION,     e_LANG_ALL                            },
   { "[",       CT_SQUARE_OPEN,  e_LANG_ALL                            },
   { "]",       CT_SQUARE_CLOSE, e_LANG_ALL                            },
   { "^",       CT_CARET,        e_LANG_ALL                            },
   { "{",       CT_BRACE_OPEN,   e_LANG_ALL                            },
   { "|",       CT_ARITH,        e_LANG_ALL                            },
   { "}",       CT_BRACE_CLOSE,  e_LANG_ALL                            },
   { "~",       CT_INV,          e_LANG_ALL                            },
};
// *INDENT-ON*
