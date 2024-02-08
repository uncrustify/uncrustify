/**
 * @file cleanup.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_CLEANUP_H_INCLUDED
#define NEWLINES_CLEANUP_H_INCLUDED

void newlines_cleanup_angles();
void newlines_cleanup_braces(bool first);
void newlines_cleanup_dup();

#endif /* NEWLINES_CLEANUP_H_INCLUDED */
