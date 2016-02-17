/**
 * @file unc_tools.cpp
 * This file contains lot of tools for debugging
 *
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */

void prot_the_line(int theLine)
{
   chunk_t *pc;
   LOG_FMT(LGUY, "(%d) ", theLine);
   for (pc = chunk_get_head(); pc != NULL; pc = pc->next)
   {
      LOG_FMT(LGUY, " %s", pc->str.c_str());
   }
   LOG_FMT(LGUY, "\n");
}
