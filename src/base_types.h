/**
 * @file base_types.h
 *
 * Defines some base types
 *
 * $Id$
 */

#ifndef BASE_TYPES_H_INCLUDED
#define BASE_TYPES_H_INCLUDED

#include <stdint.h>


/* some of my favorite aliases */

typedef char       CHAR;

typedef int8_t     INT8;
typedef int16_t    INT16;
typedef int32_t    INT32;
typedef int64_t    INT64;

typedef uint8_t    UINT8;
typedef uint16_t   UINT16;
typedef uint32_t   UINT32;
typedef uint64_t   UINT64;


/* and the good old SUCCESS/FAILURE */

#define SUCCESS     0
#define FAILURE     -1


/* and a nice macro to keep SlickEdit happy */

#define static_inline     static inline

/* and the ever-so-important array size macro */
#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x)     (sizeof(x) / sizeof((x)[0]))
#endif

#endif   /* BASE_TYPES_H_INCLUDED */

