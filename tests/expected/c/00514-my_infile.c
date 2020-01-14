#include <stdlib.h> /* exit */
#include <unistd.h> /* _exit */

int foo(int);

extern int baz;

int foo (int bar)
{
/* Switch blocks: */
    switch (c)
    {
        case 1:
        case 2:
            bar += 2;
        break;

        case 3:
            bar++;
            baz++;

        case 4:
        break;

        default:
        break;
    }

    switch (bar)
    {
        case 0:
            bar++;
        break;

        case 1:
            bar++;
            return bar;

        case 2:
            bar++;
            goto x;

        case 3:
            bar++;

        /*FALLTHROUGH*/
        case 4:
            bar++;
            exit(bar);

        /*NOTREACHED*/
        case 5:
            bar++;
            _exit(bar);

        /*NOTREACHED*/
        case 6:
            bar++;
            if (baz > 2)
            {
                break;      /* inside if statement; don't align with case */
            }
            else
            {
                return baz; /* inside if statement; don't align with case */
            }

        /*NOTREACHED*/
        case 7:
            switch (baz)
            {
                case 0: /* do nothing */
                break;

                case 1:
                    return baz;

                case 2:
                    baz--;
                    goto x;

                case 3:
                    exit(baz);

                /*NOTREACHED*/
                case 4:
                    _exit(baz);

                /*NOTREACHED*/
                case 5:
                    baz--;

                /*FALLTHROUGH*/
                default:
                    for (; baz > 0; baz--) {
                        if (baz == bar)
                        {
                            break; /* break out of for loop, unrelated to switch
                                      statement */
                        }
                        else
                        {
                            bar++;
                        }
                    }
                break;
            }
        break;

        default:
            bar++;
        break;
    }


    switch (a)
    {
        case 0:
            // code
        break;
    }


    switch (a)
    {
        case 0:
            if (k > 0)
            {
                break;
            }
            z = 1;
        break;

        case 1:
            if (k < 0)
            {
                break;
            }
            z = 2;

        case 2:
            z = 3;
        break;
    }


    return bar;
}


int foo2 (int op)
{
    switch (op)
    {
        case 1:
            do_something();
        break;

        case 2:
            do_something_else();

        case 3:
            do_something_different();
            do_something_more();
        break;
    }
    return -1;
}

int foo3 (int op)
{
    for (int nnn = op; nnn <= 100; nnn++)
    {
        switch (nnn)
        {
            case 1:
                do_something();
            break;

            case 2:
                do_something_else();

            case 3:
                if (do_something_different())
                {
                    do_this();
                    break;
                }
                do_something_more();
            break;

            default:
                ; // nothing
            break;

        }
    }
    return -1;
}
