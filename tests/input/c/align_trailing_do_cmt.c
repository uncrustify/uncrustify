int foo(void) {
    int r;
    do // this comment should be aligned with the one below
    {
        r = bar();
    }
    while (r);

    do            // this comment should be aligned with the one above
    {
        r = baz();
    }
    while (r);
}
