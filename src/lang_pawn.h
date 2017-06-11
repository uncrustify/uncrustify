/**
 * @file long_pawn.h
 * prototypes for long_pawn.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef LONG_PAWN_H_INCLUDED
#define LONG_PAWN_H_INCLUDED

#include "uncrustify_types.h"


/**
 * Does a scan of level 0 BEFORE stuff in combine.cpp is called.
 * At this point, VSemis have been added only in VBraces.
 * Otherwise, all level info is correct, except for unbraced functions.
 *
 * We are looking for unbraced functions.
 */
void pawn_prescan(void);


void pawn_add_virtual_semicolons(void);


/**
 * We are in a virtual brace and hit a newline.
 * If this should end the vbrace, then insert a VSEMICOLON and return that.
 *
 * @param pc  The newline (CT_NEWLINE)
 *
 * @return Either the newline or the newly inserted virtual semicolon
 */
chunk_t *pawn_check_vsemicolon(chunk_t *pc);


/**
 * Turns certain virtual semicolons invisible.
 *  - after a close brace with a parent of switch, case, else, if
 */
void pawn_scrub_vsemi(void);


//! add a semicolon after ...
chunk_t *pawn_add_vsemi_after(chunk_t *pc);


#endif /* LONG_PAWN_H_INCLUDED */
