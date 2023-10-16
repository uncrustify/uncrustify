#ifndef DOXYGEN_SHOULD_SKIP_THIS  // TEST1

void foo()
{
	int i = 0;
}

#elif defined(sgi) && sgi // TEST2

void bar()
{
	int j = 0;
}

#else // TEST3

void foobar()
{
	int j = 0;
}

#endif // END TEST

void main();
