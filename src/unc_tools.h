/**
 * @file unc_tools.h
 *
 * @author  Guy Maurel since version 0.62 for uncrustify4Qt
 *          October 2015, 2016
 * @license GPL v2+
 */

#ifndef UNC_TOOLS_H_INCLUDED
#define UNC_TOOLS_H_INCLUDED

#include "prototypes.h"
#include "uncrustify_types.h"
#include "chunk_list.h"

void prot_the_line(int theLine);
void examine_Data(const char *func_name, int theLine, int what);
// examine_Data(__func__, __LINE__, 2);

#endif /* UNC_TOOLS_H_INCLUDED */
