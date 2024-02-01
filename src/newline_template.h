/**
 * @file newline_template.h
 * prototype for newline_template.cpp
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINE_TEMPLATE_H_INCLUDED
#define NEWLINE_TEMPLATE_H_INCLUDED

#include "chunk.h"


using namespace uncrustify;


void newline_template(Chunk *start);


iarf_e newline_template_option(Chunk *pc, iarf_e special, iarf_e base, iarf_e fallback);

#endif /* NEWLINE_TEMPLATE_H_INCLUDED */
