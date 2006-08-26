/**
 * @file uncrustify_types.h
 *
 * Defines some types for the uncrustify program
 *
 * $Id$
 */

#ifndef UNCRUSTIFY_TYPES_H_INCLUDED
#define UNCRUSTIFY_TYPES_H_INCLUDED


#include "base_types.h"
#include "options.h"
#include "token_enum.h"    /* c_token_t */
#include "log_levels.h"
#include "logger.h"
#include <cstdio>


/**
 * Brace stage enum used in brace_cleanup
 */
enum brstage_e
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
   brstage_e stage;
   bool      in_preproc;   /**> whether this was created in a preprocessor */
};

/* TODO: put this on a linked list */
struct parse_frame
{
   int                      level;           // level of parens/square/angle/brace
   int                      brace_level;     // level of brace/vbrace
   int                      pp_level;        // level of preproc #if stuff

   int                      sparen_count;

   struct paren_stack_entry pse[128];
   int                      pse_tos;
   int                      paren_count;

   int                      in_ifdef;
   int                      stmt_count;
   int                      expr_count;

   bool                     maybe_decl;
   bool                     maybe_cast;
};

#define PCF_STMT_START         (1 << 0)  /* marks the start of a statment */
#define PCF_EXPR_START         (1 << 1)
#define PCF_IN_PREPROC         (1 << 2)  /* in a preprocessor */
#define PCF_DONT_INDENT        (1 << 3)  /* already aligned! */
#define PCF_MFC_PAREN          (1 << 4)  /* macro function close paren */
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
#define PCF_OPTIONAL           (1 << 15)
#define PCF_IN_TYPEDEF         (1 << 16)
#define PCF_IN_CONST_ARGS      (1 << 17)

/* flags that get copied when a new chunk is inserted */
#define PCF_COPY_FLAGS         (PCF_IN_PREPROC | PCF_IN_SPAREN | PCF_IN_ENUM | \
                                PCF_IN_FCN_DEF | PCF_IN_FCN_CALL | PCF_IN_TYPEDEF)

#ifdef DEFINE_PCF_NAMES
static const char *pcf_names[32] =
{
   "STMT_START",
   "EXPR_START",
   "IN_PREPROC",
   "DONT_INDENT",
   "MFC_PAREN",
   "VAR_DEF",
   "VAR_1ST",
   "VAR_1ST_DEF",
   "VAR_INLINE",
   "DEF_ALIGNED",
   "IN_FCN_DEF",
   "IN_FCN_CALL",
   "IN_SPAREN",
   "RIGHT_COMMENT",
   "OLD_FCN_PARAMS",
   "WAS_ALIGNED",
   "OPTIONAL",
   "IN_TYPEDEF",
};
#endif

/** This is the main type of this program */
struct chunk_t
{
   chunk_t    *next;
   chunk_t    *prev;
   c_token_t  type;
   c_token_t  parent_type;     /* usually CT_NONE */
   UINT32     orig_line;
   UINT32     orig_col;
   UINT32     orig_col_end;
   UINT32     flags;            /* see PCF_xxx */
   int        column;           /* column of chunk */
   int        nl_count;         /* number of newlines in CT_NEWLINE */
   int        level;            /* nest level in {, (, or [ */
   int        brace_level;      /* nest level in braces only */
   int        pp_level;         /* nest level in #if stuff */
   bool       after_tab;
   int        len;
   const char *str;
};

enum
{
   LANG_C    = 0x01,
   LANG_CPP  = 0x02,
   LANG_D    = 0x04,
   LANG_CS   = 0x08,     /*<< C# or C-sharp */
   LANG_JAVA = 0x10,
   LANG_PAWN = 0x20,
   LANG_ALLC = 0x1f,
   LANG_ALL  = 0x2f,

   FLAG_PP = 0x80,  /*<< only appears in a preprocessor */
};

typedef struct
{
   const char *tag;
   c_token_t  type;
   UINT8      lang_flags;
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

struct cp_data
{
   FILE               *fout;

   UINT32             error_count;
   const char         *filename;

   UINT8              lang_flags; // LANG_xxx

   UINT16             line_number;
   UINT16             column;  /* column for parsing */

   /* stuff to auto-detect line endings */
   UINT32             le_counts[LE_AUTO];
   char               newline[5];

   bool               consumed;

   int                did_newline;
   c_token_t          in_preproc;
   int                preproc_ncnl_count;

   /* bumped up when a line is split or indented */
   int                changes;

   /* dummy entries */
   chunk_t            list_chunks;

   struct align_t     al[80];
   int                al_cnt;
   bool               al_c99_array;

   /* Here are all the settings */
   op_val_t           settings[UO_option_count];

   struct parse_frame frames[16];
   int                frame_count;
   int                pp_level;
};

extern struct cp_data cpd;

#endif   /* UNCRUSTIFY_TYPES_H_INCLUDED */
