/**
 * @file kw_subst.c
 * Description
 *
 * $Id$
 */
#include <string>

/**
 * foo1
 * TODO: DESCRIPTION
 * @return TODO
 */
int foo1()
{
}

/** header comment */
#if 2
/**
 * foo2
 * TODO: DESCRIPTION
 * @return TODO
 */
int foo2(void)
{
}
#endif

#if 1
/**
 * foo3
 * TODO: DESCRIPTION
 * @param a TODO
 */
void foo3(int a)
{
}
#endif

/**
 * foo4
 * TODO: DESCRIPTION
 * @param a TODO
 * @param b TODO
 * @param c TODO
 * @return TODO
 */
void *foo4(int a, int b, int c)
{
}
