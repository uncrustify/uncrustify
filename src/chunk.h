/**
 * @file chunk.h
 * Manages and navigates the list of chunks.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef CHUNK_LIST_H_INCLUDED
#define CHUNK_LIST_H_INCLUDED

#include "uncrustify_types.h"
// necessary to not sort it
#include "char_table.h"
#include "language_tools.h"


/*
 * TODO: better use a namespace for all chunk related operations.
 * The function "chunk_is_comment()" would for instance
 * become "chunk::is_comment()". This makes the usage of the chunks easier
 * and more intuitive.
 */


static constexpr int ANY_LEVEL = -1;


/**
 * Specifies which chunks should/should not be found.
 * ALL (default)
 *  - return the true next/prev
 *
 * PREPROC
 *  - If not in a preprocessor, skip over any encountered preprocessor stuff
 *  - If in a preprocessor, fail to leave (return nullptr)
 */
enum class E_Scope : unsigned int
{
   ALL,      //! search in all kind of chunks
   PREPROC,  //! search only in preprocessor chunks
};


/**
 * Specifies which direction or location an operation shall be performed.
 */
enum class E_Direction : unsigned int
{
   FORWARD,
   BACKWARD
};


/**
 * Temporary internal typedef. Will be progressively be replaced by Chunk::Check_t.
 *
 * @brief prototype for a function that checks a chunk to have a given type
 *
 * @note this typedef defines the function type "check_t"
 * for a function pointer of type
 * bool function(Chunk *pc)
 */
// TODO remove when finished
typedef bool (*check_t)(Chunk *pc);


// This is the main type of this program
class Chunk
{
public:
   static Chunk        NullChunk;          // Null Chunk
   static Chunk *const NullChunkPtr;       // Pointer to the Null Chunk

   //! constructors
   Chunk(bool null_c = false);       // default
   Chunk(const Chunk &o);            // !!! partial copy: chunk is not linked to others

   Chunk &operator=(const Chunk &o); // !!! partial copy: chunk is not linked to others

   //! whether this is a null Chunk or not
   bool IsNullChunk() const { return(null_chunk); }
   bool IsNotNullChunk() const { return(!null_chunk); }

   //! sets all elements of the struct to their default value
   void reset();

   //! provides the number of characters of string
   size_t len() const;

   //! provides the content of a string a zero terminated character pointer
   const char *text() const;

   // Issue #2984, fill up, if necessary, a copy of the first chars of the text() string
   const char *elided_text(char *for_the_copy) const;


   /**
    * @brief returns the head of the chunk list
    *
    * @return pointer to the first chunk
    */
   static Chunk *GetHead(void);


   /**
    * @brief returns the tail of the chunk list
    *
    * @return pointer to the last chunk
    */
   static Chunk *GetTail(void);


   /**
    * @brief returns the next chunk in a list of chunks
    *
    * @param scope code region to search in
    *
    * @return pointer to next chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNext(E_Scope scope = E_Scope::ALL) const;


   /**
    * @brief returns the previous chunk in a list of chunks
    *
    * @param scope code region to search in
    *
    * @return pointer to previous chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrev(E_Scope scope = E_Scope::ALL) const;


   /**
    * @brief returns the next newline chunk
    *
    * @param scope code region to search in
    *
    * @return pointer to next newline chunk or Chunk::NullChunkPtr if no chunk was found
    */
   // TODO make it a const member
   Chunk *get_next_nl(E_Scope scope = E_Scope::ALL);


   /**
    * @brief returns the prev newline chunk
    *
    * @param scope code region to search in
    *
    * @return pointer to prev newline chunk or Chunk::NullChunkPtr if no chunk was found
    */
   // TODO make it a const member
   Chunk *get_prev_nl(E_Scope scope = E_Scope::ALL);


   /**
    * @brief returns the next non-newline chunk
    *
    * @param scope code region to search in
    *
    * @return pointer to next non-newline chunk or Chunk::NullChunkPtr if no chunk was found
    */
   // TODO make it a const member
   Chunk *get_next_nnl(E_Scope scope = E_Scope::ALL);


   /**
    * @brief returns the prev non-newline chunk
    *
    * @param scope code region to search in
    *
    * @return pointer to prev non-newline chunk or Chunk::NullChunkPtr if no chunk was found
    */
   // TODO make it a const member
   Chunk *get_prev_nnl(E_Scope scope = E_Scope::ALL);


