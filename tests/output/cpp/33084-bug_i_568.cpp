enum
{
	kEnumValue = 5,
};

struct foo
{
	int bar : kEnumValue;
	int pad : 3;
}
