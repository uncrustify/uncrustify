/**
 * @file keywords.cpp
 * Manages the table of keywords.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */
#include "keywords.h"
#include "uncrustify_types.h"
#include "prototypes.h"
#include "char_table.h"
#include "args.h"
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <map>
#include "unc_ctype.h"
#include "uncrustify.h"

using namespace std;

// Dynamic keyword map
typedef map<string, c_token_t> dkwmap;
static dkwmap dkwm;


/**
 * Compares two chunk_tag_t entries using strcmp on the strings
 *
 * @param the 'left' entry
 * @param the 'right' entry
 *
 * @return == 0  if both keywords are equal
 * @return  < 0  p1 is smaller than p2
 * @return  > 0  p2 is smaller than p1
 */
static int kw_compare(const void *p1, const void *p2);


/**
 * search in static keywords for first occurrence of a given tag
 *
 * @param tag/keyword to search for
 */
static const chunk_tag_t *kw_static_first(const chunk_tag_t *tag);


static const chunk_tag_t *kw_static_match(const chunk_tag_t *tag);

/**
 * interesting static keywords - keep sorted.
 * Table includes the Name, Type, and Language flags.
 */
static const chunk_tag_t keywords[] =
{
   // TODO: it might be useful if users could add there custom keywords to this list
   { "@catch",             CT_CATCH,            LANG_OC                                                                     },
   { "@dynamic",           CT_OC_DYNAMIC,       LANG_OC                                                                     },
   { "@end",               CT_OC_END,           LANG_OC                                                                     },
   { "@finally",           CT_FINALLY,          LANG_OC                                                                     },
   { "@implementation",    CT_OC_IMPL,          LANG_OC                                                                     },
   { "@interface",         CT_OC_INTF,          LANG_OC                                                                     },
   { "@interface",         CT_CLASS,            LANG_JAVA                                                                   },
   { "@private",           CT_PRIVATE,          LANG_OC                                                                     },
   { "@property",          CT_OC_PROPERTY,      LANG_OC                                                                     },
   { "@protocol",          CT_OC_PROTOCOL,      LANG_OC                                                                     },
   { "@selector",          CT_OC_SEL,           LANG_OC                                                                     },
   { "@synthesize",        CT_OC_DYNAMIC,       LANG_OC                                                                     },
   { "@throw",             CT_THROW,            LANG_OC                                                                     },
   { "@try",               CT_TRY,              LANG_OC                                                                     },
   { "BOOL",               CT_TYPE,             LANG_OC                                                                     },
   { "NS_ENUM",            CT_ENUM,             LANG_OC                                                                     },
   { "NS_OPTIONS",         CT_ENUM,             LANG_OC                                                                     },
   { "Q_EMIT",             CT_Q_EMIT,           LANG_CPP                                                                    }, // guy 2015-10-16
   { "Q_FOREACH",          CT_FOR,              LANG_CPP                                                                    }, // guy 2015-09-23
   { "Q_FOREVER",          CT_Q_FOREVER,        LANG_CPP                                                                    }, // guy 2015-10-18
   { "Q_GADGET",           CT_Q_GADGET,         LANG_CPP                                                                    }, // guy 2016-05-04
   { "Q_OBJECT",           CT_COMMENT_EMBED,    LANG_CPP                                                                    },
   { "_Bool",              CT_TYPE,             LANG_C | LANG_OC                                                            },
   { "_Complex",           CT_TYPE,             LANG_C | LANG_CPP | LANG_OC                                                 },
   { "_Imaginary",         CT_TYPE,             LANG_C | LANG_CPP | LANG_OC                                                 },
   { "_Nonnull",           CT_QUALIFIER,        LANG_OC                                                                     },
   { "_Null_unspecified",  CT_QUALIFIER,        LANG_OC                                                                     },
   { "_Nullable",          CT_QUALIFIER,        LANG_OC                                                                     },
   { "__DI__",             CT_DI,               LANG_C | LANG_CPP | LANG_OC                                                 },
   { "__HI__",             CT_HI,               LANG_C | LANG_CPP | LANG_OC                                                 },
   { "__QI__",             CT_QI,               LANG_C | LANG_CPP | LANG_OC                                                 },
   { "__SI__",             CT_SI,               LANG_C | LANG_CPP | LANG_OC                                                 },
   { "__asm__",            CT_ASM,              LANG_C | LANG_CPP | LANG_OC                                                 },
   { "__attribute__",      CT_ATTRIBUTE,        LANG_C | LANG_CPP | LANG_OC                                                 },
   { "__block",            CT_QUALIFIER,        LANG_OC                                                                     },
   { "__const__",          CT_QUALIFIER,        LANG_C | LANG_CPP | LANG_OC                                                 },
   { "__except",           CT_CATCH,            LANG_C | LANG_CPP                                                           },
   { "__finally",          CT_FINALLY,          LANG_C | LANG_CPP                                                           },
   { "__inline__",         CT_QUALIFIER,        LANG_C | LANG_CPP | LANG_OC                                                 },
   { "__nonnull",          CT_QUALIFIER,        LANG_OC                                                                     },
   { "__nothrow__",        CT_NOTHROW,          LANG_C | LANG_CPP | LANG_OC                                                 },
   { "__null_unspecified", CT_QUALIFIER,        LANG_OC                                                                     },
   { "__nullable",         CT_QUALIFIER,        LANG_OC                                                                     },
   { "__restrict",         CT_QUALIFIER,        LANG_C | LANG_CPP | LANG_OC                                                 },
   { "__signed__",         CT_TYPE,             LANG_C | LANG_CPP | LANG_OC                                                 },
   { "__thread",           CT_QUALIFIER,        LANG_C | LANG_CPP | LANG_OC                                                 },
   { "__traits",           CT_QUALIFIER,        LANG_D                                                                      },
   { "__try",              CT_TRY,              LANG_C | LANG_CPP                                                           },
   { "__typeof__",         CT_SIZEOF,           LANG_C | LANG_CPP | LANG_OC                                                 },
   { "__volatile__",       CT_QUALIFIER,        LANG_C | LANG_CPP | LANG_OC                                                 },
   { "__word__",           CT_WORD_,            LANG_C | LANG_CPP | LANG_OC                                                 },
   { "abstract",           CT_QUALIFIER,        LANG_CS | LANG_D | LANG_JAVA | LANG_VALA | LANG_ECMA                        },
   { "add",                CT_GETSET,           LANG_CS                                                                     },
   { "alias",              CT_QUALIFIER,        LANG_D                                                                      },
   { "align",              CT_ALIGN,            LANG_D                                                                      },
   { "alignof",            CT_SIZEOF,           LANG_CPP                                                                    },
   { "and",                CT_SBOOL,            LANG_CPP                                                                    },
   { "and_eq",             CT_SASSIGN,          LANG_CPP                                                                    },
   { "as",                 CT_AS,               LANG_CS | LANG_VALA                                                         },
   { "asm",                CT_ASM,              LANG_C | LANG_CPP | LANG_OC | LANG_D                                        },
   { "asm",                CT_PP_ASM,           LANG_ALL | FLAG_PP                                                          },
   { "assert",             CT_ASSERT,           LANG_JAVA                                                                   },
   { "assert",             CT_FUNCTION,         LANG_D | LANG_PAWN                                                          }, // PAWN
   { "assert",             CT_PP_ASSERT,        LANG_PAWN | FLAG_PP                                                         }, // PAWN
   { "auto",               CT_TYPE,             LANG_C | LANG_CPP | LANG_OC | LANG_D                                        },
   { "base",               CT_BASE,             LANG_CS | LANG_VALA                                                         },
   { "bit",                CT_TYPE,             LANG_D                                                                      },
   { "bitand",             CT_ARITH,            LANG_C | LANG_CPP | LANG_OC                                                 },
   { "bitor",              CT_ARITH,            LANG_C | LANG_CPP | LANG_OC                                                 },
   { "body",               CT_BODY,             LANG_D                                                                      },
   { "bool",               CT_TYPE,             LANG_C | LANG_CPP | LANG_OC | LANG_CS | LANG_VALA                           },
   { "boolean",            CT_TYPE,             LANG_JAVA | LANG_ECMA                                                       },
   { "break",              CT_BREAK,            LANG_ALL                                                                    }, // PAWN
   { "byte",               CT_TYPE,             LANG_CS | LANG_D | LANG_JAVA | LANG_ECMA                                    },
   { "callback",           CT_QUALIFIER,        LANG_VALA                                                                   },
   { "case",               CT_CASE,             LANG_ALL                                                                    }, // PAWN
   { "cast",               CT_D_CAST,           LANG_D                                                                      },
   { "catch",              CT_CATCH,            LANG_CPP | LANG_CS | LANG_VALA | LANG_D | LANG_JAVA | LANG_ECMA             },
   { "cdouble",            CT_TYPE,             LANG_D                                                                      },
   { "cent",               CT_TYPE,             LANG_D                                                                      },
   { "cfloat",             CT_TYPE,             LANG_D                                                                      },
   { "char",               CT_CHAR,             LANG_PAWN                                                                   }, // PAWN
   { "char",               CT_TYPE,             LANG_ALLC                                                                   },
   { "checked",            CT_QUALIFIER,        LANG_CS                                                                     },
   { "class",              CT_CLASS,            LANG_CPP | LANG_CS | LANG_D | LANG_JAVA | LANG_VALA | LANG_ECMA             },
   { "compl",              CT_ARITH,            LANG_CPP                                                                    },
   { "const",              CT_QUALIFIER,        LANG_ALL                                                                    }, // PAWN
   { "const_cast",         CT_TYPE_CAST,        LANG_CPP                                                                    },
   { "constexpr",          CT_QUALIFIER,        LANG_CPP                                                                    },
   { "construct",          CT_CONSTRUCT,        LANG_VALA                                                                   },
   { "continue",           CT_CONTINUE,         LANG_ALL                                                                    }, // PAWN
   { "creal",              CT_TYPE,             LANG_D                                                                      },
   { "dchar",              CT_TYPE,             LANG_D                                                                      },
   { "debug",              CT_DEBUG,            LANG_D                                                                      },
   { "debugger",           CT_DEBUGGER,         LANG_ECMA                                                                   },
   { "decltype",           CT_SIZEOF,           LANG_CPP                                                                    },
   { "default",            CT_DEFAULT,          LANG_ALL                                                                    }, // PAWN
   { "define",             CT_PP_DEFINE,        LANG_ALL | FLAG_PP                                                          }, // PAWN
   { "defined",            CT_DEFINED,          LANG_PAWN                                                                   }, // PAWN
   { "defined",            CT_PP_DEFINED,       LANG_ALLC | FLAG_PP                                                         },
   { "delegate",           CT_DELEGATE,         LANG_CS | LANG_VALA | LANG_D                                                },
   { "delete",             CT_DELETE,           LANG_CPP | LANG_D | LANG_ECMA | LANG_VALA                                   },
   { "deprecated",         CT_QUALIFIER,        LANG_D                                                                      },
   { "do",                 CT_DO,               LANG_ALL                                                                    }, // PAWN
   { "double",             CT_TYPE,             LANG_ALLC                                                                   },
   { "dynamic_cast",       CT_TYPE_CAST,        LANG_CPP                                                                    },
   { "elif",               CT_PP_ELSE,          LANG_ALLC | FLAG_PP                                                         },
   { "else",               CT_ELSE,             LANG_ALL                                                                    }, // PAWN
   { "else",               CT_PP_ELSE,          LANG_ALL | FLAG_PP                                                          }, // PAWN
   { "elseif",             CT_PP_ELSE,          LANG_PAWN | FLAG_PP                                                         }, // PAWN
   { "emit",               CT_PP_EMIT,          LANG_PAWN | FLAG_PP                                                         }, // PAWN
   { "endif",              CT_PP_ENDIF,         LANG_ALL | FLAG_PP                                                          }, // PAWN
   { "endinput",           CT_PP_ENDINPUT,      LANG_PAWN | FLAG_PP                                                         }, // PAWN
   { "endregion",          CT_PP_ENDREGION,     LANG_ALL | FLAG_PP                                                          },
   { "endscript",          CT_PP_ENDINPUT,      LANG_PAWN | FLAG_PP                                                         }, // PAWN
   { "enum",               CT_ENUM,             LANG_ALL                                                                    }, // PAWN
   { "error",              CT_PP_ERROR,         LANG_PAWN | FLAG_PP                                                         }, // PAWN
   { "event",              CT_TYPE,             LANG_CS                                                                     },
   { "exit",               CT_FUNCTION,         LANG_PAWN                                                                   }, // PAWN
   { "explicit",           CT_TYPE,             LANG_CPP | LANG_CS                                                          },
   { "export",             CT_EXPORT,           LANG_CPP | LANG_D | LANG_ECMA                                               },
   { "extends",            CT_QUALIFIER,        LANG_JAVA | LANG_ECMA                                                       },
   { "extern",             CT_EXTERN,           LANG_C | LANG_CPP | LANG_OC | LANG_CS | LANG_D | LANG_VALA                  },
   { "false",              CT_WORD,             LANG_ALL                                                                    },
   { "file",               CT_PP_FILE,          LANG_PAWN | FLAG_PP                                                         }, // PAWN
   { "final",              CT_QUALIFIER,        LANG_CPP | LANG_D | LANG_ECMA                                               },
   { "finally",            CT_FINALLY,          LANG_D | LANG_CS | LANG_VALA | LANG_ECMA | LANG_JAVA                        },
   { "flags",              CT_TYPE,             LANG_VALA                                                                   },
   { "float",              CT_TYPE,             LANG_ALLC                                                                   },
   { "for",                CT_FOR,              LANG_ALL                                                                    }, // PAWN
   { "foreach",            CT_FOR,              LANG_CS | LANG_D | LANG_VALA                                                },
   { "foreach_reverse",    CT_FOR,              LANG_D                                                                      },
   { "forward",            CT_FORWARD,          LANG_PAWN                                                                   }, // PAWN
   { "friend",             CT_FRIEND,           LANG_CPP                                                                    },
   { "function",           CT_FUNCTION,         LANG_D | LANG_ECMA                                                          },
   { "get",                CT_GETSET,           LANG_CS | LANG_VALA                                                         },
   { "goto",               CT_GOTO,             LANG_ALL                                                                    }, // PAWN
   { "idouble",            CT_TYPE,             LANG_D                                                                      },
   { "if",                 CT_IF,               LANG_ALL                                                                    }, // PAWN
   { "if",                 CT_PP_IF,            LANG_ALL | FLAG_PP                                                          }, // PAWN
   { "ifdef",              CT_PP_IF,            LANG_ALLC | FLAG_PP                                                         },
   { "ifloat",             CT_TYPE,             LANG_D                                                                      },
   { "ifndef",             CT_PP_IF,            LANG_ALLC | FLAG_PP                                                         },
   { "implements",         CT_QUALIFIER,        LANG_JAVA | LANG_ECMA                                                       },
   { "implicit",           CT_QUALIFIER,        LANG_CS                                                                     },
   { "import",             CT_IMPORT,           LANG_D | LANG_JAVA | LANG_ECMA                                              }, // fudged to get indenting
   { "import",             CT_PP_INCLUDE,       LANG_OC | FLAG_PP                                                           }, // ObjectiveC version of include
   { "in",                 CT_IN,               LANG_D | LANG_CS | LANG_VALA | LANG_ECMA | LANG_OC                          },
   { "include",            CT_PP_INCLUDE,       LANG_C | LANG_CPP | LANG_OC | LANG_PAWN | FLAG_PP                           }, // PAWN
   { "inline",             CT_QUALIFIER,        LANG_C | LANG_CPP | LANG_OC                                                 },
   { "inout",              CT_QUALIFIER,        LANG_D                                                                      },
   { "instanceof",         CT_SIZEOF,           LANG_JAVA | LANG_ECMA                                                       },
   { "int",                CT_TYPE,             LANG_ALLC                                                                   },
   { "interface",          CT_CLASS,            LANG_CPP | LANG_CS | LANG_D | LANG_JAVA | LANG_VALA | LANG_ECMA             },
   { "internal",           CT_QUALIFIER,        LANG_CS                                                                     },
   { "invariant",          CT_INVARIANT,        LANG_D                                                                      },
   { "ireal",              CT_TYPE,             LANG_D                                                                      },
   { "is",                 CT_SCOMPARE,         LANG_D | LANG_CS | LANG_VALA                                                },
   { "lazy",               CT_LAZY,             LANG_D                                                                      },
   { "line",               CT_PP_LINE,          LANG_PAWN | FLAG_PP                                                         }, // PAWN
   { "lock",               CT_LOCK,             LANG_CS | LANG_VALA                                                         },
   { "long",               CT_TYPE,             LANG_ALLC                                                                   },
   { "macro",              CT_D_MACRO,          LANG_D                                                                      },
   { "mixin",              CT_CLASS,            LANG_D                                                                      }, // may need special handling
   { "module",             CT_D_MODULE,         LANG_D                                                                      },
   { "mutable",            CT_QUALIFIER,        LANG_CPP                                                                    },
   { "namespace",          CT_NAMESPACE,        LANG_CPP | LANG_CS | LANG_VALA                                              },
   { "native",             CT_NATIVE,           LANG_PAWN                                                                   }, // PAWN
   { "native",             CT_QUALIFIER,        LANG_JAVA | LANG_ECMA                                                       },
   { "new",                CT_NEW,              LANG_CPP | LANG_CS | LANG_D | LANG_JAVA | LANG_PAWN | LANG_VALA | LANG_ECMA }, // PAWN
   { "noexcept",           CT_NOEXCEPT,         LANG_CPP                                                                    },
   { "nonnull",            CT_TYPE,             LANG_OC                                                                     },
   { "not",                CT_SARITH,           LANG_CPP                                                                    },
   { "not_eq",             CT_SCOMPARE,         LANG_CPP                                                                    },
   { "null",               CT_TYPE,             LANG_CS | LANG_D | LANG_JAVA | LANG_VALA                                    },
   { "null_resettable",    CT_OC_PROPERTY_ATTR, LANG_OC                                                                     },
   { "null_unspecified",   CT_TYPE,             LANG_OC                                                                     },
   { "nullable",           CT_TYPE,             LANG_OC                                                                     },
   { "object",             CT_TYPE,             LANG_CS                                                                     },
   { "operator",           CT_OPERATOR,         LANG_CPP | LANG_CS | LANG_PAWN                                              }, // PAWN
   { "or",                 CT_SBOOL,            LANG_CPP                                                                    },
   { "or_eq",              CT_SASSIGN,          LANG_CPP                                                                    },
   { "out",                CT_QUALIFIER,        LANG_CS | LANG_D | LANG_VALA                                                },
   { "override",           CT_QUALIFIER,        LANG_CPP | LANG_CS | LANG_D | LANG_VALA                                     },
   { "package",            CT_PRIVATE,          LANG_D                                                                      },
   { "package",            CT_PACKAGE,          LANG_ECMA | LANG_JAVA                                                       },
   { "params",             CT_TYPE,             LANG_CS | LANG_VALA                                                         },
   { "pragma",             CT_PP_PRAGMA,        LANG_ALL | FLAG_PP                                                          }, // PAWN
   { "private",            CT_PRIVATE,          LANG_ALLC                                                                   }, // not C
   { "property",           CT_PP_PROPERTY,      LANG_CS | FLAG_PP                                                           },
   { "protected",          CT_PRIVATE,          LANG_ALLC                                                                   }, // not C
   { "public",             CT_PRIVATE,          LANG_ALL                                                                    }, // PAWN // not C
   { "readonly",           CT_QUALIFIER,        LANG_CS                                                                     },
   { "real",               CT_TYPE,             LANG_D                                                                      },
   { "ref",                CT_QUALIFIER,        LANG_CS | LANG_VALA                                                         },
   { "region",             CT_PP_REGION,        LANG_ALL | FLAG_PP                                                          },
   { "register",           CT_QUALIFIER,        LANG_C | LANG_CPP | LANG_OC                                                 },
   { "reinterpret_cast",   CT_TYPE_CAST,        LANG_CPP                                                                    },
   { "remove",             CT_GETSET,           LANG_CS                                                                     },
   { "restrict",           CT_QUALIFIER,        LANG_C | LANG_OC                                                            },
   { "return",             CT_RETURN,           LANG_ALL                                                                    }, // PAWN
   { "sbyte",              CT_TYPE,             LANG_CS                                                                     },
   { "scope",              CT_D_SCOPE,          LANG_D                                                                      },
   { "sealed",             CT_QUALIFIER,        LANG_CS                                                                     },
   { "section",            CT_PP_SECTION,       LANG_PAWN | FLAG_PP                                                         }, // PAWN
   { "set",                CT_GETSET,           LANG_CS | LANG_VALA                                                         },
   { "short",              CT_TYPE,             LANG_ALLC                                                                   },
   { "signal",             CT_PRIVATE,          LANG_VALA                                                                   },
   { "signals",            CT_PRIVATE,          LANG_CPP                                                                    },
   { "signed",             CT_TYPE,             LANG_C | LANG_CPP | LANG_OC                                                 },
   { "sizeof",             CT_SIZEOF,           LANG_C | LANG_CPP | LANG_OC | LANG_CS | LANG_VALA | LANG_PAWN               }, // PAWN
   { "sleep",              CT_SIZEOF,           LANG_PAWN                                                                   }, // PAWN
   { "stackalloc",         CT_NEW,              LANG_CS                                                                     },
   { "state",              CT_STATE,            LANG_PAWN                                                                   }, // PAWN
   { "static",             CT_QUALIFIER,        LANG_ALL                                                                    }, // PAWN
   { "static_cast",        CT_TYPE_CAST,        LANG_CPP                                                                    },
   { "stock",              CT_STOCK,            LANG_PAWN                                                                   }, // PAWN
   { "strictfp",           CT_QUALIFIER,        LANG_JAVA                                                                   },
   { "string",             CT_TYPE,             LANG_CS | LANG_VALA                                                         },
   { "struct",             CT_STRUCT,           LANG_C | LANG_CPP | LANG_OC | LANG_CS | LANG_D | LANG_VALA                  },
   { "super",              CT_SUPER,            LANG_D | LANG_JAVA | LANG_ECMA                                              },
   { "switch",             CT_SWITCH,           LANG_ALL                                                                    }, // PAWN
   { "synchronized",       CT_QUALIFIER,        LANG_D | LANG_ECMA                                                          },
   { "synchronized",       CT_SYNCHRONIZED,     LANG_JAVA                                                                   },
   { "tagof",              CT_TAGOF,            LANG_PAWN                                                                   }, // PAWN
   { "template",           CT_TEMPLATE,         LANG_CPP | LANG_D                                                           },
   { "this",               CT_THIS,             LANG_CPP | LANG_CS | LANG_D | LANG_JAVA | LANG_VALA | LANG_ECMA             },
   { "throw",              CT_THROW,            LANG_CPP | LANG_CS | LANG_VALA | LANG_D | LANG_JAVA | LANG_ECMA             },
   { "throws",             CT_QUALIFIER,        LANG_JAVA | LANG_ECMA | LANG_VALA                                           },
   { "transient",          CT_QUALIFIER,        LANG_JAVA | LANG_ECMA                                                       },
   { "true",               CT_WORD,             LANG_ALL                                                                    },
   { "try",                CT_TRY,              LANG_CPP | LANG_CS | LANG_D | LANG_JAVA | LANG_ECMA | LANG_VALA             },
   { "tryinclude",         CT_PP_INCLUDE,       LANG_PAWN | FLAG_PP                                                         }, // PAWN
   { "typedef",            CT_TYPEDEF,          LANG_C | LANG_CPP | LANG_D | LANG_OC                                        },
   { "typeid",             CT_SIZEOF,           LANG_CPP | LANG_D                                                           },
   { "typename",           CT_TYPENAME,         LANG_CPP                                                                    },
   { "typeof",             CT_SIZEOF,           LANG_C | LANG_CPP | LANG_OC | LANG_CS | LANG_D | LANG_VALA | LANG_ECMA      },
   { "ubyte",              CT_TYPE,             LANG_D                                                                      },
   { "ucent",              CT_TYPE,             LANG_D                                                                      },
   { "uint",               CT_TYPE,             LANG_CS | LANG_VALA | LANG_D                                                },
   { "ulong",              CT_TYPE,             LANG_CS | LANG_VALA | LANG_D                                                },
   { "unchecked",          CT_QUALIFIER,        LANG_CS                                                                     },
   { "undef",              CT_PP_UNDEF,         LANG_ALL | FLAG_PP                                                          }, // PAWN
   { "union",              CT_UNION,            LANG_C | LANG_CPP | LANG_OC | LANG_D                                        },
   { "unittest",           CT_UNITTEST,         LANG_D                                                                      },
   { "unsafe",             CT_UNSAFE,           LANG_CS                                                                     },
   { "unsigned",           CT_TYPE,             LANG_C | LANG_CPP | LANG_OC                                                 },
   { "ushort",             CT_TYPE,             LANG_CS | LANG_VALA | LANG_D                                                },
   { "using",              CT_USING,            LANG_CPP | LANG_CS | LANG_VALA                                              },
   { "var",                CT_TYPE,             LANG_VALA | LANG_ECMA                                                       },
   { "version",            CT_D_VERSION,        LANG_D                                                                      },
   { "virtual",            CT_QUALIFIER,        LANG_CPP | LANG_CS | LANG_VALA                                              },
   { "void",               CT_TYPE,             LANG_ALLC                                                                   },
   { "volatile",           CT_QUALIFIER,        LANG_C | LANG_CPP | LANG_OC | LANG_CS | LANG_JAVA | LANG_ECMA               },
   { "volatile",           CT_VOLATILE,         LANG_D                                                                      },
   { "wchar",              CT_TYPE,             LANG_D                                                                      },
   { "wchar_t",            CT_TYPE,             LANG_C | LANG_CPP | LANG_OC                                                 },
   { "weak",               CT_QUALIFIER,        LANG_VALA                                                                   },
   { "when",               CT_WHEN,             LANG_CS                                                                     },
   { "while",              CT_WHILE,            LANG_ALL                                                                    }, // PAWN
   { "with",               CT_D_WITH,           LANG_D | LANG_ECMA                                                          },
   { "xor",                CT_SARITH,           LANG_CPP                                                                    },
   { "xor_eq",             CT_SASSIGN,          LANG_CPP                                                                    },
};