   /**
    * @brief returns the next non-comment chunk
    *
    * @param scope code region to search in
    *
    * @return pointer to next non-comment chunk or Chunk::NullChunkPtr if no chunk was found
    */
   // TODO make it a const member
   Chunk *get_next_nc(E_Scope scope = E_Scope::ALL);


   /**
    * @brief returns the prev non-comment chunk
    *
    * @param scope code region to search in
    *
    * @return pointer to prev non-comment chunk or Chunk::NullChunkPtr if no chunk was found
    */
   // TODO make it a const member
   Chunk *get_prev_nc(E_Scope scope = E_Scope::ALL);


   /**
    * @brief defines a member function pointer for a function of type
    * Chunk *Chunk::function(E_Scope scope)
    */
   typedef Chunk *(Chunk::*Search_t)(E_Scope scope) const;


   /**
    * @brief determines the search direction to use and returns a pointer
    *        to the corresponding search function.
    *
    * @param dir search direction
    *
    * @return pointer to search function
    */
   static Search_t Search_dir_fct(const E_Direction dir = E_Direction::FORWARD);


   /**
    * @brief search for a chunk that satisfies a condition in a chunk list
    *
    * A generic function that traverses a chunks list either
    * in forward or reverse direction. The traversal continues until a
    * chunk satisfies the condition defined by the compare function.
    * Depending on the parameter cond the condition will either be
    * checked to be true or false.
    *
    * @param  check_fct  compare function
    * @param  scope      code parts to consider for search
    * @param  dir        search direction (forward or backward)
    * @param  cond       success condition
    *
    * @return pointer to the found chunk or Chunk::NullChunkPtr if no chunk was found
    */
// TODO replace ::check_t with Chunk::Check_t when feasible
   Chunk *Search(const ::check_t check_fct, const E_Scope scope = E_Scope::ALL, const E_Direction dir = E_Direction::FORWARD, const bool cond = true);


   Chunk        *next;          //! pointer to next chunk in list
   Chunk        *prev;          //! pointer to previous chunk in list
   Chunk        *parent;        //! pointer to parent chunk(not always set)

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
   size_t   nl_count;             //! number of newlines in CT_NEWLINE
   size_t   nl_column;            //! column of the subsequent newline entries(all of them should have the same column)
   size_t   level;                /** nest level in {, (, or [
                                   * only to help vim command } */
   size_t   brace_level;          //! nest level in braces only
   size_t   pp_level;             //! nest level in preprocessor
   bool     after_tab;            //! whether this token was after a tab
   unc_text str;                  //! the token text

   // for debugging purpose only
   track_list *tracking;

private:
   void copyFrom(const Chunk &o); // !!! partial copy: chunk is not linked to others

   const bool null_chunk;         //! true for null chunks
};


/**
 * @brief Add a copy of a chunk to a chunk list after the given position.
 *
 * @note If ref is nullptr, add at the tail of the chunk list
 *
 * @todo is ref=nullptr really useful ?
 *
 * @param pc_in  pointer to chunk to add to list
 * @param ref    position where insertion takes place
 *
 * @return pointer to the added chunk
 */
Chunk *chunk_add_after(const Chunk *pc_in, Chunk *ref);


/**
 * @brief Add a copy of a chunk to a chunk list before the given position
 *
 * @note If ref is nullptr, add at the head of the chunk list
 *
 * @todo is ref=nullptr really useful ?
 *
 * \bug code adds it before the tail, either code or comment is wrong
 *
 * @param pc_in  pointer to chunk to add to list
 * @param ref    position where insertion takes place
 *
 * @retval pointer to the added chunk
 */
Chunk *chunk_add_before(const Chunk *pc_in, Chunk *ref);


/**
 * delete a chunk from a chunk list
 *
 * @param pc  chunk to delete
 */
void chunk_del(Chunk * &pc);


/**
 * move a chunk to after the reference position in a chunk list
 *
 * @param pc_in  chunk to move
 * @param ref    chunk after which to move
 */
void chunk_move_after(Chunk *pc_in, Chunk *ref);


/**
 * Swaps two chunks
 *
 * @param pc1  The first chunk
 * @param pc2  The second chunk
 */
void chunk_swap(Chunk *pc1, Chunk *pc2);


/**
 * Swaps two lines that are started with the specified chunks.
 *
 * @param pc1  The first chunk of line 1
 * @param pc2  The first chunk of line 2
 */
void chunk_swap_lines(Chunk *pc1, Chunk *pc2);


