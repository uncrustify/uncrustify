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

#include "base_types.h"
#include "enum_flags.h"
#include "log_levels.h"
#include "logger.h"
#include "option_enum.h"
#include "options.h"
#include "pcf_flags.h"
#include "token_enum.h"    // c_token_t
#include "unc_text.h"

#include <assert.h>
#include <cstdio>
#include <deque>
#include <vector>
#ifdef HAVE_UTIME_H
#include <utime.h>
#endif


class ParseFrame;


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

//! returns type (with removed reference) of a variable
#define noref_decl_t(X)              std::remove_reference<decltype((X))>::type

//! returns type (with removed const and reference) of a variable
#define nocref_decl_t(X)             std::remove_const<noref_decl_t((X))>::type

//! static casts Y to the type (with removed reference) of X
#define s_cast_noref_decl_t(X, Y)    static_cast<nocref_decl_t(X)>(Y)

//! performs abs on Y after static casting it to the type (with removed reference) of X
#define cast_abs(X, Y)               s_cast_noref_decl_t(X, abs(Y))

/**
 * @brief Macro to inform the compiler that a variable is intentionally
 * not in use.
 *
 * @param [in] variableName: The unused variable.
 */
#define UNUSED(variableName)         ((void)variableName)


//! Brace stage enum used in brace_cleanup
enum class brace_stage_e : unsigned int
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


struct chunk_t; //forward declaration


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


struct align_ptr_t
{
   chunk_t *next;       //! nullptr or the chunk that should be under this one
   bool    right_align; //! AlignStack.m_right_align
   size_t  star_style;  //! AlignStack.m_star_style
   size_t  amp_style;   //! AlignStack.m_amp_style
   int     gap;         //! AlignStack.m_gap

   /*
    * col_adj is the amount to alter the column for the token.
    * For example, a dangling '*' would be set to -1.
    * A right-aligned word would be a positive value.
    */
   int     col_adj;
   chunk_t *ref;
   chunk_t *start;
};


// This is the main type of this program
struct chunk_t
{
   chunk_t()
   {
      reset();
   }


   //! sets all elements of the struct to their default value
   void reset()
   {
      memset(&align, 0, sizeof(align));
      memset(&indent, 0, sizeof(indent));
      next          = nullptr;
      prev          = nullptr;
      parent        = nullptr;
      type          = CT_NONE;
      parent_type   = CT_NONE;
      orig_line     = 0;
      orig_col      = 0;
      orig_col_end  = 0;
      orig_prev_sp  = 0;
      flags         = PCF_NONE;
      column        = 0;
      column_indent = 0;
      nl_count      = 0;
      nl_column     = 0;
      level         = 0;
      brace_level   = 0;
      pp_level      = 0;
      after_tab     = false;
      str.clear();
   }


   //! provides the number of characters of string
   size_t len() const
   {
      return(str.size());
   }


   //! provides the content of a string a zero terminated character pointer
   const char *text() const
   {
      return(str.c_str());
   }

   chunk_t      *next;            //! pointer to next chunk in list
   chunk_t      *prev;            //! pointer to previous chunk in list
   chunk_t      *parent;          //! pointer to parent chunk(not always set)
   align_ptr_t  align;
   indent_ptr_t indent;
   c_token_t    type;             //! type of the chunk itself
   c_token_t    parent_type;      //! type of the parent chunk usually CT_NONE
                                  //! might be different from parent->parent_type (above)
   size_t       orig_line;        //! line number of chunk in input file
   size_t       orig_col;         //! column where chunk started in the input file, is always > 0
   size_t       orig_col_end;     //! column where chunk ended in the input file, is always > 1
   UINT32       orig_prev_sp;     //! whitespace before this token
   pcf_flags_t  flags;            //! see PCF_xxx
   size_t       column;           //! column of chunk
   size_t       column_indent;    /** if 1st on a line, set to the 'indent'
                                   * column, which may be less than the real
                                   * column used to indent with tabs          */
   size_t       nl_count;         //! number of newlines in CT_NEWLINE
   size_t       nl_column;        //! column of the subsequent newline entries(all of them should have the same column)
   size_t       level;            //! nest level in {, (, or [
   size_t       brace_level;      //! nest level in braces only
   size_t       pp_level;         //! nest level in preprocessor
   bool         after_tab;        //! whether this token was after a tab
   unc_text     str;              //! the token text
};


