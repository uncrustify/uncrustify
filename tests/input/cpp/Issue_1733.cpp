class X15
{
enum Enum
{
	e1
};

operator Enum();
};

::X15::operator ::X15::Enum()
{
	return e1;
}