void init_keywords()
{
}


static int kw_compare(const void *p1, const void *p2)
{
   const chunk_tag_t *t1 = static_cast<const chunk_tag_t *>(p1);
   const chunk_tag_t *t2 = static_cast<const chunk_tag_t *>(p2);

   return(strcmp(t1->tag, t2->tag));
}


bool keywords_are_sorted(void)
{
   for (int idx = 1; idx < static_cast<int> ARRAY_SIZE(keywords); idx++)
   {
      if (kw_compare(&keywords[idx - 1], &keywords[idx]) > 0)
      {
         fprintf(stderr, "%s: bad sort order at idx %d, words '%s' and '%s'\n",
                 __func__, idx - 1, keywords[idx - 1].tag, keywords[idx].tag);
         log_flush(true);
         cpd.error_count++;
         return(false);
      }
   }

   return(true);
}


void add_keyword(const char *tag, c_token_t type)
{
   string ss = tag;

   // See if the keyword has already been added
   dkwmap::iterator it = dkwm.find(ss);

   if (it != dkwm.end())
   {
      LOG_FMT(LDYNKW, "%s: changed '%s' to %d\n", __func__, tag, type);
      (*it).second = type;
      return;
   }

   // Insert the keyword
   dkwm.insert(dkwmap::value_type(ss, type));
   LOG_FMT(LDYNKW, "%s: added '%s' as %d\n", __func__, tag, type);
}


