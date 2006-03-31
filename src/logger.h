/**
 * @file logger.h
 *
 * Functions to do logging.
 * The macros will perform conditional logging. Use those.
 *
 * If a log statement ends in a newline, the current log is ended.
 * When the log severity changes, an implicit newline is inserted.
 *
 * $Id$
 */
#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#include "logmask.h"
#include <string.h>     /* strlen() */
#include <stdio.h>      /* FILE */


/**
 * Initializes the log subsystem - call this first.
 * This function sets the log stream and enables the top 3 sevs (0-2).
 *
 * @param log_file   NULL for stderr or the FILE stream for logs.
 */
void log_init(FILE *log_file);


/**
 * Show or hide the severity prefix "<1>"
 *
 * @param TRUE=show  FALSE=hide
 */
void log_show_sev(BOOL show);


/**
 * Returns whether a log severity is active.
 *
 * @param sev  The severity
 * @return     TRUE/FALSE
 */
BOOL log_sev_on(log_sev_t sev);


/**
 * Sets a log sev on or off
 *
 * @param sev  The severity
 * @return     TRUE/FALSE
 */
void log_set_sev(log_sev_t sev, BOOL value);


/**
 * Sets the log mask
 *
 * @param mask The mask to copy
 */
void log_set_mask(const log_mask_t *mask);


/**
 * Gets the log mask
 *
 * @param mask Where to copy the mask
 */
void log_get_mask(log_mask_t *mask);


/**
 * Logs a string of known length
 *
 * @param sev  The severity
 * @param str  The pointer to the string
 * @param len  The length of the string from strlen(str)
 */
void log_str(log_sev_t sev, const char *str, int len);

#define LOG_STR(sev, str, len) \
   do { if (log_sev_on(sev)) { log_str(sev, str, len); } } while (0)

#define LOG_STRING(sev, str) \
   do { if (log_sev_on(sev)) { log_str(sev, str, strlen(str)); } } while (0)


/**
 * Logs a formatted string -- similiar to printf()
 *
 * @param sev     The severity
 * @param fmt     The format string
 * @param ...     Additional arguments
 */
void log_fmt(log_sev_t sev, const char *fmt, ...) __attribute__((format(printf, 2, 3)));

#define LOG_FMT(sev, args...) \
   do { if (log_sev_on(sev)) { log_fmt(sev, ## args); } } while (0)


/**
 * Dumps hex characters inline, no newlines inserted
 *
 * @param sev     The severity
 * @param data    The data to log
 * @param len     The number of bytes to log
 */
void log_hex(log_sev_t sev, const void *vdata, int len);

#define LOG_HEX(sev, ptr, len) \
   do { if (log_active(sev)) { log_hex(sev, ptr, len); } } while (0)


/**
 * Logs a block of data in a pretty hex format
 * Numbers on the left, characters on the right, just like I like it.
 *
 * "nnn | XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX | ................"
 *  0     ^6                                            54^ ^56           72^
 *
 *  nnn is the line number or index/16
 *
 * @param sev     The severity
 * @param data    The data to log
 * @param len     The number of bytes to log
 */
void log_hex_blk(log_sev_t sev, const void *data, int len);

#define LOG_HEX_BLK(sev, ptr, len) \
   do { if (log_active(sev)) { log_hex_blk(sev, ptr, len); } } while (0)


/**
 * Returns the HEX digit for a low nibble in a number
 *
 * @param nibble  The nibble
 * @return        '0', '1', '2', '3', '4', '5', '6', '7',
 *                '8', '9', 'a', 'b', 'c', 'd', 'e', or 'f'
 */
static_inline char to_hex_char(int nibble)
{
   const char *hex_string = "0123456789abcdef";

   return(hex_string[nibble & 0x0F]);
}


#endif   /* LOGGER_H_INCLUDED */

