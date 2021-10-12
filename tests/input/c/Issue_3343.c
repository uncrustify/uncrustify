int x[] =
{
#if 1
#define X1
#include "x1.c"
#else
#define X2
#include "x2.c"
#endif
};
