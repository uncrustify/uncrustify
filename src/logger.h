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


#ifdef __MINGW_PRINTF_FORMAT
// On MinGW, the printf functions can be provided by a number of different
// implementations, with different format string support. Annontate log_fmt
// below with the same format attribute as the currently chosen default printf
// function.
#define PRINTF_FORMAT    __MINGW_PRINTF_FORMAT
#else
#define PRINTF_FORMAT    printf
#endif

/**
 * Logs a formatted string -- similar to printf()
 *
 * @param sev  The severity
 * @param fmt  The format string
 * @param ...  Additional arguments
 */
void log_fmt(log_sev_t sev, const char *fmt, ...) __attribute__((format(PRINTF_FORMAT, 2, 3)));


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
#define LOG_FMT(sev, ...) \
   if (log_sev_on(sev)) { log_fmt(sev, __VA_ARGS__); }
#endif


#define __unqualified_func__    get_unqualified_func_name(__func__)


#define LOG_CHUNK(sev, pc_current)                                                                                                                                                             \
   if (pc_current->Is(CT_NEWLINE))                                                                                                                                                             \
   {                                                                                                                                                                                           \
      LOG_FMT(sev, "%s(%d): orig line is %zu, orig col is %zu, <Newline>, PRE is %s\n",                                                                                                        \
              __func__, __LINE__, pc_current->GetOrigLine(), pc_current->GetOrigCol(), pc_current->IsPreproc() ? "true" : "false");                                                            \
   }                                                                                                                                                                                           \
   else if (pc_current->Is(CT_NL_CONT))                                                                                                                                                        \
   {                                                                                                                                                                                           \
      LOG_FMT(sev, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s, PRE is %s\n",                                                                                          \
              __func__, __LINE__, pc_current->GetOrigLine(), pc_current->GetOrigCol(), pc_current->Text(), get_token_name(pc_current->GetType()), pc_current->IsPreproc() ? "true" : "false"); \
   }                                                                                                                                                                                           \
   else                                                                                                                                                                                        \
   {                                                                                                                                                                                           \
      LOG_FMT(sev, "%s(%d): orig line is %zu, orig col is %zu, Text() '%s', type is %s, PRE is %s\n",                                                                                          \
              __func__, __LINE__, pc_current->GetOrigLine(), pc_current->GetOrigCol(), pc_current->Text(), get_token_name(pc_current->GetType()), pc_current->IsPreproc() ? "true" : "false"); \
   }


#ifdef DEBUG
/**
 * This should be called as the first thing in a function.
 * It uses the log_func class to add an entry to the function log stack.
 * It is automatically removed when the function returns.
 */
#define LOG_FUNC_ENTRY()    log_func log_fe = log_func(__unqualified_func__, __LINE__)


#else
#define LOG_FUNC_ENTRY()
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


void log_func_stack(log_sev_t sev, const char *prefix = 0, const char *suffix = "\n", size_t skip_cnt = 0);


/**
 * Return the unqualified function name from the input argument
 * @param  the qualified function name, usually provided by __func__ macro
 * @return the corresponding unqualified name
 */
const char *get_unqualified_func_name(const char *func);


#define log_func_stack_inline(_sev)    log_func_stack((_sev), " [CallStack:", "]\n", 0)


#endif /* LOGGER_H_INCLUDED */
