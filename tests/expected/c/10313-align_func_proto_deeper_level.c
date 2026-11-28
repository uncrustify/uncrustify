// Test: align_func_proto_span must not lose the pending alignments when a
// deeper parenthesis level is met at a lower brace level

void outer(void)
{
	int           helper(int a);
	unsigned long other_helper(int a);
}
int value = ((((1 + 2) * 3) - 4) / 5);

void second(void)
{
	{
		short  inner_helper(void);
		double inner_compute(double x);
	}
	int count = (((1)));
}

int  last_one(void);
long last_two(int a);
