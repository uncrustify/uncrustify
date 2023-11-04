/**
 * @file semicolons.h
 * prototypes for semicolons.cpp
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef SEMICOLONS_H_INCLUDED
#define SEMICOLONS_H_INCLUDED

#include "uncrustify_types.h"


/**
 * Removes superfluous semicolons:
 *  - after brace close whose parent is IF, ELSE, SWITCH, WHILE, FOR, NAMESPACE
 *  - after another semicolon where parent is not FOR
 *  - (D) after brace close whose parent is ENUM/STRUCT/UNION
 *  - (Java) after brace close whose parent is SYNCHRONIZED
 *  - after an open brace
 *  - when not in a #DEFINE
 */
void remove_extra_semicolons();


#endif /* SEMICOLONS_H_INCLUDED */
