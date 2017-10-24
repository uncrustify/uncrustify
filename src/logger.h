/**
 * @file logger.h
 *
 * Functions to do logging.
 * The macros check whether the logsev is active before evaluating the
 * parameters.  Use them instead of the functions.
 *
 * If a log statement ends in a newline, the current log is ended.
 * When the log severity changes, an implicit newline is inserted.
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef LOGGER_H_INCLUDED
#define LOGGER_H_INCLUDED

#include "logmask.h"
#include <cstring>     // strlen()
#include <cstdio>      // FILE


/**
 * Initializes the log subsystem - call this first.
 * This function sets the log stream and enables the top 3 sevs (0-2).
 *
 * @param log_file  nullptr for stderr or the FILE stream for logs.
 */
void log_init(FILE *log_file);


/**
 * Show or hide the severity prefix "<1>"
 *
 * @param show  true=show, false=hide
 */
void log_show_sev(bool show);


/**
 * Returns whether a log severity is active.
 *
 * @param sev  severity log level
 *
 * @return true/false
 */
bool log_sev_on(log_sev_t sev);


/**
 * Sets a log sev on or off
 *
 * @param sev  severity log level to modify
 * @param val  new value for severity log level
 *
 * @return true/false
 */
void log_set_sev(log_sev_t sev, bool value);


/**
 * Sets the log mask
 *
 * @param mask  The mask to copy
 */
void log_set_mask(const log_mask_t &mask);


/**
 * Gets the log mask
 *
 * @param mask  Where to copy the mask
 */
void log_get_mask(log_mask_t &mask);


/**
 * Logs a string of known length
 *
 * @param sev  The severity
 * @param str  The pointer to the string
 * @param len  The length of the string from strlen(str)
 *
 * TODO call strlen internally instead of providing len
 */
void log_str(log_sev_t sev, const char *str, size_t len);


#define LOG_STR(sev, str, len)                           \
   do { if (log_sev_on(sev)) { log_str(sev, str, len); } \
   } while (0)


#define LOG_STRING(sev, str)                                     \
   do { if (log_sev_on(sev)) { log_str(sev, str, strlen(str)); } \
   } while (0)


/**
 * Logs a formatted string -- similar to printf()
 *
 * @param sev  The severity
 * @param fmt  The format string
 * @param ...  Additional arguments
 */
void log_fmt(log_sev_t sev, const char *fmt, ...) __attribute__((format(printf, 2, 3)));


/**
 * Flushes the cached log text to the stream
 *
 * @param force_nl   Append NL if not present
 */
void log_flush(bool force_nl);


// it is necessary to make at least one time pro change a check of all the
// uses of the MACRO LOG_FMT under Linux. This to detect all the used pointers,
// which might be nullptr.
// uncomment the define to do that.
// #define NO_MACRO_VARARG

#ifdef NO_MACRO_VARARG
#define LOG_FMT    log_fmt
// TODO during debugging add source file and line number
#else
#define LOG_FMT(sev, args...)                           \
   do { if (log_sev_on(sev)) { log_fmt(sev, ## args); } \
   } while (0)
#endif


/**
 * Dumps hex characters inline, no newlines inserted
 *
 * @param sev     The severity
 * @param data    The data to log
 * @param len     The number of bytes to log
 */
void log_hex(log_sev_t sev, const void *vdata, size_t len);


#define LOG_HEX(sev, ptr, len)                           \
   do { if (log_sev_on(sev)) { log_hex(sev, ptr, len); } \
   } while (0)


/**
 * Logs a block of data in a pretty hex format
 * Numbers on the left, characters on the right, just like I like it.
 *
 * "nnn | XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX XX | ................"
 *  0     ^6                                            54^ ^56           72^
 *
 *  nnn is the line number or index/16
 *
 * @param sev   The severity
 * @param data  The data to log
 * @param len   The number of bytes to log
 */
void log_hex_blk(log_sev_t sev, const void *data, size_t len);


#define LOG_HEX_BLK(sev, ptr, len)                           \
   do { if (log_sev_on(sev)) { log_hex_blk(sev, ptr, len); } \
   } while (0)


/**
 * Returns the HEX digit for a low nibble in a number
 *
 * @param nibble  The nibble
 *
 * @return '0', '1', '2', '3', '4', '5', '6', '7','8', '9',
 *         'a', 'b', 'c', 'd', 'e', or 'f'
 */
static_inline char to_hex_char(int nibble)
{
   const char *hex_string = "0123456789abcdef";

   return(hex_string[nibble & 0x0F]);
}


#ifdef DEBUG
/**
 * This should be called as the first thing in a function.
 * It uses the log_func class to add an entry to the function log stack.
 * It is automatically removed when the function returns.
 */
#define LOG_FUNC_ENTRY()    log_func log_fe = log_func(__func__, __LINE__)


/**
 * This should be called right before a repeated function call to trace where
 * the function was called. It does not add an entry, but rather updates the
 * line number of the top entry.
 */
#define LOG_FUNC_CALL()    log_func_call(__LINE__)

#else
#define LOG_FUNC_ENTRY()
#define LOG_FUNC_CALL()
#endif


/**
 * This class just adds a entry to the top of the stack on construction and
 * removes it on destruction.
 * RAII for the win.
 */
class log_func
{
public:
   log_func(const char *name, int line);


   ~log_func();
};


void log_func_call(int line);


void log_func_stack(log_sev_t sev, const char *prefix = 0, const char *suffix = "\n", size_t skip_cnt = 0);


#define log_func_stack_inline(_sev)    log_func_stack((_sev), " [CallStack:", "]\n", 1)


#endif /* LOGGER_H_INCLUDED */
