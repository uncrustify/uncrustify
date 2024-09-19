/**
 * @file eat_start_end.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "newlines/eat_start_end.h"

#include "chunk.h"
#include "log_rules.h"
#include "mark_change.h"


constexpr static auto LCURRENT = LNEWLINE;


using namespace uncrustify;


void newlines_eat_start_end()
{
   LOG_FUNC_ENTRY();

   Chunk *pc;

   // Process newlines at the start of the file
   if (  cpd.frag_cols == 0
      && (  (options::nl_start_of_file() & IARF_REMOVE)
         || (  (options::nl_start_of_file() & IARF_ADD)
            && (options::nl_start_of_file_min() > 0))))
   {
      log_rule_B("nl_start_of_file");
      log_rule_B("nl_start_of_file_min");
      pc = Chunk::GetHead();

      if (pc->IsNotNullChunk())
      {
         if (pc->Is(CT_NEWLINE))
         {
            if (options::nl_start_of_file() == IARF_REMOVE)
            {
               log_rule_B("nl_start_of_file");
               LOG_FMT(LBLANKD, "%s(%d): eat_blanks_start_of_file %zu\n",
                       __func__, __LINE__, pc->GetOrigLine());
               Chunk::Delete(pc);
               MARK_CHANGE();
            }
            else if (  options::nl_start_of_file() == IARF_FORCE
                    || (pc->GetNlCount() < options::nl_start_of_file_min()))
            {
               log_rule_B("nl_start_of_file");
               LOG_FMT(LBLANKD, "%s(%d): set_blanks_start_of_file %zu\n",
                       __func__, __LINE__, pc->GetOrigLine());
               pc->SetNlCount(options::nl_start_of_file_min());
               log_rule_B("nl_start_of_file_min");
               MARK_CHANGE();
            }
         }
         else if (  (options::nl_start_of_file() & IARF_ADD)
                 && (options::nl_start_of_file_min() > 0))
         {
            log_rule_B("nl_start_of_file");
            log_rule_B("nl_start_of_file_min");
            Chunk chunk;
            chunk.SetType(CT_NEWLINE);
            chunk.SetOrigLine(pc->GetOrigLine());
            chunk.SetOrigCol(pc->GetOrigCol());
            chunk.SetPpLevel(pc->GetPpLevel());
            chunk.SetNlCount(options::nl_start_of_file_min());
            log_rule_B("nl_start_of_file_min");
            chunk.CopyAndAddBefore(pc);
            LOG_FMT(LNEWLINE, "%s(%d): %zu:%zu add newline before '%s'\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
            Chunk *prev = pc->GetPrev();
            log_ruleNL("nl_start_of_file_min", prev);
            MARK_CHANGE();
         }
      }
   }

   // Process newlines at the end of the file
   if (  cpd.frag_cols == 0
      && (  (options::nl_end_of_file() & IARF_REMOVE)
         || (  (options::nl_end_of_file() & IARF_ADD)
            && (options::nl_end_of_file_min() > 0))))
   {
      log_rule_B("nl_end_of_file");
      log_rule_B("nl_end_of_file_min");
      pc = Chunk::GetTail();

      if (pc->IsNotNullChunk())
      {
         if (pc->Is(CT_NEWLINE))
         {
            if (options::nl_end_of_file() == IARF_REMOVE)
            {
               log_rule_B("nl_end_of_file");
               LOG_FMT(LBLANKD, "%s(%d): eat_blanks_end_of_file %zu\n",
                       __func__, __LINE__, pc->GetOrigLine());
               Chunk::Delete(pc);
               MARK_CHANGE();
            }
            else if (  options::nl_end_of_file() == IARF_FORCE
                    || (pc->GetNlCount() < options::nl_end_of_file_min()))
            {
               log_rule_B("nl_end_of_file");
               log_rule_B("nl_end_of_file_min");

               if (pc->GetNlCount() != options::nl_end_of_file_min())
               {
                  log_rule_B("nl_end_of_file_min");
                  LOG_FMT(LBLANKD, "%s(%d): set_blanks_end_of_file %zu\n",
                          __func__, __LINE__, pc->GetOrigLine());
                  pc->SetNlCount(options::nl_end_of_file_min());
                  log_rule_B("nl_end_of_file_min");
                  MARK_CHANGE();
               }
            }
         }
         else if (  (options::nl_end_of_file() & IARF_ADD)
                 && (options::nl_end_of_file_min() > 0))
         {
            log_rule_B("nl_end_of_file");
            log_rule_B("nl_end_of_file_min");
            Chunk chunk;
            chunk.SetType(CT_NEWLINE);
            chunk.SetOrigLine(pc->GetOrigLine());
            chunk.SetOrigCol(pc->GetOrigCol());
            chunk.SetPpLevel(pc->GetPpLevel());
            chunk.SetNlCount(options::nl_end_of_file_min());
            log_rule_B("nl_end_of_file_min");
            chunk.CopyAndAddBefore(Chunk::NullChunkPtr);
            LOG_FMT(LNEWLINE, "%s(%d): %zu:%zu add newline after '%s'\n",
                    __func__, __LINE__, pc->GetOrigLine(), pc->GetOrigCol(), pc->Text());
            MARK_CHANGE();
         }
      }
   }
} // newlines_eat_start_end
