/**
 * @file language_tools.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 */

#include "language_tools.h"


/**
 * check if the language(s) is/are set
 */
bool language_is_set(size_t lang)
{
   return((cpd.lang_flags & lang) != 0);
}
