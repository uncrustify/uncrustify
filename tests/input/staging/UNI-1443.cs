public class Class
{
    public static bool Foo()
    {
#if SOME_DEFINE
        if (!boo)
        {
#endif
            if (bar)
            {
                return false;
            }
#if SOME_DEFINE
        }
#endif
        return true;
    }
}
