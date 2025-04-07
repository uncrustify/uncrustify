void foo(void)
{
        if ((a != 0)
            && (b == 0)
            && (c < 0) && (d > 0))
        {
                printf("hi");
        }

        if ((a != 0)
            && (b == 0)
            && (c < 0))
        {
                printf("hi");
        }

        if ((a != 0)
            &&
            (b == 0)
            &&
            (c < 0))
        {
                printf("hi");
        }
}
