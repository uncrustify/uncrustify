void f(int x)
{
    if (x)
        a = 1;

    b = 2;
}

void g(int x)
{
    if (x)
        a = 1;

#ifdef X
    b = 2;
#endif
}

void h(int x)
{
    for (;;)
        a = 1;

#ifdef X
    b = 2;
#endif
    while (x)
        a = 1;

    b = 2;
}
