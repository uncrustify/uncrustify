/**
 * @file uncrustify_types.cpp
 * Defines some types for the uncrustify program
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "uncrustify_types.h"


const char *get_brace_stage_name(brace_stage_e brace_stage)
{
   switch (brace_stage)
   {
   case brace_stage_e::NONE:
      return("NONE");

   case brace_stage_e::PAREN1:
      return("PAREN1");

   case brace_stage_e::OP_PAREN1:
      return("OP_PAREN1");

   case brace_stage_e::WOD_PAREN:
      return("WOD_PAREN");

   case brace_stage_e::WOD_SEMI:
      return("WOD_SEMI");

   case brace_stage_e::BRACE_DO:
      return("BRACE_DO");

   case brace_stage_e::BRACE2:
      return("BRACE2");

   case brace_stage_e::ELSE:
      return("ELSE");

   case brace_stage_e::ELSEIF:
      return("ELSEIF");

   case brace_stage_e::WHILE:
      return("WHILE");

   case brace_stage_e::CATCH:
      return("CATCH");

   case brace_stage_e::CATCH_WHEN:
      return("CATCH_WHEN");
   }
   return("?????");
} // get_brace_stage_name


const char *get_unc_stage_name(unc_stage_e unc_stage)
{
   switch (unc_stage)
   {
   case unc_stage_e::TOKENIZE:
      return("TOKENIZE");

   case unc_stage_e::HEADER:
      return("HEADER");

   case unc_stage_e::TOKENIZE_CLEANUP:
      return("TOKENIZE_CLEANUP");

   case unc_stage_e::BRACE_CLEANUP:
      return("BRACE_CLEANUP");

   case unc_stage_e::FIX_SYMBOLS:
      return("FIX_SYMBOLS");

   case unc_stage_e::MARK_COMMENTS:
      return("MARK_COMMENTS");

   case unc_stage_e::COMBINE_LABELS:
      return("COMBINE_LABELS");

   case unc_stage_e::OTHER:
      return("OTHER");

   case unc_stage_e::CLEANUP:
      return("CLEANUP");
   }
   return("?????");
} // get_unc_stage_name
