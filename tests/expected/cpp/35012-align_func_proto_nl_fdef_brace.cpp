// Test: align_func_proto_span with nl_fdef_brace=add: the newline added
// before the open brace of a function definition counts for the span

struct Foo
{
	int           a();
	Foo()
	{
	}
	long          bbbbb();

	int           c();
	explicit Foo(int v) : value(v)
	{
	}
	unsigned long dd();
	int value;
};

int  first();
Foo::Foo(long v) : value(v)
{
}
long second();
