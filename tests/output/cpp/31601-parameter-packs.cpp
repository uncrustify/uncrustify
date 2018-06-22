template<typename ... A, int... B>
struct foo1 : foo1<A..., (sizeof...(A)+B)...>
{
	foo1() {
		int x = sizeof...(A);
	}
}

template<int... X> int bar1()
{
	auto s = sizeof...(X);
	chomp(X)...;
	return X+...;
}

template<class R, typename ... Args>
struct invoke1<R(fp*)(FArgs...)>
{
};

template  <  typename  ... A, int  ... B  >
struct foo2 :  foo2<  A  ..., (  sizeof  ...  (  A  )  +  B  )...  >
{
	foo2() {
		int x = sizeof  ...  (  A  );
	}
}

template  <  int  ... X  > int bar2()
{
	auto s = sizeof  ...  (  X  );
	chomp(  X  )...;
	return X  +  ...;
}

template  <  class R, typename  ... Args  >
struct invoke2  <  R  (  fp*  )  (  FArgs  ...  )  >
{
};
