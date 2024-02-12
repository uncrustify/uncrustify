/**
 * @file mark_change.h
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#ifndef MARK_CHANGE_H_INCLUDED
#define MARK_CHANGE_H_INCLUDED

#include "cstddef"

#define MARK_CHANGE()    mark_change(__func__, __LINE__)

void mark_change(const char *func, size_t line);

#endif /* MARK_CHANGE_H_INCLUDED */
