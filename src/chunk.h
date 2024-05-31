/**
 * @file chunk.h
 * Manages and navigates the list of chunks.
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#ifndef CHUNK_LIST_H_INCLUDED
#define CHUNK_LIST_H_INCLUDED

#include "uncrustify_types.h"
// necessary to not sort it
#include "char_table.h"
#include "language_names.h"
#include "language_tools.h"


static constexpr int ANY_LEVEL = -1;


/**
 * Specifies which chunks should/should not be found.
 * ALL (default)
 *  - return the true next/prev
 *
 * PREPROC
 *  - If not in a preprocessor, skip over any encountered preprocessor stuff
 *  - If in a preprocessor, fail to leave (return Chunk::NullChunkPtr)
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


class ChunkListManager;

// This is the main type of this program
class Chunk
{
   friend ChunkListManager;

public:
   //! constructors
   Chunk(bool null_c = false);                    // default
   Chunk(const Chunk &o);                         // !!! partial copy: chunk is not linked to others

   Chunk &operator=(const Chunk &o);              // !!! partial copy: chunk is not linked to others

   //! whether this is a null Chunk or not
   bool IsNullChunk() const { return(m_nullChunk); }
   bool IsNotNullChunk() const { return(!m_nullChunk); }

   //! sets all elements of the struct to their default value
   void Reset();


   // --------- Access methods

   /**
    * @brief returns the type of the chunk
    */
   E_Token GetType() const;

   /**
    * @brief Sets the chunk type
    * @param token the type to set
    */
   void SetType(const E_Token token);

   /**
    * @brief Returns the parent type of the chunk
    */
   E_Token GetParentType() const;

   /**
    * @brief Sets the type of the parent chunk
    * @param token the type to set
    */
   void SetParentType(const E_Token token);

   /**
    * @brief Returns the parent of the chunk
    */
   Chunk *GetParent() const;

   /**
    * @brief Sets the parent of the chunk
    * @param parent the parent chunk to set
    */
   void SetParent(Chunk *parent);

   /**
    * @brief Returns the alignment data of the chunk as a const reference
    */
   const AlignmentData &GetAlignData() const;

   /**
    * @brief Returns the alignment data of the chunk as a modifiable reference
    */
   AlignmentData &AlignData();

   /**
    * @brief Returns the indentation data of the chunk as a const reference
    */
   const IndentationData &GetIndentData() const;

   /**
    * @brief Returns the indentation data of the chunk as a modifiable reference
    */
   IndentationData &IndentData();

   /**
    * @brief Returns the text data of the chunk as a const reference
    */
   const UncText &GetStr() const;

   /**
    * @brief Returns the text data of the chunk as a modifiable reference
    */
   UncText &Str();

   /**
    * @brief returns the number of characters in the chunk text	string
    */
   size_t Len() const;

   /**
    * @brief returns the content of the chunk text as C string
    */
   const char *Text() const;

   /**
    * Returns a filled up (if necessary) copy of the first chars of the Text() string
    */
   const char *ElidedText(char *for_the_copy) const;

   /**
    * @brief Returns the tracking data of the chunk as a const reference
    */
   const TrackList *GetTrackingData() const;

   /**
    * @brief Returns the tracking data of the chunk as a modifiable reference
    */
   TrackList * &TrackingData();

   /**
    * @brief Returns the type of the parent chunk
    */
   E_Token GetTypeOfParent() const;

   /**
    * @brief Returns the chunk flags
    */
   PcfFlags GetFlags() const;

   /**
    * @brief Sets the chunk flags
    * @param flags the new chunk flags
    */
   void SetFlags(PcfFlags flags);

   /**
    * @brief Tests if some chunk flags are set
    * @param flags the flag bits to test
    * @return true if the specified bits are set, false otherwise
    */
   bool TestFlags(PcfFlags flags) const;

   /**
    * @brief Resets some of the chunk flag bits
    * @param resetBits the flag bits to reset
    */
   void ResetFlagBits(PcfFlags resetBits);

   /**
    * @brief Sets some of the chunk flag bits
    * @param setBits the flag bits to set
    */
   void SetFlagBits(PcfFlags setBits);

   /**
    * @brief Sets and reset some of the chunk flag bits
    * @param resetBits the flag bits to reset
    * @param setBits the flag bits to set
    */
   void UpdateFlagBits(PcfFlags resetBits, PcfFlags setBits);

   /**
    * @brief Returns the line number of the chunk in the input file
    */
   size_t GetOrigLine() const;

   /**
    * @brief Sets the line number of the chunk in the input file
    * @param line the line number of the chunk
    */
   void SetOrigLine(size_t line);

   /**
    * @brief Returns the column number of the chunk in the input file
    */
   size_t GetOrigCol() const;

   /**
    * @brief Sets the column number of the chunk in the input file
    * @param col the column number of the chunk
    */
   void SetOrigCol(size_t col);

   /**
    * @brief Returns the end column number of the chunk in the input file
    */
   size_t GetOrigColEnd() const;

   /**
    * @brief Sets the end column number of the chunk in the input file
    * @param col the end column number of the chunk
    */
   void SetOrigColEnd(size_t col);

   /**
    * @brief Returns the position of the whitespace before this chunk
    */
   size_t GetOrigPrevSp() const;

   /**
    * @brief Sets the position of the whitespace before this chunk
    * @param col the end column number of the chunk in the input file
    */
   void SetOrigPrevSp(size_t col);

   /**
    * @brief Returns the column of the chunk
    */
   size_t GetColumn() const;

   /**
    * @brief Sets the column of the chunk
    * @param col the column of the chunk
    */
   void SetColumn(size_t col);

   /**
    * @brief Returns the column indentation of the chunk
    */
   size_t GetColumnIndent() const;

   /**
    * @brief Sets the column indentation of the chunk
    * @param col the column indentation of the chunk
    */
   void SetColumnIndent(size_t col);

   /**
    * @brief Returns the number of newlines in a CT_NEWLINE chunk
    */
   size_t GetNlCount() const;

   /**
    * @brief Sets the number of newlines in a CT_NEWLINE chunk
    * @param cnt the number of newlines
    */
   void SetNlCount(size_t cnt);

   /**
    * @brief Returns the column of the newline entries
    */
   size_t GetNlColumn() const;

   /**
    * @brief Sets the column of the newline entries
    * @param col the number of the column
    */
   void SetNlColumn(size_t col);

   /**
    * @brief Returns the level of the chunk
    */
   size_t GetLevel() const;

   /**
    * @brief Sets the level of the chunk
    * @param col the level of the chunk
    */
   void SetLevel(size_t level);

   /**
    * @brief Returns the brace level of the chunk
    */
   size_t GetBraceLevel() const;

   /**
    * @brief Sets the brace level of the chunk
    * @param level the brace level of the chunk
    */
   void SetBraceLevel(size_t lvl);

   /**
    * @brief Returns the preprocessor level of the chunk
    */
   size_t GetPpLevel() const;

   /**
    * @brief Sets the preprocessor level of the chunk
    * @param level the preprocessor level of the chunk
    */
   void SetPpLevel(size_t lvl);

   /**
    * @brief Returns the after tab property of the chunk
    */
   bool GetAfterTab() const;

   /**
    * @brief Sets the after tab property of the chunk
    * @param afterTab the after tab property of the chunk
    */
   void SetAfterTab(bool afterTab);


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
    * @return Chunk::NullChunkPtr or the next chunk not in or part of square brackets
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
    * @param level  the level to match or ANY_LEVEL
    * @param scope   code region to search in
    * @return pointer to the next matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNextType(const E_Token type, const int level = ANY_LEVEL, const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the prev chunk of the given type at the level.
    * @param type    the type to look for
    * @param level  the level to match or ANY_LEVEL
    * @param scope   code region to search in
    * @return pointer to the prev matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevType(const E_Token type, int level = ANY_LEVEL, E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the next chunk that holds a given string at a given level.
    * @param str    string to search for
    * @param len    length of string
    * @param level  the level to match or ANY_LEVEL
    * @param scope  code region to search in
    * @return pointer to the next matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetNextString(const char *str, const size_t len, const int level, const E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the prev chunk that holds a given string at a given level.
    * @param str    string to search for
    * @param len    length of string
    * @param level  the level to match or ANY_LEVEL
    * @param scope  code region to search in
    * @return pointer to the prev matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetPrevString(const char *str, const size_t len, const int level, const E_Scope scope = E_Scope::ALL) const;

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
    * This just backs up until a newline or null chuck is hit.
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
    * @param level nesting level to match or ANY_LEVEL
    * @return pointer to the found chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *SearchTypeLevel(const E_Token type, const E_Scope scope = E_Scope::ALL, const E_Direction dir = E_Direction::FORWARD, const int level = ANY_LEVEL) const;

   /**
    * @brief searches a chunk that holds a specific string
    *
    * Traverses a chunk list either in forward or backward direction until a chunk
    * with the provided string was found. Additionally a nesting level can be
    * provided to narrow down the search.
    *
    * @param  str    string that searched chunk needs to have
    * @param  len    length of the string
    * @param  level  nesting level of the searched chunk, ignored when negative
    * @param  scope  code parts to consider for search
    * @param  dir    search direction
    * @return pointer to the found chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *SearchStringLevel(const char *str, const size_t len, const int level, const E_Scope scope = E_Scope::ALL, const E_Direction dir = E_Direction::FORWARD) const;

   /**
    * @brief returns the closing match for the current paren/brace/square.
    * @param scope chunk section to consider
    * @return pointer to the next matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetClosingParen(E_Scope scope = E_Scope::ALL) const;

   /**
    * @brief returns the opening match for the current paren/brace/square.
    * @param scope chunk section to consider
    * @return pointer to the prev matching chunk or Chunk::NullChunkPtr if no chunk was found
    */
   Chunk *GetOpeningParen(E_Scope scope = E_Scope::ALL) const;


   // --------- Is* functions

   /**
    * @brief checks whether the chunk is a specific token
    * @token the token to check for
    * @return true if the chunk type matches the specified token, false otherwise
    */
   bool Is(E_Token token) const;

   /**
    * @brief checks whether the chunk token name is a specific string
    * @param str string to compare token name with
    * @param caseSensitive whether to do a case sensitive or insensitive comparison
    * @return true if the chunk token name matches the specified string, false otherwise
    */
   bool IsString(const char *str, bool caseSensitive = true) const;

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
    * @brief checks whether the other chunk has the same preproc flags
    * @return true if the other chunk has the same preproc flags
    */
   bool IsSamePreproc(const Chunk *other) const;

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
   bool IsTypeAndLevel(const E_Token type, const int level) const;

   /**
    * @brief checks whether the chunk matches a given string and level
    * @param str    the expected string
    * @param len    length of the string
    * @param caseSensitive whether to do a case sensitive or insensitive comparison
    * @param level nesting level of the searched chunk, ignored when negative
    * @return true if the chunk matches a given string and level
    */
   bool IsStringAndLevel(const char *str, const size_t len, bool caseSensitive, const int level) const;

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
    * @brief checks if a chunk points to the opening parentheses of a
    * for (...in...) loop in Objective-C.
    * @return true  if the chunk is the opening parentheses of a for-in loop
    */
   bool IsOCForinOpenParen() const;

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
    * @brief checks whether the chunk is an enum or an enum class
    * @return true if the chunk is an enum or an enum class
    */
   bool IsEnum() const;

   /**
    * @brief checks whether the chunk is a class or a struct
    * @return true if the chunk is a class or a struct
    */
   bool IsClassOrStruct() const;

   /**
    * @brief checks whether the chunk is a class, struct or union
    * @return true if the chunk is a class, struct or union
    */
   bool IsClassStructOrUnion() const;

   /**
    * @brief checks whether the chunk is a class, enum, struct or union
    * @return true if the chunk is a class, enum, struct or union
    */
   bool IsClassEnumStructOrUnion() const;

   /**
    * @brief checks whether there is a newline between this chunk and the other
    * @return true if there is a newline between this chunk and the other
    */
   bool IsNewlineBetween(const Chunk *other) const;


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
    * @brief swaps the place of this chunk with the given one
    * @param other the other chunk
    */
   void Swap(Chunk *other);

   /**
    * @brief swaps the two lines that are started by the current chunk and the other chunk
    * @param other the other chunk
    */
   void SwapLines(Chunk *other);

   //!
   /**
    * @brief skips to the final word/type in a :: chain
    * @return pointer to the chunk after the final word/type in a :: chain
    */
   Chunk *SkipDcMember() const;

   /**
    * @brief compare the positions of the chunk with another one
    * @param other the other chunk
    * @return returns -1 if this chunk comes first, +1 if it comes after, or 0.
    */
   int ComparePosition(const Chunk *other) const;

   /**
    * Returns true if it is safe to delete the newline token.
    * The prev and next chunks must have the same PCF_IN_PREPROC flag AND
    * the newline can't be after a C++ comment.
    */
   bool SafeToDeleteNl() const;


protected:
   // --------- Protected util functions

   /**
    * @brief copy the values from another chunk.
    * @NOTE: this is a partial copy only: the chunk is not linked to others
    * @param o the chunk to copy from
    */
   void CopyFrom(const Chunk &o);

   /**
    * @brief add a copy of this chunk before/after the given position in a chunk list.
    * @note If pos is NullChunk, add the new chuck either at the head or tail of the
    * list based on the specified direction.
    * @param pos insert position in list
    * @param dir insert before or after the given position chunk
    * @return pointer to the newly added chunk
    */
   Chunk *CopyAndAdd(Chunk *pos, const E_Direction dir = E_Direction::FORWARD) const;

   /**
    * @brief set and/or clear the chunk flags
    * @param setBits the flag bits to set
    * @param resetBits the flag bits to reset
    */
   void SetResetFlags(PcfFlags resetBits, PcfFlags setBits);


   // --------- Data members
   E_Token         m_type;                  //! type of the chunk itself
   E_Token         m_parentType;            //! type of the parent chunk usually CT_NONE
   size_t          m_origLine;              //! line number of chunk in input file
   size_t          m_origCol;               //! column where chunk started in the input file, is always > 0
   size_t          m_origColEnd;            //! column where chunk ended in the input file, is always > 1
   size_t          m_origPrevSp;            //! whitespace before this token
   size_t          m_column;                //! column of the chunk
   size_t          m_columnIndent;          //! if 1st chunk on a line, set to the 'indent' column, which may
                                            //! be less than the real column used to indent with tabs
   size_t          m_nlCount;               //! number of newlines in CT_NEWLINE
   size_t          m_nlColumn;              //! column of the subsequent newline entries(all of them should have the same column)
   size_t          m_level;                 //! nest level in {, (, or [. Only to help vim command }
   size_t          m_braceLevel;            //! nest level in braces only
   size_t          m_ppLevel;               //! nest level in preprocessor
   bool            m_afterTab;              //! whether this token was after a tab

   PcfFlags        m_flags;                 //! see PCF_xxx
   AlignmentData   m_alignmentData;         //! alignment data of the chunk
   IndentationData m_indentationData;       //! indentation data of the chunk

   Chunk           *m_next;                 //! pointer to next chunk in list
   Chunk           *m_prev;                 //! pointer to previous chunk in list
   Chunk           *m_parent;               //! pointer to parent chunk (not always set)

   UncText         m_str;                   //! the token text
   TrackList       *m_trackingList;         //! for debugging purpose only


private:
   const bool m_nullChunk;                    //! true for null chunks


public:
   static Chunk        NullChunk;             //! Null Chunk
   static Chunk *const NullChunkPtr;          //! Pointer to the Null Chunk
};


inline Chunk::Chunk(bool null_c)
   : m_nullChunk(null_c)
{
   Reset();
}


inline Chunk::Chunk(const Chunk &o)
   : m_nullChunk(o.m_nullChunk)
{
   CopyFrom(o);
}


inline Chunk &Chunk::operator=(const Chunk &o)
{
   if (this != &o)
   {
      CopyFrom(o);
   }
   return(*this);
}


inline size_t Chunk::Len() const
{
   return(m_str.size());
}


inline const char *Chunk::Text() const
{
   return(m_str.c_str());
}


inline Chunk *Chunk::GetParent() const
{
   return(m_parent);
}


inline void Chunk::SetParent(Chunk *parent)
{
   if (this != parent)
   {
      m_parent = parent;
   }
}


inline const AlignmentData &Chunk::GetAlignData() const
{
   return(m_alignmentData);
}


inline AlignmentData &Chunk::AlignData()
{
   return(m_alignmentData);
}


inline const IndentationData &Chunk::GetIndentData() const
{
   return(m_indentationData);
}


inline IndentationData &Chunk::IndentData()
{
   return(m_indentationData);
}


inline const UncText &Chunk::GetStr() const
{
   return(m_str);
}


inline UncText &Chunk::Str()
{
   return(m_str);
}


inline const TrackList *Chunk::GetTrackingData() const
{
   return(m_trackingList);
}


inline TrackList * &Chunk::TrackingData()
{
   return(m_trackingList);
}


inline E_Token Chunk::GetType() const
{
   return(m_type);
}


inline E_Token Chunk::GetParentType() const
{
   return(m_parentType);
}


inline E_Token Chunk::GetTypeOfParent() const
{
   if (GetParent()->IsNullChunk())
   {
      return(CT_PARENT_NOT_SET);
   }
   return(GetParent()->GetType());
}


inline PcfFlags Chunk::GetFlags() const
{
   return(m_flags);
}


inline void Chunk::SetFlags(PcfFlags flags)
{
   m_flags = flags;
}


inline bool Chunk::TestFlags(PcfFlags flags) const
{
   return(m_flags.test(flags));
}


inline void Chunk::ResetFlagBits(PcfFlags resetBits)
{
   SetResetFlags(resetBits, PCF_NONE);
}


inline void Chunk::SetFlagBits(PcfFlags setBits)
{
   SetResetFlags(PCF_NONE, setBits);
}


inline void Chunk::UpdateFlagBits(PcfFlags resetBits, PcfFlags setBits)
{
   SetResetFlags(resetBits, setBits);
}


inline size_t Chunk::GetOrigLine() const
{
   return(m_origLine);
}


inline void Chunk::SetOrigLine(size_t line)
{
   m_origLine = line;
}


inline size_t Chunk::GetOrigCol() const
{
   return(m_origCol);
}


inline void Chunk::SetOrigCol(size_t col)
{
   m_origCol = col;
}


inline size_t Chunk::GetOrigColEnd() const
{
   return(m_origColEnd);
}


inline void Chunk::SetOrigColEnd(size_t col)
{
   m_origColEnd = col;
}


inline size_t Chunk::GetOrigPrevSp() const
{
   return(m_origPrevSp);
}


inline void Chunk::SetOrigPrevSp(size_t col)
{
   m_origPrevSp = col;
}


inline size_t Chunk::GetColumn() const
{
   return(m_column);
}


inline void Chunk::SetColumn(size_t col)
{
   m_column = col;
}


inline size_t Chunk::GetColumnIndent() const
{
   return(m_columnIndent);
}


inline void Chunk::SetColumnIndent(size_t col)
{
   m_columnIndent = col;
}


inline size_t Chunk::GetNlCount() const
{
   return(m_nlCount);
}


inline void Chunk::SetNlCount(size_t cnt)
{
   m_nlCount = cnt;
}


inline size_t Chunk::GetNlColumn() const
{
   return(m_nlColumn);
}


inline void Chunk::SetNlColumn(size_t col)
{
   m_nlColumn = col;
}


inline size_t Chunk::GetLevel() const
{
   return(m_level);
}


inline void Chunk::SetLevel(size_t level)
{
   m_level = level;
}


inline size_t Chunk::GetBraceLevel() const
{
   return(m_braceLevel);
}


inline void Chunk::SetBraceLevel(size_t lvl)
{
   m_braceLevel = lvl;
}


inline size_t Chunk::GetPpLevel() const
{
   return(m_ppLevel);
}


inline void Chunk::SetPpLevel(size_t lvl)
{
   m_ppLevel = lvl;
}


inline bool Chunk::GetAfterTab() const
{
   return(m_afterTab);
}


inline void Chunk::SetAfterTab(bool afterTab)
{
   m_afterTab = afterTab;
}


inline Chunk *Chunk::GetNextNl(const E_Scope scope) const
{
   return(Search(&Chunk::IsNewline, scope, E_Direction::FORWARD, true));
}


inline Chunk *Chunk::GetPrevNl(const E_Scope scope) const
{
   return(Search(&Chunk::IsNewline, scope, E_Direction::BACKWARD, true));
}


inline Chunk *Chunk::GetNextNnl(const E_Scope scope) const
{
   return(Search(&Chunk::IsNewline, scope, E_Direction::FORWARD, false));
}


inline Chunk *Chunk::GetPrevNnl(const E_Scope scope) const
{
   return(Search(&Chunk::IsNewline, scope, E_Direction::BACKWARD, false));
}


inline Chunk *Chunk::GetNextNc(const E_Scope scope) const
{
   return(Search(&Chunk::IsComment, scope, E_Direction::FORWARD, false));
}


inline Chunk *Chunk::GetPrevNc(const E_Scope scope) const
{
   return(Search(&Chunk::IsComment, scope, E_Direction::BACKWARD, false));
}


inline Chunk *Chunk::GetNextNcNnl(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentOrNewline, scope, E_Direction::FORWARD, false));
}


