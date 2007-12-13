/**
 * @file log_levels.h
 *
 * Enum for log levels.
 * Use these for the log severities in LOG_FMT(), etc.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 *
 * $Id$
 */
#ifndef LOG_LEVELS_H_INCLUDED
#define LOG_LEVELS_H_INCLUDED

enum
{
   LSYS      = 0,
   LERR      = 1,
   LWARN     = 2,
   LNOTE     = 3,
   LINFO     = 4,
   LDATA     = 5,

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
   LINDPC    = 22,    /* indent play-by-play */
   LNEWLINE  = 23,    /* newlines */
   LPF       = 24,    /* Parse Frame */
   LSTMT     = 25,    /* Marking statements/expressions */
   LTOK      = 26,    /* Tokenize */
   LALRC     = 27,    /* align right comment */
   LCMTIND   = 28,    /* Comment Indent */
   LINDLINE  = 29,    /* indent line */
   LSIB      = 30,    /* Scan IB */
   LRETURN   = 31,    /* add/remove parens for return */
   LBRDEL    = 32,    /* brace removal */
   LFCN      = 33,    /* function detection */
   LFCNP     = 34,    /* function parameters */
   LPCU      = 35,    /* parse cleanup */
   LDYNKW    = 36,    /* dynamic keywords */
   LOUTIND   = 37,    /* output indent */
   LBCSAFTER = 38,    /* Brace cleanup stack - after each token */
   LBCSPOP   = 39,    /* Brace cleanup stack - log pops */
   LBCSPUSH  = 40,    /* Brace cleanup stack - log push */
   LBCSSWAP  = 41,    /* Brace cleanup stack - log swaps */
   LFTOR     = 42,    /* Class Ctor or Dtor */
   LAS       = 43,    /* align_stack */
   LPPIS     = 44,    /* Preprocessor Indent and Space */
   LTYPEDEF  = 45,    /* Typedef and function types */
   LVARDEF   = 46,    /* Variable def marking */
   LDEFVAL   = 47,    /* define values */
   LPVSEMI   = 48,    /* Pawn: virtual semicolons */
   LPFUNC    = 49,    /* Pawn: function recognition */
   LSPLIT    = 50,    /* Line splitting */
   LFTYPE    = 51,    /* Function type detection */
   LTEMPL    = 52,    /* Template detection */
   LINDENT2  = 53,    /* indent_text tab level */
   LPARADD   = 54,    /* adding parens in if/while */
   LPARADD2  = 55,    /* adding parens in if/while - details */
   LCMTNL    = 56,    /* newlines before comments */
   LTEMPFUNC = 57,    /* Template function detection */
   LDELSEMI  = 58,    /* Removing semicolons */
   LFPARAM   = 59,    /* Testing for a full parameter */
   LNL1LINE  = 60,    /* NL check for 1 liners */
   LPFCHK    = 61,    /* Parse Frame check fcn call */
   LAVDB     = 62,    /* align var def braces */
   LSORT     = 63,    /* Sorting */
};

#endif   /* LOG_LEVELS_H_INCLUDED */
