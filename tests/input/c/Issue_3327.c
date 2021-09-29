int main(void)
{
#if 1
#pragma GCC warning "This code won't compile"
#define FOO 1
#line 7
#error
#endif
	return 0;
}
