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
   LNEWLINE  = 21,    /* newlines */
   LPF       = 22,    /* Parse Frame */
   LSTMT     = 23,    /* Marking statements/expressions */
   LTOK      = 24,    /* Tokenize */
   LALRC     = 25,    /* align right comment */
   LCMTIND   = 26,    /* Comment Indent */
   LINDLINE  = 27,    /* indent line */
   LSIB      = 28,    /* Scan IB */
   LRETURN   = 29,    /* add/remove parens for return */
   LBRDEL    = 30,    /* brace removal */
   LFCN      = 31,    /* function detection */
   LFCNP     = 32,    /* function parameters */
   LPCU      = 33,    /* parse cleanup */
   LDYNKW    = 34,    /* dynamic keywords */
   LOUTIND   = 35,    /* output indent */
   LBCSAFTER = 36,    /* Brace cleanup stack - after each token */
   LBCSPOP   = 37,    /* Brace cleanup stack - log pops */
   LBCSPUSH  = 38,    /* Brace cleanup stack - log push */
   LBCSSWAP  = 39,    /* Brace cleanup stack - log swaps */
   LFTOR     = 40,    /* Class Ctor or Dtor */
   LAS       = 41,    /* align_stack */
};

#endif   /* LOG_LEVELS_H_INCLUDED */

