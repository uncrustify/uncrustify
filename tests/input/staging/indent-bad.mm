- (void)bar { if (foo) [_obj bar]; }

void foo() { if (foo) bar(); bar(); }

class Foo
{
	void foo()
	{
		if (bar) { bar() } else i++;
	}
};
