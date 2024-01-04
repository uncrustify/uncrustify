/**
 * @file ifdef_over_whole_file.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel October 2015- 2023
 * extract from indent.cpp
 * @license GPL v2+
 */

#include "ifdef_over_whole_file.h"

#include "chunk.h"
#include "log_levels.h"
#include "logger.h"

using namespace uncrustify;


bool ifdef_over_whole_file()
{
   LOG_FUNC_ENTRY();

   // if requested, treat an #if that guards the entire file the same as any other #if
   // if running as frag, assume #if is not a guard
   if (  options::pp_indent_in_guard()
      || cpd.frag)
   {
      return(false);
   }

   // the results for this file are cached
   if (cpd.ifdef_over_whole_file)
   {
      return(cpd.ifdef_over_whole_file > 0);
   }
   Chunk  *start_pp = Chunk::NullChunkPtr;
   Chunk  *end_pp   = Chunk::NullChunkPtr;
   size_t IFstage   = 0;

   for (Chunk *pc = Chunk::GetHead(); pc->IsNotNullChunk(); pc = pc->GetNext())
   {
      LOG_FMT(LNOTE, "%s(%d): pc->pp level is %zu, pc orig line is %zu, orig col is %zu, pc->Text() is '%s'\n",
              __func__, __LINE__, pc->GetPpLevel(), pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());

      if (pc->IsCommentOrNewline())
      {
         continue;
      }

      if (IFstage == 0)                   // 0 is BEGIN
      {
         // Check the first preprocessor, make sure it is an #if type
         if (pc->IsNot(CT_PREPROC))
         {
            break;
         }
         Chunk *next = pc->GetNext();

         if (  next->IsNullChunk()
            || next->IsNot(CT_PP_IF))
         {
            break;
         }
         IFstage  = 1;                      // 1 is CT_PP_IF found
         start_pp = pc;
      }
      else if (IFstage == 1)                // 1 is CT_PP_IF found
      {
         // Scan until a preprocessor at level 0 is found - the close to the #if
         if (pc->Is(CT_PREPROC))
         {
            if (pc->GetPpLevel() == 0)
            {
               IFstage = 2;
               end_pp  = pc;
            }
         }
         continue;
      }
      else if (IFstage == 2)
      {
         // We should only see the rest of the preprocessor
         if (  pc->Is(CT_PREPROC)
            || !pc->TestFlags(PCF_IN_PREPROC))
         {
            IFstage = 0;
            break;
         }
      }
   }

   cpd.ifdef_over_whole_file = (IFstage == 2) ? 1 : -1;

   if (cpd.ifdef_over_whole_file > 0)
   {
      start_pp->SetFlagBits(PCF_WF_IF);
      end_pp->SetFlagBits(PCF_WF_ENDIF);
   }
   LOG_FMT(LNOTE, "The whole file is%s covered by a #IF\n",
           (cpd.ifdef_over_whole_file > 0) ? "" : " NOT");
   return(cpd.ifdef_over_whole_file > 0);
} // ifdef_over_whole_file
