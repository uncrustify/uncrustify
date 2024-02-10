/**
 * @file min_after.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_MIN_AFTER_H_INCLUDED
#define NEWLINES_MIN_AFTER_H_INCLUDED

#include "pcf_flags.h"
#include <cstddef>

class Chunk;

void newline_min_after(Chunk *ref, size_t count, E_PcfFlag flag);

#endif /* NEWLINES_MIN_AFTER_H_INCLUDED */
