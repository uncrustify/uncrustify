/**
 * @file nl_handle_define.h
 * prototype for nl_handle_define.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NL_HANDLE_DEFINE_H_INCLUDED
#define NL_HANDLE_DEFINE_H_INCLUDED

#include "chunk.h"
#include "mark_change.h"


#define MARK_CHANGE()    mark_change(__func__, __LINE__)


//! Find the next newline or nl_cont
void nl_handle_define(Chunk *pc);


#endif /* NL_HANDLE_DEFINE_H_INCLUDED */
