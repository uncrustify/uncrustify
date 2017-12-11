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

#include "compat.h"
#include "logger.h"
#include "uncrustify_types.h"
#include "unc_ctype.h"
#include "log_levels.h"
#include <cstdio>
#include <deque>
#include <stdarg.h>


struct log_fcn_info
{
   log_fcn_info(const char *name_, int line_)
      : name(name_)
      , line(line_)
   {
   }

   const char *name;
   int        line;
};
static std::deque<log_fcn_info> g_fq;

//! Private log structure
struct log_buf
{
   log_buf()
      : log_file(nullptr)
      , sev(LSYS)
      , in_log(0)
      , buf_len(0)
      , show_hdr(false)
   {
   }

   FILE       *log_file;  //! file where the log messages are stored into
   log_sev_t  sev;        //! log level determines which messages are logged
   int        in_log;     //! flag indicates if a log operation is going on
   char       buf[256];   //! buffer holds the log message
   size_t     buf_len;    //! number of characters currently stored in buffer
   log_mask_t mask;
   bool       show_hdr;   //! flag determine if a header gets added to log message
};
static struct log_buf g_log;


/**
 * Starts the log statement by flushing if needed and printing the header
 *
 * @param sev  The log severity
 *
 * @return The number of bytes available
 */
static size_t log_start(log_sev_t sev);


/**
 * Cleans up after a log statement by detecting whether the log is done,
 * (it ends in a newline) and possibly flushing the log.
 */
static void log_end(void);


/**
 * Initializes the log subsystem - call this first.
 * This function sets the log stream and enables the top 3 sevs (0-2).
 *
 * @param log_file  NULL for stderr or the FILE stream for logs.
 */
void log_init(FILE *log_file)
{
   // set the top 3 severities
   logmask_set_all(g_log.mask, false);
   log_set_sev(LSYS, true);
   log_set_sev(LERR, true);
   log_set_sev(LWARN, true);

   g_log.log_file = (log_file != nullptr) ? log_file : stderr;
}


void log_show_sev(bool show)
{
   g_log.show_hdr = show;
}


bool log_sev_on(log_sev_t sev)
{
   return(logmask_test(g_log.mask, sev));
}


void log_set_sev(log_sev_t sev, bool value)
{
   logmask_set_sev(g_log.mask, sev, value);
}


void log_set_mask(const log_mask_t &mask)
{
   g_log.mask = mask;
}


void log_get_mask(log_mask_t &mask)
{
   mask = g_log.mask;
}


void log_flush(bool force_nl)
{
   if (g_log.buf_len > 0)
   {
      if (force_nl && g_log.buf[g_log.buf_len - 1] != '\n')
      {
         g_log.buf[g_log.buf_len++] = '\n';
         g_log.buf[g_log.buf_len]   = 0;
      }
      if (fwrite(g_log.buf, g_log.buf_len, 1, g_log.log_file) != 1)
      {
         // maybe we should log something to complain... =)
      }

      g_log.buf_len = 0;
   }
}


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

   // If not in a log, the buffer is empty. Add the header, if enabled.
   if (!g_log.in_log && g_log.show_hdr)
   {
      g_log.buf_len = static_cast<size_t>(snprintf(g_log.buf, sizeof(g_log.buf), "<%d>", sev));
   }

   size_t cap = (sizeof(g_log.buf) - 2) - g_log.buf_len;

   return((cap > 0) ? cap : 0);
}


static void log_end(void)
{
   g_log.in_log = (g_log.buf[g_log.buf_len - 1] != '\n');
   if (  !g_log.in_log
      || (g_log.buf_len > static_cast<int>(sizeof(g_log.buf) / 2)))
   {
      log_flush(false);
   }
}


void log_str(log_sev_t sev, const char *str, size_t len)
{
   if (  str == nullptr
      || len == 0
      || !log_sev_on(sev))
   {
      return;
   }

   size_t cap = log_start(sev);
   if (cap > 0)
   {
      if (len > cap)
      {
         len = cap;
      }
      memcpy(&g_log.buf[g_log.buf_len], str, len);
      g_log.buf_len           += len;
      g_log.buf[g_log.buf_len] = 0;
   }
   log_end();
}


