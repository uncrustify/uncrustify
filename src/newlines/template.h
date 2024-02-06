/**
 * @file template.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * extract from newlines.cpp
 * @license GPL v2+
 */
#ifndef NEWLINES_TEMPLATE_H_INCLUDED
#define NEWLINES_TEMPLATE_H_INCLUDED

#include "chunk.h"


using namespace uncrustify;


void newline_template(Chunk *start);


iarf_e newline_template_option(Chunk *pc, iarf_e special, iarf_e base, iarf_e fallback);

#endif /* NEWLINES_TEMPLATE_H_INCLUDED */
