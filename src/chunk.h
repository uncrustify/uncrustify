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
 * Temporary internal typedef. Will be progressively be replaced by Chunk::CheckFnPtr.
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
   static Chunk        NullChunk;                       // Null Chunk
   static Chunk *const NullChunkPtr;                    // Pointer to the Null Chunk

   //! constructors
   Chunk(bool null_c = false);                    // default
   Chunk(const Chunk &o);                         // !!! partial copy: chunk is not linked to others

   Chunk &operator=(const Chunk &o);              // !!! partial copy: chunk is not linked to others

   //! whether this is a null Chunk or not
   bool IsNullChunk() const { return(null_chunk); }
   bool IsNotNullChunk() const { return(!null_chunk); }

   //! sets all elements of the struct to their default value
   void Reset();

   //! provides the number of characters of string
   size_t Len() const;

   //! provides the content of a string a zero terminated character pointer
   const char *Text() const;

   // Issue #2984, fill up, if necessary, a copy of the first chars of the Text() string
   const char *ElidedText(char *for_the_copy) const;


   // --------- Get* chunk functions

   /**
    * @brief returns the head of the chunk list
    * @return pointer to the first chunk
    */
   static Chunk *GetHead(void);

   /**
    * @brief returns the tail of the chunk list
    * @return pointer to the last chunk
    */
   static Chunk *GetTail(void);

   /**
    * @brief returns the next chunk in a list of chunks
    * @param scope code region to search in
    * @return pointer to next chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNext(const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the previous chunk in a list of chunks
    * @param scope code region to search in
    * @return pointer to previous chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrev(const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the next newline chunk
    * @param scope code region to search in
    * @return pointer to next newline chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNextNl(const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the prev newline chunk
    * @param scope code region to search in
    * @return pointer to prev newline chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevNl(const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the next non-newline chunk
    * @param scope code region to search in
    * @return pointer to next non-newline chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNextNnl(const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the prev non-newline chunk
    * @param scope code region to search in
    * @return pointer to prev non-newline chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevNnl(const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the next non-comment chunk
    * @param scope code region to search in
    * @return pointer to next non-comment chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNextNc(const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the prev non-comment chunk
    * @param scope code region to search in
    * @return pointer to prev non-comment chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevNc(const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the next non-comment and non-newline chunk
    * @param scope code region to search in
    * @return pointer to next non-comment and non-newline chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNextNcNnl(const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the prev non-comment and non-newline chunk
    * @param scope code region to search in
    * @return pointer to prev non-comment and non-newline chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevNcNnl(const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the next non-comment, non-newline, non-preprocessor chunk
    * @param scope code region to search in
    * @return pointer to next non-comment, non-newline, non-preprocessor chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNextNcNnlNpp(const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the prev non-comment, non-newline, non-preprocessor chunk
    * @param scope code region to search in
    * @return pointer to prev non-comment, non-newline, non-preprocessor chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevNcNnlNpp(const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the next non-preprocessor or non-comment, non-newline chunk
    * @param scope code region to search in
    * @return pointer to next non-preprocessor or non-comment, non-newline chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNextNppOrNcNnl(const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the prev non-preprocessor or non-comment, non-newline chunk
    * @param scope code region to search in
    * @return pointer to prev non-preprocessor or non-comment, non-newline chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevNppOrNcNnl(const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the next preprocessor aware non-comment and non-newline chunk
    *        Unlike Chunk::GetNextNcNnl, this will also ignore a line continuation if
    *        the starting chunk is in a preprocessor directive, and may return a newline
    *        if the search reaches the end of a preprocessor directive.
    * @return pointer to next preprocessor aware non-comment and non-newline chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *PpaGetNextNcNnl() const;

   /**
    * @brief returns the next non-comment, non-newline, non-empty text chunk
    * @param scope code region to search in
    * @return pointer to next non-comment, non-newline, non-empty text chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNextNcNnlNet(const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the prev non-comment, non-newline, non-empty text chunk
    * @param scope code region to search in
    * @return pointer to prev non-comment, non-newline, non-empty text chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevNcNnlNet(const E_Scope scope = E_Scope::ALL) const;


   /**
    * @brief returns the prev non-comment, non-newline, non-ignored chunk
    * @param scope code region to search in
    * @return pointer to prev non-comment, non-newline, non-ignored chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevNcNnlNi(const E_Scope scope = E_Scope::ALL) const;


   /**
    * @brief returns the next chunk not in or part of balanced square
    *        brackets. This handles stacked [] instances to accommodate
    *        multi-dimensional array declarations
    * @param scope  code region to search in
    * @return nullptr or the next chunk not in or part of square brackets
    */
   Chunk *GetNextNisq(const E_Scope scope = E_Scope::ALL) const;


   /**
    * @brief returns the next non-virtual brace chunk
    * @param scope code region to search in
    * @return pointer to next non-virtual brace chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNextNvb(const E_Scope scope = E_Scope::ALL) const;


   /**
    * @brief returns the prev non-virtual brace chunk
    * @param scope code region to search in
    * @return pointer to prev non-virtual brace chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevNvb(const E_Scope scope = E_Scope::ALL) const;


   /**
    * @brief returns the next chunk of the given type at the level.
    * @param cType  the type to look for
    * @param cLevel the level to match, -1 or ANY_LEVEL (any level)
    * @param scope  code region to search in
    * @return pointer to the next matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNextType(const E_Token cType, const int cLevel, const E_Scope scope = E_Scope::ALL) const;


   /**
    * @brief returns the prev chunk of the given type at the level.
    * @param cType  the type to look for
    * @param cLevel the level to match, -1 or ANY_LEVEL (any level)
    * @param scope  code region to search in
    * @return pointer to the prev matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevType(const E_Token type, int level, E_Scope scope = E_Scope::ALL) const;


   /**
    * @brief returns the next chunk that holds a given string at a given level.
    * @param cStr   string to search for
    * @param len    length of string
    * @param cLevel -1 or ANY_LEVEL (any level) or the level to match
    * @param scope  code region to search in
    * @return pointer to the next matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNextString(const char *cStr, const size_t len, const int cLevel, const E_Scope scope = E_Scope::ALL) const;


   /**
    * @brief returns the prev chunk that holds a given string at a given level.
    * @param cStr   string to search for
    * @param len    length of string
    * @param cLevel -1 or ANY_LEVEL (any level) or the level to match
    * @param scope  code region to search in
    * @return pointer to the prev matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevString(const char *cStr, const size_t len, const int cLevel, const E_Scope scope = E_Scope::ALL) const;


   // --------- Search functions

   /**
    * @brief defines a member function pointer for a function of type
    * Chunk *Chunk::function(const E_Scope scope)
    * that will search for a new chunk
    */
   typedef Chunk *(Chunk::*T_SearchFnPtr)(const E_Scope scope) const;

   /**
    * @brief defines a member function pointer for a function of type
    * bool Chunk::function() const;
    * that checks whether a chunk satisty a specific condition
    */
   typedef bool (Chunk::*T_CheckFnPtr)() const;

   /**
    * @brief determines the search direction to use and returns a pointer
    *        to the corresponding search function.
    * @param dir search direction
    * @return pointer to search function
    */
   static T_SearchFnPtr GetSearchFn(const E_Direction dir = E_Direction::FORWARD);

   /**
    * @brief search for a chunk that satisfies a condition in a chunk list
    *
    * A generic function that traverses a chunks list either
    * in forward or reverse direction. The traversal continues until a
    * chunk satisfies the condition defined by the compare function.
    * Depending on the parameter cond the condition will either be
    * checked to be true or false.
    *
    * @param  checkFn  compare function
    * @param  scope    code parts to consider for search
    * @param  dir      search direction (forward or backward)
    * @param  cond     success condition
    *
    * @return pointer to the found chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *Search(const T_CheckFnPtr checkFn, const E_Scope scope = E_Scope::ALL, const E_Direction dir = E_Direction::FORWARD, const bool cond = true) const;

   /**
    * @brief search for a chunk that satisfies a condition in a chunk list,
    *        but being aware of preprocessor chucks.
    *
    * This function is similar to Search, except that it is tweaked to
    * handle searches inside of preprocessor directives. Specifically, if the
    * starting token is inside a preprocessor directive, it will ignore a line
    * continuation, and will abort the search if it reaches the end of the
    * directive. This function only searches forward.
    *
    * @param  checkFn  compare function
    * @param  cond     success condition
    *
    * @return pointer to the found chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *SearchPpa(const T_CheckFnPtr checkFn, const bool cond = true) const;

   /**
    * @brief search a chunk of a given type and level. Traverses a chunk list in the
    * specified direction until a chunk of a given type is found.
    *
    * This function is a specialization of Chunk::Search.
    *
    * @param cType  category to search for
    * @param scope  code parts to consider for search
    * @param dir    search direction
    * @param cLevel nesting level to match or -1 / ANY_LEVEL
    *
    * @return pointer to the found chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *SearchTypeLevel(const E_Token cType, const E_Scope scope = E_Scope::ALL, const E_Direction dir = E_Direction::FORWARD, const int cLevel = -1) const;


   /**
    * @brief searches a chunk that holds a specific string
    *
    * Traverses a chunk list either in forward or backward direction until a chunk
    * with the provided string was found. Additionally a nesting level can be
    * provided to narrow down the search.
    *
    * @param  cStr   string that searched chunk needs to have
    * @param  len    length of the string
    * @param  cLevel nesting level of the searched chunk, ignored when negative
    * @param  scope  code parts to consider for search
    * @param  dir    search direction
    *
    * @return pointer to the found chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *SearchStringLevel(const char *cStr, const size_t len, const int cLevel, const E_Scope scope = E_Scope::ALL, const E_Direction dir = E_Direction::FORWARD) const;


   // --------- Is* functions

   /**
    * @brief checks whether the chunk is a specific token
    * @token the token to check for
    * @return true if the chunk type matches the specified token, false otherwise
    */
   bool Is(E_Token token) const;

   /**
    * @brief checks whether the chunk is not a specific token
    * @token the token to check for
    * @return true if the chunk type does not matches the specified token, false otherwise
    */
   bool IsNot(E_Token token) const;

   /**
    * @brief checks whether the chunk is a newline
    * @return true if the chunk is a newline, false otherwise
    */
   bool IsNewline() const;

   /**
    * @brief checks whether the chunk is a comment
    * This means any kind of:
    *   - single line comment
    *   - multiline comment
    *   - C comment
    *   - C++ comment
    */
   bool IsComment() const;

   /**
    * @brief checks whether the chunk is valid and has an empty text
    * @return true if the chunk is valid and has an empty text
    */
   bool IsEmptyText() const;

   /**
    * @brief checks whether the chunk is a preprocessor
    * @return true if the chunk is a preprocessor, false otherwise
    */
   bool IsPreproc() const;

   /**
    * @brief checks whether the chunk is either a comment or a newline
    * @return true if the chunk is either a comment or a newline, false otherwise
    */
   bool IsCommentOrNewline() const;

   /**
    * @brief checks whether the chunk is either a comment, a newline or ignored
    * @return true if the chunk is either a comment, a newline or ignored, false otherwise
    */
   bool IsCommentNewlineOrIgnored() const;

   /**
    * @brief checks whether the chunk is a comment, a newline or a preprocessor
    * @return true if the chunk is a comment, a newline or a preprocessor, false otherwise
    */
   bool IsCommentNewlineOrPreproc() const;

   /**
    * @brief checks whether the chunk is a preprocessor and either a comment or a newline
    * @return true if the chunk is a preprocessor and either a comment or a newline, false otherwise
    */
   bool IsCommentOrNewlineInPreproc() const;

   /**
    * @brief checks whether the chunk is a comment, a newline or has an empty text
    * @return true if the chunk is a comment, a newline or has an empty text
    */
   bool IsCommentNewlineOrEmptyText() const;

   /**
    * @brief checks whether the chunk is a square bracket
    * @return true if the chunk is a square bracket
    */
   bool IsSquareBracket() const;

   /**
    * @brief checks whether the chunk is a virtual brace
    * @return true if the chunk is a virtual brace
    */
   bool IsVBrace() const;

   /**
    * @brief checks whether the chunk matches a given type and level
    * @param type  category to search for
    * @param level nesting level to match
    * @return true if the chunk matches a given type and level
    */
   bool IsTypeAndLevel(const E_Token cType, const int cLevel) const;


   /**
    * @brief checks whether the chunk matches a given string and level
    * @param cStr   the expected string
    * @param len    length of the string
    * @param cLevel nesting level of the searched chunk, ignored when negative
    * @return true if the chunk matches a given string and level
    */
   bool IsStringAndLevel(const char *cStr, const size_t len, const int cLevel) const;


   /**
    * @brief checks whether the chunk is a star/asterisk
    * @return true if the chunk is a star/asterisk
    */
   bool IsStar() const;


   // --------- Data members

   Chunk        *next;                       //! pointer to next chunk in list
   Chunk        *prev;                       //! pointer to previous chunk in list
   Chunk        *parent;                     //! pointer to parent chunk(not always set)

   align_ptr_t  align;
   indent_ptr_t indent;
   E_Token      type;                        //! type of the chunk itself
   E_Token      parent_type;                 //! type of the parent chunk usually CT_NONE
   //! might be different from parent->parent_type (above)
   size_t       orig_line;                   //! line number of chunk in input file
   size_t       orig_col;                    //! column where chunk started in the input file, is always > 0
   size_t       orig_col_end;                //! column where chunk ended in the input file, is always > 1
   UINT32       orig_prev_sp;                //! whitespace before this token
   pcf_flags_t  flags;                       //! see PCF_xxx
   size_t       column;                      //! column of chunk
   size_t       column_indent;               /** if 1st on a line, set to the 'indent'
                                              * column, which may be less than the real
                                              * column used to indent with tabs          */
   size_t   nl_count;                        //! number of newlines in CT_NEWLINE
   size_t   nl_column;                       //! column of the subsequent newline entries(all of them should have the same column)
   size_t   level;                           /** nest level in {, (, or [
                                              * only to help vim command } */
   size_t   brace_level;                     //! nest level in braces only
   size_t   pp_level;                        //! nest level in preprocessor
   bool     after_tab;                       //! whether this token was after a tab
   unc_text str;                             //! the token text

   // for debugging purpose only
   track_list *tracking;

