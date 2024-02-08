/**
 * @file namespace.h
 *
 * @author  Ben Gardner
 * @author  Guy Maurel
 * @license GPL v2+
 */
#ifndef NEWLINES_NAMESPACE_H_INCLUDED
#define NEWLINES_NAMESPACE_H_INCLUDED

class Chunk;

// namespace {
// namespace word {
// namespace type::word {
void newlines_namespace(Chunk *start); // Issue #2186

#endif /* NEWLINES_NAMESPACE_H_INCLUDED */
