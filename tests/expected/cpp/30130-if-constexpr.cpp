int foo()
{
        if constexpr (a == 0)
        {
                return 1;
        }
        return 0;
}