inline Chunk *Chunk::GetPrevNcNnl(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentOrNewline, scope, E_Direction::BACKWARD, false));
}


inline Chunk *Chunk::GetNextNcNnlNpp(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentNewlineOrPreproc, scope, E_Direction::FORWARD, false));
}


inline Chunk *Chunk::GetPrevNcNnlNpp(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentNewlineOrPreproc, scope, E_Direction::BACKWARD, false));
}


inline Chunk *Chunk::GetNextNppOrNcNnl(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentOrNewlineInPreproc, scope, E_Direction::FORWARD, false));
}


inline Chunk *Chunk::GetPrevNppOrNcNnl(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentOrNewlineInPreproc, scope, E_Direction::BACKWARD, false));
}


inline Chunk *Chunk::PpaGetNextNcNnl() const
{
   return(SearchPpa(&Chunk::IsCommentOrNewline, false));
}


inline Chunk *Chunk::GetNextNcNnlNet(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentNewlineOrEmptyText, scope, E_Direction::FORWARD, false));
}


inline Chunk *Chunk::GetPrevNcNnlNet(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentNewlineOrEmptyText, scope, E_Direction::BACKWARD, false));
}


inline Chunk *Chunk::GetPrevNcNnlNi(const E_Scope scope) const
{
   return(Search(&Chunk::IsCommentNewlineOrIgnored, scope, E_Direction::BACKWARD, false));
}