void remove_keyword(const string &tag)
{
   if (tag.empty())
   {
      return;
   }

   // See if the keyword exists in the map
   dkwmap::iterator it = dkwm.find(tag);
   if (it == dkwm.end())
   {
      return;
   }

   // Remove the keyword
   dkwm.erase(it);
   LOG_FMT(LDYNKW, "%s: removed '%s'\n", __func__, tag.c_str());
}


static const chunk_tag_t *kw_static_first(const chunk_tag_t *tag)
{
   const chunk_tag_t *prev = tag - 1;

   // TODO: avoid pointer arithmetics
   // loop over static keyword array
   while (  prev >= &keywords[0]                // not at beginning of keyword array
         && strcmp(prev->tag, tag->tag) == 0)   // tags match
   {
      tag = prev;
      prev--;
   }
   return(tag);
}


static const chunk_tag_t *kw_static_match(const chunk_tag_t *tag)
{
   bool in_pp = (  cpd.in_preproc != CT_NONE
                && cpd.in_preproc != CT_PP_DEFINE);

   for (const chunk_tag_t *iter = kw_static_first(tag);
        iter < &keywords[ARRAY_SIZE(keywords)];
        iter++)
   {
      bool pp_iter = (iter->lang_flags & FLAG_PP) != 0; // forcing value to bool
      if (  (strcmp(iter->tag, tag->tag) == 0)
         && (cpd.lang_flags & iter->lang_flags)
         && in_pp == pp_iter)
      {
         return(iter);
      }
   }
   return(nullptr);
}


