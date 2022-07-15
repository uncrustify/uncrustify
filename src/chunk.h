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


   // --------- Access methods

   // @brief returns the number of characters in the string
   size_t Len() const;

   // @brief returns the content of the string
   const char *Text() const;

   // Issue #2984, fill up, if necessary, a copy of the first chars of the Text() string
   const char *ElidedText(char *for_the_copy) const;

   /**
    * @brief returns the type of the chunk
    */
   E_Token GetType() const;

   /**
    * @brief Sets the chunk type
    * @param token the type to set
    * @param func the name of the function from where this method is called (for log purposes)
    * @param line the line number from where this method is called (for log purposes)
    */
   void SetTypeReal(const E_Token token, const char *func, const int line);

   /**
    * @brief returns the type of the parent chunk
    */
   E_Token GetParentType() const;

   /**
    * @brief Sets the type of the parent chunk
    * @param token the type to set
    * @param func the name of the function from where this method is called (for log purposes)
    * @param line the line number from where this method is called (for log purposes)
    */
   void SetParentTypeReal(const E_Token token, const char *func, const int line);

   /**
    * @brief returns the parent of the chunk
    */
   Chunk *GetParent() const;

   /**
    * @brief Sets the parent of the chunk
    * @param parent the parent chunk to set
    */
   void SetParent(Chunk *parent);


   // --------- Get* chunk functions

   /**
    * @brief returns the head of the chunk list
    * @return pointer to the first chunk
    */
   static Chunk *GetHead();

   /**
    * @brief returns the tail of the chunk list
    * @return pointer to the last chunk
    */
   static Chunk *GetTail();

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
    * @param type    the type to look for
    * @param cLevel  the level to match or ANY_LEVEL
    * @param scope   code region to search in
    * @return pointer to the next matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNextType(const E_Token type, const int cLevel = ANY_LEVEL, const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the prev chunk of the given type at the level.
    * @param type    the type to look for
    * @param cLevel  the level to match or ANY_LEVEL
    * @param scope   code region to search in
    * @return pointer to the prev matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevType(const E_Token type, int level = ANY_LEVEL, E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the next chunk that holds a given string at a given level.
    * @param cStr   string to search for
    * @param len    length of string
    * @param cLevel the level to match or ANY_LEVEL
    * @param scope  code region to search in
    * @return pointer to the next matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNextString(const char *cStr, const size_t len, const int cLevel, const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the prev chunk that holds a given string at a given level.
    * @param cStr   string to search for
    * @param len    length of string
    * @param cLevel the level to match or ANY_LEVEL
    * @param scope  code region to search in
    * @return pointer to the prev matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevString(const char *cStr, const size_t len, const int cLevel, const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the next chunk that is not part of balanced square brackets.
    * This handles stacked[] instances to accommodate multidimensional arrays.
    * @return pointer to the next matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNextNbsb() const;

   /**
    * @brief returns the prev chunk that is not part of balanced square brackets.
    * This handles stacked[] instances to accommodate multidimensional arrays.
    * @return pointer to the prev matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevNbsb() const;

   /**
    * @brief returns the corresponding start chunk if the given chunk is within a
    * preprocessor directive, or Chunk::NullChunkPtr otherwise.
    * @return start chunk of the preprocessor directive or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPpStart() const;

   /**
    * @brief Finds the first chunk on the line the current chunk is on.
    * This just backs up until a newline or nullptr is hit.
    *
    * given: [ a - b - c - n1 - d - e - n2 ]
    * input: [ a | b | c | n1 ] => a
    * input: [ d | e | n2 ]     => d
    *
    * @return pointer to the first chunk on the line the current chunk is on.
    */
   Chunk *GetFirstChunkOnLine() const;


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
    * @return pointer to the found chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *SearchPpa(const T_CheckFnPtr checkFn, const bool cond = true) const;

   /**
    * @brief search a chunk of a given type and level. Traverses a chunk list in the
    * specified direction until a chunk of a given type and level is found.
    *
    * This function is a specialization of Chunk::Search.
    *
    * @param type   category to search for
    * @param scope  code parts to consider for search
    * @param dir    search direction
    * @param cLevel nesting level to match or ANY_LEVEL
    * @return pointer to the found chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *SearchTypeLevel(const E_Token type, const E_Scope scope = E_Scope::ALL, const E_Direction dir = E_Direction::FORWARD, const int cLevel = ANY_LEVEL) const;

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
    * @return pointer to the found chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *SearchStringLevel(const char *cStr, const size_t len, const int cLevel, const E_Scope scope = E_Scope::ALL, const E_Direction dir = E_Direction::FORWARD) const;

   /**
    * @brief skip to the closing match for the current paren/brace/square.
    * @param scope chunk section to consider
    * @return pointer to the next matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *SkipToMatch(E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief skip to the opening match for the current paren/brace/square.
    * @param scope chunk section to consider
    * @return pointer to the prev matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *SkipToMatchRev(E_Scope scope = E_Scope::ALL) const;


   // --------- Is* functions

   /**
    * @brief checks whether the chunk is a specific token
    * @token the token to check for
    * @return true if the chunk type matches the specified token, false otherwise
    */
   bool Is(E_Token token) const;

   /**
    * @brief checks whether the chunk token name is a specific string
    * @param cStr string to compare token name with
    * @param caseSensitive whether to do a case sensitive or insensitive comparison
    * @return true if the chunk token name matches the specified string, false otherwise
    */
   bool IsString(const char *cStr, bool caseSensitive = true) const;

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
    * @brief checks whether the chunk is a single line comment
    * @return true if the chunk is a single line comment
    */
   bool IsSingleLineComment() const;

   /**
    * @brief checks whether the chunk is a Doxygen comment
    * @return true if the chunk is a Doxygen comment
    */
   bool IsDoxygenComment() const;

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
   bool IsTypeAndLevel(const E_Token type, const int cLevel) const;

   /**
    * @brief checks whether the chunk matches a given string and level
    * @param cStr   the expected string
    * @param len    length of the string
    * @param caseSensitive whether to do a case sensitive or insensitive comparison
    * @param cLevel nesting level of the searched chunk, ignored when negative
    * @return true if the chunk matches a given string and level
    */
   bool IsStringAndLevel(const char *cStr, const size_t len, bool caseSensitive, const int cLevel) const;

   /**
    * @brief checks whether the chunk is a star/asterisk
    * @return true if the chunk is a star/asterisk
    */
   bool IsStar() const;

   /**
    * @brief checks whether the chunk is a colon
    * @return true if the chunk is a colon
    */
   bool IsColon() const;

   /**
    * @brief checks whether the chunk is a semicolon
    * @return true if the chunk is a semicolon
    */
   bool IsSemicolon() const;

   /**
    * @brief checks whether the chunk is a pointer operator
    * @return true if the chunk is a pointer operator
    */
   bool IsPointerOperator() const;

   /**
    * @brief checks whether the chunk is a pointer or a reference
    * @return true if the chunk is a pointer or a reference
    */
   bool IsPointerOrReference() const;

   /**
    * @brief checks whether the chunk is an inheritance access specifier
    * @return true if the chunk is an inheritance access specifier
    */
   bool IsCppInheritanceAccessSpecifier() const;

   /**
    * @brief checks whether the chunk is a pointer, reference or a qualifier
    * @return true if the chunk is a pointer, reference or a qualifier
    */
   bool IsPointerReferenceOrQualifier() const;

   /**
    * @brief checks whether the chunk is an address
    * @return true if the chunk is an address
    */
   bool IsAddress() const;

   /**
    * @brief checks whether the chunk is a MS reference
    * @return true if the chunk is a MS reference
    * NOTE: MS compilers for C++/CLI and WinRT use '^' instead of '*'
    * for marking up reference types vs pointer types
    */
   bool IsMsRef() const;

   /**
    * @brief checks whether the chunk is nullable
    * @return true if the chunk is nullable
    */
   bool IsNullable() const;

   /**
    * @brief Checks if a given chunk is the last on its line
    * @return true or false depending on whether a given chunk is the last on its line
    */
   bool IsLastChunkOnLine() const;

   /**
    * @brief checks whether the current chunk is on same line of the given 'end' chunk.
    * The current chunk must be before the 'end' chunk
    * @param end the end chunk
    * @return true if there is no newline between the current chunk and end chunk
    */
   bool IsOnSameLine(const Chunk *end) const;

   /**
    * @brief checks whether the chunk is an opening brace
    * @return true if the chunk is an opening brace
    */
   bool IsBraceOpen() const;

   /**
    * @brief checks whether the chunk is a closing brace
    * @return true if the chunk is a closing brace
    */
   bool IsBraceClose() const;

   /**
    * @brief checks whether the chunk is an opening parenthesis
    * @return true if the chunk is an opening parenthesis
    */
   bool IsParenOpen() const;

   /**
    * @brief checks whether the chunk is a closing parenthesis
    * @return true if the chunk is a closing parenthesis
    */
   bool IsParenClose() const;

   /**
    * @brief checks whether the chunk is a type defining token
    * @return true if the chunk is a type defining token
    */
   bool IsTypeDefinition() const;

   /**
    * @brief checks whether the chunk is a word
    * @return true if the chunk is a word
    */
   bool IsWord() const;

   /**
    * @brief checks whether the chunk is a CT_ENUM or CT_ENUM_CLASS
    * @return true if the chunk is a CT_ENUM or CT_ENUM_CLASS
    */
   bool IsEnum() const;


   // --------- Util functions

   /**
    * @brief delete the chunk from the chunk list
    * @param pc the chunk to remove from the list
    */
   static void Delete(Chunk * &pc);

   /**
    * @brief add a copy of this chunk after the given position in a chunk list.
    * @note If pos is NullChunk, add at the tail of the chunk list
    * @param pos insert position in list
    * @return pointer to the newly added chunk
    */
   Chunk *CopyAndAddAfter(Chunk *pos) const;

   /**
    * @brief add a copy of this chunk before the given position in a chunk list.
    * @note If pos is NullChunk, add at the head of the chunk list
    * @param pos insert position in list
    * @return pointer to the newly added chunk
    */
   Chunk *CopyAndAddBefore(Chunk *pos) const;

   /**
    * @brief move the chunk after the reference position in the chunk list
    * @param ref chunk after which to move the current chunk
    */
   void MoveAfter(Chunk *ref);

   /**
    * @brief Swaps the place of this chunk with the given one
    * @param other the other chunk
    */
   void Swap(Chunk *other);

   /**
    * @brief Swaps the two lines that are started by the current chunk and the other chunk
    * @param other the other chunk
    */
   void SwapLines(Chunk *other);


   // --------- Data members

   Chunk        *next;                       //! pointer to next chunk in list
   Chunk        *prev;                       //! pointer to previous chunk in list

   align_ptr_t  align;
   indent_ptr_t indent;
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


protected:
   void copyFrom(const Chunk &o);            // !!! partial copy: chunk is not linked to others


   // --------- Data members
   Chunk   *m_parent;                        //! pointer to parent chunk (not always set)
   E_Token m_type;                           //! type of the chunk itself
   E_Token m_parentType;                     //! type of the parent chunk usually CT_NONE


   // --------- Private util functions

   /**
    * @brief add a copy of this chunk before/after the given position in a chunk list.
    * @note If pos is NullChunk, add the new chuck either at the head or tail of the
    * list based on the specified direction.
    * @param pos insert position in list
    * @param dir insert before or after the given position chunk
    * @return pointer to the newly added chunk
    */
   Chunk *CopyAndAdd(Chunk *pos, const E_Direction dir = E_Direction::FORWARD) const;


private:
   const bool null_chunk;                      //! true for null chunks
};


inline Chunk *Chunk::GetParent() const
{
   return(m_parent);
}


inline E_Token Chunk::GetType() const
{
   return(m_type);
}


inline E_Token Chunk::GetParentType() const
{
   return(m_parentType);
}


inline bool Chunk::IsTypeAndLevel(const E_Token type, const int cLevel) const
{
   return(  (  cLevel < 0
            || level == static_cast<size_t>(cLevel))
         && m_type == type);
}


inline bool Chunk::IsStringAndLevel(const char *cStr, const size_t len,
                                    bool caseSensitive, const int cLevel) const
{
   return(  (  cLevel < 0
            || level == static_cast<size_t>(cLevel))
         && Len() == len                                    // the length is as expected
         && (  (  caseSensitive
               && memcmp(Text(), cStr, len) == 0)
            || (  !caseSensitive
               && strncasecmp(Text(), cStr, len) == 0)));                                             // the strings are equal
}


inline bool Chunk::Is(E_Token token) const
{
   return(  IsNotNullChunk()
         && m_type == token);
}


inline bool Chunk::IsString(const char *cStr, bool caseSensitive) const
{
   return(IsStringAndLevel(cStr, strlen(cStr), caseSensitive, ANY_LEVEL));
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


inline bool Chunk::IsSingleLineComment() const
{
   return(  Is(CT_COMMENT)
         || Is(CT_COMMENT_CPP));
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


inline Chunk *Chunk::SkipToMatch(E_Scope scope) const
{
   if (  Is(CT_PAREN_OPEN)
      || Is(CT_SPAREN_OPEN)
      || Is(CT_FPAREN_OPEN)
      || Is(CT_TPAREN_OPEN)
      || Is(CT_BRACE_OPEN)
      || Is(CT_VBRACE_OPEN)
      || Is(CT_ANGLE_OPEN)
      || Is(CT_SQUARE_OPEN))
   {
      return(GetNextType((E_Token)(m_type + 1), level, scope));
   }
   return(const_cast<Chunk *>(this));
}


inline Chunk *Chunk::SkipToMatchRev(E_Scope scope) const
{
   if (  Is(CT_PAREN_CLOSE)
      || Is(CT_SPAREN_CLOSE)
      || Is(CT_FPAREN_CLOSE)
      || Is(CT_TPAREN_CLOSE)
      || Is(CT_BRACE_CLOSE)
      || Is(CT_VBRACE_CLOSE)
      || Is(CT_ANGLE_CLOSE)
      || Is(CT_SQUARE_CLOSE))
   {
      return(GetPrevType((E_Token)(m_type - 1), level, scope));
   }
   return(const_cast<Chunk *>(this));
}


//! skip to the final word/type in a :: chain
Chunk *chunk_skip_dc_member(Chunk *start, E_Scope scope = E_Scope::ALL);
Chunk *chunk_skip_dc_member_rev(Chunk *start, E_Scope scope = E_Scope::ALL);


inline bool Chunk::IsCppInheritanceAccessSpecifier() const
{
   return(  language_is_set(LANG_CPP)
         && (  Is(CT_ACCESS)
            || Is(CT_QUALIFIER))
         && (  IsString("private")
            || IsString("protected")
            || IsString("public")));
}


inline bool Chunk::IsColon() const
{
   return(  Is(CT_ACCESS_COLON)
         || Is(CT_ASM_COLON)
         || Is(CT_BIT_COLON)
         || Is(CT_CASE_COLON)
         || Is(CT_CLASS_COLON)
         || Is(CT_COLON)
         || Is(CT_COND_COLON)
         || Is(CT_CONSTR_COLON)
         || Is(CT_CS_SQ_COLON)
         || Is(CT_D_ARRAY_COLON)
         || Is(CT_FOR_COLON)
         || Is(CT_LABEL_COLON)
         || Is(CT_OC_COLON)
         || Is(CT_OC_DICT_COLON)
         || Is(CT_TAG_COLON)
         || Is(CT_WHERE_COLON));
}


inline bool Chunk::IsSemicolon() const
{
   return(  Is(CT_SEMICOLON)
         || Is(CT_VSEMICOLON));
}


inline bool Chunk::IsDoxygenComment() const
{
   if (!IsComment())
   {
      return(false);
   }

   if (Len() < 3)
   {
      return(false);
   }
   // check the third character
   const char *sComment = Text();
   return(  (sComment[2] == '/')
         || (sComment[2] == '!')
         || (sComment[2] == '@'));
}


inline bool Chunk::IsTypeDefinition() const
{
   return(  Is(CT_TYPE)
         || Is(CT_PTR_TYPE)
         || Is(CT_BYREF)
         || Is(CT_DC_MEMBER)
         || Is(CT_QUALIFIER)
         || Is(CT_STRUCT)
         || Is(CT_ENUM)
         || Is(CT_UNION));
}


inline bool Chunk::IsWord() const
{
   return(  Len() >= 1
         && CharTable::IsKw1(str[0]));
}


inline bool Chunk::IsNullable() const
{
   return(  language_is_set(LANG_CS | LANG_VALA)
         && Len() == 1
         && str[0] == '?');
}


inline bool Chunk::IsAddress() const
{
   if (  IsNotNullChunk()
      && (  Is(CT_BYREF)
         || (  Len() == 1
            && str[0] == '&'
            && IsNot(CT_OPERATOR_VAL))))
   {
      Chunk *prevc = GetPrev();

      if (  flags.test(PCF_IN_TEMPLATE)
         && (  prevc->Is(CT_COMMA)
            || prevc->Is(CT_ANGLE_OPEN)))
      {
         return(false);
      }
      return(true);
   }
   return(false);
}


inline bool Chunk::IsMsRef() const
{
   return(  language_is_set(LANG_CPP)
         && Len() == 1
         && str[0] == '^'
         && IsNot(CT_OPERATOR_VAL));
}


inline bool Chunk::IsPointerOperator() const
{
   return(  IsStar()
         || IsAddress()
         || IsMsRef()
         || IsNullable());
}


inline bool Chunk::IsPointerOrReference() const
{
   return(  IsPointerOperator()
         || Is(CT_BYREF));
}


//! Check to see if there is a newline between the two chunks
bool chunk_is_newline_between(Chunk *start, Chunk *end);


inline bool Chunk::IsBraceOpen() const
{
   return(  Is(CT_BRACE_OPEN)
         || Is(CT_VBRACE_OPEN));
}


inline bool Chunk::IsBraceClose() const
{
   return(  Is(CT_BRACE_CLOSE)
         || Is(CT_VBRACE_CLOSE));
}


inline bool Chunk::IsParenOpen() const
{
   return(  Is(CT_PAREN_OPEN)
         || Is(CT_SPAREN_OPEN)
         || Is(CT_TPAREN_OPEN)
         || Is(CT_FPAREN_OPEN)
         || Is(CT_LPAREN_OPEN));
}


inline bool Chunk::IsParenClose() const
{
   return(  Is(CT_PAREN_CLOSE)
         || Is(CT_SPAREN_CLOSE)
         || Is(CT_TPAREN_CLOSE)
         || Is(CT_FPAREN_CLOSE));
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

   if (tmp->Is(CT_COMMENT_CPP))
   {
      return(false);
   }
   return(chunk_same_preproc(tmp, nl->GetNext()));
}


/**
 * Checks if a chunk points to the opening parentheses of a
 * for(...in...) loop in Objective-C.
 *
 * @return true  - the chunk is the opening parentheses of a for in loop
 */
static inline bool chunk_is_forin(Chunk *pc)
{
   if (  language_is_set(LANG_OC)
      && pc->Is(CT_SPAREN_OPEN))
   {
      Chunk *prev = pc->GetPrevNcNnl();

      if (prev->Is(CT_FOR))
      {
         Chunk *next = pc;

         while (  next->IsNotNullChunk()
               && next->IsNot(CT_SPAREN_CLOSE)
               && next->IsNot(CT_IN))
         {
            next = next->GetNextNcNnl();
         }

         if (next->Is(CT_IN))
         {
            return(true);
         }
      }
   }
   return(false);
}


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


inline bool Chunk::IsEnum() const
{
   return(  Is(CT_ENUM)
         || Is(CT_ENUM_CLASS));
}


#define SetType(tt)          SetTypeReal((tt), __unqualified_func__, __LINE__)


#define SetParentType(tt)    SetParentTypeReal((tt), __unqualified_func__, __LINE__)


void chunk_flags_set_real(Chunk *pc, pcf_flags_t clr_bits, pcf_flags_t set_bits);


#define chunk_flags_upd(pc, cc, ss)    chunk_flags_set_real((pc), (cc), (ss))


#define chunk_flags_set(pc, ss)        chunk_flags_set_real((pc), {}, (ss))


#define chunk_flags_clr(pc, cc)        chunk_flags_set_real((pc), (cc), {})


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
