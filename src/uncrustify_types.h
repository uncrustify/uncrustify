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
#include <cstdio>
#include "token_enum.h"    /* c_token_t */
#include "log_levels.h"
#include "logger.h"


/**
 * Brace stage enum used in brace_cleanup
 */
typedef enum
{
   BS_NONE,
   BS_PAREN1,   /* if/for/switch/while */
   BS_PAREN2,   /* while of do */
   BS_BRACE_DO, /* do */
   BS_BRACE2,   /* if/else/for/switch/while */
   BS_ELSE,     /* expecting 'else' after 'if' */
   BS_ELSEIF,   /* expecting 'if' after 'else' */
   BS_WHILE,    /* expecting 'while' after 'do' */
} brstage_t;

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

   struct paren_stack_entry pse[64];
   int                      pse_tos;
   int                      paren_count;

   int                      in_ifdef;
   int                      stmt_count;
   int                      expr_count;

   bool                     maybe_decl;
   bool                     maybe_cast;
};

#define PCF_STMT_START         0x01  /* marks the start of a statment */
#define PCF_EXPR_START         0x02
#define PCF_IN_PREPROC         0x04  /* in a preprocessor */

#define PCF_MFC_PAREN          0x10  /* macro function close paren */
#define PCF_VAR_DEF            0x20  /* variable name in a variable def */
#define PCF_VAR_1ST            0x40  /* 1st variable def in a statement */
#define PCF_VAR_1ST_DEF        (PCF_VAR_DEF | PCF_VAR_1ST)
#define PCF_VAR_INLINE         0x80  /* type was an inline struct/enum/union */
#define PCF_DEF_ALIGNED        0x100
#define PCF_IN_FCN_DEF         0x200 /* inside function def parens */
#define PCF_IN_FCN_CALL        0x400 /* inside function call parens */
#define PCF_IN_SPAREN          0x800 /* inside for/if/while/switch parens */
#define PCF_RIGHT_COMMENT      0x1000
#define PCF_OLD_FCN_PARAMS     0x2000
#define PCF_WAS_ALIGNED        0x4000
#define PCF_OPTIONAL           0x8000

/* flags that get copied when a new chunk is inserted */
#define PCF_COPY_FLAGS         (PCF_IN_PREPROC | PCF_IN_SPAREN | \
                                PCF_IN_FCN_DEF | PCF_IN_FCN_CALL)

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
   bool       after_tab;
   const char *str;
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
   const char *tag;
   c_token_t  type;
   uint8_t    lang_flags;
} chunk_tag_t;

//enum pp_type
//{
//   PP_NONE,
//   PP_UNKNOWN,
//   PP_OTHER,
//   PP_INCLUDE,
//   PP_DEFINE,
//   PP_DEFINE_BODY,
//   PP_IF,
//   PP_ELSE,
//   PP_ENDIF
//};

struct align_t
{
   int       col;
   c_token_t type;
   int       len;
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

   uint8_t            lang_flags; // LANG_xxx

   uint16_t           line_number;
   uint16_t           column;  /* column for parsing */

   bool               consumed;

   int                did_newline;
   //enum pp_type       in_preproc;
   c_token_t          in_preproc;
   int                preproc_ncnl_count;

   /* dummy entries */
   chunk_t            list_chunks;

   struct align_t     al[80];
   int                al_cnt;

   /* Here are all the settings */
   op_val_t           settings[UO_option_count];

   struct parse_frame frames[16];
   int                frame_count;

   /* a very simple chunk stack - managed in chunk_list.c/h */
   //chunk_stack_t      cs;
   //int                cs_len;    /* active entries */
   //int                cs_size;   /* total entry count (private) */
};

extern struct cp_data cpd;

#endif   /* UNCRUSTIFY_TYPES_H_INCLUDED */

