/**
 * @file uncrustify_types.cpp
 * Defines some types for the uncrustify program
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "uncrustify_types.h"


const char *get_brace_stage_name(E_BraceStage brace_stage)
{
   switch (brace_stage)
   {
   case E_BraceStage::NONE:
      return("NONE");

   case E_BraceStage::PAREN1:
      return("PAREN1");

   case E_BraceStage::OP_PAREN1:
      return("OP_PAREN1");

   case E_BraceStage::WOD_PAREN:
      return("WOD_PAREN");

   case E_BraceStage::WOD_SEMI:
      return("WOD_SEMI");

   case E_BraceStage::BRACE_DO:
      return("BRACE_DO");

   case E_BraceStage::BRACE2:
      return("BRACE2");

   case E_BraceStage::ELSE:
      return("ELSE");

   case E_BraceStage::ELSEIF:
      return("ELSEIF");

   case E_BraceStage::WHILE:
      return("WHILE");

   case E_BraceStage::CATCH:
      return("CATCH");

   case E_BraceStage::CATCH_WHEN:
      return("CATCH_WHEN");
   }
   return("?????");
} // get_brace_stage_name


const char *get_tracking_type_e_name(tracking_type_e type)
{
   switch (type)
   {
   case tracking_type_e::TT_NONE:
      return("NONE");

   case tracking_type_e::TT_SPACE:
      return("space");

   case tracking_type_e::TT_NEWLINE:
      return("newline");

   case tracking_type_e::TT_START:
      return("start");
   }
   return("?????");
} // get_tracking_type_e_name


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


const char *get_char_encoding(char_encoding_e encoding)
{
   switch (encoding)
   {
   case char_encoding_e::e_ASCII:
      return("ASCII");

   case char_encoding_e::e_BYTE:
      return("BYTE");

   case char_encoding_e::e_UTF8:
      return("UTF8");

   case char_encoding_e::e_UTF16_LE:
      return("UTF16_LE");

   case char_encoding_e::e_UTF16_BE:
      return("UTF16_BE");
   }
   return("?????");
} // get_char_encoding


const char *get_pattern_class(pattern_class_e p_class)
{
   switch (p_class)
   {
   case pattern_class_e::NONE:
      return("NONE");

   case pattern_class_e::BRACED:
      return("BRACED");

   case pattern_class_e::PBRACED:
      return("PBRACED");

   case pattern_class_e::OPBRACED:
      return("OPBRACED");

   case pattern_class_e::VBRACED:
      return("VBRACED");

   case pattern_class_e::PAREN:
      return("PAREN");

   case pattern_class_e::OPPAREN:
      return("OPPAREN");

   case pattern_class_e::ELSE:
      return("ELSE");
   }
   return("?????");
} // get_pattern_class
