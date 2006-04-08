/**
 * @file uncrustify_types.d
 *
 * Defines some types for the uncrustify program
 *
 * $Id: cparse_types.h 125 2006-03-27 03:32:30Z bengardner $
 */

module uncrustify.types;

//import uncrustify.tokens;


enum brstage_t
{
   BS_NONE,
   BS_PAREN1,   /* if/for/switch/while */
   BS_PAREN2,   /* while of do */
   BS_BRACE_DO, /* do */
   BS_BRACE2,   /* if/else/for/switch/while */
   BS_ELSE,     /* expecting 'else' after 'if' */
   BS_ELSEIF,   /* expecting 'if' after 'else' */
   BS_WHILE,    /* expecting 'while' after 'do' */
};

/**
 * Structure for counting nested level
 */
struct paren_stack_entry
{
   c_token_t type;         /**< the type that opened the entry */
   int       open_line;    /**< line that open symbol is on */
   int       indent;       /**< indent level (depends on use) */
   int       indent_tmp;   /**< temporary indent level (depends on use) */
   int       ref;
   int       min_col;
   c_token_t parent;       /**< if, for, function, etc */
   brstage_t stage;
   bool      in_preproc;   /**> whether this was created in a preprocessor */
};

/* TODO: put this on a linked list */
struct parse_frame
{
   int                      level;           // level of paren
   int                      brace_level;     // level of brace/vbrace

   int                      sparen_count;

   paren_stack_entry        pse[64];
   int                      pse_tos;
   int                      paren_count;

   int                      in_ifdef;
   int                      stmt_count;
   int                      expr_count;

   bool                     maybe_decl;
   bool                     maybe_cast;
};

const int PCF_STMT_START       = 0x01;  /* marks the start of a statment */
const int PCF_EXPR_START       = 0x02;
const int PCF_IN_PREPROC       = 0x04;  /* in a preprocessor */
const int PCF_MFC_PAREN        = 0x10;  /* macro function close paren */
const int PCF_VAR_DEF          = 0x20;  /* variable name in a variable def */
const int PCF_VAR_1ST          = 0x40;  /* 1st variable def in a statement */
const int PCF_VAR_1ST_DEF      = (PCF_VAR_DEF | PCF_VAR_1ST);
const int PCF_VAR_INLINE       = 0x80;  /* type was an inline struct/enum/union */
const int PCF_DEF_ALIGNED      = 0x100;
const int PCF_IN_FCN_DEF       = 0x200; /* inside function def parens */
const int PCF_IN_FCN_CALL      = 0x400; /* inside function call parens */
const int PCF_IN_SPAREN        = 0x800; /* inside for/if/while/switch parens */
const int PCF_RIGHT_COMMENT    = 0x1000;
const int PCF_OLD_FCN_PARAMS   = 0x2000;
const int PCF_WAS_ALIGNED      = 0x4000;
const int PCF_OPTIONAL         = 0x8000;

/* flags that get copied when a new chunk is inserted */
const int PCF_COPY_FLAGS       = (PCF_IN_PREPROC | PCF_IN_SPAREN |
                                  PCF_IN_FCN_DEF | PCF_IN_FCN_CALL);

/* Forward define struct */
typedef struct chunk_s chunk_t;

struct chunk_s
{
   chunk_t    *next;
   chunk_t    *prev;
   c_token_t  type;
   c_token_t  parent_type;     /* usually CT_NONE */
   uint32_t   orig_line;
   uint32_t   orig_col;
   uint32_t   orig_col_end;
   uint16_t   flags;            /* see PCF_xxx */
   int        column;           /* column of chunk */
   int        nl_count;         /* number of newlines in CT_NEWLINE */
   int        len;
   int        level;            /* nest level in {, (, or [ */
   int        brace_level;
   BOOL       after_tab;
   char []    str;
};

enum
{
   LANG_C    = 0x01,
   LANG_CPP  = 0x02,
   LANG_D    = 0x04,
   LANG_CS   = 0x08,     /*<< C# or C-sharp */
   LANG_JAVA = 0x10,
   LANG_ALL  = 0x1f,

   FLAG_PP = 0x80,  /*<< only appears in a preprocessor */
};

typedef struct
{
   char []    tag;
   c_token_t  type;
   uint8_t    lang_flags;
} chunk_tag_t;

enum pp_type
{
   PP_NONE,
   PP_UNKNOWN,
   PP_OTHER,
   PP_INCLUDE,
   PP_DEFINE,
   PP_DEFINE_BODY,
   PP_IF,
   PP_ELSE,
   PP_ENDIF
};

struct align_t
{
   int       col;
   c_token_t type;
   int       len;
};

struct chunk_stack
{
   chunk_t **pc;
   int     len;
   int     size;
};

struct cp_data
{
   FILE               *fout;

   uint8_t            lang_flags; // LANG_xxx

   uint16_t           line_number;
   uint16_t           column;  /* column for parsing */

   BOOL               consumed;

   int                did_newline;
   pp_type            in_preproc;
   int                preproc_ncnl_count;

   /* dummy entries */
   chunk_t            list_chunks;

   align_t            al[80];
   int                al_cnt;

   /* Here are all the settings */
   int                settings[UO_option_count];

   parse_frame        frames[16];
   int                frame_count;

   /* a very simple chunk stack - managed in chunk_list.c/h */
   chunk_stack_t      cs;
   int                cs_len;    /* active entries */
   int                cs_size;   /* total entry count (private) */
};



cp_data cpd;

