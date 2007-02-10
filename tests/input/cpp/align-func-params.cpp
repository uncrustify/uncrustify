class test
{
    public:

        TYPE_EXPORT method1(int     a,
                                     float      b);

        TYPE_EXPORT method2(int&     d,
                                     float      e);

        TYPE_EXPORT method3(int*     f,
                   float      g);

        TYPE_EXPORT method4(int     a);
        TYPE_EXPORT method5(int         &     a);
        TYPE_EXPORT method6(int    *     a);

        TYPE_EXPORT method7(float     a);
        TYPE_EXPORT method8(float         &     a);
        TYPE_EXPORT method9(float    *     a);

        TYPE_EXPORT method3(int*     f, char foo,
                   float      g);

        TYPE_EXPORT method1(int     a,
                                     float      b)
                                     {
                                        int c;

                                        if ( true ) callProc;
                                        // do stuff.
                                    }
}


