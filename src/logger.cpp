/**
 * @file logger.cpp
 *
 * Functions to do logging.
 *
 * If a log statement ends in a newline, the current log is ended.
 * When the log severity changes, an implicit newline is inserted.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#include "logger.h"

#include <cstdio>
#include <stdarg.h>
#include "unc_ctype.h"


/** Private log structure */
struct log_buf
{
   log_buf() : log_file(0), sev(0), in_log(0), buf_len(0), show_hdr(false)
   {
   }

   FILE       *log_file;
   log_sev_t  sev;
   int        in_log;
   char       buf[256];
   int        buf_len;
   log_mask_t mask;
   bool       show_hdr;
};
static struct log_buf g_log;


/**
 * Initializes the log subsystem - call this first.
 * This function sets the log stream and enables the top 3 sevs (0-2).
 *
 * @param log_file   NULL for stderr or the FILE stream for logs.
 */
void log_init(FILE *log_file)
{
   /* set the top 3 severities */
   logmask_set_all(g_log.mask, false);
   log_set_sev(0, true);
   log_set_sev(1, true);
   log_set_sev(2, true);

   g_log.log_file = (log_file != NULL) ? log_file : stderr;
}


/**
 * Show or hide the severity prefix "<1>"
 *
 * @param true=show  false=hide
 */
void log_show_sev(bool show)
{
   g_log.show_hdr = show;
}


/**
 * Returns whether a log severity is active.
 *
 * @param sev  The severity
 * @return     true/false
 */
bool log_sev_on(log_sev_t sev)
{
   return(logmask_test(g_log.mask, sev));
}


/**
 * Sets a log sev on or off
 *
 * @param sev  The severity
 * @return     true/false
 */
void log_set_sev(log_sev_t sev, bool value)
{
   logmask_set_sev(g_log.mask, sev, value);
}


/**
 * Sets the log mask
 *
 * @param mask The mask to copy
 */
void log_set_mask(const log_mask_t& mask)
{
   g_log.mask = mask;
}


/**
 * Gets the log mask
 *
 * @param mask Where to copy the mask
 */
void log_get_mask(log_mask_t& mask)
{
   mask = g_log.mask;
}


/**
 * Flushes the cached log text to the stream
 *
 * @param force_nl   Append NL if not present
 */
static void log_flush(bool force_nl)
{
   if (g_log.buf_len > 0)
   {
      if (force_nl && (g_log.buf[g_log.buf_len - 1] != '\n'))
      {
         g_log.buf[g_log.buf_len++] = '\n';
         g_log.buf[g_log.buf_len]   = 0;
      }
      if (fwrite(g_log.buf, g_log.buf_len, 1,g_log .log_file) != 1)
      {
         /* maybe we should log something to complain... =) */
      }

      g_log.buf_len = 0;
   }
}


/**
 * Starts the log statement by flushing if needed and printing the header
 *
 * @param sev  The log severity
 * @return     The number of bytes available
 */
static size_t log_start(log_sev_t sev)
{
   if (sev != g_log.sev)
   {
      if (g_log.buf_len > 0)
      {
         log_flush(true);
      }
      g_log.sev    = sev;
      g_log.in_log = false;
   }

   /* If not in a log, the buffer is empty. Add the header, if enabled. */
   if (!g_log.in_log && g_log.show_hdr)
   {
      g_log.buf_len = snprintf(g_log.buf, sizeof(g_log.buf), "<%d>", sev);
   }

   int cap = ((int)sizeof(g_log.buf) - 2) - g_log.buf_len;

   return((cap > 0) ? (size_t)cap : 0);
}


/**
 * Cleans up after a log statement by detecting whether the log is done,
 * (it ends in a newline) and possibly flushing the log.
 */
static void log_end(void)
{
   g_log.in_log = (g_log.buf[g_log.buf_len - 1] != '\n');
   if (!g_log.in_log || (g_log.buf_len > (int)(sizeof(g_log.buf) / 2)))
   {
      log_flush(false);
   }
}


/**
 * Logs a string of known length
 *
 * @param sev  The severity
 * @param str  The pointer to the string
 * @param len  The length of the string from strlen(str)
 */
