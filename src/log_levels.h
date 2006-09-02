/**
 * @file log_levels.h
 *
 * Enum for log levels.
 * Use these for the log severities in LOG_FMT(), etc.
 *
 * $Id$
 */
#ifndef LOG_LEVELS_H_INCLUDED
#define LOG_LEVELS_H_INCLUDED

enum
{
   LSYS  = 0,
   LERR  = 1,
   LWARN = 2,
   LNOTE = 3,
   LINFO = 4,
   LDATA = 5,

   LFILELIST = 8,     /* Files in the file list file */
   LLINEENDS = 9,     /* Show which line endings are used */
   LCASTS    = 10,    /* align casts */
   LALBR     = 11,    /* align braces */
   LALTD     = 12,    /* Align Typedef */
   LALPP     = 13,    /* align #define */
   LALPROTO  = 14,    /* align prototype */
   LALNLC    = 15,    /* align backslash-newline */
   LALTC     = 16,    /* align trailing comments */
   LALADD    = 17,    /* align add */
   LALASS    = 18,    /* align assign */
   LFVD      = 19,    /* fix_var_def */
   LINDENT   = 20,    /* indent_text */
   LINDPSE   = 21,    /* indent_text stack */
   LNEWLINE  = 22,    /* newlines */
   LPF       = 23,    /* Parse Frame */
   LSTMT     = 24,    /* Marking statements/expressions */
   LTOK      = 25,    /* Tokenize */
   LALRC     = 26,    /* align right comment */
   LCMTIND   = 27,    /* Comment Indent */
   LINDLINE  = 28,    /* indent line */
   LSIB      = 29,    /* Scan IB */
   LRETURN   = 30,    /* add/remove parens for return */
   LBRDEL    = 31,    /* brace removal */
   LFCN      = 32,    /* function detection */
   LFCNP     = 33,    /* function parameters */
   LPCU      = 34,    /* parse cleanup */
   LDYNKW    = 35,    /* dynamic keywords */
   LOUTIND   = 36,    /* output indent */
   LBCSAFTER = 37,    /* Brace cleanup stack - after each token */
   LBCSPOP   = 38,    /* Brace cleanup stack - log pops */
   LBCSPUSH  = 39,    /* Brace cleanup stack - log push */
   LBCSSWAP  = 40,    /* Brace cleanup stack - log swaps */
   LFTOR     = 41,    /* Class Ctor or Dtor */
   LAS       = 42,    /* align_stack */
   LPPIS     = 43,    /* Preprocessor Indent and Space */
   LTYPEDEF  = 44,    /* Typedef and function types */
   LVARDEF   = 45,    /* Variable def marking */
   LDEFVAL   = 46,    /* define values */
   LPVSEMI   = 47,    /* Pawn: virtual semicolons */
   LPFUNC    = 48,    /* Pawn: fucntion recognition */
   LSPLIT    = 49,    /* Line splitting */
   LFTYPE    = 50,    /* Function type detection */
   LTEMPL    = 51,    /* Template detection */
};

#endif   /* LOG_LEVELS_H_INCLUDED */
