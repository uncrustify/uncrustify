/**
 * @file logmask.h
 *
 * Functions to manipulate a log severity mask.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef LOGMASK_H_INCLUDED
#define LOGMASK_H_INCLUDED

#include "base_types.h"
#include <bitset>

using namespace std;


typedef UINT8   log_sev_t;


/** A simple array of 256 bits */
typedef struct
{
   bitset<256> bits;   /* 256 levels */
} log_mask_t;


/**
 * Tests whether a sev bit is set in the mask
 *
 * @param sev  The severity to check
 * @return     true (is set) or false (not set)
 */
static_inline bool logmask_test(const log_mask_t *mask, log_sev_t sev)
{
	return mask->bits.test(sev);
}


/**
 * Sets a set bit in the mask
 *
 * @param sev     The severity to check
 * @param value   true (set bit) or false (clear bit)
 */
static_inline void logmask_set_sev(log_mask_t *mask, log_sev_t sev, bool value)
{
   if (value)
   {
	   mask->bits.set(sev);
   }
   else
   {
	   mask->bits.reset(sev);
   }
}


/**
 * Sets all bits to the same value
 *
 * @param value   true (set bit) or false (clear bit)
 */
static_inline void logmask_set_all(log_mask_t *mask, bool value)
{
   if (value)
   {
	   mask->bits.set();
   }
   else
   {
	   mask->bits.reset();
   }
}


/**
 * Convert a logmask into a string.
 * The string is a comma-delimited list of severities.
 * Example: 1,3,5-10
 *
 * @param mask the mask to convert
 * @param buf  the buffer to hold the string
 * @param size the size of the buffer
 * @return     buf (pass through)
 */
char *logmask_to_str(const log_mask_t *mask, char *buf, int size);


/**
 * Parses a string into a log severity
 *
 * @param str     The string to parse
 * @param mask    The mask to populate
 */
void logmask_from_string(const char *str, log_mask_t *mask);


#endif   /* LOGMASK_H_INCLUDED */
