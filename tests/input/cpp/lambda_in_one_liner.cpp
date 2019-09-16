void bar();

struct foo
{
	foo() { []{ bar(); }(); }
};