inline Chunk *Chunk::GetNextNisq(const E_Scope scope) const
{
   return(Search(&Chunk::IsSquareBracket, scope, E_Direction::FORWARD, false));
}


inline Chunk *Chunk::GetNextType(const E_Token type, const int level, const E_Scope scope) const
{
   return(SearchTypeLevel(type, scope, E_Direction::FORWARD, level));
}


inline Chunk *Chunk::GetPrevType(const E_Token type, const int level, const E_Scope scope) const
{
   return(SearchTypeLevel(type, scope, E_Direction::BACKWARD, level));
}


inline Chunk *Chunk::GetNextString(const char *str, const size_t len, const int level, const E_Scope scope) const
{
   return(SearchStringLevel(str, len, level, scope, E_Direction::FORWARD));
}


inline Chunk *Chunk::GetPrevString(const char *str, const size_t len, const int level, const E_Scope scope) const
{
   return(SearchStringLevel(str, len, level, scope, E_Direction::BACKWARD));
}


inline Chunk *Chunk::GetNextNvb(const E_Scope scope) const
{
   return(Search(&Chunk::IsVBrace, scope, E_Direction::FORWARD, false));
}


inline Chunk *Chunk::GetPrevNvb(const E_Scope scope) const
{
   return(Search(&Chunk::IsVBrace, scope, E_Direction::BACKWARD, false));
}