c_token_t find_keyword_type(const char *word, size_t len)
{
   if (len <= 0)
   {
      return(CT_NONE);
   }

   // check the dynamic word list first
   string           ss(word, len);
   dkwmap::iterator it = dkwm.find(ss);
   if (it != dkwm.end())
   {
      return((*it).second);
   }

   chunk_tag_t key;
   key.tag = ss.c_str();

   // check the static word list
   const chunk_tag_t *p_ret = static_cast<const chunk_tag_t *>(
      bsearch(&key, keywords, ARRAY_SIZE(keywords), sizeof(keywords[0]), kw_compare));

   if (p_ret != nullptr)
   {
      p_ret = kw_static_match(p_ret);
   }
   return((p_ret != nullptr) ? p_ret->type : CT_WORD);
}


int load_keyword_file(const char *filename)
{
   FILE *pf = fopen(filename, "r");

   if (pf == nullptr)
   {
      LOG_FMT(LERR, "%s: fopen(%s) failed: %s (%d)\n",
              __func__, filename, strerror(errno), errno);
      cpd.error_count++;
      return(EX_IOERR);
   }

#define MAXLENGTHOFLINE    256
#define NUMBEROFARGS       2
   // maximal length of a line in the file
   char   buf[MAXLENGTHOFLINE];
   char   *args[NUMBEROFARGS];
   size_t line_no = 0;

   // read file line by line
   while (fgets(buf, MAXLENGTHOFLINE, pf) != nullptr)
   {
      line_no++;

      // remove comments after '#' sign
      char *ptr;
      if ((ptr = strchr(buf, '#')) != nullptr)
      {
         *ptr = 0; // set string end where comment begins
      }

      size_t argc = Args::SplitLine(buf, args, NUMBEROFARGS);

      if (argc > 0)
      {
         if (argc == 1 && CharTable::IsKw1(*args[0]))
         {
            add_keyword(args[0], CT_TYPE);
         }
         else
         {
            LOG_FMT(LWARN, "%s:%zu Invalid line (starts with '%s')\n",
                    filename, line_no, args[0]);
            cpd.error_count++;
         }
      }
      else
      {
         continue; // the line is empty
      }
   }

   fclose(pf);
   return(EX_OK);
} // load_keyword_file


