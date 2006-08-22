/**
 * @file keywords.cpp
 * Manages the table of keywords.
 *
 * $Id$
 */

#include "uncrustify_types.h"
#include "char_table.h"
#include "args.h"
#include <cstring>
#include <cerrno>
#include <cstdlib>
#include <cctype>

typedef struct
{
   chunk_tag_t *p_tags;
   int         total;            /* number of items at p_tags */
   int         active;           /* number of valid entries */
} dynamic_word_list_t;

/* A dynamic list of keywords - add via add_keyword() */
static dynamic_word_list_t wl;


/**
 * interesting static keywords - keep sorted.
 * Table should include the Name, Type, and Language flags.
 */
static const chunk_tag_t keywords[] =
{
   { "__const__",        CT_QUALIFIER,   LANG_C | LANG_CPP                                   },
   { "__inline__",       CT_QUALIFIER,   LANG_C | LANG_CPP                                   },
   { "__signed__",       CT_TYPE,        LANG_C | LANG_CPP                                   },
   { "__typeof__",       CT_SIZEOF,      LANG_C | LANG_CPP                                   },
   { "__volatile__",     CT_QUALIFIER,   LANG_C | LANG_CPP                                   },
   { "_Bool",            CT_TYPE,        LANG_CPP                                            },
   { "_Complex",         CT_TYPE,        LANG_CPP                                            },
   { "_Imaginary",       CT_TYPE,        LANG_CPP                                            },
   { "abstract",         CT_QUALIFIER,   LANG_CS | LANG_D | LANG_JAVA                        },
   { "alias",            CT_QUALIFIER,   LANG_D                                              },
   { "align",            CT_ALIGN,       LANG_D                                              },
   { "alignof",          CT_SIZEOF,      LANG_C | LANG_CPP                                   },
   { "and",              CT_SBOOL,       LANG_C | LANG_CPP                                   },
   { "and_eq",           CT_SASSIGN,     LANG_C | LANG_CPP                                   },
   { "as",               CT_AS,          LANG_CS                                             },
   { "asm",              CT_ASM,         LANG_C | LANG_CPP | LANG_D                          },
   { "assert",           CT_FUNCTION,    LANG_D | LANG_PAWN                                  }, // PAWN
   { "assert",           CT_PP_ASSERT,   LANG_PAWN | FLAG_PP                                 }, // PAWN
   { "auto",             CT_QUALIFIER,   LANG_C | LANG_CPP | LANG_D                          },
   { "base",             CT_BASE,        LANG_CS                                             },
   { "bit",              CT_TYPE,        LANG_D                                              },
   { "bitand",           CT_ARITH,       LANG_C | LANG_CPP                                   },
   { "bitor",            CT_ARITH,       LANG_C | LANG_CPP                                   },
   { "body",             CT_BRACED,      LANG_D                                              },
   { "bool",             CT_TYPE,        LANG_CPP | LANG_CS                                  },
   { "boolean",          CT_TYPE,        LANG_JAVA                                           },
   { "break",            CT_BREAK,       LANG_ALL                                            }, // PAWN
   { "byte",             CT_TYPE,        LANG_CS | LANG_D | LANG_JAVA                        },
   { "case",             CT_CASE,        LANG_ALL                                            }, // PAWN
   { "cast",             CT_CAST,        LANG_D                                              },
   { "catch",            CT_POBRACED,    LANG_CPP | LANG_CS | LANG_D | LANG_JAVA             },
   { "cdouble",          CT_TYPE,        LANG_D                                              },
   { "cent",             CT_TYPE,        LANG_D                                              },
   { "cfloat",           CT_TYPE,        LANG_D                                              },
   { "char",             CT_CHAR,        LANG_PAWN                                           }, // PAWN
   { "char",             CT_TYPE,        LANG_ALLC                                           },
   { "checked",          CT_QUALIFIER,   LANG_CS                                             },
   { "class",            CT_CLASS,       LANG_CPP | LANG_CS | LANG_D | LANG_JAVA             },
   { "compl",            CT_ARITH,       LANG_C | LANG_CPP                                   },
   { "const",            CT_QUALIFIER,   LANG_ALL                                            }, // PAWN
   { "const_cast",       CT_TYPE_CAST,   LANG_CPP                                            },
   { "continue",         CT_CONTINUE,    LANG_ALL                                            }, // PAWN
   { "creal",            CT_TYPE,        LANG_D                                              },
   { "dchar",            CT_TYPE,        LANG_D                                              },
   { "debug",            CT_PBRACED,     LANG_D                                              },
   { "default",          CT_DEFAULT,     LANG_ALL                                            }, // PAWN
   { "define",           CT_PP_DEFINE,   LANG_ALL | FLAG_PP                                  }, // PAWN
   { "defined",          CT_DEFINED,     LANG_PAWN                                           }, // PAWN
   { "defined",          CT_PP_DEFINED,  LANG_ALLC | FLAG_PP                                 },
   { "delegate",         CT_DELEGATE,    LANG_CS | LANG_D                                    },
   { "delete",           CT_DELETE,      LANG_CPP | LANG_D                                   },
   { "deprecated",       CT_QUALIFIER,   LANG_D                                              },
   { "do",               CT_DO,          LANG_ALL                                            }, // PAWN
   { "double",           CT_TYPE,        LANG_ALLC                                           },
   { "dynamic_cast",     CT_TYPE_CAST,   LANG_CPP                                            },
   { "elif",             CT_PP_ELSE,     LANG_ALLC | FLAG_PP                                 },
   { "else",             CT_ELSE,        LANG_ALL                                            }, // PAWN
   { "else",             CT_PP_ELSE,     LANG_ALL | FLAG_PP                                  }, // PAWN
   { "elseif",           CT_PP_ELSE,     LANG_PAWN | FLAG_PP                                 }, // PAWN
   { "emit",             CT_PP_EMIT,     LANG_PAWN | FLAG_PP                                 }, // PAWN
   { "endif",            CT_PP_ENDIF,    LANG_ALL | FLAG_PP                                  }, // PAWN
   { "endinput",         CT_PP_ENDINPUT, LANG_PAWN | FLAG_PP                                 }, // PAWN
   { "endscript",        CT_PP_ENDINPUT, LANG_PAWN | FLAG_PP                                 }, // PAWN
   { "endregion",        CT_PP_ENDIF,    LANG_CS | FLAG_PP                                   },
   { "enum",             CT_ENUM,        LANG_ALL                                            }, // PAWN
   { "error",            CT_PP_ERROR,    LANG_PAWN | FLAG_PP                                 }, // PAWN
   { "event",            CT_TYPE,        LANG_CS                                             },
   { "exit",             CT_FUNCTION,    LANG_PAWN                                           }, // PAWN
   { "explicit",         CT_TYPE,        LANG_C | LANG_CPP | LANG_CS                         },
   { "export",           CT_EXPORT,      LANG_C | LANG_CPP | LANG_D                          },
   { "extends",          CT_QUALIFIER,   LANG_JAVA                                           },
   { "extern",           CT_TYPE,        LANG_C | LANG_CPP | LANG_CS | LANG_D                },
   { "false",            CT_WORD,        LANG_CPP | LANG_CS | LANG_D | LANG_JAVA             },
   { "file",             CT_PP_FILE,     LANG_PAWN | FLAG_PP                                 }, // PAWN
   { "final",            CT_QUALIFIER,   LANG_D                                              },
   { "float",            CT_TYPE,        LANG_ALLC                                           },
   { "for",              CT_FOR,         LANG_ALL                                            }, // PAWN
   { "foreach",          CT_FOR,         LANG_CS | LANG_D                                    },
   { "forward",          CT_FORWARD,     LANG_PAWN                                           }, // PAWN
   { "friend",           CT_FRIEND,      LANG_CPP                                            },
   { "function",         CT_FUNCTION,    LANG_D                                              },
   { "get",              CT_GETSET,      LANG_CS                                             },
   { "goto",             CT_GOTO,        LANG_ALL                                            }, // PAWN
   { "idouble",          CT_TYPE,        LANG_D                                              },
   { "if",               CT_IF,          LANG_ALL                                            }, // PAWN
   { "if",               CT_PP_IF,       LANG_ALL | FLAG_PP                                  }, // PAWN
   { "ifdef",            CT_PP_IF,       LANG_ALLC | FLAG_PP                                 },
   { "ifloat",           CT_TYPE,        LANG_D                                              },
   { "ifndef",           CT_PP_IF,       LANG_ALLC | FLAG_PP                                 },
   { "implements",       CT_QUALIFIER,   LANG_JAVA                                           },
   { "implicit",         CT_QUALIFIER,   LANG_CS                                             },
   { "import",           CT_TYPE,        LANG_D | LANG_JAVA                                  }, // fudged to get indenting
   { "in",               CT_IN,          LANG_D | LANG_CS                                    },
   { "include",          CT_PP_INCLUDE,  LANG_C | LANG_CPP | LANG_PAWN | FLAG_PP             }, // PAWN
   { "inline",           CT_QUALIFIER,   LANG_C | LANG_CPP                                   },
   { "inout",            CT_QUALIFIER,   LANG_D                                              },
   { "instanceof",       CT_SIZEOF,      LANG_JAVA                                           },
   { "int",              CT_TYPE,        LANG_ALLC                                           },
   { "interface",        CT_CLASS,       LANG_CS | LANG_D | LANG_JAVA                        },
   { "internal",         CT_QUALIFIER,   LANG_CS                                             },
   { "invariant",        CT_BRACED,      LANG_D                                              },
   { "ireal",            CT_TYPE,        LANG_D                                              },
   { "is",               CT_SCOMPARE,    LANG_D | LANG_CS                                    },
   { "line",             CT_PP_LINE,     LANG_PAWN | FLAG_PP                                 }, // PAWN
   { "lock",             CT_PBRACED,     LANG_CS                                             },
   { "long",             CT_TYPE,        LANG_ALLC                                           },
   { "mixin",            CT_CLASS,       LANG_D                                              }, // may need special handling
   { "module",           CT_USING,       LANG_D                                              },
   { "mutable",          CT_MUTABLE,     LANG_C | LANG_CPP                                   },
   { "namespace",        CT_NAMESPACE,   LANG_C | LANG_CPP | LANG_CS                         },
   { "native",           CT_QUALIFIER,   LANG_JAVA                                           },
   { "native",           CT_NATIVE,      LANG_PAWN                                           }, // PAWN
   { "new",              CT_NEW,         LANG_CPP | LANG_CS | LANG_D | LANG_JAVA | LANG_PAWN }, // PAWN
   { "not",              CT_SARITH,      LANG_C | LANG_CPP                                   },
   { "not_eq",           CT_SCOMPARE,    LANG_C | LANG_CPP                                   },
   { "null",             CT_TYPE,        LANG_CS | LANG_D | LANG_JAVA                        },
   { "object",           CT_TYPE,        LANG_CS                                             },
   { "operator",         CT_OPERATOR,    LANG_CPP | LANG_CS | LANG_PAWN                      }, // PAWN
   { "or",               CT_SBOOL,       LANG_C | LANG_CPP                                   },
   { "or_eq",            CT_SASSIGN,     LANG_C | LANG_CPP                                   },
   { "out",              CT_QUALIFIER,   LANG_CS | LANG_D                                    },
   { "override",         CT_QUALIFIER,   LANG_CS | LANG_D                                    },
   { "package",          CT_NAMESPACE,   LANG_D | LANG_JAVA                                  },
   { "params",           CT_TYPE,        LANG_CS                                             },
   { "pragma",           CT_PP_PRAGMA,   LANG_D | LANG_PAWN | FLAG_PP                        }, // PAWN
   { "private",          CT_PRIVATE,     LANG_ALLC                                           }, // not C
   { "protected",        CT_PRIVATE,     LANG_ALLC                                           }, // not C
   { "public",           CT_PRIVATE,     LANG_ALL                                            }, // PAWN // not C
   { "readonly",         CT_QUALIFIER,   LANG_CS                                             },
   { "real",             CT_TYPE,        LANG_D                                              },
   { "ref",              CT_QUALIFIER,   LANG_CS                                             },
   { "region",           CT_PP_IF,       LANG_CS | FLAG_PP                                   },
   { "register",         CT_QUALIFIER,   LANG_C | LANG_CPP                                   },
   { "reinterpret_cast", CT_TYPE_CAST,   LANG_C | LANG_CPP                                   },
   { "restrict",         CT_QUALIFIER,   LANG_C | LANG_CPP                                   },
   { "return",           CT_RETURN,      LANG_ALL                                            }, // PAWN
   { "sbyte",            CT_TYPE,        LANG_CS                                             },
   { "section",          CT_PP_SECTION,  LANG_PAWN | FLAG_PP                                 }, // PAWN
   { "sealed",           CT_QUALIFIER,   LANG_CS                                             },
   { "set",              CT_GETSET,      LANG_CS                                             },
   { "short",            CT_TYPE,        LANG_ALLC                                           },
   { "signed",           CT_TYPE,        LANG_C | LANG_CPP                                   },
   { "sizeof",           CT_SIZEOF,      LANG_C | LANG_CPP | LANG_CS | LANG_PAWN             }, // PAWN
   { "sleep",            CT_FUNCTION,    LANG_C | LANG_CPP | LANG_CS | LANG_PAWN             }, // PAWN
   { "stackalloc",       CT_NEW,         LANG_CS                                             },
   { "state",            CT_STATE,       LANG_PAWN                                           }, // PAWN
   { "static",           CT_QUALIFIER,   LANG_ALL                                            }, // PAWN
   { "static_cast",      CT_TYPE_CAST,   LANG_CPP                                            },
   { "stock",            CT_STOCK,       LANG_PAWN                                           }, // PAWN
   { "strictfp",         CT_QUALIFIER,   LANG_JAVA                                           },
   { "string",           CT_TYPE,        LANG_CS                                             },
   { "struct",           CT_STRUCT,      LANG_C | LANG_CPP | LANG_CS | LANG_D                },
   { "super",            CT_SUPER,       LANG_D | LANG_JAVA                                  },
   { "switch",           CT_SWITCH,      LANG_ALL                                            }, // PAWN
   { "synchronized",     CT_QUALIFIER,   LANG_D | LANG_JAVA                                  },
   { "tagof",            CT_TAGOF,       LANG_PAWN                                           }, // PAWN
   { "template",         CT_TEMPLATE,    LANG_CPP | LANG_D                                   },
   { "this",             CT_THIS,        LANG_CPP | LANG_CS | LANG_D | LANG_JAVA             },
   { "throw",            CT_THROW,       LANG_CPP | LANG_CS | LANG_D | LANG_JAVA             },
   { "throws",           CT_QUALIFIER,   LANG_JAVA                                           },
   { "transient",        CT_QUALIFIER,   LANG_JAVA                                           },
   { "true",             CT_WORD,        LANG_CPP | LANG_CS | LANG_D | LANG_JAVA             },
   { "try",              CT_BRACED,      LANG_CPP | LANG_CS | LANG_D | LANG_JAVA             },
   { "tryinclude",       CT_PP_INCLUDE,  LANG_PAWN | FLAG_PP                                 }, // PAWN
   { "typedef",          CT_TYPEDEF,     LANG_C | LANG_CPP | LANG_D                          },
   { "typeid",           CT_SIZEOF,      LANG_C | LANG_CPP | LANG_D                          },
   { "typename",         CT_TYPENAME,    LANG_CPP                                            },
   { "typeof",           CT_SIZEOF,      LANG_C | LANG_CPP | LANG_CS | LANG_D                },
   { "ubyte",            CT_TYPE,        LANG_D                                              },
   { "ucent",            CT_TYPE,        LANG_D                                              },
   { "uint",             CT_TYPE,        LANG_CS | LANG_D                                    },
   { "ulong",            CT_TYPE,        LANG_CS | LANG_D                                    },
   { "unchecked",        CT_QUALIFIER,   LANG_CS                                             },
   { "undef",            CT_PP_UNDEF,    LANG_ALL | FLAG_PP                                  }, // PAWN
   { "union",            CT_UNION,       LANG_C | LANG_CPP | LANG_D                          },
   { "unittest",         CT_BRACED,      LANG_D                                              },
   { "unsafe",           CT_BRACED,      LANG_CS                                             },
   { "unsigned",         CT_TYPE,        LANG_C | LANG_CPP                                   },
   { "ushort",           CT_TYPE,        LANG_CS | LANG_D                                    },
   { "using",            CT_USING,       LANG_CS                                             },
   { "version",          CT_VERSION,     LANG_D                                              },
   { "virtual",          CT_QUALIFIER,   LANG_CPP | LANG_CS                                  },
   { "void",             CT_TYPE,        LANG_ALLC                                           },
   { "volatile",         CT_QUALIFIER,   LANG_C | LANG_CPP | LANG_CS | LANG_JAVA             },
   { "volatile",         CT_VOLATILE,    LANG_D                                              },
   { "wchar",            CT_TYPE,        LANG_D                                              },
   { "wchar_t",          CT_TYPE,        LANG_C | LANG_CPP                                   },
   { "while",            CT_WHILE,       LANG_ALL                                            }, // PAWN
   { "with",             CT_USING,       LANG_D                                              },
   { "xor",              CT_SARITH,      LANG_C | LANG_CPP                                   },
   { "xor_eq",           CT_SASSIGN,     LANG_C | LANG_CPP                                   },
};


