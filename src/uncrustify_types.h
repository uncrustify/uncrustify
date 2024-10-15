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

#include "options.h"
#include "pcf_flags.h"
#include "token_enum.h"    // E_Token
#include "unc_text.h"
#include "uncrustify_limits.h"

#include <assert.h>
#include <cstddef>      // to get size_t

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif


/**
 * abbreviations used:
 *   SS = star style
 */

/**
 * special strings to mark a part of the input file where
 * uncrustify shall not change anything
 */
#define UNCRUSTIFY_OFF_TEXT    " *INDENT-OFF*"
#define UNCRUSTIFY_ON_TEXT     " *INDENT-ON*"

/**
 * @brief Macro to inform the compiler that a variable is intentionally
 * not in use.
 *
 * @param [in] variableName: The unused variable.
 */
#define UNUSED(variableName)    ((void)variableName)


//! Brace stage enum used in brace_cleanup
enum class E_BraceStage : unsigned int
{
   NONE,
   PAREN1,      //! expected paren after if/catch (C++)/for/switch/synchronized/while
   OP_PAREN1,   //! optional paren after catch (C#)
   WOD_PAREN,   //! while of do parens
   WOD_SEMI,    //! semicolon after while of do
   BRACE_DO,    //! do
   BRACE2,      //! if/catch/else/finally/for/switch/synchronized/while
   ELSE,        //! expecting 'else' after 'if'
   ELSEIF,      //! expecting 'if' after 'else'
   WHILE,       //! expecting 'while' after 'do'
   CATCH,       //! expecting 'catch' or 'finally' after 'try'
   CATCH_WHEN,  //! optional 'when' after 'catch'
};


enum class char_encoding_e : unsigned int
{
   e_ASCII,     //! 0-127
   e_BYTE,      //! 0-255, not UTF-8
   e_UTF8,      //! utf 8 bit wide
   e_UTF16_LE,  //! utf 16 bit wide, little endian
   e_UTF16_BE   //! utf 16 bit wide, big endian
};

enum class tracking_type_e : unsigned int
{
   TT_NONE,
   TT_SPACE,
   TT_NEWLINE,
   TT_START
};

class Chunk; //forward declaration


/**
 * Sort of like the aligning stuff, but the token indent is relative to the
 * indent of another chunk. This is needed, as that chunk may be aligned and
 * so the indent cannot be determined in the indent code.
 */
struct IndentationData
{
   Chunk *ref;
   int   delta;
};


struct AlignmentData
{
   Chunk  *next;       //! Chunk::NullChunkPtr or the chunk that should be under this one
   bool   right_align; //! AlignStack.m_right_align
   size_t star_style;  //! AlignStack.m_star_style
   size_t amp_style;   //! AlignStack.m_amp_style
   int    gap;         //! AlignStack.m_gap

   /*
    * col_adj is the amount to alter the column for the token.
    * For example, a dangling '*' would be set to -1.
    * A right-aligned word would be a positive value.
    */
   int   col_adj;
   Chunk *ref;
   Chunk *start;
};


// for debugging purpose only
typedef std::pair<size_t, char *>   TrackNumber;   // track for "trackNumber" and "rule"
typedef std::vector<TrackNumber>    TrackList;     // list of tracks

////! list of all programming languages Uncrustify supports
//enum lang_flag_e
//{
//   LANG_C    = 0x0001,
//   LANG_CPP  = 0x0002,
//   LANG_D    = 0x0004,
//   LANG_CS   = 0x0008,     //! C# (C-Sharp)
//   LANG_JAVA = 0x0010,
//   LANG_OC   = 0x0020,     //! Objective-C
//   LANG_VALA = 0x0040,
//   LANG_PAWN = 0x0080,
//   LANG_ECMA = 0x0100,     //! ECMA Script (JavaScript)
//
//   LANG_ALLC = 0x017f,     /** LANG_C    | LANG_CPP | LANG_D    | LANG_CS   |
//                            *  LANG_JAVA | LANG_OC  | LANG_VALA | LANG_ECMA   */
//   LANG_ALL  = 0x0fff,     //! applies to all languages
//
//   FLAG_HDR  = 0x2000,     /*<< Header file for C family languages */
//   FLAG_DIG  = 0x4000,     //! digraph/trigraph
//   FLAG_PP   = 0x8000,     //! only appears in a preprocessor
//};