/**
 * Finds the first chunk on the line that pc is on.
 * This just backs up until a newline or nullptr is hit.
 *
 * given: [ a - b - c - n1 - d - e - n2 ]
 * input: [ a | b | c | n1 ] => a
 * input: [ d | e | n2 ]     => d
 *
 * @param pc  chunk to start with
 */
Chunk *chunk_first_on_line(Chunk *pc);


//! check if a given chunk is the last on its line
bool chunk_is_last_on_line(Chunk *pc);


/**
 * Gets the next non-NEWLINE and non-comment chunk
 *
 * @param cur    chunk to use as start point
 * @param scope  code region to search in
 */
Chunk *chunk_get_next_nc_nnl(Chunk *cur, E_Scope scope = E_Scope::ALL);


/**
 * Gets the next non-NEWLINE and non-comment chunk, non-preprocessor chunk
 *
 * @param cur    chunk to use as start point
 * @param scope  code region to search in
 */
Chunk *chunk_get_next_nc_nnl_np(Chunk *cur, E_Scope scope = E_Scope::ALL);


/**
 * Gets the prev non-NEWLINE and non-comment chunk, non-preprocessor chunk
 *
 * @param cur    chunk to use as start point
 * @param scope  code region to search in
 */
Chunk *chunk_get_prev_nc_nnl_np(Chunk *cur, E_Scope scope = E_Scope::ALL);


/**
 * Gets the next non-NEWLINE and non-comment chunk inside a preprocessor block
 *
 * @param cur    chunk to use as start point
 * @param scope  code region to search in
 */
Chunk *chunk_get_next_nc_nnl_in_pp(Chunk *cur, E_Scope scope = E_Scope::ALL);


/**
 * Gets the prev non-NEWLINE and non-comment chunk inside a preprocessor block
 *
 * @param cur    chunk to use as start point
 * @param scope  code region to search in
 */
Chunk *chunk_get_prev_nc_nnl_in_pp(Chunk *cur, E_Scope scope = E_Scope::ALL);


/**
 * Gets the next non-NEWLINE and non-comment chunk (preprocessor aware).
 * Unlike chunk_get_next_nc_nnl, this will also ignore a line continuation if
 * the starting chunk is in a preprocessor directive, and may return a newline
 * if the search reaches the end of a preprocessor directive.
 *
 * @param cur    chunk to use as start point
 * @param scope  code region to search in
 */
Chunk *chunk_ppa_get_next_nc_nnl(Chunk *cur);


/**
 * Gets the prev non-NEWLINE and non-comment chunk
 *
 * @param cur    chunk to use as start point
 * @param scope  code region to search in
 */
Chunk *chunk_get_prev_nc_nnl(Chunk *cur, E_Scope scope = E_Scope::ALL);


/**
 * Gets the next non-comment, non-newline, non blank chunk
 *
 * @param cur    chunk to use as start point
 * @param scope  code region to search in
 */
Chunk *chunk_get_next_nc_nnl_nb(Chunk *cur, E_Scope scope = E_Scope::ALL);


/**
 * Gets the prev non-comment, non-newline, non blank chunk
 *
 * @param cur    chunk to use as start point
 * @param scope  code region to search in
 */
Chunk *chunk_get_prev_nc_nnl_nb(Chunk *cur, E_Scope scope = E_Scope::ALL);


/**
 * Gets the next chunk not in or part of balanced square
 * brackets. This handles stacked [] instances to accommodate
 * multi-dimensional array declarations
 *
 * @param  cur    chunk to use as start point
 * @param  scope  code region to search in
 *
 * @return nullptr or the next chunk not in or part of square brackets
 */
Chunk *chunk_get_next_nisq(Chunk *cur, E_Scope scope = E_Scope::ALL);


/**
 * Gets the prev non-NEWLINE and non-comment and non-ignored chunk
 *
 * @param cur    chunk to use as start point
 * @param scope  code region to search in
 */
Chunk *chunk_get_prev_nc_nnl_ni(Chunk *cur, E_Scope scope = E_Scope::ALL);


/**
 * Grabs the next chunk of the given type at the level.
 *
 * @param cur    chunk to use as start point
 * @param type   the type to look for
 * @param level  -1 or ANY_LEVEL (any level) or the level to match
 * @param scope  code region to search in
 *
 * @return nullptr or the match
 */
Chunk *chunk_get_next_type(Chunk *cur, c_token_t type, int level, E_Scope scope = E_Scope::ALL);