/**
 * Compares two chunk_tag_t entries using strcmp on the strings
 *
 * @param p1   The 'left' entry
 * @param p2   The 'right' entry
 */
static int kw_compare(const void *p1, const void *p2)
{
   const chunk_tag_t *t1 = (const chunk_tag_t *)p1;
   const chunk_tag_t *t2 = (const chunk_tag_t *)p2;

   return(strcmp(t1->tag, t2->tag));
}


/**
 * Adds a keyword to the list of dynamic keywords
 *
 * @param tag        The tag (string) must be zero terminated
 * @param type       The type, usually CT_TYPE
 * @param lang_flags Language flags, typically LANG_ALL
 */
void add_keyword(const char *tag, c_token_t type, UINT8 lang_flags)
{
   /* Do we need to allocate more memory? */
   if ((wl.total == wl.active) || (wl.p_tags == NULL))
   {
      wl.total += 16;
      if (wl.p_tags == NULL)
      {
         wl.p_tags = (chunk_tag_t *)malloc(sizeof(chunk_tag_t) * wl.total);
      }
      else
      {
         wl.p_tags = (chunk_tag_t *)realloc(wl.p_tags, sizeof(chunk_tag_t) * wl.total);
      }
   }
   if (wl.p_tags != NULL)
   {
      wl.p_tags[wl.active].tag        = strdup(tag);
      wl.p_tags[wl.active].type       = type;
      wl.p_tags[wl.active].lang_flags = lang_flags;
      wl.active++;

      /* Todo: add in sorted order instead of resorting the whole list? */
      qsort(wl.p_tags, wl.active, sizeof(chunk_tag_t), kw_compare);

      LOG_FMT(LDYNKW, "%s: added '%s'\n", __func__, tag);
   }
}

