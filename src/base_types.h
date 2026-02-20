/**
 * @file base_types.h
 *
 * Defines some base types, includes config.h
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef BASE_TYPES_H_INCLUDED
#define BASE_TYPES_H_INCLUDED

#include "error_types.h"

#ifdef WIN32

#include "windows_compat.h"

#else // not WIN32

#include "config.h"

#define PATH_SEP    '/'

#define __STDC_FORMAT_MACROS

#endif /* ifdef WIN32 */

#include <cinttypes>

// and the ever-so-important array size macro
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)    (sizeof(x) / sizeof((x)[0]))
#endif


#endif /* BASE_TYPES_H_INCLUDED */
