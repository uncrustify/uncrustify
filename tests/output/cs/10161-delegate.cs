void foo()
{
	obj.cb += () => { };

	funcwithverylongname(() =>
		{
			func();
		});
	funcwithverylongname(delegate
		{
			func();
		});
	funcwithverylongname(delegate(int i)
		{
			func();
		});
}
