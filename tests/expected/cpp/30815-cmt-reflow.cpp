
typedef enum stuff
{
    Value1 = 0x00000400, /* Just a comment for the value */
    Value2 = 0x00000800, /* A much longer comment that needs
                          * to be truncated to fit within a
                          * set character width. In this
                          * case, its 80 characters so two
                          * truncates are required. */
} JustAnEnum;

/* this is another comment that is meant to exceed the code
 * width so that it can be wrapped
 * and combined to see how that works. */

/* this is another comment that is meant to exceed the code
 * width so that it can be wrapped
 * and combined to see how that works. */

/* Line A */

/* Line 1
 * line 2
 * line 3
 * line 4
 */

int cnt;    /* This is a counter variable with a long
             * comment. this should cause the comment to be
             * wrapped. */

/**
 * Multi-line comment
 */
void foo(void)
{
/**
 * Multi-line comment
 */
    int idx;
    /**
     * Multi-line comment
     */
}

/* Start Change #95
 * INITIALIZE Variable(contExtnElgInd); /# Change #61 #/
 * /# Start Change #35 #/
 */

/**
 * OneBigWordThatCannotBeSplitYetExceedsTheCommentWidthSettingSoThatWrappingShouldBeAttempted.
 */
