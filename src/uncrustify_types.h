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

#include <vector>
#include <deque>
using namespace std;

#include "base_types.h"
#include "options.h"
#include "token_enum.h"    /* c_token_t */
#include "log_levels.h"
#include "logger.h"
#include "unc_text.h"
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

enum CharEncoding
{
   ENC_ASCII,     /* 0-127 */
   ENC_BYTE,      /* 0-255, not UTF-8 */
   ENC_UTF8,
   ENC_UTF16_LE,
   ENC_UTF16_BE,
};

struct chunk_t;


/**
 * Sort of like the aligning stuff, but the token indent is relative to the
 * indent of another chunk. This is needed, as that chunk may be aligned and
 * so the indent cannot be determined in the indent code.
 */
struct indent_ptr_t
{
   chunk_t *ref;
   int     delta;
};


/**
 * Structure for counting nested level
 */
struct paren_stack_entry
{
   c_token_t    type;         /**< the type that opened the entry */
   int          level;        /**< Level of opening type */
   int          open_line;    /**< line that open symbol is on */
   chunk_t      *pc;          /**< Chunk that opened the level */
   int          brace_indent; /**< indent for braces - may not relate to indent */
   int          indent;       /**< indent level (depends on use) */
   int          indent_tmp;   /**< temporary indent level (depends on use) */
   int          indent_tab;   /**< the 'tab' indent (always <= real column) */
   bool         indent_cont;  /**< indent_continue was applied */
   int          ref;
   c_token_t    parent;       /**< if, for, function, etc */
   brstage_e    stage;
   bool         in_preproc;   /**< whether this was created in a preprocessor */
   int          ns_cnt;
   bool         non_vardef;   /**< Hit a non-vardef line */
   indent_ptr_t ip;
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

#define PCF_BIT(b)   (1ULL << b)

/* Copy flags are in the lower 16 bits */
#define PCF_COPY_FLAGS         0x0000ffff
#define PCF_IN_PREPROC         PCF_BIT(0)  /* in a preprocessor */
#define PCF_IN_STRUCT          PCF_BIT(1)  /* in a struct */
#define PCF_IN_ENUM            PCF_BIT(2)  /* in enum */
#define PCF_IN_FCN_DEF         PCF_BIT(3)  /* inside function def parens */
#define PCF_IN_FCN_CALL        PCF_BIT(4)  /* inside function call parens */
#define PCF_IN_SPAREN          PCF_BIT(5)  /* inside for/if/while/switch parens */
#define PCF_IN_TEMPLATE        PCF_BIT(6)
#define PCF_IN_TYPEDEF         PCF_BIT(7)
#define PCF_IN_CONST_ARGS      PCF_BIT(8)
#define PCF_IN_ARRAY_ASSIGN    PCF_BIT(9)
#define PCF_IN_CLASS           PCF_BIT(10)
#define PCF_IN_CLASS_BASE      PCF_BIT(11)
#define PCF_IN_NAMESPACE       PCF_BIT(12)
#define PCF_IN_FOR             PCF_BIT(13)
#define PCF_IN_OC_MSG          PCF_BIT(14)

/* Non-Copy flags are in the upper 48 bits */
#define PCF_FORCE_SPACE        PCF_BIT(16)  /* must have a space after this token */
#define PCF_STMT_START         PCF_BIT(17)  /* marks the start of a statement */
#define PCF_EXPR_START         PCF_BIT(18)
#define PCF_DONT_INDENT        PCF_BIT(19)  /* already aligned! */
#define PCF_ALIGN_START        PCF_BIT(20)
#define PCF_WAS_ALIGNED        PCF_BIT(21)
#define PCF_VAR_TYPE           PCF_BIT(22)  /* part of a variable def type */
#define PCF_VAR_DEF            PCF_BIT(23)  /* variable name in a variable def */
#define PCF_VAR_1ST            PCF_BIT(24)  /* 1st variable def in a statement */
#define PCF_VAR_1ST_DEF        (PCF_VAR_DEF | PCF_VAR_1ST)
#define PCF_VAR_INLINE         PCF_BIT(25)  /* type was an inline struct/enum/union */
#define PCF_RIGHT_COMMENT      PCF_BIT(26)
#define PCF_OLD_FCN_PARAMS     PCF_BIT(27)
#define PCF_LVALUE             PCF_BIT(28) /* left of assignment */
#define PCF_ONE_LINER          PCF_BIT(29)
#define PCF_ONE_CLASS          (PCF_ONE_LINER | PCF_IN_CLASS)
#define PCF_EMPTY_BODY         PCF_BIT(30)
#define PCF_ANCHOR             PCF_BIT(31)  /* aligning anchor */
#define PCF_PUNCTUATOR         PCF_BIT(32)
#define PCF_INSERTED           PCF_BIT(33)  /* chunk was inserted from another file */
#define PCF_LONG_BLOCK         PCF_BIT(34)  /* the block is 'long' by some measure */
#define PCF_OC_BOXED           PCF_BIT(35)  /* inside OC boxed expression */
#define PCF_KEEP_BRACE         PCF_BIT(36)  /* do not remove brace */
#define PCF_OC_RTYPE           PCF_BIT(37)  /* inside OC return type */
#define PCF_OC_ATYPE           PCF_BIT(38)  /* inside OC arg type */

#ifdef DEFINE_PCF_NAMES
static const char *pcf_names[] =
{
   "IN_PREPROC",        // 0
   "IN_STRUCT",         // 1
   "IN_ENUM",           // 2
   "IN_FCN_DEF",        // 3
   "IN_FCN_CALL",       // 4
   "IN_SPAREN",         // 5
   "IN_TEMPLATE",       // 6
   "IN_TYPEDEF",        // 7
   "IN_CONST_ARGS",     // 8
   "IN_ARRAY_ASSIGN",   // 9
   "IN_CLASS",          // 10
   "IN_CLASS_BASE",     // 11
   "IN_NAMESPACE",      // 12
   "IN_FOR",            // 13
   "IN_OC_MSG",         // 14
   "#15",               // 15
   "FORCE_SPACE",       // 16
   "STMT_START",        // 17
   "EXPR_START",        // 18
   "DONT_INDENT",       // 19
   "ALIGN_START",       // 20
   "WAS_ALIGNED",       // 21
   "VAR_TYPE",          // 22
   "VAR_DEF",           // 23
   "VAR_1ST",           // 24
   "VAR_INLINE",        // 25
   "RIGHT_COMMENT",     // 26
   "OLD_FCN_PARAMS",    // 27
   "LVALUE",            // 28
   "ONE_LINER",         // 29
   "EMPTY_BODY",        // 30
   "ANCHOR",            // 31
   "PUNCTUATOR",        // 32
   "INSERTED",          // 33
   "LONG_BLOCK",        // 34
   "OC_BOXED",          // 35
   "KEEP_BRACE",        // 36
   "OC_RTYPE",          // 37
   "OC_ATYPE",          // 38
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
   chunk_t()
   {
      reset();
   }
   void reset()
   {
      memset(&align, 0, sizeof(align));
      memset(&indent, 0, sizeof(indent));
      next = 0;
      prev = 0;
      type = CT_NONE;
      parent_type = CT_NONE;
      orig_line = 0;
      orig_col = 0;
      orig_col_end = 0;
      orig_prev_sp = 0;
      flags = 0;
      column = 0;
      column_indent = 0;
      nl_count = 0;
      level = 0;
      brace_level = 0;
      pp_level = 0;
      after_tab = false;
      str.clear();
   }
   int len()
   {
      return str.size();
   }
   const char *text()
   {
      return str.c_str();
   }

   chunk_t      *next;
   chunk_t      *prev;
   align_ptr_t  align;
   indent_ptr_t indent;
   c_token_t    type;
   c_token_t    parent_type;      /* usually CT_NONE */
   UINT32       orig_line;
   UINT32       orig_col;
   UINT32       orig_col_end;
   UINT32       orig_prev_sp;     /* whitespace before this token */
   UINT64       flags;            /* see PCF_xxx */
   int          column;           /* column of chunk */
   int          column_indent;    /* if 1st on a line, set to the 'indent'
                                   * column, which may be less than the real column
                                   * used to indent with tabs */
   int          nl_count;         /* number of newlines in CT_NEWLINE */
   int          level;            /* nest level in {, (, or [ */
   int          brace_level;      /* nest level in braces only */
   int          pp_level;         /* nest level in #if stuff */
   bool         after_tab;        /* whether this token was after a tab */
   unc_text     str;              /* the token text */
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

struct chunk_tag_t
{
   const char *tag;
   c_token_t  type;
   int        lang_flags;
};

struct lookup_entry_t
{
   char              ch;
   char              left_in_group;
   UINT16            next_idx;
   const chunk_tag_t *tag;
};

struct align_t
{
   int       col;
   c_token_t type;
   int       len;    // of the token + space
};

struct file_mem
{
   vector<UINT8>  raw;
   deque<int>     data;
   bool           bom;
   CharEncoding   enc;
#ifdef HAVE_UTIME_H
   struct utimbuf utb;
#endif
};

struct cp_data
{
   deque<UINT8>       *bout;
   FILE               *fout;
   int                last_char;
   bool               do_check;
   int                check_fail_cnt; // total failures

   UINT32             error_count;
   const char         *filename;

   file_mem           file_hdr;   /* for cmt_insert_file_header */
   file_mem           file_ftr;   /* for cmt_insert_file_footer */
   file_mem           func_hdr;   /* for cmt_insert_func_header */
   file_mem           oc_msg_hdr; /* for cmt_insert_oc_msg_header */
   file_mem           class_hdr;  /* for cmt_insert_class_header */

   int                lang_flags; // LANG_xxx
   bool               lang_forced;

   bool               unc_off;
   UINT32             line_number;
   UINT16             column;  /* column for parsing */
   UINT16             spaces;  /* space count on output */

   int                ifdef_over_whole_file;

   bool               frag;
   UINT16             frag_cols;

   /* stuff to auto-detect line endings */
   UINT32             le_counts[LE_AUTO];
   unc_text           newline;

   bool               consumed;

   int                did_newline;
   c_token_t          in_preproc;
   int                preproc_ncnl_count;
   bool               output_trailspace;
   bool               output_tab_as_space;

   bool               bom;
   CharEncoding       enc;

   /* bumped up when a line is split or indented */
   int                changes;
   int                pass_count;

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
