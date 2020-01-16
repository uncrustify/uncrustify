int foo(int op)
{
        switch (op)
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
                        break; // this should be indented like the surrounding code
                }
                do_something_more();
        break;
        }
        return -1;
        for (;;)
        {
                break;
        }
}