/**
 * Grabs the prev chunk of the given type at the level.
 *
 * @param cur    chunk to use as start point
 * @param type   The type to look for
 * @param level  -1 or ANY_LEVEL (any level) or the level to match
 * @param scope  code region to search in
 *
 * @return nullptr or the match
 */
Chunk *chunk_get_prev_type(Chunk *cur, c_token_t type, int level, E_Scope scope = E_Scope::ALL);


/**
 * @brief find a chunk that holds a given string
 *
 * Traverses a chunk list in forward direction until a chunk of a given category is found.
 *
 * @param cur    chunk to use as start point
 * @param str    string to search for
 * @param len    length of string
 * @param level  the level to match or -1 or ANY_LEVEL
 * @param scope  code region to search in
 *
 * @retval nullptr  no chunk found or invalid parameters provided
 * @retval Chunk  pointer to the found chunk
 */
Chunk *chunk_get_next_str(Chunk *cur, const char *str, size_t len, int level, E_Scope scope = E_Scope::ALL);


/**
 * @brief find a chunk that holds a given string
 *
 * Traverses a chunk list in backward direction until a chunk of a given category is found.
 *
 * @param cur    chunk to use as start point
 * @param str    string to search for
 * @param len    length of string
 * @param level  the level to match or -1 or ANY_LEVEL
 * @param scope  code region to search in
 *
 * @retval nullptr  no chunk found or invalid parameters provided
 * @retval Chunk  pointer to the found chunk
 */
Chunk *chunk_get_prev_str(Chunk *cur, const char *str, size_t len, int level, E_Scope scope = E_Scope::ALL);


/**
 * @brief Gets the next non-vbrace chunk
 *
 * @param  cur    chunk to start search
 * @param  scope  chunk section to consider
 *
 * @return pointer to found chunk or nullptr if no chunk was found
 */
Chunk *chunk_get_next_nvb(Chunk *cur, const E_Scope scope = E_Scope::ALL);


/**
 * @brief Gets the previous non-vbrace chunk
 *
 * @param  cur    chunk to start search
 * @param  scope  chunk section to consider
 *
 * @return pointer to found chunk or nullptr if no chunk was found
 */
Chunk *chunk_get_prev_nvb(Chunk *cur, const E_Scope scope = E_Scope::ALL);


/**
 * Gets the next chunk not in or part of balanced square
 * brackets.This handles stacked[] instances to accommodate
 * multi - dimensional array declarations
 *
 * @param  cur    chunk to use as start point
 *
 * @return nullptr or the next chunk not in or part of square brackets
 */
Chunk *chunk_get_next_ssq(Chunk *cur);

/**
 * Gets the prev chunk not in or part of balanced square
 * brackets.This handles stacked[] instances to accommodate
 * multi - dimensional array declarations
 *
 * @param  cur    chunk to use as start point
 *
 * @return nullptr or the prev chunk not in or part of square brackets
 */
Chunk *chunk_get_prev_ssq(Chunk *cur);

/**
 * Gets the corresponding start chunk if the given chunk is within a
 * preprocessor directive, or nullptr otherwise.
 *
 * @param  cur    chunk to use as start point
 *
 * @return nullptr or start chunk of the preprocessor directive
 */
Chunk *chunk_get_pp_start(Chunk *cur);

/**
 * @brief reverse search a chunk of a given category in a chunk list
 *
 * @param  pc   chunk list to search in
 * @param  cat  category to search for
 *
 * @retval nullptr  no object found, or invalid parameters provided
 * @retval Chunk  pointer to the found object
 */
Chunk *chunk_search_prev_cat(Chunk *pc, const c_token_t cat);


/**
 * @brief forward search a chunk of a given category in a chunk list
 *
 * @param  pc   chunk list to search in
 * @param  cat  category to search for
 *
 * @retval nullptr  no object found, or invalid parameters provided
 * @retval Chunk  pointer to the found object
 */
Chunk *chunk_search_next_cat(Chunk *pc, const c_token_t cat);

/**
 * @brief checks wether two chunks are in same line
 *
 * @param  start
 * @param  end
 *
 * @return true if there is no newline between start and end chunks
 */
bool are_chunks_in_same_line(Chunk *start, Chunk *end);

/*
 * TODO: better move the function implementations to the source file.
 * No need to make the implementation public.
 */


/*
 * TODO: I doubt that inline is required for the functions below.
 * The compiler should know how to optimize the code itself.
 * To clarify do a profiling run with and without inline
 */