inline bool Chunk::IsTypeAndLevel(const E_Token type, const int level) const
{
   return(  (  level < 0
            || m_level == static_cast<size_t>(level))
         && m_type == type);
}


inline bool Chunk::Is(E_Token token) const
{
   return(  IsNotNullChunk()
         && m_type == token);
}


inline bool Chunk::IsString(const char *str, bool caseSensitive) const
{
   return(IsStringAndLevel(str, strlen(str), caseSensitive, ANY_LEVEL));
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
         && TestFlags(PCF_IN_PREPROC));
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
         && m_str[0] == '*'
         && IsNot(CT_OPERATOR_VAL));
}


inline bool Chunk::IsSemicolon() const
{
   return(  Is(CT_SEMICOLON)
         || Is(CT_VSEMICOLON));
}


inline bool Chunk::IsWord() const
{
   return(  Len() >= 1
         && CharTable::IsKw1(m_str[0]));
}


inline bool Chunk::IsNullable() const
{
   return(  (  language_is_set(lang_flag_e::LANG_CS)
            || language_is_set(lang_flag_e::LANG_VALA))
         && Len() == 1
         && m_str[0] == '?');
}


inline bool Chunk::IsMsRef() const
{
   return(  language_is_set(lang_flag_e::LANG_CPP)
         && Len() == 1
         && m_str[0] == '^'
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
         || Is(CT_FPAREN_OPEN)
         || Is(CT_LPAREN_OPEN)
         || Is(CT_PPAREN_OPEN)
         || Is(CT_RPAREN_OPEN)
         || Is(CT_SPAREN_OPEN)
         || Is(CT_TPAREN_OPEN));
}


