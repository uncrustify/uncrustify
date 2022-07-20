   enum
     class
       angle_state_e 
  : 
 unsigned
int {
      NONE=0, OPEN=1, //'<' found
CLOSE            =    2   ,         //'>' found
};

// align.cpp
enum class comment_align_e : unsigned int
{
   REGULAR,
   BRACE,
   ENDIF,
};

// chunk.h
enum class E_Scope : unsigned int
{
   ALL,      /**< search in all kind of chunks */
   PREPROC,  /**< search only in preprocessor chunks */
};

// chunk.cpp
enum class E_Direction : unsigned int
{
   FORWARD,
   BACKWARD
};

// combine.cpp
{
   enum class angle_state_e : unsigned int
   {
      NONE  = 0,
      OPEN  = 1, // '<' found
      CLOSE = 2, // '>' found
   };
}

// indent.cpp
enum class align_mode_e : unsigned int
{
   SHIFT,     /* shift relative to the current column */
   KEEP_ABS,  /* try to keep the original absolute column */
   KEEP_REL,  /* try to keep the original gap */
};

// align_stack.h
{
   enum StarStyle
   {
      SS_IGNORE,  // don't look for prev stars
      SS_INCLUDE, // include prev * before add
      SS_DANGLE   // include prev * after add
   };
}

// log_levels.h
enum log_sev_t
{
   LSYS      = 0,
   LERR      = 1,
   LWARN     = 2,
   LNOTE     = 3,
   LINFO     = 4,
   LDATA     = 5,

   LFILELIST = 8,  /* Files in the file list file */
   LLINEENDS = 9,  /* Show which line endings are used */
   LCASTS    = 10, /* align casts */
   LALBR     = 11, /* align braces */
   LALTD     = 12, /* Align Typedef */
   LALPP     = 13, /* align #define */
   LALPROTO  = 14, /* align prototype */
   LALNLC    = 15, /* align backslash-newline */
   LALTC     = 16, /* align trailing comments */
   LALADD    = 17, /* align add */
   LALASS    = 18, /* align assign */
   LFVD      = 19, /* fix_var_def */
   LFVD2     = 20, /* fix_var_def-2 */
   LINDENT   = 21, /* indent_text */
   LINDENT2  = 22, /* indent_text tab level */
   LINDPSE   = 23, /* indent_text stack */
   LINDPC    = 24, /* indent play-by-play */
   LNEWLINE  = 25, /* newlines */
   LPF       = 26, /* Parse Frame */
   LSTMT     = 27, /* Marking statements/expressions */
   LTOK      = 28, /* Tokenize */
   LALRC     = 29, /* align right comment */
   LCMTIND   = 30, /* Comment Indent */
   LINDLINE  = 31, /* indent line */
   LSIB      = 32, /* Scan IB */
   LRETURN   = 33, /* add/remove parens for return */
   LBRDEL    = 34, /* brace removal */
   LFCN      = 35, /* function detection */
   LFCNP     = 36, /* function parameters */
   LPCU      = 37, /* parse cleanup */
   LDYNKW    = 38, /* dynamic keywords */
   LOUTIND   = 39, /* output indent */
   LBCSAFTER = 40, /* Brace cleanup stack - after each token */
   LBCSPOP   = 41, /* Brace cleanup stack - log pops */
   LBCSPUSH  = 42, /* Brace cleanup stack - log push */
   LBCSSWAP  = 43, /* Brace cleanup stack - log swaps */
   LFTOR     = 44, /* Class Ctor or Dtor */
   LAS       = 45, /* align_stack */
   LPPIS     = 46, /* Preprocessor Indent and Space */
   LTYPEDEF  = 47, /* Typedef and function types */
   LVARDEF   = 48, /* Variable def marking */
   LDEFVAL   = 49, /* define values */
   LPVSEMI   = 50, /* Pawn: virtual semicolons */
   LPFUNC    = 51, /* Pawn: function recognition */
   LSPLIT    = 52, /* Line splitting */
   LFTYPE    = 53, /* Function type detection */
   LTEMPL    = 54, /* Template detection */
   LPARADD   = 55, /* adding parens in if/while */
   LPARADD2  = 56, /* adding parens in if/while - details */
   LBLANKD   = 57, /* blank line details */
   LTEMPFUNC = 58, /* Template function detection */
   LSCANSEMI = 59, /* scan semi colon removal */
   LDELSEMI  = 60, /* Removing semicolons */
   LFPARAM   = 61, /* Testing for a full parameter */
   LNL1LINE  = 62, /* NL check for 1 liners */
   LPFCHK    = 63, /* Parse Frame check fcn call */
   LAVDB     = 64, /* align var def braces */
   LSORT     = 65, /* Sorting */
   LSPACE    = 66, /* Space */
   LALIGN    = 67, /* align */
   LALAGAIN  = 68, /* align again */
   LOPERATOR = 69, /* operator */
   LASFCP    = 70, /* Align Same Function Call Params */
   LINDLINED = 71, /* indent line details */
   LBCTRL    = 72, /* beautifier control */
   LRMRETURN = 73, /* remove 'return;' */
   LPPIF     = 74, /* #if/#else/#endif pair processing */
   LMCB      = 75, /* mod_case_brace */
   LBRCH     = 76, /* if brace chain */
   LFCNR     = 77, /* function return type */
   LOCCLASS  = 78, /* OC Class stuff */
   LOCMSG    = 79, /* OC Message stuff */
   LBLANK    = 80, /* Blank Lines */
   LOBJCWORD = 81, /* Convert keyword to CT_WORD in certain circumstances */
   LCHANGE   = 82, /* something changed */
   LCONTTEXT = 83, /* comment cont_text set */
   LANNOT    = 84, /* Java annotation */
   LOCBLK    = 85, /* OC Block stuff */
   LFLPAREN  = 86, /* Flag paren */
   LOCMSGD   = 87, /* OC Message declaration */
   LINDENTAG = 88, /* indent again */
   LNFD      = 89, /* newline-function-def */
   LJDBI     = 90, /* Java Double Brace Init */
   LSETPAR   = 91, /* Chunk::SetParentTypeReal() */
   LSETTYP   = 92, /* Chunk::SetTypeReal() */
   LSETFLG   = 93, /* set_chunk_flags() */
   LNLFUNCT  = 94, /* newlines before function */
   LCHUNK    = 95, /* Add or del chunk */
   LGUY98    = 98, /* for guy-test */
   LGUY      = 99, /* for guy-test */
};