private:
   void copyFrom(const Chunk &o);              // !!! partial copy: chunk is not linked to others

   const bool null_chunk;                      //! true for null chunks
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
Chunk *chunk_search_prev_cat(Chunk *pc, const E_Token cat);


/**
 * @brief forward search a chunk of a given category in a chunk list
 *
 * @param  pc   chunk list to search in
 * @param  cat  category to search for
 *
 * @retval nullptr  no object found, or invalid parameters provided
 * @retval Chunk  pointer to the found object
 */
Chunk *chunk_search_next_cat(Chunk *pc, const E_Token cat);


/**
 * @brief checks wether two chunks are in same line
 *
 * @param  start
 * @param  end
 *
 * @return true if there is no newline between start and end chunks
 */
bool are_chunks_in_same_line(Chunk *start, Chunk *end);


inline bool Chunk::IsTypeAndLevel(const E_Token cType, const int cLevel) const
{
   return(  (  cLevel < 0
            || level == static_cast<size_t>(cLevel))
         && type == cType);
}


inline bool Chunk::IsStringAndLevel(const char *cStr, const size_t len, const int cLevel) const
{
   return(  (  cLevel < 0
            || level == static_cast<size_t>(cLevel))
         && Len() == len                       // the length is as expected
         && memcmp(cStr, Text(), len) == 0);   // the strings are equal
}


