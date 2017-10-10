// Asm blocks have their own special indentation where lables must remain at indent 0 relative to __asm__ block.
// They few ways of being opened and closed depending on the compiler.
// For now, we can at least detect and ignore the contents, including alignment.

// Workaround: can always fall back on disable/enable_processing_cmt.

void foo()
{
    int head, bar;
    __asm__ __volatile__
    (
        "movq %0,%%xmm0\n\t"    /* asm template */
    "0:\n\t"
        "bar	%0, [%4]\n\t"   // in template
    "1:\n\t"
        : "=a", (bar)
        : "=&b", (&head), "+m", (bar)
        : "cc"
    );
}