void log_fmt(log_sev_t sev, const char *fmt, ...)
{
   if (fmt == nullptr || !log_sev_on(sev))
   {
      return;
   }

   // Issue #1203
   if (strlen(fmt) == 0)
   {
      return;
   }

#define BUFFERLENGTH    200
   char         buf[BUFFERLENGTH];
   // it MUST be a 'unsigned int' variable to be runable under windows
   unsigned int length = strlen(fmt);
   if (length >= BUFFERLENGTH)
   {
      fprintf(stderr, "FATAL: The variable 'buf' is not big enought:\n");
      fprintf(stderr, "   it should be bigger as = %u\n", length);
      exit(EX_SOFTWARE);
   }
   memcpy(buf, fmt, length);
   buf[length] = 0;
   convert_log_zu2lu(buf);

   /* Some implementation of vsnprintf() return the number of characters
    * that would have been stored if the buffer was large enough instead of
    * the number of characters actually stored.
    *
    * this gets the number of characters that fit into the log buffer
    */
   size_t cap = log_start(sev);

   // Add on the variable log parameters to the log string
   va_list args;        // determine list of arguments ...
   va_start(args, fmt); //  ... that follow after parameter fmt
   size_t  len = static_cast<size_t>(vsnprintf(&g_log.buf[g_log.buf_len], cap, fmt, args));
   va_end(args);

   if (len > 0)
   {
      if (len > cap)
      {
         len = cap;
      }
      g_log.buf_len           += len;
      g_log.buf[g_log.buf_len] = 0;
   }

   log_end();
} // log_fmt


void log_hex(log_sev_t sev, const void *vdata, size_t len)
{
   if (vdata == nullptr || !log_sev_on(sev))
   {
      return;
   }

   char        buf[80];
   const UINT8 *dat = static_cast<const UINT8 *>(vdata);
   size_t      idx  = 0;
   while (len-- > 0)
   {
      buf[idx++] = to_hex_char(*dat >> 4);
      buf[idx++] = to_hex_char(*dat);
      dat++;

      if (idx >= (sizeof(buf) - 3))
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


void log_hex_blk(log_sev_t sev, const void *data, size_t len)
{
   if (data == nullptr || !log_sev_on(sev))
   {
      return;
   }

   static char buf[80] = "nnn | XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX | cccccccccccccccc\n";
   const UINT8 *dat    = static_cast<const UINT8 *>(data);
   int         str_idx = 0;
   int         chr_idx = 0;

   /*
    * Dump the specified number of bytes in hex, 16 byte per line by
    * creating a string and then calling log_str()
    */

   // Loop through the data of the current iov
   int count = 0;
   int total = 0;
   for (size_t idx = 0; idx < len; idx++)
   {
      if (count == 0)
      {
         str_idx = 6;
         chr_idx = 56;

         buf[0] = to_hex_char(total >> 12);
         buf[1] = to_hex_char(total >> 8);
         buf[2] = to_hex_char(total >> 4);
      }

      int tmp = dat[idx];

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

   // Print partial line if any
   if (count != 0)
   {
      // Clear out any junk
      while (count < 16)
      {
         buf[str_idx]     = ' ';   // MSB hex
         buf[str_idx + 1] = ' ';   // LSB hex
         str_idx         += 3;

         buf[chr_idx++] = ' ';

         count++;
      }
      log_str(sev, buf, 73);
   }
} // log_hex_blk


log_func::log_func(const char *name, int line)
{
   g_fq.push_back(log_fcn_info(name, line));
}


log_func::~log_func()
{
   g_fq.pop_back();
}


void log_func_call(int line)
{
   // REVISIT: pass the __func__ and verify it matches the top entry?
   if (!g_fq.empty())
   {
      g_fq.back().line = line;
   }
}


void log_func_stack(log_sev_t sev, const char *prefix, const char *suffix, size_t skip_cnt)
{
   UNUSED(skip_cnt);

   if (prefix)
   {
      LOG_FMT(sev, "%s", prefix);
   }
#ifdef DEBUG
   const char *sep      = "";
   size_t     g_fq_size = g_fq.size();
   size_t     begin_with;
   if (g_fq_size > (skip_cnt + 1))
   {
      begin_with = g_fq_size - (skip_cnt + 1);
      for (size_t idx = begin_with; idx != 0; idx--)
      {
         LOG_FMT(sev, "%s %s:%d", sep, g_fq[idx].name, g_fq[idx].line);
         sep = ",";
      }
      LOG_FMT(sev, "%s %s:%d", sep, g_fq[0].name, g_fq[0].line);
   }
#else
   LOG_FMT(sev, "-DEBUG NOT SET-");
#endif
   if (suffix)
   {
      LOG_FMT(sev, "%s", suffix);
   }
}