inline bool Chunk::Is(E_Token token) const
{
   return(  IsNotNullChunk()
         && type == token);
}


inline bool Chunk::IsNot(E_Token token) const
{
   return(!Is(token));
}


inline bool Chunk::IsNewline() const
{
   return(  Is(CT_NEWLINE)
         || Is(CT_NL_CONT));
}


inline bool Chunk::IsComment() const
{
   return(  Is(CT_COMMENT)
         || Is(CT_COMMENT_MULTI)
         || Is(CT_COMMENT_CPP));
}


inline bool Chunk::IsEmptyText() const
{
   return(  IsNotNullChunk()
         && Len() == 0);
}


inline bool Chunk::IsPreproc() const
{
   return(  IsNotNullChunk()
         && flags.test(PCF_IN_PREPROC));
}


inline bool Chunk::IsCommentOrNewline() const
{
   return(  IsComment()
         || IsNewline());
}


inline bool Chunk::IsCommentNewlineOrPreproc() const
{
   return(  IsComment()
         || IsNewline()
         || IsPreproc());
}


inline bool Chunk::IsCommentOrNewlineInPreproc() const
{
   return(  IsPreproc()
         && (  IsComment()
            || IsNewline()));
}


inline bool Chunk::IsCommentNewlineOrEmptyText() const
{
   return(  IsComment()
         || IsNewline()
         || IsEmptyText());
}


