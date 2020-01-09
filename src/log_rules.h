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

void log_rule(const char *rule);


void log_rule_B(const char *rule);


#define log_rule(rule)                                             \
   do { if (log_sev_on(LSPACE)) {                                  \
           log_rule2(__func__, __LINE__, (rule), first, second); } \
   } while (0)


#define log_rule_B(rule)                            \
   do { if (log_sev_on(LSPACE)) {                   \
           log_rule3(__func__, __LINE__, (rule)); } \
   } while (0)


void log_rule2(const char *func, size_t line, const char *rule, chunk_t *first, chunk_t *second);


void log_rule3(const char *func, size_t line, const char *rule);

#endif /* LOG_RULES_H_INCLUDED */