inline bool Chunk::IsParenClose() const
{
   return(  Is(CT_PAREN_CLOSE)
         || Is(CT_FPAREN_CLOSE)
         || Is(CT_LPAREN_CLOSE)
         || Is(CT_PPAREN_CLOSE)
         || Is(CT_RPAREN_CLOSE)
         || Is(CT_SPAREN_CLOSE)
         || Is(CT_TPAREN_CLOSE));
}


inline bool Chunk::IsSamePreproc(const Chunk *other) const
{
   return(  IsNotNullChunk()
         && other->IsNotNullChunk()
         && (TestFlags(PCF_IN_PREPROC) == other->TestFlags(PCF_IN_PREPROC)));
}


inline bool Chunk::SafeToDeleteNl() const
{
   Chunk *tmp = GetPrev();

   if (tmp->Is(CT_COMMENT_CPP))
   {
      return(false);
   }
   return(tmp->IsSamePreproc(GetNext()));
}


inline bool Chunk::IsEnum() const
{
   return(  Is(CT_ENUM)
         || Is(CT_ENUM_CLASS));
}


inline bool Chunk::IsClassOrStruct() const
{
   return(  Is(CT_CLASS)
         || Is(CT_STRUCT));
}


inline bool Chunk::IsClassStructOrUnion() const
{
   return(  IsClassOrStruct()
         || Is(CT_UNION));
}


inline bool Chunk::IsClassEnumStructOrUnion() const
{
   return(  IsClassStructOrUnion()
         || IsEnum());
}


inline Chunk *Chunk::CopyAndAddAfter(Chunk *ref) const
{
   return(CopyAndAdd(ref, E_Direction::FORWARD));
}


inline Chunk *Chunk::CopyAndAddBefore(Chunk *ref) const
{
   return(CopyAndAdd(ref, E_Direction::BACKWARD));
}


// shift all the tokens in this line to the right  Issue #3236
void shift_the_rest_of_the_line(Chunk *first);


#endif /* CHUNK_LIST_H_INCLUDED */