static inline bool is_expected_type_and_level(Chunk *pc, c_token_t type, int level)
{
   // we don't care if the pointer is invalid or about the level (if it is negative),
   // or it is as expected and the type is as expected
   return(  pc == nullptr
         || (  (  level < 0
               || pc->level == static_cast<size_t>(level))
            && pc->type == type));
}


static inline bool is_expected_string_and_level(Chunk *pc, const char *str, int level, size_t len)
{
   // we don't care if the pointer is invalid or about the level (if it is negative) or it is as expected
   return(  pc == nullptr
         || (  (  level < 0
               || pc->level == static_cast<size_t>(level))
            && pc->len() == len                        // and the length is as expected
            && memcmp(str, pc->text(), len) == 0));    // and the strings are equal
}


static inline bool chunk_is_token(const Chunk *pc, c_token_t c_token)
{
   return(  pc != nullptr
         && pc->IsNotNullChunk()
         && pc->type == c_token);
}


static inline bool chunk_is_not_token(const Chunk *pc, c_token_t c_token)
{
   return(  pc != nullptr
         && pc->IsNotNullChunk()
         && pc->type != c_token);
}


/**
 * Skips to the closing match for the current paren/brace/square.
 *
 * @param  cur    The opening or closing paren/brace/square
 * @param  scope  chunk section to consider
 *
 * @return nullptr or the matching paren/brace/square
 */
static inline Chunk *chunk_skip_to_match(Chunk *cur, E_Scope scope = E_Scope::ALL)
{
   if (  cur != nullptr
      && (  chunk_is_token(cur, CT_PAREN_OPEN)
         || chunk_is_token(cur, CT_SPAREN_OPEN)
         || chunk_is_token(cur, CT_FPAREN_OPEN)
         || chunk_is_token(cur, CT_TPAREN_OPEN)
         || chunk_is_token(cur, CT_BRACE_OPEN)
         || chunk_is_token(cur, CT_VBRACE_OPEN)
         || chunk_is_token(cur, CT_ANGLE_OPEN)
         || chunk_is_token(cur, CT_SQUARE_OPEN)))
   {
      return(chunk_get_next_type(cur, (c_token_t)(cur->type + 1), cur->level, scope));
   }
   return(cur);
}


static inline Chunk *chunk_skip_to_match_rev(Chunk *cur, E_Scope scope = E_Scope::ALL)
{
   if (  cur != nullptr
      && (  chunk_is_token(cur, CT_PAREN_CLOSE)
         || chunk_is_token(cur, CT_SPAREN_CLOSE)
         || chunk_is_token(cur, CT_FPAREN_CLOSE)
         || chunk_is_token(cur, CT_TPAREN_CLOSE)
         || chunk_is_token(cur, CT_BRACE_CLOSE)
         || chunk_is_token(cur, CT_VBRACE_CLOSE)
         || chunk_is_token(cur, CT_ANGLE_CLOSE)
         || chunk_is_token(cur, CT_SQUARE_CLOSE)))
   {
      return(chunk_get_prev_type(cur, (c_token_t)(cur->type - 1), cur->level, scope));
   }
   return(cur);
}


//! skip to the final word/type in a :: chain
Chunk *chunk_skip_dc_member(Chunk *start, E_Scope scope = E_Scope::ALL);
Chunk *chunk_skip_dc_member_rev(Chunk *start, E_Scope scope = E_Scope::ALL);


/**
 * checks if a chunk is valid and is a comment
 *
 * comment means any kind of
 * - single line comment
 * - multiline comment
 * - C comment
 * - C++ comment
 */
static inline bool chunk_is_comment(Chunk *pc)
{
   return(  chunk_is_token(pc, CT_COMMENT)
         || chunk_is_token(pc, CT_COMMENT_MULTI)
         || chunk_is_token(pc, CT_COMMENT_CPP));
}


/**
 * Returns true if the chunk under test is an inheritance access specifier
 */
static inline bool chunk_is_cpp_inheritance_access_specifier(Chunk *pc)
{
   return(  language_is_set(LANG_CPP)
         && pc != nullptr
         && pc->IsNotNullChunk()
         && (  chunk_is_token(pc, CT_ACCESS)
            || chunk_is_token(pc, CT_QUALIFIER))
         && (  std::strncmp(pc->str.c_str(), "private", 7) == 0
            || std::strncmp(pc->str.c_str(), "protected", 9) == 0
            || std::strncmp(pc->str.c_str(), "public", 6) == 0));
} // chunk_is_cpp_inheritance_access_specifier


