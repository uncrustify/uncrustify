/**
 * @file log_rules.h
 * prototypes for log_rules.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef LOG_RULES_H_INCLUDED
#define LOG_RULES_H_INCLUDED

#include "chunk.h"
#include "uncrustify.h"

//using namespace uncrustify;

#define log_rule(rule)                                   \
   log_rule2(__func__, __LINE__, (rule), first, second); \
   log_rule4((rule), first)

// if you need more debug information, remove the comment at the next line
#define SUPER_LOG    1
#ifdef SUPER_LOG
#define log_rule_B(rule) \
   log_rule3(LCURRENT, __func__, __LINE__, (rule))
#else
#define log_rule_B(rule) \
   log_rule3(LCURRENT, __func__, (rule))
#endif

#ifdef SUPER_LOG
#define log_rule_NL(rule) \
   log_ruleNL((rule), pc)
#else
#define log_rule_NL(rule) \
   log_ruleNL(LCURRENT, __func__, (rule))
#endif

void log_rule2(const char *func, size_t line, const char *rule, Chunk *first, Chunk *second);


#ifdef SUPER_LOG
void log_rule3(log_sev_t sev, const char *func, size_t line, const char *rule);

#else
void log_rule3(log_sev_t sev, const char *func, const char *rule);

#endif

void log_rule4(const char *rule, Chunk *first);

void log_ruleStart(const char *rule, Chunk *first);

void log_ruleNL(const char *rule, Chunk *pc);

#endif /* LOG_RULES_H_INCLUDED */
