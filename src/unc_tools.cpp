/**
 * @file unc_tools.cpp
 * This file contains lot of tools for debugging
 *
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */

#include "unc_tools.h"

// examine_Data(__func__, __LINE__, n);

void prot_the_line(int theLine)
{
   chunk_t *pc;
   LOG_FMT(LGUY, "P:(%d) ", theLine);
   for (pc = chunk_get_head(); pc != NULL; pc = pc->next)
   {
      if (pc->type == CT_NEWLINE) {
        LOG_FMT(LGUY, "<NL>\n");
      } else {
        LOG_FMT(LGUY, " %s", pc->text());
      }
   }
   LOG_FMT(LGUY, "\n");
}

void examine_Data(const char *func_name, int theLine, int what)
{
   chunk_t *pc;

   LOG_FMT(LGUY, "\n%s:", func_name);
   switch (what) {
   case 1:
      for (pc = chunk_get_head(); pc != NULL; pc = pc->next) {
         if ((pc->type == CT_SQUARE_CLOSE) ||
             (pc->type == CT_TSQUARE)) {
           LOG_FMT(LGUY, "\n");
           LOG_FMT(LGUY, "1:(%d),", theLine);
           LOG_FMT(LGUY, "%s, orig_col=%d, orig_col_end=%d\n", pc->text(), pc->orig_col, pc->orig_col_end);
         }
      }
      break;
   case 2:
      LOG_FMT(LGUY, "2:(%d)\n", theLine);
      for (pc = chunk_get_head(); pc != NULL; pc = pc->next) {
        if (pc->orig_line == 7) {
          if (pc->type == CT_NEWLINE) {
            LOG_FMT(LGUY, "(%d)<NL> col=%d\n\n", pc->orig_line, pc->orig_col);
          } else {
            LOG_FMT(LGUY, "(%d)%s %s, col=%d, column=%d\n", pc->orig_line, pc->text(), get_token_name(pc->type), pc->orig_col, pc->column);
          }
        }
      }
      break;
   case 3:
      LOG_FMT(LGUY, "3:(%d)\n", theLine);
      for (pc = chunk_get_head(); pc != NULL; pc = pc->next) {
           if (pc->type == CT_NEWLINE) {
             LOG_FMT(LGUY, "(%d)<NL> col=%d\n\n", pc->orig_line, pc->orig_col);
           } else {
             LOG_FMT(LGUY, "(%d)%s %s, col=%d, column=%d\n", pc->orig_line, pc->text(), get_token_name(pc->type), pc->orig_col, pc->column);
           }
      }
   case 4:
      LOG_FMT(LGUY, "4:(%d)\n", theLine);
      for (pc = chunk_get_head(); pc != NULL; pc = pc->next) {
         if (pc->orig_line == 6) {
           if (pc->type == CT_NEWLINE) {
             LOG_FMT(LGUY, "(%d)<NL> col=%d\n\n", pc->orig_line, pc->orig_col);
           } else {
             LOG_FMT(LGUY, "(%d)%s %s, col=%d, column=%d\n", pc->orig_line, pc->text(), get_token_name(pc->type), pc->orig_col, pc->column);
           }
        }
      }
      break;
   }
}