static inline bool chunk_is_colon(Chunk *pc)
{
   return(  chunk_is_token(pc, CT_ACCESS_COLON)
         || chunk_is_token(pc, CT_ASM_COLON)
         || chunk_is_token(pc, CT_BIT_COLON)
         || chunk_is_token(pc, CT_CASE_COLON)
         || chunk_is_token(pc, CT_CLASS_COLON)
         || chunk_is_token(pc, CT_COLON)
         || chunk_is_token(pc, CT_COND_COLON)
         || chunk_is_token(pc, CT_CONSTR_COLON)
         || chunk_is_token(pc, CT_CS_SQ_COLON)
         || chunk_is_token(pc, CT_D_ARRAY_COLON)
         || chunk_is_token(pc, CT_FOR_COLON)
         || chunk_is_token(pc, CT_LABEL_COLON)
         || chunk_is_token(pc, CT_OC_COLON)
         || chunk_is_token(pc, CT_OC_DICT_COLON)
         || chunk_is_token(pc, CT_TAG_COLON)
         || chunk_is_token(pc, CT_WHERE_COLON));
}


static inline bool chunk_is_single_line_comment(Chunk *pc)
{
   return(  chunk_is_token(pc, CT_COMMENT)
         || chunk_is_token(pc, CT_COMMENT_CPP));
}


static inline bool chunk_is_newline(Chunk *pc)
{
   return(  chunk_is_token(pc, CT_NEWLINE)
         || chunk_is_token(pc, CT_NL_CONT));
}


static inline bool chunk_is_semicolon(Chunk *pc)
{
   return(  chunk_is_token(pc, CT_SEMICOLON)
         || chunk_is_token(pc, CT_VSEMICOLON));
}


/**
 * checks if a chunk is valid and is a blank character
 *
 * @note check compares if len == 0
 *
 * @todo rename function: blank is a space not an empty string
 */
static inline bool chunk_is_blank(Chunk *pc)
{
   return(  pc != nullptr
         && pc->IsNotNullChunk()
         && (pc->len() == 0));
}


//! checks if a chunk is valid and either a comment or newline
static inline bool chunk_is_comment_or_newline(Chunk *pc)
{
   return(  chunk_is_comment(pc)
         || chunk_is_newline(pc));
}


//! checks if a chunk is valid and either a comment or newline or ignored
static inline bool chunk_is_comment_or_newline_or_ignored(Chunk *pc)
{
   return(  chunk_is_comment(pc)
         || chunk_is_newline(pc)
         || chunk_is_token(pc, CT_IGNORED));
}


static inline bool chunk_is_balanced_square(Chunk *pc)
{
   return(  chunk_is_token(pc, CT_SQUARE_OPEN)
         || chunk_is_token(pc, CT_TSQUARE)
         || chunk_is_token(pc, CT_SQUARE_CLOSE));
}


static inline bool chunk_is_preproc(Chunk *pc)
{
   return(  pc != nullptr
         && pc->IsNotNullChunk()
         && pc->flags.test(PCF_IN_PREPROC));
}


static inline bool chunk_is_comment_or_newline_in_preproc(Chunk *pc)
{
   return(  pc != nullptr
         && pc->IsNotNullChunk()
         && chunk_is_preproc(pc)
         && (  chunk_is_comment(pc)
            || chunk_is_newline(pc)));
}


static inline bool chunk_is_comment_newline_or_preproc(Chunk *pc)
{
   return(  chunk_is_comment(pc)
         || chunk_is_newline(pc)
         || chunk_is_preproc(pc));
}


static inline bool chunk_is_comment_newline_or_blank(Chunk *pc)
{
   return(  chunk_is_comment(pc)
         || chunk_is_newline(pc)
         || chunk_is_blank(pc));
}


static inline bool chunk_is_Doxygen_comment(Chunk *pc)
{
   if (  pc == nullptr
      || !chunk_is_comment(pc))
   {
      return(false);
   }
   // check the third character
   const char   *sComment = pc->text();
   const size_t len       = strlen(sComment);

   if (len < 3)
   {
      return(false);
   }
   return(  (sComment[2] == '/')
         || (sComment[2] == '!')
         || (sComment[2] == '@'));
}


