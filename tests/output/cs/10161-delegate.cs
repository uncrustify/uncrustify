delegate void MyDelegate(int i);

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
	myCallback =
		new MyCallback(
			delegate
				{ return true; });
}
