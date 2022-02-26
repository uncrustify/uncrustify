#include <stdio.h>

class
      alignas(32)
                  Foobar;

int main()
{
    typedef
            int
                int32;
    int foo,
             bar,
                  baz;
    foo = 5
            + 6
                + 7;
    if (printf("%d %d",
                        5
                          + 6,
                               7)
                                  < 0)
    {
        return 1;
    }

    return 0;
}
