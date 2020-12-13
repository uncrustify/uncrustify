/**
 * @file log_rules.h
 * prototypes for log_rules.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#ifndef LOG_RULES_H_INCLUDED
#define LOG_RULES_H_INCLUDED

#include "chunk_list.h"
#include "uncrustify.h"
#include "uncrustify_types.h"

using namespace uncrustify;

#if defined DEBUG
#define log_rule(rule)                                      \
   do {                                                     \
      log_rule2(__func__, __LINE__, (rule), first, second); \
      log_rule4((rule), first);                             \
                                                            \
   } while (0)
#else
#define log_rule(rule)                                      \
   do {                                                     \
      log_rule2(__func__, __LINE__, (rule), first, second); \
   } while (0)
#endif

#define log_rule_short(rule)                                \
   do {                                                     \
      log_rule2(__func__, __LINE__, (rule), first, second); \
   } while (0)

#define log_rule_B(rule)           \
   do {                            \
      log_rule3(__func__, (rule)); \
   } while (0)


void log_rule2(const char *func, size_t line, const char *rule, chunk_t *first, chunk_t *second);


void log_rule3(const char *func, const char *rule);

void log_rule4(const char *rule, chunk_t *first);

#endif /* LOG_RULES_H_INCLUDED */
