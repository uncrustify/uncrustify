/**
 * @file scope_enum.h
 *
 * @author
 * @license GPL v2+
 * extract from chunk_list.h
 */

#ifndef SCOPE_ENUM_H_INCLUDED
#define SCOPE_ENUM_H_INCLUDED

/**
 * Specifies which chunks should/should not be found.
 * ALL (default)
 *  - return the true next/prev
 *
 * PREPROC
 *  - If not in a preprocessor, skip over any encountered preprocessor stuff
 *  - If in a preprocessor, fail to leave (return nullptr)
 */
enum class scope_e : unsigned int
{
   ALL,      //! search in all kind of chunks
   PREPROC,  //! search only in preprocessor chunks
};

#endif /* SCOPE_ENUM_H_INCLUDED */
