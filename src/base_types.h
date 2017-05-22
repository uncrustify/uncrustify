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

#else /* not WIN32 */

#include "config.h"

#define PATH_SEP    '/'

#define __STDC_FORMAT_MACROS

#if defined HAVE_INTTYPES_H
#include <inttypes.h>
#else
#error "Don't know where int8_t is defined"
#endif


/* some of my favorite aliases */

using CHAR = char;

using INT8  = int8_t;
using INT16 = int16_t;
using INT32 = int32_t;

using UINT8  = uint8_t;
using UINT16 = uint16_t;
using UINT32 = uint32_t;
using UINT64 = uint64_t;
#endif   /* ifdef WIN32 */


/* and a nice macro to keep SlickEdit happy */
#define static_inline    static inline

/* and the ever-so-important array size macro */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)    (sizeof(x) / sizeof((x)[0]))
#endif

#endif /* BASE_TYPES_H_INCLUDED */
