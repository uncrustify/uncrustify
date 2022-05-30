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
#include "log_levels.h"

#include <bitset>
#include <cstring>     // memset()

/*
 * the test suite Coveralls: https://coveralls.io
 * will complains because these functions are only
 * used at development time.
 * Don't worry about unused lines for the functions:
 *   logmask_test
 *   logmask_set_all
 */

//! A simple array of 256 bits
typedef std::bitset<256> log_mask_t;


/**
 * Tests whether a sev bit is set in the mask
 *
 * @param mask  log mask to evaluate
 * @param sev   severity bit to check
 *
 * @return true (is set) or false (not set)
 */
static inline bool logmask_test(const log_mask_t &mask, log_sev_t sev)
{
   return(mask.test(sev));
}


/**
 * Sets a set bit in the mask
 *
 * @param mask   log mask to modify
 * @param sev    The severity to check
 * @param value  true (set bit) or false (clear bit)
 */
static inline void logmask_set_sev(log_mask_t &mask, log_sev_t sev, bool value)
{
   mask.set(sev, value);
}


/**
 * Sets all bits to the same value
 *
 * @param mast   log mask to operate on
 * @param value  true (set bit) or false (clear bit)
 */
static inline void logmask_set_all(log_mask_t &mask, bool value)
{
   if (value)
   {
      mask.set();
   }
   else
   {
      mask.reset();
   }
}


#ifdef DEVELOP_ONLY
/**
 * Convert a logmask into a string.
 * The string is a comma-delimited list of severities.
 * Example: 1,3,5-10
 *
 * @param mask  the mask to convert
 * @param buf   the buffer to hold the string
 * @param size  the size of the buffer
 *
 * @return buf (pass through)
 */
char *logmask_to_str(const log_mask_t &mask, char *buf, int size);

#endif /* DEVELOP_ONLY */


/**
 * Parses a string into a log severity
 *
 * @param str   string to parse
 * @param mask  bit mask to populate with log levels
 */
void logmask_from_string(const char *str, log_mask_t &mask);


#endif /* LOGMASK_H_INCLUDED */
