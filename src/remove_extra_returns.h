/**
 * @file remove_extra_returns.h
 * prototypes for remove_extra_returns.cpp
 *
 * @author  Guy Maurel
 * @license GPL v2+
 * extract from combine.h
 */

#ifndef REMOVE_EXTRA_RETURNS_H_INCLUDED
#define REMOVE_EXTRA_RETURNS_H_INCLUDED


/**
 * @brief Remove unnecessary returns
 * that is remove 'return;' that appears as the last statement in a function
 */
void remove_extra_returns(void);


#endif /* REMOVE_EXTRA_RETURNS_H_INCLUDED */