/**
 * Backs up to the first string match in keywords.
 */
static const chunk_tag_t *kw_static_first(const chunk_tag_t *tag)
{
   const chunk_tag_t *prev = tag - 1;

   while ((prev >= &keywords[0]) && (strcmp(prev->tag, tag->tag) == 0))
   {
      tag = prev;
      prev--;
   }
   //fprintf(stderr, "first:%s -", tag->tag);
   return(tag);
}

static const chunk_tag_t *kw_static_match(const chunk_tag_t *tag)
{
   bool in_pp = ((cpd.in_preproc != CT_NONE) && (cpd.in_preproc != CT_PP_DEFINE));
   bool pp_iter;
   const chunk_tag_t *iter;

   for (iter = kw_static_first(tag);
        iter < &keywords[ARRAY_SIZE(keywords)];
        iter++)
   {
      //fprintf(stderr, " check:%s", iter->tag);
      pp_iter = (iter->lang_flags & FLAG_PP) != 0;
      if ((strcmp(iter->tag, tag->tag) == 0) &&
          ((cpd.lang_flags & iter->lang_flags) != 0) &&
          (in_pp == pp_iter))
      {
         //fprintf(stderr, " match:%s", iter->tag);
         return(iter);
      }
   }
   return(NULL);
}

/**
 * Search first the dynamic and then the static table for a matching keyword
 *
 * @param word    Pointer to the text -- NOT zero terminated
 * @param len     The length of the text
 * @return        NULL (no match) or the keyword entry
 */
