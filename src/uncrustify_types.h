/**
 * @file uncrustify_types.h
 *
 * Defines some types for the uncrustify program
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef UNCRUSTIFY_TYPES_H_INCLUDED
#define UNCRUSTIFY_TYPES_H_INCLUDED


#include "base_types.h"
#include "options.h"
#include "token_enum.h"    /* c_token_t */
#include "log_levels.h"
#include "logger.h"
#include <cstdio>
#include <assert.h>
#ifdef HAVE_UTIME_H
#include <utime.h>
#endif

#define UNCRUSTIFY_OFF_TEXT    " *INDENT-OFF*"
#define UNCRUSTIFY_ON_TEXT     " *INDENT-ON*"

/**
 * Brace stage enum used in brace_cleanup
 */
enum brstage_e
{
   BS_NONE,
   BS_PAREN1,    /* if/for/switch/while */
   BS_OP_PAREN1, /* optional paren: catch () { */
   BS_WOD_PAREN, /* while of do parens */
   BS_WOD_SEMI,  /* semicolon after while of do */
   BS_BRACE_DO,  /* do */
   BS_BRACE2,    /* if/else/for/switch/while */
   BS_ELSE,      /* expecting 'else' after 'if' */
   BS_ELSEIF,    /* expecting 'if' after 'else' */
   BS_WHILE,     /* expecting 'while' after 'do' */
   BS_CATCH,     /* expecting 'catch' or 'finally' after 'try' */
};

struct chunk_t;

/**
 * Structure for counting nested level
 */
struct paren_stack_entry
{
   c_token_t type;         /**< the type that opened the entry */
   int       level;        /**< Level of opening type */
   int       open_line;    /**< line that open symbol is on */
   chunk_t   *pc;          /**< Chunk that opened the level */
   int       brace_indent; /**< indent for braces - may not relate to indent */
   int       indent;       /**< indent level (depends on use) */
   int       indent_tmp;   /**< temporary indent level (depends on use) */
   int       indent_tab;   /**< the 'tab' indent (always <= real column) */
   int       ref;
   c_token_t parent;       /**< if, for, function, etc */
   brstage_e stage;
   bool      in_preproc;   /**< whether this was created in a preprocessor */
   bool      non_vardef;   /**< Hit a non-vardef line */
};

/* TODO: put this on a linked list */
struct parse_frame
{
   int                      ref_no;
   int                      level;           // level of parens/square/angle/brace
   int                      brace_level;     // level of brace/vbrace
   int                      pp_level;        // level of preproc #if stuff

   int                      sparen_count;

   struct paren_stack_entry pse[128];
   int                      pse_tos;
   int                      paren_count;

   c_token_t                in_ifdef;
   int                      stmt_count;
   int                      expr_count;

   bool                     maybe_decl;
   bool                     maybe_cast;
};

