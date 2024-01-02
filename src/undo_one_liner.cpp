/**
 * @file undo_one_liner.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */

#include "undo_one_liner.h"

using namespace uncrustify;


#define MARK_CHANGE()    mark_change(__func__, __LINE__)


void undo_one_liner(Chunk *pc)
{
   LOG_FUNC_ENTRY();

   if (  pc != nullptr
      && pc->TestFlags(PCF_ONE_LINER))
   {
      LOG_FMT(LNL1LINE, "%s(%d): pc->Text() '%s', orig line is %zu, orig col is %zu",
              __func__, __LINE__, pc->Text(), pc->GetOrigLine(), pc->GetOrigCol());
      pc->ResetFlagBits(PCF_ONE_LINER);

      // scan backward
      LOG_FMT(LNL1LINE, "%s(%d): scan backward\n", __func__, __LINE__);
      Chunk *tmp = pc;

      while ((tmp = tmp->GetPrev())->IsNotNullChunk())
      {
         if (!tmp->TestFlags(PCF_ONE_LINER))
         {
            LOG_FMT(LNL1LINE, "%s(%d): tmp->Text() '%s', orig line is %zu, orig col is %zu, --> break\n",
                    __func__, __LINE__, tmp->Text(), tmp->GetOrigLine(), tmp->GetOrigCol());
            break;
         }
         LOG_FMT(LNL1LINE, "%s(%d): clear for tmp->Text() '%s', orig line is %zu, orig col is %zu",
                 __func__, __LINE__, tmp->Text(), tmp->GetOrigLine(), tmp->GetOrigCol());
         tmp->ResetFlagBits(PCF_ONE_LINER);
      }
      // scan forward
      LOG_FMT(LNL1LINE, "%s(%d): scan forward\n", __func__, __LINE__);
      tmp = pc;
      LOG_FMT(LNL1LINE, "%s(%d): - \n", __func__, __LINE__);

      while ((tmp = tmp->GetNext())->IsNotNullChunk())
      {
         if (!tmp->TestFlags(PCF_ONE_LINER))
         {
            LOG_FMT(LNL1LINE, "%s(%d): tmp->Text() '%s', orig line is %zu, orig col is %zu, --> break\n",
                    __func__, __LINE__, tmp->Text(), tmp->GetOrigLine(), tmp->GetOrigCol());
            break;
         }
         LOG_FMT(LNL1LINE, "%s(%d): clear for tmp->Text() '%s', orig line is %zu, orig col is %zu",
                 __func__, __LINE__, tmp->Text(), tmp->GetOrigLine(), tmp->GetOrigCol());
         tmp->ResetFlagBits(PCF_ONE_LINER);
      }
      LOG_FMT(LNL1LINE, "\n");
   }
} // undo_one_liner
