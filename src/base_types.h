/**
 * @file base_types.h
 *
 * Defines some base types
 *
 * $Id: base_types.h,v 1.1 2006/02/11 02:22:59 bengardner Exp $
 */

#ifndef BASE_TYPES_H_INCLUDED
#define BASE_TYPES_H_INCLUDED

#include <stdint.h>


/* Define a nice boolean */
typedef enum
{
   FALSE = 0,
   TRUE  = 1,
} BOOL;

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


#endif   /* BASE_TYPES_H_INCLUDED */

