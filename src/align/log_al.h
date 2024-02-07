/**
 * @file log_al.h
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */

#ifndef ALIGN_LOG_AL_H_INCLUDED
#define ALIGN_LOG_AL_H_INCLUDED

#include <cstddef>

#include "log_levels.h"

void align_log_al(log_sev_t sev, size_t line);

#endif /* ALIGN_LOG_AL_H_INCLUDED */
