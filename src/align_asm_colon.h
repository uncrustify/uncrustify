/**
 * @file align_asm_colon.h
 *
 * @author  Guy Maurel
 * split from align.cpp
 * @author  Ben Gardner
 * @license GPL v2+
 */
#ifndef ALIGN_ASM_COLON_H_INCLUDED
#define ALIGN_ASM_COLON_H_INCLUDED


/**
 * Aligns asm declarations on the colon
 * asm volatile (
 *    "xxx"
 *    : "x"(h),
 *      "y"(l),
 *    : "z"(h)
 *    );
 */
void align_asm_colon(void);

#endif /* ALIGN_ASM_COLON_H_INCLUDED */