//! Pattern classes for special keywords
enum class pattern_class_e : unsigned int
{
   NONE,
   BRACED,   /** keyword + braced statement:
              *    do, try, finally, body, unittest, unsafe, volatile
              *    add, get, remove, set                                      */
   PBRACED,  /** keyword + parens + braced statement:
              *    if, elseif, switch, for, while, synchronized,
              *    using, lock, with, version, CT_D_SCOPE_IF                  */
   OPBRACED, /** keyword + optional parens + braced statement:
              *    catch, version, debug                                      */
   VBRACED,  /** keyword + value + braced statement:
              *    namespace                                                  */
   PAREN,    /** keyword + parens:
              *    while-of-do                                                */
   OPPAREN,  /** keyword + optional parens:
              *    invariant (D lang)                                         */
   ELSE,     /** Special case of pattern_class_e::BRACED for handling CT_IF
              *    else                                                       */
};

//! used to link language keywords with some addition information
struct chunk_tag_t
{
   const char *tag;       //! name of the keyword e.g. "bool"
   E_Token    type;       //! uncrustify type assigned to that keyword
   size_t     lang_flags; //! programming language that uses this keyword
};


struct align_t
{
   size_t  col;
   E_Token type;
   size_t  len;      //! length of the token + space
   Chunk   *ref;     // Issue #3786
};

//! holds information and data of a file
struct file_mem
{
   std::vector<UINT8> raw;  //! raw content of file
   std::deque<int>    data; //! processed content of file
   bool               bom;
   char_encoding_e    enc;  //! character encoding of file ASCII, utf, etc.
#ifdef HAVE_UTIME_H
   struct utimbuf     utb;
#endif
};

enum class unc_stage_e : unsigned int
{
   TOKENIZE,
   HEADER,
   TOKENIZE_CLEANUP,
   BRACE_CLEANUP,
   FIX_SYMBOLS,
   MARK_COMMENTS,
   COMBINE_LABELS,
   OTHER,
   CLEANUP
};

struct cp_data_t
{
   std::deque<UINT8> *bout;
   FILE              *fout;
   int               last_char;
   bool              do_check;
   unc_stage_e       unc_stage;
   int               check_fail_cnt;       //! total failure count
   bool              if_changed;

   std::string       filename;

   file_mem          file_hdr;          // for cmt_insert_file_header
   file_mem          file_ftr;          // for cmt_insert_file_footer
   file_mem          func_hdr;          // for cmt_insert_func_header
   file_mem          oc_msg_hdr;        // for cmt_insert_oc_msg_header
   file_mem          class_hdr;         // for cmt_insert_class_header
   file_mem          reflow_fold_regex; // for cmt_reflow_fold_regex_file

   size_t            lang_flags;        //! defines the language of the source input
   bool              lang_forced;       //! overwrites automatic language detection

   bool              unc_off;
   bool              unc_off_used;       //! true if the `disable_processing_cmt` option was actively used in the processed file
   UINT32            line_number;
   size_t            column;             //! column for parsing
   UINT16            spaces;             //! space count on output

   int               ifdef_over_whole_file;

   bool              frag;          //! activates code fragment option
   UINT32            frag_cols;

   // stuff to auto-detect line endings
   UINT32            le_counts[uncrustify::line_end_styles];
   UncText           newline;

   bool              did_newline;       //! flag indicates if a newline was added or converted
   E_Token           in_preproc;
   int               preproc_ncnl_count;
   bool              output_trailspace;
   bool              output_tab_as_space;

   bool              bom;
   char_encoding_e   enc;

   // bumped up when a line is split or indented
   int               changes;
   int               pass_count;       //! indicates how often the chunk list shall be processed

   align_t           al[uncrustify::limits::AL_SIZE];
   size_t            al_cnt;
   bool              al_c99_array;

   bool              warned_unable_string_replace_tab_chars;

   int               pp_level;       // TODO: can this ever be -1?

   const char        *phase_name;
   const char        *dumped_file;
   // for debugging purpose only
   tracking_type_e   html_type       = tracking_type_e::TT_NONE;
   const char        *html_file      = nullptr;
   bool              find_deprecated = false;
};

extern cp_data_t cpd;  // TODO: can we avoid this external variable?

const char *get_brace_stage_name(E_BraceStage brace_stage);

const char *get_tracking_type_e_name(tracking_type_e type);

const char *get_unc_stage_name(unc_stage_e unc_stage);

const char *get_char_encoding(char_encoding_e encoding);

const char *get_pattern_class(pattern_class_e p_class);

#endif /* UNCRUSTIFY_TYPES_H_INCLUDED */
