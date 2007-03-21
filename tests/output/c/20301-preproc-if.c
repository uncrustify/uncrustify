
int main()
{
    int a;

#ifndef SOMEDEF
    int b;
#endif /* SOMEDEF */

    if (a)
    {
    }
#ifndef SOMEDEF
    else if (b)
    {
    }
#endif /* SOMEDEF */

/* same thing w/o preprocs for reference */
    if (a)
    {
    }
    else if (b)
    {
    }

#ifdef FOO
    do
    {
        Foo();
    }
#endif
    while (Loop--);
}


