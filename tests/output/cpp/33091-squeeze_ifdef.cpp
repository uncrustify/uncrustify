int foo()
{
#if defined(A)

	return 1;

#elif defined(B)

	return 2;

#else

	return 3;

#endif
}
