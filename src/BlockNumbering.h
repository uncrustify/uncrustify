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
 *    1. the level of the chunk is the same
 *    2. the block number of the statements are the same.
 *
 * A new block is opened if the type of the chunk is:
 *    CT_BRACE_OPEN
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

void NumberTheBlocks();

#endif /* BLOCKNUMBERING_H_INCLUDED */