inline bool Chunk::IsCommentNewlineOrIgnored() const
{
   return(  IsComment()
         || IsNewline()
         || Is(CT_IGNORED));
}


inline bool Chunk::IsSquareBracket() const
{
   return(  Is(CT_SQUARE_OPEN)
         || Is(CT_TSQUARE)
         || Is(CT_SQUARE_CLOSE));
}


inline bool Chunk::IsVBrace() const
{
   return(  Is(CT_VBRACE_OPEN)
         || Is(CT_VBRACE_CLOSE));
}


inline bool Chunk::IsStar() const
{
   return(  Len() == 1
         && str[0] == '*'
         && IsNot(CT_OPERATOR_VAL));
}


// TODO remove when possible
static inline bool chunk_is_token(const Chunk *pc, E_Token c_token)
{
   return(  pc != nullptr
         && pc->IsNotNullChunk()
         && pc->type == c_token);
}


// TODO remove when possible
static inline bool chunk_is_not_token(const Chunk *pc, E_Token c_token)
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
      return(cur->GetNextType((E_Token)(cur->type + 1), cur->level, scope));
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
      return(cur->GetPrevType((E_Token)(cur->type - 1), cur->level, scope));
   }
   return(cur);
}


//! skip to the final word/type in a :: chain
Chunk *chunk_skip_dc_member(Chunk *start, E_Scope scope = E_Scope::ALL);
Chunk *chunk_skip_dc_member_rev(Chunk *start, E_Scope scope = E_Scope::ALL);


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


