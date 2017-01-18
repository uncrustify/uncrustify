/**
 * @file unc_tools.cpp
 * This file contains lot of tools for debugging
 *
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */

#include "unc_tools.h"
#include "uncrustify.h"

// examples:
// prot_the_line(__LINE__, pc->orig_line);
// prot_the_line(__LINE__, 6);
// examine_Data(__func__, __LINE__, n);


// protocol of the line
void prot_the_line(int theLine, unsigned int actual_line)
{
   LOG_FMT(LGUY, "Prot_the_line:(%d) \n", theLine);
   for (chunk_t *pc = chunk_get_head(); pc != NULL; pc = pc->next)
   {
      if (pc->orig_line == actual_line)
      {
         LOG_FMT(LGUY, "(%d) orig_line=%d, ", theLine, actual_line);
         if (pc->type == CT_VBRACE_OPEN)
         {
            LOG_FMT(LGUY, "<VBRACE_OPEN>\n");
         }
         else if (pc->type == CT_NEWLINE)
         {
            LOG_FMT(LGUY, "<NL>(%zu)\n", pc->nl_count);
         }
         else if (pc->type == CT_VBRACE_CLOSE)
         {
            LOG_FMT(LGUY, "<CT_VBRACE_CLOSE>\n");
         }
         else if (pc->type == CT_VBRACE_OPEN)
         {
            LOG_FMT(LGUY, "<CT_VBRACE_OPEN>\n");
         }
         else if (pc->type == CT_SPACE)
         {
            LOG_FMT(LGUY, "<CT_SPACE>\n");
         }
         else
         {
            LOG_FMT(LGUY, "text() %s, type %s, orig_col=%zu, column=%zu\n",
                    pc->text(), get_token_name(pc->type), pc->orig_col, pc->column);
         }
      }
   }
   LOG_FMT(LGUY, "\n");
}


void examine_Data(const char *func_name, int theLine, int what)
{
   LOG_FMT(LGUY, "\n%s:", func_name);

   chunk_t *pc;
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
            LOG_FMT(LGUY, "%s, orig_col=%zu, orig_col_end=%d\n", pc->text(), pc->orig_col, pc->orig_col_end);
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
               LOG_FMT(LGUY, "(%zu)<NL> col=%zu\n\n", pc->orig_line, pc->orig_col);
            }
            else
            {
               LOG_FMT(LGUY, "(%zu)%s %s, col=%zu, column=%zu\n", pc->orig_line, pc->text(), get_token_name(pc->type), pc->orig_col, pc->column);
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
            LOG_FMT(LGUY, "(%zu)<NL> col=%zu\n\n", pc->orig_line, pc->orig_col);
         }
         else
         {
            LOG_FMT(LGUY, "(%zu)%s %s, col=%zu, column=%zu\n", pc->orig_line, pc->text(), get_token_name(pc->type), pc->orig_col, pc->column);
         }
      }
      break;

   case 4:
      LOG_FMT(LGUY, "4:(%d)\n", theLine);
      for (pc = chunk_get_head(); pc != NULL; pc = pc->next)
      {
         if (pc->orig_line == 6)
         {
            if (pc->type == CT_NEWLINE)
            {
               LOG_FMT(LGUY, "(%zu)<NL> col=%zu\n\n", pc->orig_line, pc->orig_col);
            }
            else
            {
               LOG_FMT(LGUY, "(%zu)%s %s, col=%zu, column=%zu\n", pc->orig_line, pc->text(), get_token_name(pc->type), pc->orig_col, pc->column);
            }
         }
      }
      break;
   } // switch
}    // examine_Data
