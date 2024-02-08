/**
 * @file template.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#include "newlines/template.h"

#include "log_rules.h"
#include "newlines/iarf.h"
#include "options.h"


using namespace uncrustify;


constexpr static auto LCURRENT = LNEWLINE;


void newline_template(Chunk *start)
{
   LOG_FUNC_ENTRY();

   LOG_FMT(LNFD, "%s(%d): called on %zu:%zu '%s' [%s/%s]\n",
           __func__, __LINE__, start->GetOrigLine(), start->GetOrigCol(),
           start->Text(), get_token_name(start->GetType()), get_token_name(start->GetParentType()));

   log_rule_B("nl_template_start");
   bool add_start = options::nl_template_start();

   log_rule_B("nl_template_args");
   bool add_args = options::nl_template_args();

   log_rule_B("nl_template_end");
   bool add_end = options::nl_template_end();

   if (  !add_start
      && !add_args
      && !add_end)
   {
      return;
   }
   Chunk *pc = start->GetNextNcNnl();

   while (  pc->IsNotNullChunk()
         && pc->GetLevel() > start->GetLevel())
   {
      pc = pc->GetNextNcNnl();
   }

   if (pc->Is(CT_ANGLE_CLOSE))
   {
      if (add_start)
      {
         newline_iarf(start, IARF_ADD);
      }

      if (add_end)
      {
         newline_iarf(pc->GetPrev(), IARF_ADD);
      }

      if (add_args)
      {
         Chunk *pc_1;

         for (pc_1 = start->GetNextNcNnl();
              pc_1->IsNotNullChunk() && pc_1->GetLevel() > start->GetLevel();
              pc_1 = pc_1->GetNextNcNnl())
         {
            if (  pc_1->Is(CT_COMMA)
               && (pc_1->GetLevel() == (start->GetLevel() + 1)))
            {
               Chunk *tmp = pc_1->GetNext();

               if (tmp->IsComment())
               {
                  pc_1 = tmp;
               }

               if (!pc_1->GetNext()->IsNewline())
               {
                  newline_iarf(pc_1, IARF_ADD);
               }
            }
         }
      }
   }
} // newline_template


iarf_e newline_template_option(Chunk *pc, iarf_e special, iarf_e base, iarf_e fallback)
{
   Chunk *const prev = pc->GetPrevNcNnl();

   if (  prev->Is(CT_ANGLE_OPEN)
      && special != IARF_IGNORE)
   {
      return(special);
   }
   else if (base != IARF_IGNORE)
   {
      return(base);
   }
   else
   {
      return(fallback);
   }
} // newline_template_option
