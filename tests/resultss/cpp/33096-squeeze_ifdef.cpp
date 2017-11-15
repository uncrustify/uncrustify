
#if defined(A)
extern int a;
#elif defined(B)
extern int b;
#else
extern int c;
#endif

int foo()
{
#if defined(A)
	return a;
#elif defined(B)
	return b;
#else
	return c;
#endif
}
