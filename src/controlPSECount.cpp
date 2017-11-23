/**
 * @file controlPSECount.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "controlPSECount.h"


void controlPSECount(size_t value)
{
   if (value >= PSE_SIZE)
   {
      fprintf(stderr, "Index of 'frm.pse' is too big for the current value %d,\n", PSE_SIZE);
      fprintf(stderr, "Please make a report.\n");
      log_flush(true);
      exit(EX_SOFTWARE);
   }
}