// TODO remove when possible
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


static inline bool chunk_is_Doxygen_comment(Chunk *pc)
{
   if (  pc == nullptr
      || !pc->IsComment())
   {
      return(false);
   }
   // check the third character
   const char   *sComment = pc->Text();
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


static inline bool chunk_is_str(Chunk *pc, const char *str)
{
   size_t len = strlen(str);

   return(  pc != nullptr                         // valid pc pointer
         && (pc->Len() == len)                    // token size equals size parameter
         && (memcmp(pc->Text(), str, len) == 0)); // token name is the same as str parameter

   /*
    * TODO: possible access beyond array for memcmp, check this
    * why not use strncmp here?
    */
}


static inline bool chunk_is_str_case(Chunk *pc, const char *str, size_t len)
{
   return(  pc != nullptr
         && (pc->Len() == len)
         && (strncasecmp(pc->Text(), str, len) == 0));
}


static inline bool chunk_is_word(Chunk *pc)
{
   return(  pc != nullptr
         && (pc->Len() >= 1)
         && CharTable::IsKw1(pc->str[0]));
}


static inline bool chunk_is_nullable(Chunk *pc)
{
   return(  language_is_set(LANG_CS | LANG_VALA)
         && (pc != nullptr)
         && (pc->Len() == 1)
         && (pc->str[0] == '?'));
}


static inline bool chunk_is_addr(Chunk *pc)
{
   if (  pc != nullptr
      && pc->IsNotNullChunk()
      && (  chunk_is_token(pc, CT_BYREF)
         || (  (pc->Len() == 1)
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
            && (pc->Len() == 1)
            && (pc->str[0] == '^')
            && pc->type != CT_OPERATOR_VAL));
}


static inline bool chunk_is_ptr_operator(Chunk *pc)
{
   return(  pc != nullptr
         && (  (  pc->IsStar()
               || chunk_is_addr(pc)
               || chunk_is_msref(pc))
            || chunk_is_nullable(pc)));
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
      Chunk *prev = pc->GetPrevNcNnl();

      if (chunk_is_token(prev, CT_FOR))
      {
         Chunk *next = pc;

         while (  next != nullptr
               && next->type != CT_SPAREN_CLOSE
               && next->type != CT_IN)
         {
            next = next->GetNextNcNnl();
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


void set_chunk_type_real(Chunk *pc, E_Token tt, const char *func, int line);


void set_chunk_parent_real(Chunk *pc, E_Token tt, const char *func, int line);


#define set_chunk_type(pc, tt)      set_chunk_type_real((pc), (tt), __unqualified_func__, __LINE__)


#define set_chunk_parent(pc, tt)    set_chunk_parent_real((pc), (tt), __unqualified_func__, __LINE__)


E_Token get_chunk_parent_type(Chunk *pc);


void chunk_flags_set_real(Chunk *pc, pcf_flags_t clr_bits, pcf_flags_t set_bits);


#define chunk_flags_upd(pc, cc, ss)    chunk_flags_set_real((pc), (cc), (ss))


#define chunk_flags_set(pc, ss)        chunk_flags_set_real((pc), {}, (ss))


#define chunk_flags_clr(pc, cc)        chunk_flags_set_real((pc), (cc), {})


void chunk_set_parent(Chunk *pc, Chunk *parent);


E_Token get_type_of_the_parent(Chunk *pc);


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
