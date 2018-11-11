void do_something(int foo, int foobar)
{
    if (foobar < foo)
        foobar++;  /* comment after VBRACE_END with >1 spaces */
    if (foobar < foo) {
        foobar++;  /* comment after ; */
    }
}
