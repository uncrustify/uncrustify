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

#include "compat.h"

#include <cstdarg>                   // to get va_start, va_end


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
      bufX.clear();
      bufX.resize(256);
   }

   FILE              *log_file; //! file where the log messages are stored into
   log_sev_t         sev;       //! log level determines which messages are logged
   int               in_log;    //! flag indicates if a log operation is going on
   size_t            buf_len;   //! number of characters currently stored in buffer
   std::vector<char> bufX;      //! buffer holds the log message
   log_mask_t        mask;
   bool              show_hdr;  //! flag determine if a header gets added to log message
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
static void log_end();


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
      if (  force_nl
         && g_log.bufX[g_log.buf_len - 1] != '\n')
      {
         g_log.bufX[g_log.buf_len++] = '\n';
         g_log.bufX[g_log.buf_len]   = 0;
      }
      size_t retlength = fwrite(&g_log.bufX[0], g_log.buf_len, 1, g_log.log_file);

      if (retlength != 1)
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
   if (  !g_log.in_log
      && g_log.show_hdr)
   {
      g_log.buf_len = static_cast<size_t>(snprintf(&g_log.bufX[0], g_log.bufX.size(), "<%d>", sev));
   }
   size_t cap = (g_log.bufX.size() - 2) - g_log.buf_len;

   return((cap > 0) ? cap : 0);
}


static void log_end()
{
   g_log.in_log = (g_log.bufX[g_log.buf_len - 1] != '\n');

   if (  !g_log.in_log
      || (g_log.buf_len > (g_log.bufX.size() / 2)))
   {
      log_flush(false);
   }
}


void log_fmt(log_sev_t sev, const char *fmt, ...)
{
   if (  fmt == nullptr
      || !log_sev_on(sev))
   {
      return;
   }
   // Issue #1203
   unsigned int length = strlen(fmt);

   if (length == 0)
   {
      return;
   }
   // the value of buffer_length is experimental
   const int buffer_length = 40000;
   char      buf[buffer_length];

   // it MUST be a 'unsigned int' variable to be runable under windows

   if (length >= buffer_length)
   {
      fprintf(stderr, "FATAL(1): The variable 'buf' is not big enough:\n");
      fprintf(stderr, "   it should be bigger as %u\n", length);
      fprintf(stderr, "Please make a report.\n");
      fprintf(stderr, "For the buffer: %s\n", fmt);
      exit(EX_SOFTWARE);
   }
   memcpy(buf, fmt, length);
   buf[length] = 0;
   convert_log_zu2lu(buf);

   while (true)
   {
      /* Some implementation of vsnprintf() return the number of characters
       * that would have been stored if the buffer was large enough instead of
       * the number of characters actually stored.
       *
       * this gets the number of characters that fit into the log buffer
       */
      size_t  cap = log_start(sev);
      // Add on the variable log parameters to the log string
      va_list args;        // determine list of arguments ...
      va_start(args, fmt);
      size_t  which  = g_log.buf_len;
      char    *where = &g_log.bufX[which];
      size_t  lenX   = static_cast<size_t>(vsnprintf(where, cap, buf, args));
      va_end(args);

      if (lenX > 0)
      {
         // The functions snprintf() and vsnprintf() do not  write  more  than  size  bytes
         // (including  the terminating null byte ('\0')).  If the output was truncated due
         // to this limit, then the return value is the number of characters (excluding the
         // terminating  null  byte)  which  would have been written to the final string if
         // enough space had been available.  Thus, a return value of size  or  more  means
         // that the output was truncated.
         if (lenX > cap)
         {
            size_t bufXLength = g_log.bufX.size();
            size_t X          = bufXLength * 2;

            if (X >= buffer_length)
            {
               fprintf(stderr, "FATAL(2): The variable 'buf' is not big enough:\n");
               fprintf(stderr, "   it should be bigger as %zu\n", X);
               fprintf(stderr, "Please make a report.\n");
               fprintf(stderr, "For the buffer: %s\n", fmt);
               exit(EX_SOFTWARE);
            }
            g_log.bufX.resize(X);
         }
         else
         {
            g_log.buf_len            += lenX;
            g_log.bufX[g_log.buf_len] = 0;
            break;
         }
      }
   }
   log_end();
} // log_fmt


log_func::log_func(const char *name, int line)
{
   g_fq.push_back(log_fcn_info(name, line));
}


log_func::~log_func()
{
   g_fq.pop_back();
}


void log_func_stack(log_sev_t sev, const char *prefix, const char *suffix, size_t skip_cnt)
{
   UNUSED(skip_cnt);

   if (prefix != nullptr)
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

   if (suffix != nullptr)
   {
      LOG_FMT(sev, "%s", suffix);
   }
}


const char *get_unqualified_func_name(const char *func)
{
   /**
    * we look for the last ':' character;
    */
   for (auto i = strlen(func); i > 0; --i)
   {
      if (func[i - 1] == ':')
      {
         /**
          * function name is qualified, so return the
          * unqualified portion
          */
         return(func + i);
      }
   }

   /**
    * input function name is unqualified
    */

   return(func);
}
