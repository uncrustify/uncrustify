/**
 * @file typedefs.h
 *
 * @author  Guy Maurel
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_TYPEDEFS_H_INCLUDED
#define ALIGN_TYPEDEFS_H_INCLUDED

#include <cstddef>

/**
 * Aligns simple typedefs that are contained on a single line each.
 * This should be called after the typedef target is marked as a type.
 *
 * typedef int        foo_t;
 * typedef char       bar_t;
 * typedef const char cc_t;
 */
void align_typedefs(size_t span);

#endif /* ALIGN_TYPEDEFS_H_INCLUDED */