static inline bool chunk_is_type(Chunk *pc)
{
   return(  chunk_is_token(pc, CT_TYPE)
         || chunk_is_token(pc, CT_PTR_TYPE)
         || chunk_is_token(pc, CT_BYREF)
         || chunk_is_token(pc, CT_DC_MEMBER)
         || chunk_is_token(pc, CT_QUALIFIER)
         || chunk_is_token(pc, CT_STRUCT)
         || chunk_is_token(pc, CT_ENUM)
         || chunk_is_token(pc, CT_UNION));
}


static inline bool chunk_is_str(Chunk *pc, const char *str, size_t len)
{
   return(  pc != nullptr                         // valid pc pointer
         && (pc->len() == len)                    // token size equals size parameter
         && (memcmp(pc->text(), str, len) == 0)); // token name is the same as str parameter

   /*
    * TODO: possible access beyond array for memcmp, check this
    * why not use strncmp here?
    */
}


static inline bool chunk_is_str_case(Chunk *pc, const char *str, size_t len)
{
   return(  pc != nullptr
         && (pc->len() == len)
         && (strncasecmp(pc->text(), str, len) == 0));
}


static inline bool chunk_is_word(Chunk *pc)
{
   return(  pc != nullptr
         && (pc->len() >= 1)
         && CharTable::IsKw1(pc->str[0]));
}


static inline bool chunk_is_star(Chunk *pc)
{
   return(  pc != nullptr
         && (pc->len() == 1)
         && (pc->str[0] == '*')
         && pc->type != CT_OPERATOR_VAL);
}


static inline bool chunk_is_nullable(Chunk *pc)
{
   return(  language_is_set(LANG_CS)
         && (pc != nullptr)
         && (pc->len() == 1)
         && (pc->str[0] == '?'));
}


static inline bool chunk_is_addr(Chunk *pc)
{
   if (  pc != nullptr
      && pc->IsNotNullChunk()
      && (  chunk_is_token(pc, CT_BYREF)
         || (  (pc->len() == 1)
            && (pc->str[0] == '&')
            && pc->type != CT_OPERATOR_VAL)))
   {
      Chunk *prev = pc->GetPrev();

      if (  pc->flags.test(PCF_IN_TEMPLATE)
         && (  chunk_is_token(prev, CT_COMMA)
            || chunk_is_token(prev, CT_ANGLE_OPEN)))
      {
         return(false);
      }
      return(true);
   }
   return(false);
}


static inline bool chunk_is_msref(Chunk *pc) // ms compilers for C++/CLI and WinRT use '^' instead of '*' for marking up reference types vs pointer types
{
   return(  language_is_set(LANG_CPP)
         && (  pc != nullptr
            && (pc->len() == 1)
            && (pc->str[0] == '^')
            && pc->type != CT_OPERATOR_VAL));
}


static inline bool chunk_is_ptr_operator(Chunk *pc)
{
   return(  (  chunk_is_star(pc)
            || chunk_is_addr(pc)
            || chunk_is_msref(pc))
         || chunk_is_nullable(pc));
}


static inline bool chunk_is_pointer_or_reference(Chunk *pc)
{
   return(  chunk_is_ptr_operator(pc)
         || chunk_is_token(pc, CT_BYREF));
}


//! Check to see if there is a newline between the two chunks
bool chunk_is_newline_between(Chunk *start, Chunk *end);


static inline bool chunk_is_closing_brace(Chunk *pc)
{
   return(  chunk_is_token(pc, CT_BRACE_CLOSE)
         || chunk_is_token(pc, CT_VBRACE_CLOSE));
}


static inline bool chunk_is_opening_brace(Chunk *pc)
{
   return(  chunk_is_token(pc, CT_BRACE_OPEN)
         || chunk_is_token(pc, CT_VBRACE_OPEN));
}


static inline bool chunk_is_vbrace(Chunk *pc)
{
   return(  chunk_is_token(pc, CT_VBRACE_CLOSE)
         || chunk_is_token(pc, CT_VBRACE_OPEN));
}


static inline bool chunk_is_paren_open(Chunk *pc)
{
   return(  chunk_is_token(pc, CT_PAREN_OPEN)
         || chunk_is_token(pc, CT_SPAREN_OPEN)
         || chunk_is_token(pc, CT_TPAREN_OPEN)
         || chunk_is_token(pc, CT_FPAREN_OPEN)
         || chunk_is_token(pc, CT_LPAREN_OPEN));
}


static inline bool chunk_is_paren_close(Chunk *pc)
{
   return(  chunk_is_token(pc, CT_PAREN_CLOSE)
         || chunk_is_token(pc, CT_SPAREN_CLOSE)
         || chunk_is_token(pc, CT_TPAREN_CLOSE)
         || chunk_is_token(pc, CT_FPAREN_CLOSE));
}


