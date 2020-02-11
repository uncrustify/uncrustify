int foo1()
{
	return std::pair<int, int>{
		1, 2
	}.first;
}

int foo2()
{
	return
	int{3} & 2;
}

int foo3()
{
	constexpr static int x = 3;
	return
	decltype(x) {x} & 2;
}

int foo4()
{
	return
	new Type();
}

int foo5()
{
	return
	veryLongMethodCall(
		arg1,
		longMethodCall(
			methodCall(
				arg2, arg3
				), arg4
			)
		);
}
