/**
 * @file scan_ib_line.h
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef SCAN_IB_LINE_H_INCLUDED
#define SCAN_IB_LINE_H_INCLUDED

#include "uncrustify_types.h"

/**
 * Scans a line for stuff to align on.
 *
 * We trigger on BRACE_OPEN, FPAREN_OPEN, ASSIGN, and COMMA.
 * We want to align the NEXT item.
 */
chunk_t *scan_ib_line(chunk_t *start, bool first_pass);

#endif /* SCAN_IB_LINE_H_INCLUDED */
