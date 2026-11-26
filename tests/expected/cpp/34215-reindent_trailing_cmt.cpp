// Test: a trailing comment must not be glued to the code when its line is
// re-indented and spaces larger than the minimum are kept before it

void g(void)
{
    int x   = 1; // c1
    long yy    =    2; /* c2 */
    short z  = 3; // c3
    if (x)
    {
        x   = 4; // c4
    }
}

void h(int a)
{
    x;// no space in the input: stays glued
    int y   = 1; /* first */ // second
    if (a) a   = 2; // after a statement without braces
    int m   = 3; /* multi
                    line */
}

#if VALUE
#define LIMIT   1 // in a preprocessor block
#endif

void k(void)
{
    int w   = 1;   // three spaces in the input: one when the line moves right
}

#define M(x) \
        do { \
            int v   = x; /* in a macro */ \
        } while (0)

void s(int a, int y)
{
    switch (a)
    {
    case 1: y   = 2; // in a case
        break;
    }
}

int table[2][3] =
{
    {1,   2, 3}, // r1
    {4,   5, 6}, // r2
};
