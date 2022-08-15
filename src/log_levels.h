/**
 * @file log_levels.h
 *
 * Enum for log levels.
 * Use these for the log severities in LOG_FMT(), etc.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel October 2015- 2021
 * @license GPL v2+
 */

#ifndef LOG_LEVELS_H_INCLUDED
#define LOG_LEVELS_H_INCLUDED

/**
 * list of available log levels
 *
 * The user defines which log level is active using the
 * -L or -log option.
 * use -L A to set all the levels
 * All messages which have a level that is active will be stored to the log
 * file.
 * All other log messages will be discarded.
 * Different parts of the software use different log levels.
 * This allows to log only operations of a specific operation.
 * This eases debugging.
 * To get all log messages use the option -La
 * By default only system messages (level=LSYS) are logged.
 */
enum log_sev_t
{
   LSYS      = 0,   //! system messages
   LERR      = 1,   //! error messages
   LWARN     = 2,   //! warnings
   LNOTE     = 3,   //! user notifications
   LINFO     = 4,   //! user information
   LDATA     = 5,   //! data logging

   LFILELIST = 8,   //! Files in the file list file
   LLINEENDS = 9,   //! Show which line endings are used
   LCASTS    = 10,  //! align casts
   LALBR     = 11,  //! align braces
   LALTD     = 12,  //! Align Typedef
   LALPP     = 13,  //! align #define
   LALPROTO  = 14,  //! align prototype
   LALNLC    = 15,  //! align backslash-newline
   LALTC     = 16,  //! align trailing comments
   LALADD    = 17,  //! align add
   LALASS    = 18,  //! align assign
   LFVD      = 19,  //! fix_var_def
   LFVD2     = 20,  //! fix_var_def-2
   LINDENT   = 21,  //! indent_text
   LINDENT2  = 22,  //! indent_text tab level
   LINDPSE   = 23,  //! indent_text stack
   LINDPC    = 24,  //! indent play-by-play
   LNEWLINE  = 25,  //! newlines
   LPF       = 26,  //! Parse Frame
   LSTMT     = 27,  //! Marking statements/expressions
   LTOK      = 28,  //! Tokenize
   LALRC     = 29,  //! align right comment
   LCMTIND   = 30,  //! Comment Indent
   LINDLINE  = 31,  //! indent line
   LSIB      = 32,  //! Scan IB
   LRETURN   = 33,  //! add/remove parens for return or throw
   LBRDEL    = 34,  //! brace removal
   LFCN      = 35,  //! function detection
   LFCNP     = 36,  //! function parameters
   LPCU      = 37,  //! parse cleanup
   LDYNKW    = 38,  //! dynamic keywords
   LOUTIND   = 39,  //! output indent
   LBCSAFTER = 40,  //! Brace cleanup stack - after each token
   LBCSPOP   = 41,  //! Brace cleanup stack - log pops
   LBCSPUSH  = 42,  //! Brace cleanup stack - log push
   LBCSSWAP  = 43,  //! Brace cleanup stack - log swaps
   LFTOR     = 44,  //! Class Ctor or Dtor
   LAS       = 45,  //! align_stack
   LPPIS     = 46,  //! Preprocessor Indent and Space
   LTYPEDEF  = 47,  //! Typedef and function types
   LVARDEF   = 48,  //! Variable def marking
   LDEFVAL   = 49,  //! define values
   LPVSEMI   = 50,  //! Pawn: virtual semicolons
   LPFUNC    = 51,  //! Pawn: function recognition
   LSPLIT    = 52,  //! Line splitting
   LFTYPE    = 53,  //! Function type detection
   LTEMPL    = 54,  //! Template detection
   LPARADD   = 55,  //! adding parens in if/while
   LPARADD2  = 56,  //! adding parens in if/while - details
   LBLANKD   = 57,  //! blank line details
   LTEMPFUNC = 58,  //! Template function detection
   LSCANSEMI = 59,  //! scan semicolon removal
   LDELSEMI  = 60,  //! Removing semicolons
   LFPARAM   = 61,  //! Testing for a full parameter
   LNL1LINE  = 62,  //! NL check for 1 liners
   LPFCHK    = 63,  //! Parse Frame check function call
   LAVDB     = 64,  //! align var def braces
   LSORT     = 65,  //! Sorting
   LSPACE    = 66,  //! Space
   LALIGN    = 67,  //! align
   LALAGAIN  = 68,  //! align again
   LOPERATOR = 69,  //! operator
   LASFCP    = 70,  //! Align Same Function Call Params
   LINDLINED = 71,  //! indent line details
   LBCTRL    = 72,  //! beautifier control
   LRMRETURN = 73,  //! remove 'return;'
   LPPIF     = 74,  //! #if/#else/#endif pair processing
   LMCB      = 75,  //! mod_case_brace
   LBRCH     = 76,  //! if brace chain
   LFCNR     = 77,  //! function return type
   LOCCLASS  = 78,  //! OC Class stuff
   LOCMSG    = 79,  //! OC Message stuff
   LBLANK    = 80,  //! Blank Lines
   LOBJCWORD = 81,  //! Convert keyword to CT_WORD in certain circumstances
   LCHANGE   = 82,  //! something changed
   LCONTTEXT = 83,  //! comment cont_text set
   LANNOT    = 84,  //! Java annotation
   LOCBLK    = 85,  //! OC Block stuff
   LFLPAREN  = 86,  //! Flag paren
   LOCMSGD   = 87,  //! OC Message declaration
   LINDENTAG = 88,  //! indent again
   LNFD      = 89,  //! newline-function-def
   LJDBI     = 90,  //! Java Double Brace Init
   LSETPAR   = 91,  //! Chunk::SetParentTypeReal()
   LSETTYP   = 92,  //! Chunk::SetTypeReal()
   LSETFLG   = 93,  //! set_chunk_flags()
   LNLFUNCT  = 94,  //! newlines before function
   LCHUNK    = 95,  //! Add or delete chunk
   LBC       = 96,  //! brace cleanup
   LCOMBINE  = 97,  //! combine
   LGUY98    = 98,  //! for guy-test
   LGUY      = 99,  //! for guy-test
   LBR       = 100, //! braces
   LOUTPUT   = 101, //! output
   LUNC      = 102, //! rules used in uncrustify.cpp
   LQT       = 103, //! load/save options for Qt
   LVARDFBLK = 104, //! newlines for variable definition blocks
   LOTHER    = 255, //! stuff that doesn't neatly fit any other category
};

#endif /* LOG_LEVELS_H_INCLUDED */
