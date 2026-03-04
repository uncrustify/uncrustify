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
   { R"_(??(??))_", E_Token::TSQUARE, e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG }, // trigraph []
   { R"_(??!??!)_", E_Token::BOOL,    e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG }, // trigraph ||
   { R"_(??=??=)_", E_Token::PP,      e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG }, // trigraph ##
};

/* 5-char symbols */
static const chunk_tag_t symbols5[] =
{
   { R"_(??!=)_", E_Token::ASSIGN, e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG }, // trigraph |=
   { R"_(??'=)_", E_Token::ASSIGN, e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG }, // trigraph ^=
   { R"_(??=@)_", E_Token::POUND,  e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG }, // trigraph #@ MS extension
};

/* 4-char symbols */
static const chunk_tag_t symbols4[] =
{
   { "!<>=",      E_Token::COMPARE, e_LANG_D                                       },
   { ">>>=",      E_Token::ASSIGN,  e_LANG_D | e_LANG_JAVA | e_LANG_PAWN           },
   { R"_(<::>)_", E_Token::TSQUARE, e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG },   // digraph []
   { R"_(%:%:)_", E_Token::PP,      e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG },   // digraph ##
};

// 3-char symbols
static const chunk_tag_t symbols3[] =
{
   { "!<=",      E_Token::COMPARE,      e_LANG_D                                                                 },
   { "!<>",      E_Token::COMPARE,      e_LANG_D                                                                 },
   { "!==",      E_Token::COMPARE,      e_LANG_D | e_LANG_ECMA                                                   },
   { "!>=",      E_Token::COMPARE,      e_LANG_D                                                                 },
   { "<=>",      E_Token::COMPARE,      e_LANG_CPP                                                               },
   { "->*",      E_Token::MEMBER,       e_LANG_C | e_LANG_CPP | e_LANG_OC | e_LANG_D                             },
   { "...",      E_Token::ELLIPSIS,     e_LANG_C | e_LANG_CPP | e_LANG_OC | e_LANG_D | e_LANG_PAWN | e_LANG_JAVA },
   { "<<=",      E_Token::ASSIGN,       e_LANG_ALL                                                               },
   { "<>=",      E_Token::COMPARE,      e_LANG_D                                                                 },
   { "===",      E_Token::COMPARE,      e_LANG_D | e_LANG_ECMA                                                   },
   { ">>=",      E_Token::ASSIGN,       e_LANG_ALL                                                               },
   { ">>>",      E_Token::ARITH,        e_LANG_D | e_LANG_JAVA | e_LANG_PAWN | e_LANG_ECMA                       },
   { "%:@",      E_Token::POUND,        e_LANG_C | e_LANG_CPP | e_LANG_OC                                        }, // digraph  #@ MS extension
   { R"_(??=)_", E_Token::POUND,        e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph #
   { R"_(??=)_", E_Token::COMPARE,      e_LANG_CS                                                                }, // cs: Null-Coalescing Assignment Operator
   { R"_(??()_", E_Token::SQUARE_OPEN,  e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph [
   { R"_(??))_", E_Token::SQUARE_CLOSE, e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph ]
   { R"_(??')_", E_Token::CARET,        e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph ^
   { R"_(??<)_", E_Token::BRACE_OPEN,   e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph {
   { R"_(??>)_", E_Token::BRACE_CLOSE,  e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph }
   { R"_(??-)_", E_Token::INV,          e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph ~
   { R"_(??!)_", E_Token::ARITH,        e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                           }, // trigraph |
};
// { R"_(??/)_", E_Token::UNKNOWN,      e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG                       }, // trigraph '\'

// 2-char symbols
static const chunk_tag_t symbols2[] =
{
   { "!<",      E_Token::COMPARE,      e_LANG_D                                       },               // 0
   { "!=",      E_Token::COMPARE,      e_LANG_ALL                                     },               // 1
   { "!>",      E_Token::COMPARE,      e_LANG_D                                       },               // 2
   { "!~",      E_Token::COMPARE,      e_LANG_D                                       },               // 3
   { "##",      E_Token::PP,           e_LANG_C | e_LANG_CPP | e_LANG_OC              },               // 4
   { "#@",      E_Token::POUND,        e_LANG_C | e_LANG_CPP | e_LANG_OC              },               // MS extension
   { "%=",      E_Token::ASSIGN,       e_LANG_ALL                                     },               // 6
   { "&&",      E_Token::BOOL,         e_LANG_ALL                                     },               // 7
   { "&=",      E_Token::ASSIGN,       e_LANG_ALL                                     },               // 8
   { "*=",      E_Token::ASSIGN,       e_LANG_ALL                                     },               // 9
   { "++",      E_Token::INCDEC_AFTER, e_LANG_ALL                                     },               // 10
   { "+=",      E_Token::ASSIGN,       e_LANG_ALL                                     },               // 11
   { "--",      E_Token::INCDEC_AFTER, e_LANG_ALL                                     },               // 12
   { "-=",      E_Token::ASSIGN,       e_LANG_ALL                                     },               // 13
   { "->",      E_Token::MEMBER,       e_LANG_ALLC                                    },               // 14
   { ".*",      E_Token::MEMBER,       e_LANG_C | e_LANG_CPP | e_LANG_OC | e_LANG_D   },               // 15
   { "..",      E_Token::RANGE,        e_LANG_D                                       },               // 16
   { "?.",      E_Token::NULLCOND,     e_LANG_CS                                      },               // null conditional operator
   { "/=",      E_Token::ASSIGN,       e_LANG_ALL                                     },               // 18
   { "::",      E_Token::DC_MEMBER,    e_LANG_ALLC                                    },               // 19
   { "<<",      E_Token::SHIFT,        e_LANG_ALL                                     },               // 20
   { "<=",      E_Token::COMPARE,      e_LANG_ALL                                     },               // 21
   { "<>",      E_Token::COMPARE,      e_LANG_D                                       },               // 22
   { "==",      E_Token::COMPARE,      e_LANG_ALL                                     },               // 23
   { ">=",      E_Token::COMPARE,      e_LANG_ALL                                     },               // 24
   { ">>",      E_Token::SHIFT,        e_LANG_ALL                                     },               // 25
   { "[]",      E_Token::TSQUARE,      e_LANG_ALL                                     },               // 26
   { "^=",      E_Token::ASSIGN,       e_LANG_ALL                                     },               // 27
   { "|=",      E_Token::ASSIGN,       e_LANG_ALL                                     },               // 28
   { "||",      E_Token::BOOL,         e_LANG_ALL                                     },               // 29
   { "~=",      E_Token::COMPARE,      e_LANG_D                                       },               // 30
   { "~~",      E_Token::COMPARE,      e_LANG_D                                       },               // 31
   { "=>",      E_Token::LAMBDA,       e_LANG_VALA | e_LANG_CS | e_LANG_D             },               // 32
   { "??",      E_Token::COMPARE,      e_LANG_CS | e_LANG_VALA                        },               // 33
   { R"_(<%)_", E_Token::BRACE_OPEN,   e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG },               // digraph {
   { R"_(%>)_", E_Token::BRACE_CLOSE,  e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG },               // digraph }
   { R"_(<:)_", E_Token::SQUARE_OPEN,  e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG },               // digraph [
   { R"_(:>)_", E_Token::SQUARE_CLOSE, e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG },               // digraph ]
   { R"_(%:)_", E_Token::POUND,        e_LANG_C | e_LANG_CPP | e_LANG_OC | e_FLAG_DIG },               // digraph #
};

// *INDENT-OFF*
// 1-char symbols
static const chunk_tag_t symbols1[] =
{
   { R"_()_", E_Token::FORM_FEED,    e_LANG_ALL                            },
   { "!",       E_Token::NOT,          e_LANG_ALL                            },
   { "#",       E_Token::POUND,        e_LANG_ALL & ~(e_LANG_JAVA | e_LANG_ECMA) },
   { "$",       E_Token::COMPARE,      e_LANG_D                              },
   { "%",       E_Token::ARITH,        e_LANG_ALL                            },
   { "&",       E_Token::AMP,          e_LANG_ALL                            },
   { "(",       E_Token::PAREN_OPEN,   e_LANG_ALL                            },
   { ")",       E_Token::PAREN_CLOSE,  e_LANG_ALL                            },
   { "*",       E_Token::STAR,         e_LANG_ALL                            },
   { "+",       E_Token::PLUS,         e_LANG_ALL                            },
   { ",",       E_Token::COMMA,        e_LANG_ALL                            },
   { "-",       E_Token::MINUS,        e_LANG_ALL                            },
   { ".",       E_Token::DOT,          e_LANG_ALL                            },
   { "/",       E_Token::ARITH,        e_LANG_ALL                            },
   { ":",       E_Token::COLON,        e_LANG_ALL                            },
   { ";",       E_Token::SEMICOLON,    e_LANG_ALL                            },
   { "<",       E_Token::ANGLE_OPEN,   e_LANG_ALL                            },
   { "=",       E_Token::ASSIGN,       e_LANG_ALL                            },
   { ">",       E_Token::ANGLE_CLOSE,  e_LANG_ALL                            },
   { "@",       E_Token::OC_AT,        e_LANG_OC                             },
   { "?",       E_Token::QUESTION,     e_LANG_ALL                            },
   { "[",       E_Token::SQUARE_OPEN,  e_LANG_ALL                            },
   { "]",       E_Token::SQUARE_CLOSE, e_LANG_ALL                            },
   { "^",       E_Token::CARET,        e_LANG_ALL                            },
   { "{",       E_Token::BRACE_OPEN,   e_LANG_ALL                            },
   { "|",       E_Token::ARITH,        e_LANG_ALL                            },
   { "}",       E_Token::BRACE_CLOSE,  e_LANG_ALL                            },
   { "~",       E_Token::INV,          e_LANG_ALL                            },
};
// *INDENT-ON*
