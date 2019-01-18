/**
 * @file align_func_proto.h
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_FUNC_PROTO_H_INCLUDED
#define ALIGN_FUNC_PROTO_H_INCLUDED

#include "chunk_list.h"

//! Aligns all function prototypes in the file.
void align_func_proto(size_t span);

#endif /* ALIGN_FUNC_PROTO_H_INCLUDED */