void log_str(log_sev_t sev, const char *str, int len)
{
   if ((str == NULL) || (len <= 0) || !log_sev_on(sev))
   {
      return;
   }

   size_t cap = log_start(sev);
   if (cap > 0)
   {
      if (len > (int)cap)
      {
         len = cap;
      }
      memcpy(&g_log.buf[g_log.buf_len], str, len);
      g_log.buf_len           += len;
      g_log.buf[g_log.buf_len] = 0;
   }
   log_end();
}


/**
 * Logs a formatted string -- similiar to printf()
 *
 * @param sev     The severity
 * @param fmt     The format string
 * @param ...     Additional arguments
 */
void log_fmt(log_sev_t sev, const char *fmt, ...)
{
   va_list args;
   int     len;
   size_t  cap;

   if ((fmt == NULL) || !log_sev_on(sev))
   {
      return;
   }

   /* Some implementation of vsnprintf() return the number of characters
    * that would have been stored if the buffer was large enough instead of
    * the number of characters actually stored.
    */
   cap = log_start(sev);

   /* Add on the variable log parameters to the log string */
   va_start(args, fmt);
   len = vsnprintf(&g_log.buf[g_log.buf_len], cap, fmt, args);
   va_end(args);

   if (len > 0)
   {
      if (len > (int)cap)
      {
         len = cap;
      }
      g_log.buf_len           += len;
      g_log.buf[g_log.buf_len] = 0;
   }

   log_end();
}


/**
 * Dumps hex characters inline, no newlines inserted
 *
 * @param sev     The severity
 * @param data    The data to log
 * @param len     The number of bytes to log
 */
void log_hex(log_sev_t sev, const void *vdata, int len)
{
   const UINT8 *dat = (const UINT8 *)vdata;
   int         idx;
   char        buf[80];

   if ((vdata == NULL) || !log_sev_on(sev))
   {
      return;
   }

   idx = 0;
   while (len-- > 0)
   {
      buf[idx++] = to_hex_char(*dat >> 4);
      buf[idx++] = to_hex_char(*dat);
      dat++;

      if (idx >= (int)(sizeof(buf) - 3))
      {
         buf[idx] = 0;
         log_str(sev, buf, idx);
         idx = 0;
      }
   }

   if (idx > 0)
   {
      buf[idx] = 0;
      log_str(sev, buf, idx);
   }
}


/**
 * Logs a block of data in a pretty hex format
 * Numbers on the left, characters on the right, just like I like it.
 *
 * @param sev     The severity
 * @param data    The data to log
 * @param len     The number of bytes to log
 */
void log_hex_blk(log_sev_t sev, const void *data, int len)
{
   static char buf[80] = "nnn | XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX | cccccccccccccccc\n";
   const UINT8 *dat    = (const UINT8 *)data;
   int         idx;
   int         count;
   int         str_idx = 0;
   int         chr_idx = 0;
   int         tmp;
   int         total;

   if ((data == NULL) || (len <= 0) || !log_sev_on(sev))
   {
      return;
   }

   /*
    * Dump the specified number of bytes in hex, 16 byte per line by
    * creating a string and then calling log_str()
    */

   /* Loop through the data of the current iov */
   count = 0;
   total = 0;
   for (idx = 0; idx < len; idx++)
   {
      if (count == 0)
      {
         str_idx = 6;
         chr_idx = 56;

         buf[0] = to_hex_char(total >> 12);
         buf[1] = to_hex_char(total >> 8);
         buf[2] = to_hex_char(total >> 4);
      }

      tmp = dat[idx];

      buf[str_idx]     = to_hex_char(tmp >> 4);
      buf[str_idx + 1] = to_hex_char(tmp);
      str_idx         += 3;

      buf[chr_idx++] = unc_isprint(tmp) ? tmp : '.';

      total++;
      count++;
      if (count >= 16)
      {
         count = 0;
         log_str(sev, buf, 73);
      }
   }

   /*
   ** Print partial line if any
   */
   if (count != 0)
   {
      /* Clear out any junk */
      while (count < 16)
      {
         buf[str_idx]     = ' ';   /* MSB hex */
         buf[str_idx + 1] = ' ';   /* LSB hex */
         str_idx         += 3;

         buf[chr_idx++] = ' ';

         count++;
      }
      log_str(sev, buf, 73);
   }
}
