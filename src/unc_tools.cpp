/**
 * @file unc_tools.cpp
 * This file contains lot of tools for debugging
 *
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */

#include "unc_tools.h"

// prot_the_line(__LINE__);
// examine_Data(__func__, __LINE__, n);


// protocoll of the line
void prot_the_line(int theLine, unsigned int actual_line)
{
   chunk_t *pc;

   LOG_FMT(LGUY, "Prot_the_line:(%d) \n", theLine);
   for (pc = chunk_get_head(); pc != NULL; pc = pc->next)
   {
      if (pc->orig_line == actual_line)
      {
         if (pc->type == CT_NEWLINE)
         {
            LOG_FMT(LGUY, "(%d) %d, <NL>(%d)\n", theLine, actual_line, pc->nl_count);
         }
         else
         {
            LOG_FMT(LGUY, "(%d) %d, %s\n", theLine, actual_line, pc->text());
         }
      }
   }
   LOG_FMT(LGUY, "\n");
}


void examine_Data(const char *func_name, int theLine, int what)
{
   chunk_t *pc;

   LOG_FMT(LGUY, "\n%s:", func_name);
   switch (what)
   {
   case 1:
      for (pc = chunk_get_head(); pc != NULL; pc = pc->next)
      {
         if ((pc->type == CT_SQUARE_CLOSE) ||
             (pc->type == CT_TSQUARE))
         {
            LOG_FMT(LGUY, "\n");
            LOG_FMT(LGUY, "1:(%d),", theLine);
            LOG_FMT(LGUY, "%s, orig_col=%d, orig_col_end=%d\n", pc->text(), pc->orig_col, pc->orig_col_end);
         }
      }
      break;

   case 2:
      LOG_FMT(LGUY, "2:(%d)\n", theLine);
      for (pc = chunk_get_head(); pc != NULL; pc = pc->next)
      {
         if (pc->orig_line == 7)
         {
            if (pc->type == CT_NEWLINE)
            {
               LOG_FMT(LGUY, "(%d)<NL> col=%d\n\n", pc->orig_line, pc->orig_col);
            }
            else
            {
               LOG_FMT(LGUY, "(%d)%s %s, col=%d, column=%d\n", pc->orig_line, pc->text(), get_token_name(pc->type), pc->orig_col, pc->column);
            }
         }
      }
      break;

   case 3:
      LOG_FMT(LGUY, "3:(%d)\n", theLine);
      for (pc = chunk_get_head(); pc != NULL; pc = pc->next)
      {
         if (pc->type == CT_NEWLINE)
         {
            LOG_FMT(LGUY, "(%d)<NL> col=%d\n\n", pc->orig_line, pc->orig_col);
         }
         else
         {
            LOG_FMT(LGUY, "(%d)%s %s, col=%d, column=%d\n", pc->orig_line, pc->text(), get_token_name(pc->type), pc->orig_col, pc->column);
         }
      }

   case 4:
      LOG_FMT(LGUY, "4:(%d)\n", theLine);
      for (pc = chunk_get_head(); pc != NULL; pc = pc->next)
      {
         if (pc->orig_line == 6)
         {
            if (pc->type == CT_NEWLINE)
            {
               LOG_FMT(LGUY, "(%d)<NL> col=%d\n\n", pc->orig_line, pc->orig_col);
            }
            else
            {
               LOG_FMT(LGUY, "(%d)%s %s, col=%d, column=%d\n", pc->orig_line, pc->text(), get_token_name(pc->type), pc->orig_col, pc->column);
            }
         }
      }
      break;
   } // switch
}    // examine_Data