/**
 * Returns true if either chunk is null or both have the same preproc flags.
 * If this is true, you can remove a newline/nl_cont between the two.
 */
static inline bool chunk_same_preproc(Chunk *pc1, Chunk *pc2)
{
   return(  pc1 == nullptr
         || pc1->IsNullChunk()
         || pc2 == nullptr
         || pc2->IsNullChunk()
         || ((pc1->flags & PCF_IN_PREPROC) == (pc2->flags & PCF_IN_PREPROC)));
}


/**
 * Returns true if it is safe to delete the newline token.
 * The prev and next chunks must have the same PCF_IN_PREPROC flag AND
 * the newline can't be after a C++ comment.
 */
static inline bool chunk_safe_to_del_nl(Chunk *nl)
{
   if (nl == nullptr)
   {
      nl = Chunk::NullChunkPtr;
   }
   Chunk *tmp = nl->GetPrev();

   if (chunk_is_token(tmp, CT_COMMENT_CPP))
   {
      return(false);
   }
   return(chunk_same_preproc(tmp, nl->GetNext()));
}


/**
 * Checks if a chunk points to the opening parenthese of a
 * for(...in...) loop in Objective-C.
 *
 * @return true  - the chunk is the opening parentheses of a for in loop
 */
static inline bool chunk_is_forin(Chunk *pc)
{
   if (  language_is_set(LANG_OC)
      && chunk_is_token(pc, CT_SPAREN_OPEN))
   {
      Chunk *prev = chunk_get_prev_nc_nnl(pc);

      if (chunk_is_token(prev, CT_FOR))
      {
         Chunk *next = pc;

         while (  next != nullptr
               && next->type != CT_SPAREN_CLOSE
               && next->type != CT_IN)
         {
            next = chunk_get_next_nc_nnl(next);
         }

         if (chunk_is_token(next, CT_IN))
         {
            return(true);
         }
      }
   }
   return(false);
}


/**
 * Returns true if pc is an CT_ATTRIBUTE or CT_DECLSPEC
 */
bool chunk_is_attribute_or_declspec(Chunk *pc);


/**
 * Returns true if pc is one of CT_CLASS, CT_ENUM, CT_ENUM_CLASS, CT_STRUCT or CT_UNION
 */
bool chunk_is_class_enum_struct_union(Chunk *pc);


/**
 * Returns true if pc is a CT_CLASS or CT_STRUCT
 */
bool chunk_is_class_or_struct(Chunk *pc);


/**
 * Returns true if pc is one of CT_CLASS, CT_STRUCT or CT_UNION
 */
bool chunk_is_class_struct_union(Chunk *pc);


/**
 * Returns true if pc is a CT_ENUM or CT_ENUM_CLASS
 */
bool chunk_is_enum(Chunk *pc);


void set_chunk_type_real(Chunk *pc, c_token_t tt, const char *func, int line);


void set_chunk_parent_real(Chunk *pc, c_token_t tt, const char *func, int line);


#define set_chunk_type(pc, tt)      set_chunk_type_real((pc), (tt), __unqualified_func__, __LINE__)


#define set_chunk_parent(pc, tt)    set_chunk_parent_real((pc), (tt), __unqualified_func__, __LINE__)


c_token_t get_chunk_parent_type(Chunk *pc);


void chunk_flags_set_real(Chunk *pc, pcf_flags_t clr_bits, pcf_flags_t set_bits);


#define chunk_flags_upd(pc, cc, ss)    chunk_flags_set_real((pc), (cc), (ss))


#define chunk_flags_set(pc, ss)        chunk_flags_set_real((pc), {}, (ss))


#define chunk_flags_clr(pc, cc)        chunk_flags_set_real((pc), (cc), {})


void chunk_set_parent(Chunk *pc, Chunk *parent);


c_token_t get_type_of_the_parent(Chunk *pc);


/**
 * @brief compare the positions of two tokens in a file.
 *
 * The function compares the two positions of two tokens.
 *
 * @param A_token
 * @param B_token
 *
 * @return returns an integer less than, equal to, or greater than zero
 *         if A_token is found, respectively, to be less/before than, to
 *         match, or be greater/after than B_token.
 */
int chunk_compare_position(const Chunk *A_token, const Chunk *B_token);


#endif /* CHUNK_LIST_H_INCLUDED */
