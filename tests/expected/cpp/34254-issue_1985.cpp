// Don't break a prototype followed by a one-liner
class foo1
{
foo1();
foo1(int) {}


int bar();
int bar(int) { return 0; }


foo1(long);
foo1(short) {}


int x;
};

// Don't break a one-liner followed by a prototype
class foo2
{
foo2(int) {}
foo2();


int bar(int) { return 0; }
int bar();


foo2(short) {}
foo2(long);


int x;
};

// Do break a prototype followed by a multi-line definition
class foo3
{
foo3();


foo3(int)
{
	x = 0;
}
int bar();


int bar(int)
{
	return 0;
}
foo3(long);


foo3(short)
{
	x = 0;
}
int x;
};
