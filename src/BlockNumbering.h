/**
 * @file BlockNumbering.h
 *
 * @author  Guy Maurel
 *          Juni 2018
 * @license GPL v2+
 */

#ifndef BLOCKNUMBERING_H_INCLUDED
#define BLOCKNUMBERING_H_INCLUDED

/*
 * To align (or not align) the assign character it is important to know if:
 *    1. the levels of the chunks are the same
 *    2. the block numbers of the statements are the same.
 *
 * Introducing block numbering as:
 * A new block is opened if the type of the chunk is:
 *    CT_BRACE_OPEN
 *    CT_VBRACE_OPEN
 *    CT_FPAREN_OPEN
 *    CT_ANGLE_OPEN
 *
 * With this we get:
 *    virtual void f(int x, int y) = 133;
 *    void g(int x = 144);
 *
 * and not (Issue #1760)
 *    virtual void f(int x, int y) = 133;
 *    void g(int x                 = 144);
 */

void numberTheBlocks();

#endif /* BLOCKNUMBERING_H_INCLUDED */
