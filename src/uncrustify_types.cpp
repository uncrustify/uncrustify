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
