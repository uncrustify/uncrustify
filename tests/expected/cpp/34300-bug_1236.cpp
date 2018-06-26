void foo()
{
    int head, bar;
    __asm__ __volatile__
    (
        "movq %0,%%xmm0\n\t"    /* asm template */
    "0:\n\t"
        "bar\t%0, [%4]\n\t" // in template
    "1:\n\t"
        : "=a", (bar)
        : "=&b", (&head), "+m", (bar)
        : "cc"
    );
}
