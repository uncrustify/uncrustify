/**
 * @file template.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_TEMPLATE_H_INCLUDED
#define NEWLINES_TEMPLATE_H_INCLUDED

#include "option.h"

class Chunk;

void newline_template(Chunk *start);
uncrustify::iarf_e newline_template_option(Chunk *pc, uncrustify::iarf_e special, uncrustify::iarf_e base, uncrustify::iarf_e fallback);

#endif /* NEWLINES_TEMPLATE_H_INCLUDED */