void print_keywords(FILE *pfile)
{
   for (const auto &keyword_pair : dkwm)
   {
      c_token_t tt = keyword_pair.second;
      if (tt == CT_TYPE)
      {
         fprintf(pfile, "type %*.s%s\n",
                 MAX_OPTION_NAME_LEN - 4, " ", keyword_pair.first.c_str());
      }
      else if (tt == CT_MACRO_OPEN)
      {
         fprintf(pfile, "macro-open %*.s%s\n",
                 MAX_OPTION_NAME_LEN - 11, " ", keyword_pair.first.c_str());
      }
      else if (tt == CT_MACRO_CLOSE)
      {
         fprintf(pfile, "macro-close %*.s%s\n",
                 MAX_OPTION_NAME_LEN - 12, " ", keyword_pair.first.c_str());
      }
      else if (tt == CT_MACRO_ELSE)
      {
         fprintf(pfile, "macro-else %*.s%s\n",
                 MAX_OPTION_NAME_LEN - 11, " ", keyword_pair.first.c_str());
      }
      else
      {
         const char *tn = get_token_name(tt);

         fprintf(pfile, "set %s %*.s%s\n", tn,
                 int(MAX_OPTION_NAME_LEN - (4 + strlen(tn))), " ", keyword_pair.first.c_str());
      }
   }
}


