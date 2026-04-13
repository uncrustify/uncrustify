/**
 * @file span_num_resolve.h
 *
 * Helper to resolve specific *_span_num_*_lines values that may be -1
 * (meaning "use the corresponding global_span_num_*_lines value").
 *
 * @license GPL v2+
 */

#ifndef ALIGN_SPAN_NUM_RESOLVE_H_INCLUDED
#define ALIGN_SPAN_NUM_RESOLVE_H_INCLUDED

#include "options.h"
#include "uncrustify_types.h"


/**
 * @brief Build a LineSkipConfig, resolving -1 values to the global defaults.
 *
 * Each specific value may be -1 (use the global) or >= 0 (override).
 */
static inline LineSkipConfig resolve_span_num_config(signed empty, signed pp, signed cmt)
{
   using namespace uncrustify;

   LineSkipConfig cfg = {};

   cfg.empty_lines = (empty < 0)
                     ? options::global_span_num_empty_lines()
                     : static_cast<size_t>(empty);
   cfg.pp_lines = (pp < 0)
                  ? options::global_span_num_pp_lines()
                  : static_cast<size_t>(pp);
   cfg.cmt_lines = (cmt < 0)
                   ? options::global_span_num_cmt_lines()
                   : static_cast<size_t>(cmt);
   return(cfg);
}

#endif /* ALIGN_SPAN_NUM_RESOLVE_H_INCLUDED */
