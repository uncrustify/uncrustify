/**
 * @file same_func_call_params.h
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef ALIGN_SAME_FUNC_CALL_PARAMS_H_INCLUDED
#define ALIGN_SAME_FUNC_CALL_PARAMS_H_INCLUDED

#include <deque>

class Chunk;

void align_params(Chunk *start, std::deque<Chunk *> &chunks);
void align_same_func_call_params();

#endif /* ALIGN_SAME_FUNC_CALL_PARAMS_H_INCLUDED */