void clear_keyword_file(void)
{
   dkwm.clear();
}


pattern_class_e get_token_pattern_class(c_token_t tok)
{
   // TODO: instead of this switch better assign the pattern class to each statement
   switch (tok)
   {
   case CT_IF:
   case CT_ELSEIF:
   case CT_SWITCH:
   case CT_FOR:
   case CT_WHILE:
   case CT_SYNCHRONIZED:
   case CT_USING_STMT:
   case CT_LOCK:
   case CT_D_WITH:
   case CT_D_VERSION_IF:
   case CT_D_SCOPE_IF:
      return(pattern_class_e::PBRACED);

   case CT_ELSE:
      return(pattern_class_e::ELSE);

   case CT_DO:
   case CT_TRY:
   case CT_FINALLY:
   case CT_BODY:
   case CT_UNITTEST:
   case CT_UNSAFE:
   case CT_VOLATILE:
   case CT_GETSET:
      return(pattern_class_e::BRACED);

   case CT_CATCH:
   case CT_D_VERSION:
   case CT_DEBUG:
      return(pattern_class_e::OPBRACED);

   case CT_NAMESPACE:
      return(pattern_class_e::VBRACED);

   case CT_WHILE_OF_DO:
      return(pattern_class_e::PAREN);

   case CT_INVARIANT:
      return(pattern_class_e::OPPAREN);

   default:
      return(pattern_class_e::NONE);
   } // switch
}    // get_token_pattern_class
