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
   LFVD2     = 20,    /* fix_var_def-2 */
   LINDENT   = 21,    /* indent_text */
   LINDENT2  = 22,    /* indent_text tab level */
   LINDPSE   = 23,    /* indent_text stack */
   LINDPC    = 24,    /* indent play-by-play */
   LNEWLINE  = 25,    /* newlines */
   LPF       = 26,    /* Parse Frame */
   LSTMT     = 27,    /* Marking statements/expressions */
   LTOK      = 28,    /* Tokenize */
   LALRC     = 29,    /* align right comment */
   LCMTIND   = 30,    /* Comment Indent */
   LINDLINE  = 31,    /* indent line */
   LSIB      = 32,    /* Scan IB */
   LRETURN   = 33,    /* add/remove parens for return */
   LBRDEL    = 34,    /* brace removal */
   LFCN      = 35,    /* function detection */
   LFCNP     = 36,    /* function parameters */
   LPCU      = 37,    /* parse cleanup */
   LDYNKW    = 38,    /* dynamic keywords */
   LOUTIND   = 39,    /* output indent */
   LBCSAFTER = 40,    /* Brace cleanup stack - after each token */
   LBCSPOP   = 41,    /* Brace cleanup stack - log pops */
   LBCSPUSH  = 42,    /* Brace cleanup stack - log push */
   LBCSSWAP  = 43,    /* Brace cleanup stack - log swaps */
   LFTOR     = 44,    /* Class Ctor or Dtor */
   LAS       = 45,    /* align_stack */
   LPPIS     = 46,    /* Preprocessor Indent and Space */
   LTYPEDEF  = 47,    /* Typedef and function types */
   LVARDEF   = 48,    /* Variable def marking */
   LDEFVAL   = 49,    /* define values */
   LPVSEMI   = 50,    /* Pawn: virtual semicolons */
   LPFUNC    = 51,    /* Pawn: function recognition */
   LSPLIT    = 52,    /* Line splitting */
   LFTYPE    = 53,    /* Function type detection */
   LTEMPL    = 54,    /* Template detection */
   LPARADD   = 55,    /* adding parens in if/while */
   LPARADD2  = 56,    /* adding parens in if/while - details */
   LCMTNL    = 57,    /* newlines before comments */
   LTEMPFUNC = 58,    /* Template function detection */
   LDELSEMI  = 59,    /* Removing semicolons */
   LFPARAM   = 60,    /* Testing for a full parameter */
   LNL1LINE  = 61,    /* NL check for 1 liners */
   LPFCHK    = 62,    /* Parse Frame check fcn call */
   LAVDB     = 63,    /* align var def braces */
   LSORT     = 64,    /* Sorting */
   LSPACE    = 65,    /* Space */
   LALIGN    = 66,    /* align */
   LALAGAIN  = 67,    /* align again */
   LOPERATOR = 68,    /* operator */
   LASFCP    = 69,    /* Align Same Function Call Params */
   LINDLINED = 70,    /* indent line details */
};

#endif   /* LOG_LEVELS_H_INCLUDED */