#define PCF_FORCE_SPACE        (1 << 0)  /* must have a space after this token */
#define PCF_STMT_START         (1 << 1)  /* marks the start of a statment */
#define PCF_EXPR_START         (1 << 2)
#define PCF_IN_PREPROC         (1 << 3)  /* in a preprocessor */
#define PCF_DONT_INDENT        (1 << 4)  /* already aligned! */
#define PCF_VAR_DEF            (1 << 5)  /* variable name in a variable def */
#define PCF_VAR_1ST            (1 << 6)  /* 1st variable def in a statement */
#define PCF_VAR_1ST_DEF        (PCF_VAR_DEF | PCF_VAR_1ST)
#define PCF_VAR_INLINE         (1 << 7)  /* type was an inline struct/enum/union */
#define PCF_IN_ENUM            (1 << 8)  /* in enum */
#define PCF_IN_FCN_DEF         (1 << 9)  /* inside function def parens */
#define PCF_IN_FCN_CALL        (1 << 10) /* inside function call parens */
#define PCF_IN_SPAREN          (1 << 11) /* inside for/if/while/switch parens */
#define PCF_RIGHT_COMMENT      (1 << 12)
#define PCF_OLD_FCN_PARAMS     (1 << 13)
#define PCF_WAS_ALIGNED        (1 << 14)
#define PCF_IN_TEMPLATE        (1 << 15)
#define PCF_IN_TYPEDEF         (1 << 16)
#define PCF_IN_CONST_ARGS      (1 << 17)
#define PCF_LVALUE             (1 << 18) /* left of assignment */
#define PCF_IN_ARRAY_ASSIGN    (1 << 19)
#define PCF_IN_CLASS           (1 << 20)
#define PCF_IN_NAMESPACE       (1 << 21)
#define PCF_IN_FOR             (1 << 22)
#define PCF_ONE_LINER          (1 << 23)
#define PCF_ONE_CLASS          (PCF_ONE_LINER | PCF_IN_CLASS)
#define PCF_EMPTY_BODY         (1 << 24)
#define PCF_ANCHOR             (1 << 25)  /* aligning anchor */
#define PCF_PUNCTUATOR         (1 << 26)
#define PCF_INSERTED           (1 << 27)  /* chunk was inserted from another file */
#define PCF_ALIGN_START        (1 << 28)
#define PCF_VAR_TYPE           (1 << 29)  /* part of a variable def type */
#define PCF_LONG_BLOCK         (1 << 30)  /* the block is 'long' by some measure */
#define PCF_OWN_STR            (1 << 31)  /* chunk owns the memory at str */


/* flags that get copied when a new chunk is inserted */
#define PCF_COPY_FLAGS                                      \
   (PCF_IN_PREPROC | PCF_IN_SPAREN | PCF_IN_ENUM |          \
    PCF_IN_FCN_DEF | PCF_IN_FCN_CALL | PCF_IN_TYPEDEF |     \
    PCF_IN_ARRAY_ASSIGN | PCF_IN_CLASS | PCF_IN_NAMESPACE | \
    PCF_IN_CLASS | PCF_IN_FOR | PCF_IN_TEMPLATE)

#ifdef DEFINE_PCF_NAMES
static const char *pcf_names[] =
{
   "FORCE_SPACE",
   "STMT_START",
   "EXPR_START",
   "IN_PREPROC",
   "DONT_INDENT",
   "VAR_DEF",
   "VAR_1ST",
   "VAR_INLINE",
   "IN_ENUM",
   "IN_FCN_DEF",
   "IN_FCN_CALL",
   "IN_SPAREN",
   "RIGHT_COMMENT",
   "OLD_FCN_PARAMS",
   "WAS_ALIGNED",
   "IN_TEMPLATE",
   "IN_TYPEDEF",
   "IN_CONST_ARGS",
   "LVALUE",
   "IN_ARRAY_ASSIGN",
   "IN_CLASS",
   "IN_NAMESPACE",
   "IN_FOR",
   "ONE_LINER",
   "EMPTY_BODY",
   "ANCHOR",
   "PUNCTUATOR",
   "INSERTED",
   "ALIGN_START",
   "VAR_TYPE",
   "LONG_BLOCK",
   "OWN_STR",
};
#endif

struct align_ptr_t
{
   chunk_t *next;       /* NULL or the chunk that should be under this one */
   bool    right_align; /* AlignStack.m_right_align */
   int     star_style;  /* AlignStack.m_star_style */
   int     amp_style;   /* AlignStack.m_amp_style */
   int     gap;         /* AlignStack.m_gap */

   /* col_adj is the amount to alter the column for the token.
    * For example, a dangling '*' would be set to -1.
    * A right-aligned word would be a positive value.
    */
   int     col_adj;
   chunk_t *ref;
   chunk_t *start;
};

