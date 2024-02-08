/**
 * @file force.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_FORCE_H_INCLUDED
#define NEWLINES_FORCE_H_INCLUDED

class Chunk;

Chunk *newline_force_after(Chunk *pc);
Chunk *newline_force_before(Chunk *pc);

#endif /* NEWLINES_FORCE_H_INCLUDED */
