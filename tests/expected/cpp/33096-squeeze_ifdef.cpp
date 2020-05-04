
#if defined(A)
// Comment
extern int ax;
void fa();
#elif defined(B)
extern int bx;
void fb();
#else
extern int cx;
void fc();
#endif

int foo()
{
#if defined(A)
	int a = ax;
#elif defined(B)
	// Comment
	int b = bx;
#else
	int c = cx;
#endif
#if defined(A)
	return a;
#elif defined(B)
	return b;
#else
	// Comment
	return c;
#endif
}