// options.h
enum argtype_e
{
   AT_BOOL,    /**< true / false */
   AT_IARF,    /**< Ignore / Add / Remove / Force */
   AT_NUM,     /**< Number */
   AT_LINE,    /**< Line Endings */
   AT_POS,     /**< start/end or Trail/Lead */
   AT_STRING,  /**< string value */
   AT_UNUM,    /**< unsigned Number */
};

enum argval_t
{
   AV_IGNORE      = 0,
   AV_ADD         = 1,
   AV_REMOVE      = 2,
   AV_FORCE       = 3, /**< remove + add */
   AV_NOT_DEFINED = 4  /* to be used with QT, SIGNAL SLOT macros */
};

enum lineends_e
{
   LE_LF,      /* "\n"   */
   LE_CRLF,    /* "\r\n" */
   LE_CR,      /* "\r"   */

   LE_AUTO,    /* keep last */
};

enum tokenpos_e
{
   TP_IGNORE      = 0,     /* don't change it */
   TP_BREAK       = 1,     /* add a newline before or after the if not present */
   TP_FORCE       = 2,     /* force a newline on one side and not the other */
   TP_LEAD        = 4,     /* at the start of a line or leading if wrapped line */
   TP_LEAD_BREAK  = (TP_LEAD | TP_BREAK),
   TP_LEAD_FORCE  = (TP_LEAD | TP_FORCE),
   TP_TRAIL       = 8,     /* at the end of a line or trailing if wrapped line */
   TP_TRAIL_BREAK = (TP_TRAIL | TP_BREAK),
   TP_TRAIL_FORCE = (TP_TRAIL | TP_FORCE),
   TP_JOIN        = 16,    /* remove newlines on both sides */
};