const chunk_tag_t *find_keyword(const char *word, int len)
{
   chunk_tag_t       tag;
   char              buf[32];
   const chunk_tag_t *p_ret;

   if (len > (int)(sizeof(buf) - 1))
   {
      LOG_FMT(LNOTE, "%s: keyword too long at %d char (%d max) : %.*s\n",
              __func__, len, sizeof(buf), len, word);
      return(NULL);
   }
   memcpy(buf, word, len);
   buf[len] = 0;

   tag.tag = buf;

   /* check the dynamic word list first */
   p_ret = (const chunk_tag_t *)bsearch(&tag, wl.p_tags, wl.active,
                                        sizeof(chunk_tag_t), kw_compare);
   if (p_ret == NULL)
   {
      /* check the static word list */
      p_ret = (const chunk_tag_t *)bsearch(&tag, keywords, ARRAY_SIZE(keywords),
                                           sizeof(keywords[0]), kw_compare);
      if (p_ret != NULL)
      {
         //fprintf(stderr, "%s: match %s -", __func__, p_ret->tag);
         p_ret = kw_static_match(p_ret);
         //fprintf(stderr, "\n");
      }
   }
   return(p_ret);
}


/**
 * Loads the dynamic keywords from a file
 *
 * @param filename   The path to the file to load
 * @return           SUCCESS or FAILURE
 */