//! list of all programming languages Uncrustify supports
enum lang_flag_e
{
   LANG_C    = 0x0001,
   LANG_CPP  = 0x0002,
   LANG_D    = 0x0004,
   LANG_CS   = 0x0008,     //! C# (C-Sharp)
   LANG_JAVA = 0x0010,
   LANG_OC   = 0x0020,     //! Objective-C
   LANG_VALA = 0x0040,
   LANG_PAWN = 0x0080,
   LANG_ECMA = 0x0100,     //! ECMA Script (JavaScript)

   LANG_ALLC = 0x017f,     /** LANG_C    | LANG_CPP | LANG_D    | LANG_CS   |
                            *  LANG_JAVA | LANG_OC  | LANG_VALA | LANG_ECMA   */
   LANG_ALL  = 0x0fff,     //! applies to all languages

   FLAG_HDR  = 0x2000,     /*<< Header file for C family languages */
   FLAG_DIG  = 0x4000,     //! digraph/trigraph
   FLAG_PP   = 0x8000,     //! only appears in a preprocessor
};

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
   const char *tag;        //! name of the keyword e.g. "bool"
   c_token_t  type;        //! uncrustify type assigned to that keyword
   size_t     lang_flags;  //! programming language that uses this keyword
};


struct align_t
{
   size_t    col;
   c_token_t type;
   size_t    len;    //! length of the token + space
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

#define MAX_OPTION_NAME_LEN    32  // sets a limit to the name padding

struct cp_data_t
{
   std::deque<UINT8>       *bout;
   FILE                    *fout;
   int                     last_char;
   bool                    do_check;
   unc_stage_e             unc_stage;
   int                     check_fail_cnt; //! total failure count
   bool                    if_changed;

   UINT32                  error_count; //! counts how many errors occurred so far
   std::string             filename;

   file_mem                file_hdr;    // for cmt_insert_file_header
   file_mem                file_ftr;    // for cmt_insert_file_footer
   file_mem                func_hdr;    // for cmt_insert_func_header
   file_mem                oc_msg_hdr;  // for cmt_insert_oc_msg_header
   file_mem                class_hdr;   // for cmt_insert_class_header

   size_t                  lang_flags;  //! defines the language of the source input
   bool                    lang_forced; //! overwrites automatic language detection

   bool                    unc_off;
   bool                    unc_off_used; //! true if the `disable_processing_cmt` option was actively used in the processed file
   UINT32                  line_number;
   size_t                  column;       //! column for parsing
   UINT16                  spaces;       //! space count on output

   int                     ifdef_over_whole_file;

   bool                    frag;    //! activates code fragment option
   UINT32                  frag_cols;

   // stuff to auto-detect line endings
   UINT32                  le_counts[uncrustify::line_end_styles];
   unc_text                newline;

   bool                    consumed;

   int                     did_newline; //! flag indicates if a newline was added or converted
   c_token_t               in_preproc;
   int                     preproc_ncnl_count;
   bool                    output_trailspace;
   bool                    output_tab_as_space;

   bool                    bom;
   char_encoding_e         enc;

   // bumped up when a line is split or indented
   int                     changes;
   int                     pass_count; //! indicates how often the chunk list shall be processed

#define AL_SIZE    8000
   align_t                 al[AL_SIZE];
   size_t                  al_cnt;
   bool                    al_c99_array;

   bool                    warned_unable_string_replace_tab_chars;

   std::vector<ParseFrame> frames;
   int                     pp_level; // TODO: can this ever be -1?

   const char              *phase_name;
   const char              *dumped_file;
};

extern cp_data_t cpd;  // TODO: can we avoid this external variable?

const char *get_brace_stage_name(brace_stage_e brace_stage);

const char *get_unc_stage_name(unc_stage_e unc_stage);

#endif /* UNCRUSTIFY_TYPES_H_INCLUDED */
