void func(void)
{
    switch (cond)
    {
        case CASE_A:
            for (;;)
                do_stuff();
            break;

        case CASE_B:
            if (cond)
                do_stuff();
            break;

        case CASE_C:
            do {
                do_stuff()
            } while (cond);
            break;

        case CASE_D:
            while(cond)
                do_stuff();
            break;

        case CASE_E:
            switch(cond)
            {
                case CASE_EE:
                    break;
            }
            break;
    }
    for (;;)
        do_stuff();
    if (cond)
        do_stuff();
    do {
        do_stuff()
    } while (cond);
    while(cond)
        do_stuff();
    switch(cond)
    {
        case CASE_A:
            do_stuff();
    }
}
