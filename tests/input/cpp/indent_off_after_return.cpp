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
	 decltype(x){x} & 2;
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

int foo6()
{
   auto my_lambda = [] ()
   {
      return 1 +
             2 +
             3;

   };
}

template<typename U>
U *
find(const std::string &name = "") const
{
    return find<U>([&name] (auto *pComposite)
                   {
                       return name.empty() ||
                              pComposite->getName() == name;
                   });
}
