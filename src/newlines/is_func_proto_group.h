/**
 * @file is_func_proto_group.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef IS_FUNC_PROTO_GROUP_H_INCLUDED
#define IS_FUNC_PROTO_GROUP_H_INCLUDED

#include "token_enum.h"

class Chunk;

/**
 * Test if a chunk may be combined with a function prototype group.
 *
 * If nl_class_leave_one_liner_groups is enabled, a chunk may be combined with
 * a function prototype group if it is a one-liner inside a class body, and is
 * a definition of the same sort as surrounding prototypes. This checks against
 * either the function name, or the function closing brace.
 */
bool is_func_proto_group(Chunk *pc, E_Token one_liner_type);

#endif /* IS_FUNC_PROTO_GROUP_H_INCLUDED */