int load_keyword_file(const char *filename)
{
   FILE *pf;
   char buf[160];
   char *ptr;
   char *args[3];
   int  argc;
   int  line_no = 0;

   pf = fopen(filename, "r");
   if (pf == NULL)
   {
      LOG_FMT(LERR, "%s: fopen(%s) failed: %s (%d)\n",
              __func__, filename, strerror(errno), errno);
      cpd.error_count++;
      return(FAILURE);
   }

   while (fgets(buf, sizeof(buf), pf) != NULL)
   {
      line_no++;

      /* remove comments */
      if ((ptr = strchr(buf, '#')) != NULL)
      {
         *ptr = 0;
      }

      argc       = Args::SplitLine(buf, args, ARRAY_SIZE(args) - 1);
      args[argc] = 0;

      if (argc > 0)
      {
         if ((argc == 1) && ((get_char_table(*args[0]) & CT_KW1) != 0))
         {
            add_keyword(args[0], CT_TYPE, LANG_ALL);
         }
         else
         {
            LOG_FMT(LWARN, "%s:%d Invalid line (starts with '%s')\n",
                    filename, line_no, args[0]);
            cpd.error_count++;
         }
      }
   }

   fclose(pf);
   return(SUCCESS);
}

void output_types(FILE *pfile)
{
   int idx;

   if (wl.active > 0)
   {
      fprintf(pfile, "-== User Types ==-\n");
   }
   for (idx = 0; idx < wl.active; idx++)
   {
      fprintf(pfile, "%s\n", wl.p_tags[idx].tag);
   }
}
