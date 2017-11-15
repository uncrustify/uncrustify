
int foo1(void)
{
    bar();

    return
        (NewClass(1));
}

int foo2(void)
{
    return
        (NewClass(2));
}

int foo3(void)
{
    bar(none);

    // comment
    return
        (3);
}

int foo4(void)
{
    return
        (4);
}

