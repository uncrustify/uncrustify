/**
 * @file compat.h
 * prototypes for compat_xxx.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef COMPAT_H_INCLUDED
#define COMPAT_H_INCLUDED

#include "uncrustify_types.h"


bool unc_getenv(const char *name, std::string &str);
bool unc_homedir(std::string &home);

#endif /* COMPAT_H_INCLUDED */
