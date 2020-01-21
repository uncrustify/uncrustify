//example file
typedef int X35GlobalT1;
typedef int X35T2;

void fooX35a()
{
	::X35GlobalT1 a1;
	X35T2         a2;

	::X35GlobalT1 a3 = 1;
	X35T2         a4 = 1;
}

void fooX35b()
{
	X35GlobalT1 a1;
	X35T2       a2;

	X35GlobalT1 a3 = 1;
	X35T2       a4 = 1;
}

class X35_1a
{
private:
::X35GlobalT1 a1;
X35T2         a2;

::X35GlobalT1 a3 = 1;
X35T2         a4 = 1;
};

class X35_1b
{
private:
X35GlobalT1 a1;
X35T2       a2;

X35GlobalT1 a3 = 1;
X35T2       a4 = 1;
};
