void foo()
{
	obj.cb += () => { };

	funcwithverylongname(() =>
	{
		func();
	});
}

Func(
	"stuff",
	foo =>
{
	bar();
});

Func(
	"stuff",
	foo =>
{
	bar();
});

data.Sort(
	delegate(InputData lhs, InputData rhs)
{
	return lhs.m_Name.CompareTo(rhs.m_Name);
});
