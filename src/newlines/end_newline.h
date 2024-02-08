/**
 * @file end_newline.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_END_NEWLINES_H_INCLUDED
#define NEWLINES_END_NEWLINES_H_INCLUDED

class Chunk;

//! Ensure that the next non-comment token after close brace is a newline
void newline_end_newline(Chunk *br_close);

#endif /* NEWLINES_END_NEWLINES_H_INCLUDED */