/** This is the main type of this program */
struct chunk_t
{
   chunk_t     *next;
   chunk_t     *prev;
   align_ptr_t align;
   c_token_t   type;
   c_token_t   parent_type;     /* usually CT_NONE */
   UINT32      orig_line;
   UINT32      orig_col;
   UINT32      orig_col_end;
   UINT32      flags;            /* see PCF_xxx */
   int         column;           /* column of chunk */
   int         column_indent;    /* if 1st on a line, set to the 'indent'
                                  * column, which may be less that the real column */
   int         nl_count;         /* number of newlines in CT_NEWLINE */
   int         level;            /* nest level in {, (, or [ */
   int         brace_level;      /* nest level in braces only */
   int         pp_level;         /* nest level in #if stuff */
   bool        after_tab;        /* whether this token was after a tab */
   int         len;              /* # of bytes at str that make up the token */
   const char  *str;             /* pointer to the token text */
};

enum
{
   LANG_C    = 0x0001,
   LANG_CPP  = 0x0002,
   LANG_D    = 0x0004,
   LANG_CS   = 0x0008,     /*<< C# or C-sharp */
   LANG_JAVA = 0x0010,
   LANG_OC   = 0x0020,     /*<< Objective C */
   LANG_VALA = 0x0040,     /*<< Like C# */
   LANG_PAWN = 0x0080,
   LANG_ECMA = 0x0100,

   LANG_ALLC = 0x017f,
   LANG_ALL  = 0x0fff,

   FLAG_PP   = 0x8000,     /*<< only appears in a preprocessor */
};

/**
 * Pattern classes for special keywords
 */
enum pattern_class
{
   PATCLS_NONE,
   PATCLS_BRACED,   // keyword + braced stmt:          do, try
   PATCLS_PBRACED,  // keyword + parens + braced stmt: switch, if, for, while
   PATCLS_OPBRACED, // keyword + optional parens + braced stmt: catch, version
   PATCLS_VBRACED,  // keyword + value + braced stmt: namespace
   PATCLS_PAREN,    // keyword + parens: while-of-do
   PATCLS_OPPAREN,  // keyword + optional parens: invariant (D lang)
   PATCLS_ELSE,     // Special case of PATCLS_BRACED for handling CT_IF
};

typedef struct
{
   const char *tag;
   c_token_t  type;
   int        lang_flags;
} chunk_tag_t;

typedef struct
{
   char              ch;
   char              left_in_group;
   UINT16            next_idx;
   const chunk_tag_t *tag;
} lookup_entry_t;

typedef struct
{
   const char *tag;
   const char *value;
} define_tag_t;


struct align_t
{
   int       col;
   c_token_t type;
   int       len;    // of the token + space
};

typedef struct
{
   chunk_t *pc;
   int     seqnum;
} chunk_stack_entry_t;

typedef struct chunk_stack
{
   chunk_stack_entry_t *cse;
   int                 len;
   int                 size;
} chunk_stack_t;

struct file_mem
{
   char           *data;
   int            length;
#ifdef HAVE_UTIME_H
   struct utimbuf utb;
#endif
};

struct cp_data
{
   FILE               *fout;

   UINT32             error_count;
   const char         *filename;

   file_mem           file_hdr;   /* for cmt_insert_file_header */
   file_mem           file_ftr;   /* for cmt_insert_file_footer */
   file_mem           func_hdr;   /* for cmt_insert_func_header */
   file_mem           class_hdr;  /* for cmt_insert_class_header */

   int                lang_flags; // LANG_xxx

   bool               unc_off;
   UINT32             line_number;
   UINT16             column;  /* column for parsing */

   /* stuff to auto-detect line endings */
   UINT32             le_counts[LE_AUTO];
   char               newline[5];

   bool               consumed;

   int                did_newline;
   c_token_t          in_preproc;
   int                preproc_ncnl_count;

   chunk_t            *bom;

   /* bumped up when a line is split or indented */
   int                changes;

   struct align_t     al[80];
   int                al_cnt;
   bool               al_c99_array;

   /* Here are all the settings */
   op_val_t           settings[UO_option_count];
   int                max_option_name_len;

   struct parse_frame frames[16];
   int                frame_count;
   int                pp_level;
};

extern struct cp_data cpd;

#endif   /* UNCRUSTIFY_TYPES_H_INCLUDED */
