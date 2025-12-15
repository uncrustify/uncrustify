// Test: align_var_def_span_num_pp_lines
// With span=1 and varying num_pp_lines settings.

void test_func(void)
{
	int               x;
	unsigned long int y;
#ifdef DEBUG
	double            z;
#endif
#ifdef EXTRA
	float             w;
#endif
#ifdef MORE
#ifdef NESTED
	long s;
#endif
#endif
}
