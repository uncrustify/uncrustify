/**
 * @file brace_cleanup.h
 * prototypes for brace_cleanup.c
 *
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef BRACE_CLEANUP_H_INCLUDED
#define BRACE_CLEANUP_H_INCLUDED

#include "uncrustify_types.h"


/**
 * Scans through the whole list and does stuff.
 * It has to do some tricks to parse preprocessors.
 */
void brace_cleanup(void);


/**
 * Called when a statement was just closed and the pse_tos was just
 * decremented.
 *
 * - if the TOS is now VBRACE, insert a CT_VBRACE_CLOSE and recurse.
 * - if the TOS is a complex statement, call handle_complex_close()
 *
 * @retval true   done with this chunk
 * @retval false  keep processing
 */
bool close_statement(parse_frame_t *frm, chunk_t *pc);


#endif /* BRACE_CLEANUP_H_INCLUDED */
